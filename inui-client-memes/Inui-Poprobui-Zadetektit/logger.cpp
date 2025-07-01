#include "logger.h"

std::string GetCurrentTimeStamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    return buffer;
}

std::string GetProcessInfo() {
    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();

    char processName[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, processName, MAX_PATH);

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "PID: %lu | TID: %lu | Process: %s",
        pid, tid, processName);

    return buffer;
}

// Получение краткой версии сообщения для консоли
std::string GetConsoleMessage(const std::string& message) {
    // Основные сообщения о состоянии
    if (message == "=== DLL INITIALIZATION START ===") return "Starting...";
    if (message == "=== DLL INITIALIZATION COMPLETE ===") return "Happy hacking!";
    if (message == "Monitor thread started") return "Monitoring started";
    if (message == "Hook installed successfully") return "Hook installed successfully";
    if (message == "Hook removed successfully") return "Hook removed successfully";
    
    // Сообщения о скриншотах
    if (message.find("Called take_screenshot") != std::string::npos) return "Detected take_screenshot()";
    if (message.find("Screenshot data obtained successfully") != std::string::npos) return "Spoofed image";
    if (message.find("Failed to get screenshot data") != std::string::npos) return "Failed to spoof image";
    
    // Сообщения об ошибках (оставляем как есть для важности)
    if (message.find("ERROR:") != std::string::npos || 
        message.find("CRITICAL ERROR:") != std::string::npos ||
        message.find("Exception") != std::string::npos) {
        return message;
    }
    
    // Остальные сообщения не выводим в консоль
    return "";
}

void SafeLogToFile(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    try {
        // Получаем текущее время
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timeBuffer[32];
        snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d.%03d",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        // Получаем краткую версию для консоли
        std::string consoleMsg = GetConsoleMessage(message);
        if (!consoleMsg.empty()) {
            std::cout << "[t.me/inuicheat] [" << timeBuffer << "] " << consoleMsg << std::endl;
        }

        // Полное логирование в файл
        std::filesystem::create_directories(LOG_PATH.parent_path());
        std::ofstream logFile(LOG_PATH, std::ios::app);
        if (logFile.is_open()) {
            logFile << "[t.me/inuicheat] [" << timeBuffer << "] " << message << std::endl;
            logFile.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[t.me/inuicheat] Error writing to log: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "[t.me/inuicheat] Unknown error writing to log" << std::endl;
    }
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    try {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), 
            nullptr, 0, nullptr, nullptr);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), 
            &strTo[0], size_needed, nullptr, nullptr);
        return strTo;
    }
    catch (...) {
        return "[Error converting string]";
    }
} 