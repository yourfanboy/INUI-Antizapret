#include <Windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <TlHelp32.h>

// Получение полного пути к DLL
std::wstring GetDllPath(const std::wstring& dllName) {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    return exePath.parent_path() / dllName;
}

// Функция для выбора исполняемого файла
std::wstring SelectExecutable() {
    wchar_t filename[MAX_PATH];
    filename[0] = '\0';

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = L"Executable Files\0*.exe\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Select Application to Launch";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&ofn)) {
        return L"";
    }

    return filename;
}

// Функция для инжекта DLL
bool InjectDll(DWORD processId, const std::wstring& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::wcout << L"Failed to open target process. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Выделяем память в процессе
    SIZE_T pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!pDllPath) {
        std::wcout << L"Failed to allocate memory in target process. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // Записываем путь к DLL
    if (!WriteProcessMemory(hProcess, pDllPath, dllPath.c_str(), pathSize, NULL)) {
        std::wcout << L"Failed to write to process memory. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Получаем адрес LoadLibraryW
    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    if (!pLoadLibrary) {
        std::wcout << L"Failed to get LoadLibraryW address. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Создаем удаленный поток
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibrary, pDllPath, 0, NULL);
    if (!hThread) {
        std::wcout << L"Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Ждем завершения
    WaitForSingleObject(hThread, INFINITE);

    // Проверяем результат
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    if (exitCode == 0) {
        std::wcout << L"Failed to load DLL (thread exit code 0)" << std::endl;
    }

    // Очищаем
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return exitCode != 0;
}

// Ждем пока процесс проинициализируется
void WaitForProcessInitialization(DWORD processId) {
    std::wcout << L"Waiting for process initialization..." << std::endl;
    Sleep(3000); // Ждем 3 секунды (можно настроить по необходимости)
}

int main() {
    // Получаем пути к DLL
    std::wstring spooferDllPath = GetDllPath(L"inui-realtime-spoofer.dll");
    std::wstring additionalDllPath = GetDllPath(L"abcdefg.dll");

    // Проверяем основную DLL
    if (!std::filesystem::exists(spooferDllPath)) {
        std::wcout << L"Error: Spoofer DLL not found at: " << spooferDllPath << std::endl;
        system("pause");
        return 1;
    }

    // Выбираем EXE
    std::wstring exePath = SelectExecutable();
    if (exePath.empty()) {
        std::wcout << L"No file selected." << std::endl;
        system("pause");
        return 1;
    }

    // Создаем процесс
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    // Добавляем параметр -no-sandbox к командной строке
    std::wstring cmdLine = L"\"" + exePath + L"\" -no-sandbox";
    std::vector<wchar_t> cmdLineBuffer(cmdLine.begin(), cmdLine.end());
    cmdLineBuffer.push_back(0);

    if (!CreateProcessW(
        NULL,
        cmdLineBuffer.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        std::wcout << L"Failed to create process. Error: " << GetLastError() << std::endl;
        system("pause");
        return 1;
    }

    // Инжектим первую DLL в приостановленный процесс
    std::wcout << L"Injecting Spoofer DLL..." << std::endl;
    if (!InjectDll(pi.dwProcessId, spooferDllPath)) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        system("pause");
        return 1;
    }

    // Запускаем процесс
    ResumeThread(pi.hThread);
    std::wcout << L"Process started. PID: " << pi.dwProcessId << std::endl;

    // Ждем инициализации
    WaitForProcessInitialization(pi.dwProcessId);

    // Пробуем инжектить вторую DLL
    if (std::filesystem::exists(additionalDllPath)) {
        std::wcout << L"Attempting to inject Additional DLL..." << std::endl;
        if (!InjectDll(pi.dwProcessId, additionalDllPath)) {
            std::wcout << L"Warning: Failed to inject Additional DLL" << std::endl;
        }
        else {
            std::wcout << L"Additional DLL injected successfully!" << std::endl;
        }
    }
    else {
        std::wcout << L"Additional DLL not found, skipping: " << additionalDllPath << std::endl;
    }

    // Закрываем хендлы
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::wcout << L"Done!" << std::endl;
    system("pause");
    return 0;
}