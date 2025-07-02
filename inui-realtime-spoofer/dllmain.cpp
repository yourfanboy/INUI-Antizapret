#include "pch.h"
#include <windows.h>
#include <detours.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <shellapi.h>
#include <vector>
#include "pch.h"
// Логгер
class Logger {
private:
    static HANDLE logFile;
    static bool isInitialized;

    static bool Init() {
        if (isInitialized) return true;

        // Get Windows Temp path
        wchar_t tempPath[MAX_PATH];
        if (GetTempPathW(MAX_PATH, tempPath) == 0) {
            OutputDebugStringA("Failed to get temp path");
            return false;
        }

        // Create full path for log file
        wchar_t logPath[MAX_PATH];
        if (GetTempFileNameW(tempPath, L"SPF", 0, logPath) == 0) {
            OutputDebugStringA("Failed to generate temp file name");
            return false;
        }

        // Create file
        logFile = CreateFileW(
            logPath,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (logFile == INVALID_HANDLE_VALUE) {
            OutputDebugStringA("Failed to create log file");
            return false;
        }

        isInitialized = true;
        Log("Logger initialized");
        return true;
    }

public:
    static void Log(const char* message) {
        if (!isInitialized) {
            Init();
        }

        if (logFile == INVALID_HANDLE_VALUE) {
            OutputDebugStringA(message);
                OutputDebugStringA("\n");
            return;
        }

        char buffer[4096];
        int offset = 0;

        // Add timestamp
        SYSTEMTIME st;
        GetLocalTime(&st);
        offset += snprintf(buffer, sizeof(buffer), "[%02d:%02d:%02d.%03d] ",
                          st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        // Add message
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s", message);

        // Add newline
        strcat_s(buffer + offset, sizeof(buffer) - offset, "\r\n");
        offset += 2;

        // Write to file
        DWORD written;
        WriteFile(logFile, buffer, offset, &written, NULL);
        FlushFileBuffers(logFile);

        // Also output to debug console
            OutputDebugStringA(buffer);
        }
    
    template<typename... Args>
    static void LogFormat(const char* format, Args... args) {
        char buffer[4096];
        snprintf(buffer, sizeof(buffer), format, args...);
        Log(buffer);
    }

    static void Close() {
        if (isInitialized && logFile != INVALID_HANDLE_VALUE) {
            Log("Logger shutting down");
            FlushFileBuffers(logFile);
            CloseHandle(logFile);
            logFile = INVALID_HANDLE_VALUE;
            isInitialized = false;
        }
    }
};

HANDLE Logger::logFile = INVALID_HANDLE_VALUE;
bool Logger::isInitialized = false;

// Генератор GUID
class GuidGenerator {
private:
    std::string currentGuid;
    std::random_device rd;
    std::mt19937 gen;
    
    // Вычисление SHA-256 хеша
    std::string CalculateSHA256(const std::string& input) const {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BYTE rgbHash[32]; // SHA-256 дает 32 байта
        DWORD cbHash = 0;
        std::string hash = "";
        
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
            Logger::LogFormat("CryptAcquireContext failed: %lu", GetLastError());
            return hash;
        }
        
        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            Logger::LogFormat("CryptCreateHash failed: %lu", GetLastError());
            CryptReleaseContext(hProv, 0);
            return hash;
        }
        
        if (!CryptHashData(hHash, (BYTE*)input.c_str(), (DWORD)input.length(), 0)) {
            Logger::LogFormat("CryptHashData failed: %lu", GetLastError());
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return hash;
        }
        
        cbHash = sizeof(rgbHash);
        if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
            Logger::LogFormat("CryptGetHashParam failed: %lu", GetLastError());
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return hash;
        }
        
        // Преобразуем хеш в строку
        std::stringstream ss;
        for (DWORD i = 0; i < cbHash; i++) {
            ss << std::hex << std::setfill('0') << std::setw(2) << (int)rgbHash[i];
        }
        
        hash = ss.str();
        
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        
        return hash;
    }

