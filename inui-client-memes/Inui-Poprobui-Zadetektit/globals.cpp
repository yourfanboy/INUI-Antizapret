#include "common.h"

// Определения глобальных переменных
std::atomic<bool> g_running = false;
std::thread g_monitorThread;
std::mutex g_hookMutex;
std::mutex g_logMutex;
bool g_isHookActive = false;

// Путь к директории DLL
std::filesystem::path g_dllPath;

// Глобальные пути (будут инициализированы после получения пути DLL)
std::filesystem::path LOG_PATH;
std::filesystem::path SCREENSHOTS_PATH;
std::filesystem::path CUSTOM_SCREENSHOT_PATH;

std::filesystem::path GetDllPath() {
    HMODULE hModule = NULL;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)&GetDllPath, 
                           &hModule)) {
        throw std::runtime_error("Failed to get module handle");
    }

    wchar_t dllPath[MAX_PATH] = { 0 };
    if (!GetModuleFileNameW(hModule, dllPath, MAX_PATH)) {
        throw std::runtime_error("Failed to get module filename");
    }

    return std::filesystem::path(dllPath).parent_path();
} 