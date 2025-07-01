#pragma once
#include "common.h"
#include "window_capture.h"

// Функции для работы со скриншотами
bool LoadCustomScreenshot(BYTE* buffer, int width, int height);
bool SaveScreenshotRaw(const std::string& filename, BYTE* buffer, int width, int height);
bool GetScreenshotData(BYTE* buffer, int width, int height);

// Прототипы функций хука
int WINAPI HookedTakeScreenshot(LPCWSTR windowName, BYTE* buffer, int width, int height);
extern int (WINAPI* OriginalTakeScreenshot)(LPCWSTR, BYTE*, int, int); 