public:
    GuidGenerator() : gen(rd()) {
        GenerateNewGuid();
    }
    
    void GenerateNewGuid() {
        // Генерируем GUID в формате XXXXXXXX-XXXX-4XXX-YXXX-XXXXXXXXXXXX (без фигурных скобок)
        // Убедимся, что длина не превышает 36 символов (36 * 2 + 12 < 86)
        std::uniform_int_distribution<> hex_dist(0, 15);
        std::stringstream ss;
        ss << std::hex << std::setfill('0'); // без std::uppercase для нижнего регистра

        // Первая группа (8 символов)
        for (int i = 0; i < 8; i++) {
            ss << std::setw(1) << hex_dist(gen);
        }
        ss << "-";

        // Вторая группа (4 символа)
        for (int i = 0; i < 4; i++) {
            ss << std::setw(1) << hex_dist(gen);
        }
        ss << "-";

        // Третья группа (4 символа, начинается с 4)
        ss << "4";
        for (int i = 0; i < 3; i++) {
            ss << std::setw(1) << hex_dist(gen);
        }
        ss << "-";

        // Четвертая группа (4 символа, начинается с 8-B)
        ss << std::setw(1) << (8 + (hex_dist(gen) % 4)); // 8, 9, A, или B
        for (int i = 0; i < 3; i++) {
            ss << std::setw(1) << hex_dist(gen);
        }
        ss << "-";

        // Последняя группа (12 символов)
        for (int i = 0; i < 12; i++) {
            ss << std::setw(1) << hex_dist(gen);
        }

        currentGuid = ss.str();
        
        // Проверяем, что GUID поместится в буфер размером 86 байт
        ULONG requiredSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + ((currentGuid.length() + 1) * sizeof(WCHAR));
        if (requiredSize > 86) {
            // Укорачиваем GUID, если нужно
            ULONG maxGuidLength = (86 - sizeof(KEY_VALUE_PARTIAL_INFORMATION)) / sizeof(WCHAR) - 1;
            if (currentGuid.length() > maxGuidLength) {
                currentGuid = currentGuid.substr(0, maxGuidLength);
                Logger::LogFormat("GUID truncated to fit in 86 bytes buffer: %s", currentGuid.c_str());
            }
        }
        
        Logger::LogFormat("Generated new machine GUID: %s", currentGuid.c_str());
        Logger::LogFormat("SHA-256 hash: %s", GetSHA256Hash().c_str());
        Logger::LogFormat("GUID buffer size: %lu bytes", sizeof(KEY_VALUE_PARTIAL_INFORMATION) + ((currentGuid.length() + 1) * sizeof(WCHAR)));
    }
    
    const std::string& GetCurrentGuid() const {
        return currentGuid;
    }
    
    // Преобразование GUID в широкую строку
    std::wstring GetWideGuid() const {
        return std::wstring(currentGuid.begin(), currentGuid.end());
    }
    
    // Получение GUID с фигурными скобками для реестра (некоторые API могут требовать)
    std::string GetGuidWithBraces() const {
        return "{" + currentGuid + "}";
    }
    
    // Преобразование GUID с фигурными скобками в широкую строку
    std::wstring GetWideGuidWithBraces() const {
        std::string withBraces = GetGuidWithBraces();
        return std::wstring(withBraces.begin(), withBraces.end());
    }
    
    // Получение SHA-256 хеша от GUID (используется в node-machine-id)
    std::string GetSHA256Hash() const {
        return CalculateSHA256(currentGuid);
    }
    
    // Получение SHA-256 хеша в виде широкой строки
    std::wstring GetWideSHA256Hash() const {
        std::string hash = GetSHA256Hash();
        return std::wstring(hash.begin(), hash.end());
    }
};

// Глобальный экземпляр генератора GUID
GuidGenerator g_guidGenerator;

// Оригинальные функции
static HANDLE (WINAPI* OriginalCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE) = CreateFileW;
static BOOL (WINAPI* OriginalReadFile)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED) = ReadFile;
static BOOL (WINAPI* OriginalCreateProcessW)(LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION) = CreateProcessW;
static HINSTANCE (WINAPI* OriginalShellExecuteW)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT) = ShellExecuteW;
static BOOL (WINAPI* OriginalShellExecuteExW)(SHELLEXECUTEINFOW*) = ShellExecuteExW;

// Оригинальные функции реестра
static LONG (WINAPI* OriginalRegOpenKeyExW)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY) = RegOpenKeyExW;
static LONG (WINAPI* OriginalRegQueryValueExW)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD) = RegQueryValueExW;

// Оригинальные функции WMI
static HRESULT (WINAPI* OriginalCoCreateInstance)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*) = CoCreateInstance;

// Оригинальные низкоуровневые функции реестра
typedef NTSTATUS (NTAPI* NtQueryValueKey_t)(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    PVOID KeyValueInformation,
    ULONG Length,
    PULONG ResultLength
);
static NtQueryValueKey_t OriginalNtQueryValueKey = nullptr;

