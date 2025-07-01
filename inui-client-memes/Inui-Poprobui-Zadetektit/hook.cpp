#include "hook.h"
#include "logger.h"
#include "screenshot.h"

void UninstallHook() {
    std::lock_guard<std::mutex> lock(g_hookMutex);
    
    if (!g_isHookActive || !OriginalTakeScreenshot) {
        return;
    }

    try {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)OriginalTakeScreenshot, HookedTakeScreenshot);
        LONG error = DetourTransactionCommit();
        
        if (error == NO_ERROR) {
            SafeLogToFile("Hook removed successfully");
        }
        else {
            SafeLogToFile("Error removing hook: " + std::to_string(error));
        }
    }
    catch (const std::exception& e) {
        SafeLogToFile("Exception during hook removal: " + std::string(e.what()));
        DetourTransactionAbort();
    }
    catch (...) {
        SafeLogToFile("Unknown exception during hook removal");
        DetourTransactionAbort();
    }

    g_isHookActive = false;
    OriginalTakeScreenshot = nullptr;
}

bool InstallHook() {
    std::lock_guard<std::mutex> lock(g_hookMutex);
    
    if (g_isHookActive) {
        SafeLogToFile("Hook is already active");
        return true;
    }

    try {
        HMODULE gameCapture = GetModuleHandleA("gameWindowCapture.dll");
        if (!gameCapture) {
            gameCapture = LoadLibraryA("gameWindowCapture.dll");
        }

        if (!gameCapture) {
            SafeLogToFile("Failed to load gameWindowCapture.dll");
            return false;
        }

        auto takeScreenshot = reinterpret_cast<decltype(OriginalTakeScreenshot)>(
            GetProcAddress(gameCapture, "take_screenshot"));

        if (!takeScreenshot) {
            SafeLogToFile("Failed to get take_screenshot function address");
            return false;
        }

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        LONG error = DetourAttach(&(PVOID&)takeScreenshot, HookedTakeScreenshot);
        
        if (error == NO_ERROR) {
            error = DetourTransactionCommit();
            if (error == NO_ERROR) {
                OriginalTakeScreenshot = takeScreenshot;
                g_isHookActive = true;
                SafeLogToFile("Hook installed successfully");
                return true;
            }
        }

        DetourTransactionAbort();
        SafeLogToFile("Failed to install hook: " + std::to_string(error));
        return false;
    }
    catch (const std::exception& e) {
        SafeLogToFile("Exception during hook installation: " + std::string(e.what()));
        DetourTransactionAbort();
        return false;
    }
    catch (...) {
        SafeLogToFile("Unknown exception during hook installation");
        DetourTransactionAbort();
        return false;
    }
}

void MonitorFunction() {
    SafeLogToFile("Monitor thread started");
    
    while (g_running) {
        try {
            if (!g_isHookActive) {
                SafeLogToFile("Hook not active, attempting to install...");
                InstallHook();
            }
        }
        catch (...) {
            SafeLogToFile("Error in monitor thread");
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    SafeLogToFile("Monitor thread stopped");
}

void ListLoadedModules() {
    SafeLogToFile("Loaded modules:");
    
    HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (hModuleSnap == INVALID_HANDLE_VALUE) {
        SafeLogToFile("Failed to create module snapshot. Error: " + std::to_string(GetLastError()));
        return;
    }

    MODULEENTRY32W me32;
    me32.dwSize = sizeof(MODULEENTRY32W);

    if (Module32FirstW(hModuleSnap, &me32)) {
        do {
            SafeLogToFile(" - " + WideToUtf8(me32.szModule) +
                         " (Base: 0x" + std::to_string(reinterpret_cast<uintptr_t>(me32.modBaseAddr)) +
                         ", Size: " + std::to_string(me32.modBaseSize) + " bytes)");
        } while (Module32NextW(hModuleSnap, &me32));
    }

    CloseHandle(hModuleSnap);
} 