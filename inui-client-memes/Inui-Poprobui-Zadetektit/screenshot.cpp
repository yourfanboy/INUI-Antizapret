#include "screenshot.h"
#include "logger.h"

// Определение указателя на оригинальную функцию
int (WINAPI* OriginalTakeScreenshot)(LPCWSTR, BYTE*, int, int) = nullptr;

bool LoadCustomScreenshot(BYTE* buffer, int width, int height) {
    try {
        SafeLogToFile("=== LoadCustomScreenshot START ===");
        SafeLogToFile("Requested dimensions: " + std::to_string(width) + "x" + std::to_string(height));
        
        if (!buffer) {
            SafeLogToFile("ERROR: Buffer is NULL");
            return false;
        }

        if (!std::filesystem::exists(CUSTOM_SCREENSHOT_PATH)) {
            SafeLogToFile("ERROR: Custom screenshot file not found at: " + CUSTOM_SCREENSHOT_PATH.string());
            return false;
        }

        uintptr_t fileSize = std::filesystem::file_size(CUSTOM_SCREENSHOT_PATH);
        size_t expectedSize = sizeof(int) * 2 + static_cast<size_t>(width) * height * 4;
        
        SafeLogToFile("File size check:");
        SafeLogToFile(" - File size: " + std::to_string(fileSize) + " bytes");
        SafeLogToFile(" - Expected minimum size: " + std::to_string(expectedSize) + " bytes");
        
        if (fileSize < sizeof(int) * 2) {
            SafeLogToFile("ERROR: File is too small to contain header information");
            return false;
        }

        std::ifstream file(CUSTOM_SCREENSHOT_PATH, std::ios::binary);
        if (!file.is_open()) {
            SafeLogToFile("ERROR: Failed to open custom screenshot file");
            return false;
        }

        int stored_width = 0, stored_height = 0;
        file.read(reinterpret_cast<char*>(&stored_width), sizeof(stored_width));
        if (file.fail()) {
            SafeLogToFile("ERROR: Failed to read width from file");
            return false;
        }
        
        file.read(reinterpret_cast<char*>(&stored_height), sizeof(stored_height));
        if (file.fail()) {
            SafeLogToFile("ERROR: Failed to read height from file");
            return false;
        }

        SafeLogToFile("Dimensions in file:");
        SafeLogToFile(" - Stored width: " + std::to_string(stored_width));
        SafeLogToFile(" - Stored height: " + std::to_string(stored_height));
        SafeLogToFile(" - Required width: " + std::to_string(width));
        SafeLogToFile(" - Required height: " + std::to_string(height));

        if (stored_width <= 0 || stored_height <= 0) {
            SafeLogToFile("ERROR: Invalid dimensions in file (negative or zero)");
            return false;
        }

        if (stored_width > 32768 || stored_height > 32768) {
            SafeLogToFile("ERROR: Unreasonable dimensions in file (too large)");
            return false;
        }

        if (stored_width != width || stored_height != height) {
            SafeLogToFile("ERROR: Dimensions mismatch");
            return false;
        }

        size_t expectedDataSize = static_cast<size_t>(stored_width) * stored_height * 4;
        if (fileSize < expectedDataSize + sizeof(int) * 2) {
            SafeLogToFile("ERROR: File is too small to contain the full image data");
            SafeLogToFile(" - Expected size: " + std::to_string(expectedDataSize + sizeof(int) * 2) + " bytes");
            SafeLogToFile(" - Actual size: " + std::to_string(fileSize) + " bytes");
            return false;
        }

        file.read(reinterpret_cast<char*>(buffer), expectedDataSize);
        if (file.fail()) {
            SafeLogToFile("ERROR: Failed to read image data");
            return false;
        }

        std::stringstream hexDump;
        hexDump << "First 32 bytes of image data: ";
        for (int i = 0; i < 32 && i < width * height * 4; ++i) {
            hexDump << std::hex << std::setw(2) << std::setfill('0') 
                   << static_cast<int>(buffer[i]) << " ";
        }
        SafeLogToFile(hexDump.str());

        SafeLogToFile("=== LoadCustomScreenshot SUCCESS ===");
        return true;
    }
    catch (const std::exception& e) {
        SafeLogToFile("EXCEPTION in LoadCustomScreenshot: " + std::string(e.what()));
        return false;
    }
    catch (...) {
        SafeLogToFile("UNKNOWN EXCEPTION in LoadCustomScreenshot");
        return false;
    }
}