// Перехваченная функция CreateFileW
HANDLE WINAPI HookedCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
) {
    if (lpFileName) {
        std::wstring fileName(lpFileName);
        std::wstring lowerFileName = fileName;
        // Преобразуем в нижний регистр для регистронезависимого сравнения
        std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);
        
        // Проверяем, связан ли файл с machine-id
        bool isMachineIdFile = false;
        bool isNodeMachineIdFile = false;
        bool isWindowsMachineGuidFile = false;
        
        // Проверка на различные форматы имен файлов
        if (lowerFileName.find(L"machine-id") != std::wstring::npos ||
            lowerFileName.find(L"machineid") != std::wstring::npos ||
            lowerFileName.find(L"machine_id") != std::wstring::npos) {
            isMachineIdFile = true;
        }

        if (lowerFileName.find(L"node_modules\\node-machine-id") != std::wstring::npos ||
            lowerFileName.find(L"node-machine-id") != std::wstring::npos ||
            lowerFileName.find(L"node_machine_id.txt") != std::wstring::npos) {
            isNodeMachineIdFile = true;
        }
        
        // Проверка на файлы, связанные с Windows MachineGuid
        if (lowerFileName.find(L"cryptography") != std::wstring::npos &&
            lowerFileName.find(L"machineguid") != std::wstring::npos) {
            isWindowsMachineGuidFile = true;
        }
        
        // Проверка на временные файлы node-machine-id
        if (lowerFileName.find(L"temp") != std::wstring::npos && 
            lowerFileName.find(L"machine") != std::wstring::npos) {
            // Проверяем содержимое процесса
            WCHAR processName[MAX_PATH] = {0};
            GetModuleFileNameW(NULL, processName, MAX_PATH);
            std::wstring procName(processName);
            std::transform(procName.begin(), procName.end(), procName.begin(), ::tolower);
            
            if (procName.find(L"node") != std::wstring::npos || 
                procName.find(L"electron") != std::wstring::npos) {
                isNodeMachineIdFile = true;
            }
        }
        
        // Если это файл, связанный с machine-id
        if (isMachineIdFile || isNodeMachineIdFile || isWindowsMachineGuidFile) {
            Logger::LogFormat("CreateFileW: Intercepted %s access: %S", 
                           isNodeMachineIdFile ? "node-machine-id" : 
                           (isWindowsMachineGuidFile ? "Windows MachineGuid" : "machine-id"), 
                           fileName.c_str());
            
            // Если файл открывается для чтения, возвращаем наш ID
            if (dwDesiredAccess & GENERIC_READ || dwDesiredAccess & FILE_READ_DATA) {
                // Создаем временный файл с нашим ID
                wchar_t tempPath[MAX_PATH];
                GetTempPathW(MAX_PATH, tempPath);
                
                static std::wstring tempMachineIdFile = std::wstring(tempPath) + L"temp_machine_id";
                static std::wstring tempNodeMachineIdFile = std::wstring(tempPath) + L"temp_node_machine_id";
                static std::wstring tempWindowsMachineGuidFile = std::wstring(tempPath) + L"temp_windows_machineguid";
                
                std::wstring tempFileName;
                if (isNodeMachineIdFile) {
                    tempFileName = tempNodeMachineIdFile;
                } else if (isWindowsMachineGuidFile) {
                    tempFileName = tempWindowsMachineGuidFile;
                } else {
                    tempFileName = tempMachineIdFile;
                }
                
                Logger::LogFormat("CreateFileW: Creating temp file: %S", tempFileName.c_str());
                
                HANDLE hTemp = CreateFileW(tempFileName.c_str(), GENERIC_WRITE, 0, NULL,
                                         CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
                
                if (hTemp != INVALID_HANDLE_VALUE) {
                    DWORD written = 0;
                    
                    if (isNodeMachineIdFile) {
                        // Для node-machine-id используем SHA-256 хеш (ASCII)
                        std::string hashId = g_guidGenerator.GetSHA256Hash();
                        WriteFile(hTemp, hashId.c_str(), (DWORD)hashId.length(), &written, NULL);
                        Logger::LogFormat("CreateFileW: Wrote %d bytes of SHA-256 hash to temp file", written);
                    } else if (isWindowsMachineGuidFile) {
                        // Для Windows MachineGuid используем GUID (Unicode)
                        std::wstring wideSpoofed = g_guidGenerator.GetWideGuid();
                        WriteFile(hTemp, wideSpoofed.c_str(), (DWORD)(wideSpoofed.length() * sizeof(WCHAR)), &written, NULL);
                        Logger::LogFormat("CreateFileW: Wrote %d bytes of GUID to temp file", written);
                    } else {
                        // Для обычного machine-id используем GUID (ASCII)
                        std::string spoofed = g_guidGenerator.GetCurrentGuid();
                        WriteFile(hTemp, spoofed.c_str(), (DWORD)spoofed.length(), &written, NULL);
                        Logger::LogFormat("CreateFileW: Wrote %d bytes of GUID to temp file", written);
                    }
                    
                    CloseHandle(hTemp);
                    
                    // Открываем временный файл для чтения
                    return CreateFileW(tempFileName.c_str(), dwDesiredAccess, dwShareMode,
                                     lpSecurityAttributes, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_TEMPORARY, NULL);
                }
            }
            
            // Для записи или других операций блокируем доступ к реальному файлу
            SetLastError(ERROR_FILE_NOT_FOUND);
            Logger::LogFormat("CreateFileW: Blocking access to %s file", 
                           isNodeMachineIdFile ? "node-machine-id" : 
                           (isWindowsMachineGuidFile ? "Windows MachineGuid" : "machine-id"));
            return INVALID_HANDLE_VALUE;
        }
        
        // Проверка на временный файл node-machine-id, созданный нами
        if (lowerFileName.find(L"node_machine_id.txt") != std::wstring::npos) {
            Logger::LogFormat("CreateFileW: Detected access to our node-machine-id file: %S", fileName.c_str());
            
            // Если файл открывается для чтения, возвращаем наш хеш
            if (dwDesiredAccess & GENERIC_READ || dwDesiredAccess & FILE_READ_DATA) {
                // Создаем временный файл с нашим хешем
                wchar_t tempPath[MAX_PATH];
                GetTempPathW(MAX_PATH, tempPath);
                
                std::wstring tempFileName = std::wstring(tempPath) + L"node_machine_id.txt";
                
                // Проверяем, существует ли файл
                DWORD fileAttrs = GetFileAttributesW(tempFileName.c_str());
                if (fileAttrs == INVALID_FILE_ATTRIBUTES) {
                    // Файл не существует, создаем его
                    HANDLE hTemp = CreateFileW(tempFileName.c_str(), GENERIC_WRITE, 0, NULL,
                                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    
                    if (hTemp != INVALID_HANDLE_VALUE) {
                        DWORD written = 0;
                        std::string hashId = g_guidGenerator.GetSHA256Hash();
                        WriteFile(hTemp, hashId.c_str(), (DWORD)hashId.length(), &written, NULL);
                        CloseHandle(hTemp);
                        
                        Logger::LogFormat("CreateFileW: Created node-machine-id file with hash: %s", hashId.c_str());
                    }
                }
                
                // Открываем файл для чтения
                return CreateFileW(tempFileName.c_str(), dwDesiredAccess, dwShareMode,
                                 lpSecurityAttributes, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, NULL);
            }
        }
    }
    
    return OriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
                             lpSecurityAttributes, dwCreationDisposition,
                             dwFlagsAndAttributes, hTemplateFile);
}

