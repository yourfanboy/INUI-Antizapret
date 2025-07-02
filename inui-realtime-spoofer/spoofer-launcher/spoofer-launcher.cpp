// spoofer-launcher.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <TlHelp32.h>

// Получение полного пути к нашей DLL
std::wstring GetDllPath() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    return exePath.parent_path() / L"inui-realtime-spoofer.dll";
}

// Функция для выбора исполняемого файла через диалог
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

// Инжект DLL в процесс
bool InjectDll(HANDLE hProcess, const std::wstring& dllPath) {
    // Выделяем память в целевом процессе для пути к DLL
    SIZE_T pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, pathSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!pDllPath) {
        std::wcout << L"Failed to allocate memory in target process\n";
        return false;
    }

    // Записываем путь к DLL в память процесса
    if (!WriteProcessMemory(hProcess, pDllPath, dllPath.c_str(), 
        pathSize, NULL)) {
        std::wcout << L"Failed to write to process memory\n";
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        return false;
    }

    // Получаем адрес LoadLibraryW
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        std::wcout << L"Failed to get kernel32.dll handle\n";
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        return false;
    }

    LPTHREAD_START_ROUTINE pLoadLibrary = 
        (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
    
    if (!pLoadLibrary) {
        std::wcout << L"Failed to get LoadLibraryW address\n";
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        return false;
    }

    // Создаем удаленный поток для загрузки DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        pLoadLibrary, pDllPath, 0, NULL);
    
    if (!hThread) {
        std::wcout << L"Failed to create remote thread\n";
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        return false;
    }

    // Ждем завершения загрузки
    WaitForSingleObject(hThread, INFINITE);

    // Очищаем
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);

    return true;
}

int main() {
    // Получаем путь к DLL
    std::wstring dllPath = GetDllPath();
    if (!std::filesystem::exists(dllPath)) {
        std::wcout << L"Error: DLL not found at: " << dllPath << std::endl;
        system("pause");
        return 1;
    }

    // Выбираем исполняемый файл
    std::wstring exePath = SelectExecutable();
    if (exePath.empty()) {
        std::wcout << L"No file selected.\n";
        system("pause");
        return 1;
    }

    // Создаем процесс в приостановленном состоянии
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    std::wstring cmdLine = exePath;
    std::vector<wchar_t> cmdLineBuffer(cmdLine.begin(), cmdLine.end());
    cmdLineBuffer.push_back(0); // Null terminator

    if (!CreateProcessW(
        NULL,                   // No module name (use command line)
        cmdLineBuffer.data(),   // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        CREATE_SUSPENDED,       // Create suspended
        NULL,                   // Use parent's environment block
        NULL,                   // Use parent's starting directory 
        &si,                    // Pointer to STARTUPINFO structure
        &pi                     // Pointer to PROCESS_INFORMATION structure
    )) {
        std::wcout << L"Failed to create process. Error: " << GetLastError() << std::endl;
        system("pause");
        return 1;
    }

    // Инжектим DLL
    std::wcout << L"Injecting DLL...\n";
    if (!InjectDll(pi.hProcess, dllPath)) {
        std::wcout << L"Failed to inject DLL\n";
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        system("pause");
        return 1;
    }

    // Возобновляем процесс
    ResumeThread(pi.hThread);

    std::wcout << L"Process started and DLL injected successfully!\n";

    // Закрываем хендлы
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    system("pause");
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
