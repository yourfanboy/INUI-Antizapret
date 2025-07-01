#include "common.h"
#include "logger.h"
#include "screenshot.h"
#include "hook.h"

namespace {
    bool InitializeDebugConsole() {
        try {
            AllocConsole();
            FILE* pFile;
            freopen_s(&pFile, "CONOUT$", "w", stdout);
            freopen_s(&pFile, "CONOUT$", "w", stderr);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool InitializePaths() {
        try {
            // Получаем путь к директории DLL
            g_dllPath = GetDllPath();

            // Инициализируем глобальные пути относительно DLL
            LOG_PATH = GetRelativePath("hook_logs\\capture_log.txt");
            //SCREENSHOTS_PATH = GetRelativePath("hook_logs\\screenshots");
            CUSTOM_SCREENSHOT_PATH = GetRelativePath("hook_logs\\custom_screenshot.raw");

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[t.me/inuicheat] Failed to initialize paths: " << e.what() << std::endl;
            return false;
        }
        catch (...) {
            std::cerr << "[t.me/inuicheat] Unknown error during path initialization" << std::endl;
            return false;
        }
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH: {
        try {
            DisableThreadLibraryCalls(hModule);
            
            // Инициализируем пути до логирования
            if (!InitializePaths()) {
                return FALSE;
            }

            if (!InitializeDebugConsole()) {
                return FALSE;
            }

            SafeLogToFile("=== DLL INITIALIZATION START ===");
            SafeLogToFile("Process info: " + GetProcessInfo());
            SafeLogToFile("DLL path: " + g_dllPath.string());
            SafeLogToFile("Log path: " + LOG_PATH.string());
            //SafeLogToFile("Screenshots path: " + SCREENSHOTS_PATH.string());

            try {
                std::filesystem::create_directories(LOG_PATH.parent_path());
                //std::filesystem::create_directories(SCREENSHOTS_PATH);
            }
            catch (const std::exception& e) {
                SafeLogToFile("CRITICAL ERROR: Failed to create directories: " + std::string(e.what()));
                return FALSE;
            }

            ListLoadedModules();

            if (InstallHook()) {
                g_running = true;
                g_monitorThread = std::thread(MonitorFunction);
                SafeLogToFile("Hook installed and monitoring started");
            }
            else {
                SafeLogToFile("CRITICAL ERROR: Failed to install hook");
                return FALSE;
            }

            SafeLogToFile("=== DLL INITIALIZATION COMPLETE ===");
            return TRUE;
        }
        catch (const std::exception& e) {
            SafeLogToFile("CRITICAL ERROR during DLL initialization: " + std::string(e.what()));
            return FALSE;
        }
        catch (...) {
            SafeLogToFile("CRITICAL ERROR: Unknown exception during DLL initialization");
            return FALSE;
        }
    }

    case DLL_PROCESS_DETACH: {
        try {
            g_running = false;
            if (g_monitorThread.joinable()) {
                g_monitorThread.join();
            }
            UninstallHook();
        }
        catch (...) {
            // Игнорируем ошибки при выгрузке
        }
        break;
    }

    default:
        break;
    }
    return TRUE;
} 