// Перехваченная функция RegQueryValueExW
LONG WINAPI HookedRegQueryValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
) {
    if (lpValueName) {
        std::wstring valueName(lpValueName);
        
        // Проверяем, что это запрос MachineGuid
        if (valueName == L"MachineGuid") {
            // Получаем информацию о ключе для отладки
            WCHAR keyName[256] = L"";
            DWORD keyNameSize = sizeof(keyName) / sizeof(WCHAR);
            RegQueryInfoKeyW(hKey, keyName, &keyNameSize, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            
            Logger::LogFormat("RegQueryValueExW: Intercepted MachineGuid registry query for key: %S", 
                            keyName[0] ? keyName : L"(unknown)");
            
            // Получаем текущий GUID для подмены (без фигурных скобок)
            std::wstring wideSpoofed = g_guidGenerator.GetWideGuid();
            Logger::LogFormat("RegQueryValueExW: Using GUID: %S", wideSpoofed.c_str());
            
            // Требуемый размер в байтах, включая нулевой символ
            DWORD requiredSize = (DWORD)((wideSpoofed.length() + 1) * sizeof(WCHAR));
            
            // Если запрашивается только размер (lpData == NULL или *lpcbData слишком мал)
            if (!lpData || (lpcbData && *lpcbData < requiredSize)) {
                if (lpcbData) {
                    *lpcbData = requiredSize;
                    Logger::LogFormat("RegQueryValueExW: Buffer too small, required: %lu bytes", requiredSize);
                }
                return ERROR_MORE_DATA;
            }
            
            // Если буфер достаточного размера, заполняем его
            if (lpData && lpcbData) {
                // Копируем GUID в буфер
                memcpy(lpData, wideSpoofed.c_str(), wideSpoofed.length() * sizeof(WCHAR));
                // Добавляем нулевой символ
                ((WCHAR*)lpData)[wideSpoofed.length()] = L'\0';
                
                *lpcbData = requiredSize;
                if (lpType) *lpType = REG_SZ;
                
                Logger::LogFormat("RegQueryValueExW: Successfully returned spoofed GUID: %S", wideSpoofed.c_str());
                return ERROR_SUCCESS;
            }
        }
    }
    
    // Для всех остальных случаев вызываем оригинальную функцию
    LONG result = OriginalRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    
    // Логируем результат для отладки, если это запрос MachineGuid
    if (lpValueName && wcscmp(lpValueName, L"MachineGuid") == 0) {
        Logger::LogFormat("RegQueryValueExW: Original function returned: %ld", result);
        if (result == ERROR_SUCCESS && lpData && lpcbData) {
            Logger::LogFormat("RegQueryValueExW: Original value: %S", (WCHAR*)lpData);
        }
    }
    
    return result;
}

// Перехваченная функция NtQueryValueKey
NTSTATUS NTAPI HookedNtQueryValueKey(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    PVOID KeyValueInformation,
    ULONG Length,
    PULONG ResultLength
) {
    static thread_local bool isProcessing = false;
    
    // Предотвращаем рекурсию и проверяем входные параметры
    if (isProcessing || !ValueName || !ValueName->Buffer || !ResultLength) {
        return OriginalNtQueryValueKey(KeyHandle, ValueName, KeyValueInformationClass,
            KeyValueInformation, Length, ResultLength);
    }

    isProcessing = true;

    // Проверяем, что это запрос MachineGuid
    if (ValueName->Length > 0 && wcsstr(ValueName->Buffer, L"MachineGuid")) {
        Logger::LogFormat("NtQueryValueKey: MachineGuid intercepted, buffer size: %lu", Length);
        
        // Сначала вызываем оригинальную функцию, чтобы получить правильные структуры
        NTSTATUS originalStatus = OriginalNtQueryValueKey(KeyHandle, ValueName, KeyValueInformationClass,
            KeyValueInformation, Length, ResultLength);
        
        // Получаем текущий GUID для подмены (без фигурных скобок)
        std::wstring wideSpoofed = g_guidGenerator.GetWideGuid();
        Logger::LogFormat("NtQueryValueKey: Using GUID: %S", wideSpoofed.c_str());
        
        // Если это первый запрос для получения размера (обычно с буфером 12 байт)
        if (Length <= 16) {
            // Для первого запроса возвращаем размер, который система ожидает (86 байт)
            if (originalStatus == STATUS_BUFFER_TOO_SMALL && *ResultLength == 86) {
                Logger::LogFormat("NtQueryValueKey: Initial size query, returning expected size: 86");
                isProcessing = false;
                return originalStatus;
            }
            
            // Если система запрашивает другой размер, просто возвращаем оригинальный результат
            Logger::LogFormat("NtQueryValueKey: Initial size query, returning original status: 0x%08X, size: %lu",
                           originalStatus, *ResultLength);
            isProcessing = false;
            return originalStatus;
        }

        // Если это второй запрос с буфером достаточного размера
        if (KeyValueInformation && KeyValueInformationClass == KeyValuePartialInformation) {
            PKEY_VALUE_PARTIAL_INFORMATION pInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueInformation;
            
            // Проверяем, что это действительно строковое значение
            if (pInfo->Type == REG_SZ) {
                // Адаптируем наш GUID под размер буфера
                ULONG dataSize = (ULONG)((wideSpoofed.length() + 1) * sizeof(WCHAR));
                ULONG totalSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + dataSize;
                
                // Если буфер слишком мал, укорачиваем GUID
                if (Length < totalSize && Length >= 86) {
                    // Вычисляем, сколько символов GUID можем поместить в буфер
                    ULONG availableDataSize = Length - sizeof(KEY_VALUE_PARTIAL_INFORMATION);
                    ULONG maxChars = availableDataSize / sizeof(WCHAR) - 1; // Оставляем место для нулевого символа
                    
                    // Обрезаем GUID до нужной длины
                    if (maxChars < wideSpoofed.length()) {
                        wideSpoofed = wideSpoofed.substr(0, maxChars);
                        Logger::LogFormat("NtQueryValueKey: Truncated GUID to fit buffer: %S", wideSpoofed.c_str());
                    }
                    
                    dataSize = (ULONG)((wideSpoofed.length() + 1) * sizeof(WCHAR));
                }
                
                // Проверяем, что буфер достаточного размера
                if (Length >= sizeof(KEY_VALUE_PARTIAL_INFORMATION) + dataSize) {
                    // Заменяем данные нашим GUID
                    pInfo->DataLength = dataSize;
            memcpy(pInfo->Data, wideSpoofed.c_str(), wideSpoofed.length() * sizeof(WCHAR));
            ((WCHAR*)pInfo->Data)[wideSpoofed.length()] = L'\0';

                    // Обновляем размер результата
                    *ResultLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + dataSize;
            
                    Logger::LogFormat("NtQueryValueKey: Successfully spoofed GUID: %S", (PWCHAR)pInfo->Data);
            isProcessing = false;
            return STATUS_SUCCESS;
                } else {
                    Logger::LogFormat("NtQueryValueKey: Buffer too small for spoofed data, need %lu, have %lu",
                                    sizeof(KEY_VALUE_PARTIAL_INFORMATION) + dataSize, Length);
                }
            } else {
                Logger::LogFormat("NtQueryValueKey: Unexpected value type: %lu", pInfo->Type);
            }
        }
        
        isProcessing = false;
        return originalStatus;
    }
    
    // Для всех остальных случаев вызываем оригинальную функцию
    NTSTATUS status = OriginalNtQueryValueKey(KeyHandle, ValueName, KeyValueInformationClass,
        KeyValueInformation, Length, ResultLength);
    
    isProcessing = false;
    return status;
}

// Перехваченная функция CoCreateInstance для WMI
HRESULT WINAPI HookedCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    LPVOID* ppv
) {
    // CLSID для WMI
    static const CLSID CLSID_WbemLocator = 
        {0x4590f811,0x1d3a,0x11d0,{0x89,0x1f,0x00,0xaa,0x00,0x4b,0x2e,0x24}};
    
    if (IsEqualCLSID(rclsid, CLSID_WbemLocator)) {
        Logger::Log("WMI access detected - intercepting WbemLocator creation");
    }
    
    return OriginalCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

// Перехваченная функция ReadFile
BOOL WINAPI HookedReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
) {
    // Вызываем оригинальную функцию
    BOOL result = OriginalReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    
    // Если чтение успешно и есть данные
    if (result && lpNumberOfBytesRead && *lpNumberOfBytesRead > 0) {
        // Получаем имя файла для отладки
        WCHAR filePath[MAX_PATH] = L"";
        if (GetFinalPathNameByHandleW(hFile, filePath, MAX_PATH, FILE_NAME_NORMALIZED) > 0) {
            // Проверяем, связан ли файл с machine-id
            std::wstring pathStr(filePath);
            if (pathStr.find(L"machine-id") != std::wstring::npos || 
                pathStr.find(L"MachineId") != std::wstring::npos ||
                pathStr.find(L"node_modules\\node-machine-id") != std::wstring::npos ||
                pathStr.find(L"temp_machine_id") != std::wstring::npos ||
                pathStr.find(L"temp_node_machine_id") != std::wstring::npos) {
                
                // Логируем информацию о чтении
                Logger::LogFormat("ReadFile: Read %lu bytes from %S", *lpNumberOfBytesRead, filePath);
                
                // Если буфер содержит текст, выводим его
                if (lpBuffer && *lpNumberOfBytesRead > 0) {
                    // Проверяем, является ли содержимое текстом
                    bool isText = true;
                    char* buffer = (char*)lpBuffer;
                    for (DWORD i = 0; i < *lpNumberOfBytesRead && i < 100; i++) {
                        if (buffer[i] != 0 && !isprint(buffer[i]) && buffer[i] != '\r' && buffer[i] != '\n' && buffer[i] != '\t') {
                            isText = false;
                            break;
                        }
                    }
                    
                    if (isText) {
                        // Создаем временную копию для вывода
                        char tempBuffer[101] = {0};
                        DWORD size = min(*lpNumberOfBytesRead, 100);
                        memcpy(tempBuffer, buffer, size);
                        tempBuffer[size] = 0;
                
                        Logger::LogFormat("ReadFile: Content (ASCII): %s", tempBuffer);
                    }
                    
                    // Проверяем, является ли содержимое Unicode текстом
                    if (*lpNumberOfBytesRead >= 2) {
                        bool isUnicode = true;
                        wchar_t* wbuffer = (wchar_t*)lpBuffer;
                        DWORD wcharCount = *lpNumberOfBytesRead / 2;
                        
                        for (DWORD i = 0; i < wcharCount && i < 50; i++) {
                            if (wbuffer[i] != 0 && !iswprint(wbuffer[i]) && wbuffer[i] != L'\r' && wbuffer[i] != L'\n' && wbuffer[i] != L'\t') {
                                isUnicode = false;
                                break;
                            }
                        }
                        
                        if (isUnicode) {
                            // Создаем временную копию для вывода
                            wchar_t wtempBuffer[51] = {0};
                            DWORD wsize = min(wcharCount, 50);
                            memcpy(wtempBuffer, wbuffer, wsize * sizeof(wchar_t));
                            wtempBuffer[wsize] = 0;
                            
                            Logger::LogFormat("ReadFile: Content (Unicode): %S", wtempBuffer);
            }
                    }
                }
            }
        }
    }
    
    return result;
}

