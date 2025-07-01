// inui-imitat0r.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace std::chrono_literals;

// Make sure we're compiling for x64
#if !defined(_WIN64)
    #error "This code must be compiled for x64 architecture"
#endif

// Define type for take_screenshot function from DLL
using TakeScreenshotFunc = int(*)(const wchar_t*, unsigned char*, int, int);

// Use std::atomic for thread safety
static std::atomic<bool> isRunning = true;
static std::atomic<bool> isCapturing = false;

void SaveScreenshot(const std::vector<uint8_t>& buffer) {
    std::ofstream file("screenshot.raw", std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    }
}

void CheckKeyPress() noexcept {
    while (isRunning.load(std::memory_order_relaxed)) {
        if (GetAsyncKeyState(VK_F6) & 0x8000) {
            isRunning.store(false, std::memory_order_release);
        }
        else if (GetAsyncKeyState(VK_F5) & 0x8000) {
            isCapturing.store(!isCapturing.load(std::memory_order_relaxed), std::memory_order_release);
            std::cout << (isCapturing.load(std::memory_order_acquire) ? "Capture started" : "Capture paused") << std::endl;
            std::this_thread::sleep_for(200ms); // Debounce key press
        }
        std::this_thread::sleep_for(100ms);
    }
}

[[nodiscard]] std::filesystem::path GetDllPath() {
    return std::filesystem::current_path() / "gameWindowCapture.dll";
}

int main() {
    // Check if DLL exists
    const auto dllPath = GetDllPath();
    if (!std::filesystem::exists(dllPath)) {
        std::cout << "Error: gameWindowCapture.dll not found in directory: " 
                  << dllPath << std::endl;
        return 1;
    }

    // Load 64-bit DLL
    HMODULE hDll = LoadLibraryW(dllPath.wstring().c_str());
    if (!hDll) {
        std::cout << "Error loading DLL. Error code: " << GetLastError() << std::endl;
        return 1;
    }

    // Get function address
    auto takeScreenshot = reinterpret_cast<TakeScreenshotFunc>(
        GetProcAddress(hDll, "take_screenshot")
    );
    
    if (!takeScreenshot) {
        std::cout << "Error getting function address. Error code: " << GetLastError() << std::endl;
        FreeLibrary(hDll);
        return 1;
    }

    // Create image buffer (1280 * 720 * 4 bytes per pixel)
    constexpr int32_t WIDTH = 1280;
    constexpr int32_t HEIGHT = 720;
    constexpr size_t BUFFER_SIZE = static_cast<size_t>(WIDTH) * HEIGHT * 4;
    
    std::vector<uint8_t> imageBuffer(BUFFER_SIZE);

    // Start thread for F5 and F6 key monitoring
    std::thread keyCheckThread(CheckKeyPress);

    std::cout << "Press F5 to start/pause capture, F6 to exit" << std::endl;

    auto lastCaptureTime = std::chrono::steady_clock::now();
    while (isRunning.load(std::memory_order_acquire)) {
        if (isCapturing.load(std::memory_order_acquire)) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastCaptureTime);
            
            if (elapsedTime.count() >= 10) {
                int result = takeScreenshot(
                    L"Counter-Strike: Global Offensive - Direct3D 9",
                    imageBuffer.data(),
                    WIDTH,
                    HEIGHT
                );

                switch (result) {
                    case 0:
                        std::cout << "Screenshot captured successfully" << std::endl;
                        SaveScreenshot(imageBuffer);
                        break;
                    case 1:
                        std::cout << "Error capturing screenshot" << std::endl;
                        break;
                    case 2:
                        std::cout << "Retry needed" << std::endl;
                        break;
                    case 3:
                        std::cout << "Unknown error" << std::endl;
                        break;
                    default:
                        std::cout << "Unexpected return code: " << result << std::endl;
                }
                
                lastCaptureTime = currentTime;
            }
        }
        
        std::this_thread::sleep_for(100ms);
    }

    // Wait for key monitoring thread to finish
    if (keyCheckThread.joinable()) {
        keyCheckThread.join();
    }

    // Unload DLL
    FreeLibrary(hDll);
    std::cout << "Program terminated" << std::endl;
    return 0;
}
