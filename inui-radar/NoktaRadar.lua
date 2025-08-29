-- HTTP Libary from workshop
local http = require 'gamesense/http'
local ffi = require('ffi')
local client_userid_to_entindex = client.userid_to_entindex
local client_color_log = client.color_log
local client_trace_line = client.trace_line

local entity_get_all = entity.get_all
local entity_get_local_player = entity.get_local_player
local entity_get_player_resource = entity.get_player_resource
local entity_get_player_name = entity.get_player_name
local entity_get_classname = entity.get_classname
local entity_is_dormant = entity.is_dormant
local entity_get_prop = entity.get_prop
local entity_get_origin = entity.get_origin

local globals_tickcount = globals.tickcount
local globals_mapname = globals.mapname

local json_stringify = json.stringify

local math_rad = math.rad
local math_sin = math.sin
local math_cos = math.cos 
local math_tan = math.tan

local clipboard = require 'lib/clipboard'
local ui_get = ui.get


local url = "http://radar.noctua.sbs/"
local userAgentInfo = "gs_cloud_radar"
local shareLink = ""
local running = false
ui.new_label("LUA", "B", "\aFFFFFFFF ------- Nokta\a0384fcff Radar\aFFFFFFFF -------")

local function ping_host()
	http.get(url, { user_agent_info = userAgentInfo }, function(success, response)
            if success and response.status == 200 then
				client.exec("playvol ui/beepclear 1")
				ui.new_label("LUA", "B", "\aFFFFFFFF Host: \a9FCA2BFFOnline")
            else
                local error_msg = response.status_message or "Unknown error"
				client.exec("playvol ui/deathnotice 1")
				ui.new_label("LUA", "B", "\aFFFFFFFF Host: \aFF000092Offline")
            end
		end)
	end


ping_host()

-- Declare room_label and copy_button outside the button callbacks
local room_label = nil
local copy_button = nil

ui.new_button("LUA", "A", "Create Room", function()
    if shareLink == "" then
        http.get(url .. "create", {user_agent_info = userAgentInfo},
        function(success, response)
            if success and response and response.body then
                shareLink = url .. "?auth=" .. response.body:sub(1, 16)
                clipboard.copy(shareLink)
                -- Create or update the label
                if room_label == nil then
                    room_label = ui.new_label("LUA", "B", "\aFFFFFFFF Room: \a9FCA2BFF" .. response.body:sub(1, 16))
                else
                    ui.set(room_label, "\aFFFFFFFF Room: \a9FCA2BFF" .. response.body:sub(1, 16))
                end
                -- Ensure the label and copy button are visible
                if room_label ~= nil then
                    ui.set_visible(room_label, true)
                end
                if copy_button ~= nil then
                    ui.set_visible(copy_button, true)
                end
                client.exec("playvol buttons/bell1 0.8")
                userAgentInfo = response.body:sub(17, 32)
                running = true
            else
                if response.timed_out then
                    client.exec("playvol ui/deathnotice 1")
                else
                    client.exec("playvol ui/deathnotice 1")
                end
            end
        end)
    else
        client.exec("playvol ui/bell1 1")
        clipboard.copy(shareLink)
    end
end)

-- Destroys the cloud radar which also disables the link from getting updated
ui.new_button("LUA", "A", "Remove Room", function()
    http.get(url .. "destroy", {user_agent_info = userAgentInfo},
        function(success, response)
            if success and response and response.body then
                client.exec("playvol buttons/bell1 0.8")
                if room_label ~= nil then
                    ui.set_visible(room_label, false) -- Hide the label
                end
                if copy_button ~= nil then
                    ui.set_visible(copy_button, false) -- Hide the copy button
                end
                userAgentInfo = "gs_cloud_radar"
                shareLink = ""
                running = false
            else
                if response.timed_out then
                    client.exec("playvol ui/deathnotice 1")
                else
                    client.exec("playvol ui/deathnotice 1")
                end
            end
        end)
end)

-- Add Copy Room ID button after the label section
if copy_button == nil then
    copy_button = ui.new_button("LUA", "B", "Copy link", function()
        if shareLink ~= "" then
            clipboard.copy(shareLink)
            client.exec("playvol buttons/bell1 0.8")
        end
    end)
    ui.set_visible(copy_button, false) -- Initially hidden
end
ui.new_label("LUA", "B", "\aFFFFFFFF Made by t.me/inuistaff & t.me/sclay")
--this will most likely be removed
local updateInterval = ui.new_slider("LUA", "A", "[Radar] Update rate", 1, 255, 1, true, "", 1, nil)

--spoofes the player name
local spoofPlayerName = ui.new_checkbox("LUA", "A", "[Radar] Spoof player name")