bool SaveScreenshotRaw(const std::string& filename, BYTE* buffer, int width, int height) {
    if (!buffer) {
        SafeLogToFile("Error: Buffer is NULL");
        return false;
    }

    if (width <= 0 || height <= 0) {
        SafeLogToFile("Error: Invalid dimensions - width: " + std::to_string(width) +
            ", height: " + std::to_string(height));
        return false;
    }

    try {
        std::filesystem::create_directories(SCREENSHOTS_PATH);
        SafeLogToFile("Created/verified directory: " + SCREENSHOTS_PATH.string());
    }
    catch (const std::exception& e) {
        SafeLogToFile("Failed to create directory: " + std::string(e.what()));
        return false;
    }

    SafeLogToFile("Saving screenshot:");
    SafeLogToFile(" - Resolution: " + std::to_string(width) + "x" + std::to_string(height));
    SafeLogToFile(" - Buffer size: " + std::to_string(width * height * 4) + " bytes");
    SafeLogToFile(" - Target file: " + filename);

    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        SafeLogToFile("Failed to open file: " + filename + 
                     " (Error: " + std::to_string(GetLastError()) + ")");
        return false;
    }

    file.write(reinterpret_cast<char*>(&width), sizeof(width));
    if (file.fail()) {
        SafeLogToFile("Failed to write width");
        file.close();
        return false;
    }

    file.write(reinterpret_cast<char*>(&height), sizeof(height));
    if (file.fail()) {
        SafeLogToFile("Failed to write height");
        file.close();
        return false;
    }

    std::streamsize totalSize = static_cast<std::streamsize>(width) * height * 4;
    file.write(reinterpret_cast<char*>(buffer), totalSize);
    if (file.fail()) {
        SafeLogToFile("Failed to write image data");
        file.close();
        return false;
    }

    file.close();
    if (file.fail()) {
        SafeLogToFile("Failed to close file properly");
        return false;
    }

    if (!std::filesystem::exists(filename)) {
        SafeLogToFile("File was not created");
        return false;
    }

    auto fileSize = std::filesystem::file_size(filename);
    SafeLogToFile("Screenshot saved successfully:");
    SafeLogToFile(" - File size: " + std::to_string(fileSize) + " bytes");
    SafeLogToFile(" - Expected size: " + std::to_string(totalSize + 8) + " bytes");
    SafeLogToFile(" - Location: " + filename);

    std::stringstream hexDump;
    hexDump << "First 32 bytes of data: ";
    int maxBytes = (width * height * 4 < 32) ? width * height * 4 : 32;
    for (int i = 0; i < maxBytes; ++i) {
        hexDump << std::setw(2) << std::setfill('0') << std::hex 
               << static_cast<int>(buffer[i]) << " ";
    }
    SafeLogToFile(hexDump.str());

    return true;
}

bool GetScreenshotData(BYTE* buffer, int width, int height) {
    if (!buffer || width <= 0 || height <= 0) {
        SafeLogToFile("Invalid parameters in GetScreenshotData");
        return false;
    }

    SafeLogToFile("Loading custom screenshot");
    return LoadCustomScreenshot(buffer, width, height);
}

int WINAPI HookedTakeScreenshot(LPCWSTR windowName, BYTE* buffer, int width, int height) {
    if (!g_isHookActive || !OriginalTakeScreenshot) {
        SafeLogToFile("Hook called but not active!");
        return -1;
    }

    try {
        if (!buffer || width <= 0 || height <= 0) {
            SafeLogToFile("Invalid parameters in hook");
            return -1;
        }

        std::wstringstream callParams;
        callParams << L"Called take_screenshot("
                  << L"windowName=" << (windowName ? windowName : L"NULL") << L", "
                  << L"buffer=0x" << std::hex << reinterpret_cast<uintptr_t>(buffer) << L", "
                  << std::dec
                  << L"width=" << width << L", "
                  << L"height=" << height << L")";
        SafeLogToFile(WideToUtf8(callParams.str()));

        if (GetScreenshotData(buffer, width, height)) {
            SafeLogToFile("Screenshot data obtained successfully");
            return 0;
        }
        else {
            SafeLogToFile("Failed to get screenshot data, falling back to original function");
            int result = OriginalTakeScreenshot(windowName, buffer, width, height);
            SafeLogToFile("take_screenshot (original) returned " + std::to_string(result));
            return result;
        }
    }
    catch (const std::exception& e) {
        SafeLogToFile("Exception in hooked function: " + std::string(e.what()));
        return -1;
    }
    catch (...) {
        SafeLogToFile("Unknown exception in hooked function");
        return -1;
    }
} 