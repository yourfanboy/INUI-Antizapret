#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <detours.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <psapi.h>
#include <chrono>
#include <ctime>
#include <tlhelp32.h>
#include <algorithm>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <locale>
#include <codecvt>

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "psapi.lib")

// Глобальные переменные для управления
extern std::atomic<bool> g_running;
extern std::thread g_monitorThread;
extern std::mutex g_hookMutex;
extern std::mutex g_logMutex;
extern bool g_isHookActive;

// Путь к директории DLL
extern std::filesystem::path g_dllPath;

// Функция для получения пути к DLL
std::filesystem::path GetDllPath();

// Функция для получения путей относительно DLL
inline std::filesystem::path GetRelativePath(const std::string& relativePath) {
    return g_dllPath / relativePath;
}

// Глобальные пути (будут инициализированы после получения пути DLL)
extern std::filesystem::path LOG_PATH;
extern std::filesystem::path SCREENSHOTS_PATH;
extern std::filesystem::path CUSTOM_SCREENSHOT_PATH; 