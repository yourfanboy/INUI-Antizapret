# INUI Imitat0r :: t.me/inuistaff

---

Что ж самое первое, что я написал после небольшого реверса `gameWindowCapture.dll` из папки с клиентом INUI. 

Античит на уровне ребят. Фейсит учись!

Исходя из названия уже можно понять, что это имитирует "бурную" деятельность клиента, а именно раз в 10 секунд скринит окно контар стрика).

---

### Как сие чудо работает?

Мы берем `gameWindowCapture.dll` и загружаем через LoadLibraryW в память:

```cpp
    // Load 64-bit DLL
    HMODULE hDll = LoadLibraryW(dllPath.wstring().c_str());
    if (!hDll) {
        std::cout << "Error loading DLL. Error code: " << GetLastError() << std::endl;
        return 1;
       }
```

Определяем тип для функции:

```cpp
// Define type for take_screenshot function from DLL
using TakeScreenshotFunc = int(*)(const wchar_t*, unsigned char*, int, int);
```

Далее, получаем адрес экспортнутой функции `take_screenshot()` в памяти:

```cpp
    // Get function address
    auto takeScreenshot = reinterpret_cast<TakeScreenshotFunc>(
        GetProcAddress(hDll, "take_screenshot")
    );
```

Ура, получили, что дальше? Дальше создаем буфер для картинки:

```cpp
    // Create image buffer (1280 * 720 * 4 bytes per pixel)
    constexpr int32_t WIDTH = 1280;
    constexpr int32_t HEIGHT = 720;
    constexpr size_t BUFFER_SIZE = static_cast<size_t>(WIDTH) * HEIGHT * 4;

    std::vector<uint8_t> imageBuffer(BUFFER_SIZE);
```

Создаем функцию для сохранения скриншота в `RAW` формате:

```cpp
void SaveScreenshot(const std::vector<uint8_t>& buffer) {
    std::ofstream file("screenshot.raw", std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    }
}
```

Далее, вызываем фукнцию внутри цикла (раз в 10 секунд):

```cpp
int result = takeScreenshot(
    L"Counter-Strike: Global Offensive - Direct3D 9",
    imageBuffer.data(),
    WIDTH,
    HEIGHT
);
```

Пихаем обработку результатов, модуль возвращает 0 если у него получилось сделать скрин, вот когда получаем 0 - тогда и будем сейвить скрин:

```cpp
switch (result) {
    case 0:
        std::cout << "Screenshot captured successfully" << std::endl;
        SaveScreenshot(imageBuffer);
    break;
}
```

---

Далее просто открываем скрин в фотошопе, выставляем разрешение 1280х720 и 4 слоя и всё.

---

Телеграм со всякими приколами: https://t.me/inuistaff
