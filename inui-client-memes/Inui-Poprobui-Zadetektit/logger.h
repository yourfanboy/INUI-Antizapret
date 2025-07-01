#pragma once
#include "common.h"
 
// Функции логирования
std::string GetCurrentTimeStamp();
std::string GetProcessInfo();
void SafeLogToFile(const std::string& message);
std::string WideToUtf8(const std::wstring& wstr); 