// Создание фиктивного файла с данными реестра
std::wstring CreateFakeRegistryFile(bool isNodeMachineId = false) {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    
    std::wstring tempRegFile = std::wstring(tempPath) + L"fake_registry.reg";
    
    // Получаем текущий GUID или SHA-256 хеш
    std::wstring valueToUse;
    if (isNodeMachineId) {
        // Для node-machine-id используем SHA-256 хеш
        std::string hashStr = g_guidGenerator.GetSHA256Hash();
        valueToUse = std::wstring(hashStr.begin(), hashStr.end());
        Logger::LogFormat("Creating fake registry file with SHA-256 hash: %s", hashStr.c_str());
    } else {
        // Для обычных запросов используем GUID
        valueToUse = g_guidGenerator.GetWideGuid();
        Logger::LogFormat("Creating fake registry file with GUID: %S", valueToUse.c_str());
    }
    
    // Формируем содержимое .reg файла
    std::wstring regContent = 
        L"Windows Registry Editor Version 5.00\r\n\r\n"
        L"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography]\r\n"
        L"\"MachineGuid\"=\"" + valueToUse + L"\"\r\n";
    
    // Записываем в файл
    HANDLE hFile = CreateFileW(
        tempRegFile.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hFile, regContent.c_str(), (DWORD)(regContent.length() * sizeof(WCHAR)), &written, NULL);
        CloseHandle(hFile);
        
        return tempRegFile;
    }
    
    return L"";
}