--updates the entity-get-player-name function
ui.set_callback(spoofPlayerName, function()
	if ui_get(spoofPlayerName) then 
		entity_get_player_name = function(entIndex)
			return "player " .. entIndex
		end
	else
		entity_get_player_name = entity.get_player_name
	end
end)

local sending = false
local last_sent_tick = 0
local last_sent_data = {}

local event_data = {}
local function insert_event_data(entIndex, event) 
	if event_data[entIndex] == nil then 
		event_data[entIndex] = {}
	end
	event_data[entIndex][#event_data[entIndex] + 1] = event
end

client.set_event_callback("paint", function()

	if running then
		-- Skipping ticks to reduce post requests
		if last_sent_tick + ui_get(updateInterval) < globals_tickcount() and not sending then
			last_sent_tick = globals_tickcount()

			local CCSPlayerResource = entity_get_player_resource()

			--this will get sent to the server
			local data = {}
			data[65] = globals_mapname()

			--bombInfo
			data[66] = {}
			local bomb_ent = nil
			if entity_get_all("CPlantedC4")[1] ~= nil then 
				data[66][1] = true
				data[66][2], data[66][3] = entity_get_prop(entity_get_all("CPlantedC4")[1], "m_vecOrigin")
			elseif entity_get_all("CC4")[1] ~= nil then 
				data[66][1] = false
				data[66][2], data[66][3] = entity_get_prop(entity_get_all("CC4")[1], "m_vecOrigin")

			else 
				data[66] = ""
			end

			--i'm using this way to loop through all players, even dormant ones
			for i = 1, 64 do 
				if entity_get_classname(i) == "CCSPlayer" then 
					player = {}
					--name
					player[1] = entity_get_player_name(i)

					local x, y, z = entity_get_origin(i)

					--this stuff tries to get the worldPos where the player is looking at, this will fail on target we don't know the location of
					if (x ~= nil) then 
						z = z + 64 - entity_get_prop(i, "m_flDuckAmount")
						local pitch, yaw = entity_get_prop(i, "m_angEyeAngles")
						player[3] = x
						player[4] = y
						player[5] = z 

						do --i'm using this instead of just going for the 5000 in the first try due to it causing lag if people look into weird locations
							--this will probably decrease the overall performance as it causes way more trace line calls but reduces the lag spikes as it never tries to trace thourgh a lot of weird stuff
							local distance = 0
							local fraction = 1

							while fraction == 1 and distance < 5000 do 
								distance = distance + 100
								local facingX = x - math_cos(math_rad(yaw + 180)) * distance
								local facingY = y - math_sin(math_rad(yaw + 180)) * distance
								local facingZ = z - math_tan(math_rad(pitch)) * distance
		
								fraction = client_trace_line(i, x, y, z, facingX, facingY, facingZ)
							end

							player[6] = x - math_cos(math_rad(yaw + 180)) * distance * fraction
							player[7] = y - math_sin(math_rad(yaw + 180)) * distance * fraction
							--player[8] = z - math_tan(math_rad(pitch)) * distance * fraction
						end
					else
						player[3] = ""
						player[4] = ""
						player[5] = ""

						player[6] = ""
						player[7] = ""
						--player[8] = ""
					end
					
					player[8] = entity_is_dormant(i)

					player[9] = entity_get_prop(CCSPlayerResource, "m_iTeam", i)

					player[10] = entity_get_prop(CCSPlayerResource, "m_bAlive", i) == 1
					player[11] = entity_get_prop(CCSPlayerResource, "m_iHealth", i)
					player[12] = entity_get_prop(CCSPlayerResource, "m_iArmor", i) > 0
					player[13] = entity_get_prop(CCSPlayerResource, "m_bHasHelmet", i) == 1

					player[14] = entity_get_prop(CCSPlayerResource, "m_iPlayerC4") == i
					player[15] = entity_get_prop(CCSPlayerResource, "m_bHasDefuser", i) == 1

					if event_data[i] ~= nil then 
						player[16] = event_data[i]
					else
						player[16] = ""
					end

					data[i] = player
				else
					data[i] = ""
				end
			end

			if (data ~= last_sent_data) then
				last_sent_data = data
				sending = true
				http.post(url .. "update", {params = {["data"] = json_stringify(data), ["auth"] = userAgentInfo}, user_agent_info = userAgentInfo},
					function(success, response) 
						sending = false
					end
				)
				event_data = {}
			end

		else
			-- just making sure that the last tick is never greater than the actuall tick. This could happen after switching server as an example
			if last_sent_tick > globals_tickcount() then
				last_sent_tick = globals_tickcount()
			end
		end
	end
end)


local times_called = 0