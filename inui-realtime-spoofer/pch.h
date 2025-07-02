// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// Сначала определяем WIN32_NO_STATUS перед включением windows.h
#ifndef WIN32_NO_STATUS
    #define WIN32_NO_STATUS
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// После windows.h можно определить статусы
#undef WIN32_NO_STATUS
#include <ntstatus.h>

// Затем включаем остальные Windows-специфичные заголовки
#include <winternl.h>
#include <detours.h>
#include <wbemidl.h>
#include <objbase.h>
#include <wincrypt.h> // Для криптографии

// Стандартные C++ заголовки
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <vector>

// Линковка библиотек
#pragma comment(lib, "detours.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "crypt32.lib") // Для криптографии

// Native API определения
typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation = 0,
    KeyNodeInformation = 1,
    KeyFullInformation = 2,
    KeyNameInformation = 3,
    KeyCachedInformation = 4,
    KeyFlagsInformation = 5,
    KeyVirtualizationInformation = 6,
    KeyHandleTagsInformation = 7,
    KeyTrustInformation = 8,
    KeyLayerInformation = 9,
    MaxKeyInfoClass = 10
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation = 0,
    KeyValueFullInformation = 1,
    KeyValuePartialInformation = 2,
    KeyValueFullInformationAlign64 = 3,
    KeyValuePartialInformationAlign64 = 4,
    KeyValueLayerInformation = 5,
    MaxKeyValueInfoClass = 6
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#endif

#endif // PCH_H