// Создание специального файла для node-machine-id
void CreateNodeMachineIdFile() {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    
    std::wstring nodeIdFile = std::wstring(tempPath) + L"node_machine_id.txt";
    
    // Получаем SHA-256 хеш
    std::string hashStr = g_guidGenerator.GetSHA256Hash();
    
    // Записываем в файл
    HANDLE hFile = CreateFileW(
        nodeIdFile.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hFile, hashStr.c_str(), (DWORD)hashStr.length(), &written, NULL);
        CloseHandle(hFile);
        
        Logger::LogFormat("Created node-machine-id file with SHA-256 hash: %s", hashStr.c_str());
    }
}

// Функция для проверки, является ли запрос связанным с MachineGuid
bool IsMachineGuidQuery(const std::wstring& cmdLine) {
    std::wstring lowerCmdLine = cmdLine;
    std::transform(lowerCmdLine.begin(), lowerCmdLine.end(), lowerCmdLine.begin(), ::tolower);
    
    return (lowerCmdLine.find(L"reg") != std::wstring::npos || 
            lowerCmdLine.find(L"regedit") != std::wstring::npos || 
            lowerCmdLine.find(L"query") != std::wstring::npos) && 
           (lowerCmdLine.find(L"cryptography") != std::wstring::npos || 
            lowerCmdLine.find(L"machineguid") != std::wstring::npos);
}

// Перехваченная функция CreateProcessW
BOOL WINAPI HookedCreateProcessW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
) {
    if (lpCommandLine) {
        std::wstring cmdLine(lpCommandLine);
        std::wstring lowerCmdLine = cmdLine;
        std::transform(lowerCmdLine.begin(), lowerCmdLine.end(), lowerCmdLine.begin(), ::tolower);

        // Проверяем, это запуск REG.exe для получения MachineGuid?
        if (lowerCmdLine.find(L"reg") != std::wstring::npos && 
            lowerCmdLine.find(L"query") != std::wstring::npos && 
            lowerCmdLine.find(L"cryptography") != std::wstring::npos && 
            lowerCmdLine.find(L"machineguid") != std::wstring::npos) {
            
            Logger::LogFormat("CreateProcessW: Intercepted REG.exe query for MachineGuid: %S", lpCommandLine);
            
            // Проверяем, вызывается ли из node-machine-id или electron
            bool isNodeMachineId = (lowerCmdLine.find(L"node") != std::wstring::npos || 
                                  lowerCmdLine.find(L"npm") != std::wstring::npos || 
                                  lowerCmdLine.find(L"electron") != std::wstring::npos);
            
            // Для node-machine-id создаем новый процесс, который выводит наш GUID
            // в формате, ожидаемом node-machine-id
            
            // Создаем временный файл для вывода
            wchar_t tempPath[MAX_PATH];
            GetTempPathW(MAX_PATH, tempPath);
            std::wstring outputFile = std::wstring(tempPath) + L"reg_output.txt";
            
            // Получаем наш GUID или хеш
            std::string valueToUse;
            if (isNodeMachineId) {
                // Для node-machine-id используем GUID (не хеш), так как хеширование выполняется самой библиотекой
                valueToUse = g_guidGenerator.GetCurrentGuid();
                Logger::LogFormat("CreateProcessW: Using GUID for node-machine-id: %s", valueToUse.c_str());
            } else {
                // Для обычных запросов используем GUID
                valueToUse = g_guidGenerator.GetCurrentGuid();
                Logger::LogFormat("CreateProcessW: Using GUID for regular query: %s", valueToUse.c_str());
            }
            
            // Формируем вывод в точном формате REG.exe
            std::string output = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\r\n    MachineGuid    REG_SZ    " + valueToUse + "\r\n\r\n";
            
            // Записываем в файл
            HANDLE hFile = CreateFileW(outputFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                DWORD written;
                WriteFile(hFile, output.c_str(), (DWORD)output.length(), &written, NULL);
                CloseHandle(hFile);
                
                Logger::LogFormat("CreateProcessW: Created fake REG.exe output with value: %s", valueToUse.c_str());
                
                // Если это node-machine-id, также создаем файл с SHA-256 хешем для прямого доступа
                if (isNodeMachineId) {
                    std::string hashValue = g_guidGenerator.GetSHA256Hash();
                    std::wstring hashFile = std::wstring(tempPath) + L"node_machine_id.txt";
                    HANDLE hHashFile = CreateFileW(hashFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hHashFile != INVALID_HANDLE_VALUE) {
                        WriteFile(hHashFile, hashValue.c_str(), (DWORD)hashValue.length(), &written, NULL);
                        CloseHandle(hHashFile);
                        Logger::LogFormat("CreateProcessW: Created node-machine-id hash file with value: %s", hashValue.c_str());
                    }
                }
                
                // Создаем новую команду, которая просто выводит содержимое нашего файла
                std::wstring newCmd = L"cmd.exe /c type \"" + outputFile + L"\"";
                
                // Копируем команду в новый буфер
                wchar_t* newCmdLine = new wchar_t[newCmd.length() + 1];
                wcscpy_s(newCmdLine, newCmd.length() + 1, newCmd.c_str());
                
                // Запускаем новую команду
                BOOL result = OriginalCreateProcessW(
                    NULL,
                    newCmdLine,
                    lpProcessAttributes,
                    lpThreadAttributes,
                    bInheritHandles,
                    dwCreationFlags,
                    lpEnvironment,
                    lpCurrentDirectory,
                    lpStartupInfo,
                    lpProcessInformation
                );
                
                // Освобождаем память
                delete[] newCmdLine;
                
                return result;
            }
        }
    }
    
    // Для всех остальных случаев вызываем оригинальную функцию
    return OriginalCreateProcessW(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation
    );
}

// Перехваченная функция ShellExecuteW
HINSTANCE WINAPI HookedShellExecuteW(
    HWND hwnd,
    LPCWSTR lpOperation,
    LPCWSTR lpFile,
    LPCWSTR lpParameters,
    LPCWSTR lpDirectory,
    INT nShowCmd
) {
    if (lpFile && lpParameters) {
        std::wstring file(lpFile);
        std::wstring params(lpParameters);
        std::wstring lowerFile = file;
        std::wstring lowerParams = params;
        
        std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
        std::transform(lowerParams.begin(), lowerParams.end(), lowerParams.begin(), ::tolower);
        
        // Проверяем, это запуск REG.exe для получения MachineGuid?
        if ((lowerFile.find(L"reg") != std::wstring::npos || lowerFile.find(L"reg.exe") != std::wstring::npos) && 
            lowerParams.find(L"query") != std::wstring::npos && 
            lowerParams.find(L"cryptography") != std::wstring::npos && 
            lowerParams.find(L"machineguid") != std::wstring::npos) {
            
            Logger::LogFormat("ShellExecuteW: Intercepted REG.exe query for MachineGuid: %S %S", lpFile, lpParameters);
            
            // Создаем временный файл с нашим GUID
            wchar_t tempPath[MAX_PATH];
            GetTempPathW(MAX_PATH, tempPath);
            
            std::wstring tempRegOutputFile = std::wstring(tempPath) + L"reg_output.txt";
            
            // Форматируем вывод как у REG.exe
            std::wstring guid = g_guidGenerator.GetWideGuid();
            std::wstring regOutput = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\r\n    MachineGuid    REG_SZ    " + guid + L"\r\n\r\n";
            
            // Записываем в файл
            HANDLE hTemp = CreateFileW(tempRegOutputFile.c_str(), GENERIC_WRITE, 0, NULL,
                                     CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
            
            if (hTemp != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteFile(hTemp, regOutput.c_str(), (DWORD)(regOutput.length() * sizeof(WCHAR)), &written, NULL);
                CloseHandle(hTemp);
                
                Logger::LogFormat("ShellExecuteW: Created fake REG.exe output with GUID: %S", guid.c_str());
                
                // Запускаем команду для отображения нашего файла
                return OriginalShellExecuteW(hwnd, L"open", L"notepad.exe", tempRegOutputFile.c_str(), lpDirectory, nShowCmd);
            }
        }
    }
    
    // Для всех остальных случаев вызываем оригинальную функцию
    return OriginalShellExecuteW(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
}

// Перехваченная функция ShellExecuteExW
BOOL WINAPI HookedShellExecuteExW(SHELLEXECUTEINFOW* pExecInfo) {
    if (pExecInfo && pExecInfo->lpFile && pExecInfo->lpParameters) {
        std::wstring file(pExecInfo->lpFile);
        std::wstring params(pExecInfo->lpParameters);
        std::wstring lowerFile = file;
        std::wstring lowerParams = params;
        
        std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
        std::transform(lowerParams.begin(), lowerParams.end(), lowerParams.begin(), ::tolower);
        
        // Проверяем, это запуск REG.exe для получения MachineGuid?
        if ((lowerFile.find(L"reg") != std::wstring::npos || lowerFile.find(L"reg.exe") != std::wstring::npos) && 
            lowerParams.find(L"query") != std::wstring::npos && 
            lowerParams.find(L"cryptography") != std::wstring::npos && 
            lowerParams.find(L"machineguid") != std::wstring::npos) {
            
            Logger::LogFormat("ShellExecuteExW: Intercepted REG.exe query for MachineGuid: %S %S", 
                           pExecInfo->lpFile, pExecInfo->lpParameters);
            
            // Создаем временный файл с нашим GUID
            wchar_t tempPath[MAX_PATH];
            GetTempPathW(MAX_PATH, tempPath);
            
            std::wstring tempRegOutputFile = std::wstring(tempPath) + L"reg_output.txt";
            
            // Форматируем вывод как у REG.exe
            std::wstring guid = g_guidGenerator.GetWideGuid();
            std::wstring regOutput = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\r\n    MachineGuid    REG_SZ    " + guid + L"\r\n\r\n";
            
            // Записываем в файл
            HANDLE hTemp = CreateFileW(tempRegOutputFile.c_str(), GENERIC_WRITE, 0, NULL,
                                     CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
            
            if (hTemp != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteFile(hTemp, regOutput.c_str(), (DWORD)(regOutput.length() * sizeof(WCHAR)), &written, NULL);
                CloseHandle(hTemp);
                
                Logger::LogFormat("ShellExecuteExW: Created fake REG.exe output with GUID: %S", guid.c_str());
                
                // Модифицируем параметры для запуска команды type
                std::wstring newFile = L"cmd.exe";
                std::wstring newParams = L"/c type \"" + tempRegOutputFile + L"\"";
                
                // Сохраняем оригинальные параметры
                LPCWSTR origFile = pExecInfo->lpFile;
                LPCWSTR origParams = pExecInfo->lpParameters;
                
                // Заменяем параметры
                pExecInfo->lpFile = newFile.c_str();
                pExecInfo->lpParameters = newParams.c_str();
                
                // Вызываем оригинальную функцию
                BOOL result = OriginalShellExecuteExW(pExecInfo);
                
                // Восстанавливаем оригинальные параметры
                pExecInfo->lpFile = origFile;
                pExecInfo->lpParameters = origParams;
                
                return result;
            }
        }
    }
    
    // Для всех остальных случаев вызываем оригинальную функцию
    return OriginalShellExecuteExW(pExecInfo);
}

// Установка хуков
void InstallHooks() {
    Logger::Log("Starting hooks installation");
    
    // Получаем адрес NtQueryValueKey
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        OriginalNtQueryValueKey = (NtQueryValueKey_t)GetProcAddress(ntdll, "NtQueryValueKey");
    }

    // Начинаем транзакцию Detours
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    // Устанавливаем хуки файловой системы
    DetourAttach(&(PVOID&)OriginalCreateFileW, HookedCreateFileW);
    
    // Устанавливаем хуки процессов
    DetourAttach(&(PVOID&)OriginalCreateProcessW, HookedCreateProcessW);
    DetourAttach(&(PVOID&)OriginalShellExecuteW, HookedShellExecuteW);
    DetourAttach(&(PVOID&)OriginalShellExecuteExW, HookedShellExecuteExW);
    
    // Устанавливаем хуки реестра
    DetourAttach(&(PVOID&)OriginalRegQueryValueExW, HookedRegQueryValueExW);
    if (OriginalNtQueryValueKey) {
        DetourAttach(&(PVOID&)OriginalNtQueryValueKey, HookedNtQueryValueKey);
    }
    
    // Устанавливаем хуки WMI
    DetourAttach(&(PVOID&)OriginalCoCreateInstance, HookedCoCreateInstance);
    
    // Устанавливаем хуки чтения файлов
    DetourAttach(&(PVOID&)OriginalReadFile, HookedReadFile);
    
    // Завершаем транзакцию
    LONG error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        Logger::LogFormat("Failed to commit detour transaction: %ld", error);
    } else {
        Logger::Log("All hooks installed successfully");
    }
}

// Удаление хуков
void UninstallHooks() {
    Logger::Log("Starting hooks removal");
    
    // Начинаем транзакцию Detours
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    // Удаляем хуки файловой системы
    DetourDetach(&(PVOID&)OriginalCreateFileW, HookedCreateFileW);
    
    // Удаляем хуки процессов
    DetourDetach(&(PVOID&)OriginalCreateProcessW, HookedCreateProcessW);
    DetourDetach(&(PVOID&)OriginalShellExecuteW, HookedShellExecuteW);
    DetourDetach(&(PVOID&)OriginalShellExecuteExW, HookedShellExecuteExW);
    
    // Удаляем хуки реестра
    DetourDetach(&(PVOID&)OriginalRegQueryValueExW, HookedRegQueryValueExW);
    if (OriginalNtQueryValueKey) {
        DetourDetach(&(PVOID&)OriginalNtQueryValueKey, HookedNtQueryValueKey);
    }
    
    // Удаляем хуки WMI
    DetourDetach(&(PVOID&)OriginalCoCreateInstance, HookedCoCreateInstance);
    
    // Удаляем хуки чтения файлов
    DetourDetach(&(PVOID&)OriginalReadFile, HookedReadFile);
    
    // Завершаем транзакцию
    LONG error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        Logger::LogFormat("Failed to commit detour removal transaction: %ld", error);
    } else {
        Logger::Log("All hooks removed successfully");
    }
}

// Защита от дампинга памяти
void ProtectFromDumping() {
    HMODULE hMod = GetModuleHandle(NULL);
    MEMORY_BASIC_INFORMATION mbi;
    
    if (VirtualQuery(hMod, &mbi, sizeof(mbi))) {
        DWORD oldProtect;
        VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READ, &oldProtect);
        Logger::Log("Memory protection applied");
    }
}

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        // Инициализируем логгер
        Logger::Log("DLL_PROCESS_ATTACH - Starting initialization");

        // Применяем защиту от дампинга памяти
        ProtectFromDumping();
        
        // Устанавливаем хуки
        InstallHooks();
        
        Logger::Log("Initialization complete");
        break;
        
    case DLL_PROCESS_DETACH:
        if (!lpReserved) {  // Если DLL выгружается динамически
            Logger::Log("DLL_PROCESS_DETACH - Starting cleanup");

            // Удаляем хуки
            UninstallHooks();
            
            Logger::Log("Cleanup complete");
            Logger::Close();
        }
        break;
    }
    return TRUE;
}

