# INUI screenshot bypass :: t.me/inuistaff

---

## Устарел. Разрабы решили прикольнуться и накрыли дллку VMProtect'ом, в следствие чего стало впадлу ковырять его.

---

Что ж мои маленькие, устали что вашу игру скринят раз в 10 секунд? Выход есть!

Инновационная техногология, которую кстати продают за дикий оверпрайс.

<img src="https://i.imgur.com/9Emz37U.png" title="" alt="https://i.imgur.com/9Emz37U.png" width="433">

Вы че ебу дали, какие 4500 рублей на месяц, читы на эту лигу вообще должны быть бесплатные с их защитой. Но что-то мы отошли от темы.

---

### Как сие чудо работает:

У клиента INUI есть модуль `gameWindowCapture.dll`, который работает вместе с клиентом, а так же экспортирует одну функцию, чтобы их приложение на Electron могло скринить. 

Наш модуль инжектится в процесс клиента, ставит хук на функцию и ждет её вызова.

```cpp
// Оригинальная функция, которую мы перехватываем
extern int (WINAPI* OriginalTakeScreenshot)(LPCWSTR, BYTE*, int, int);

// Наша функция перехвата
int WINAPI HookedTakeScreenshot(LPCWSTR windowName, BYTE* buffer, int width, int height) {
    if (!g_isHookActive || !OriginalTakeScreenshot) {
        return -1;
    }

    // Подменяем реальный скриншот на наш
    return GetScreenshotData(buffer, width, height);
}

// Установка хука
void InstallHook() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)OriginalTakeScreenshot, HookedTakeScreenshot);
    DetourTransactionCommit();
}
```

Как только мы получаем вызов функции, мы никак ей не мешаем получить скрин, а после просто подгружаем свой кастомный из той же директории что и наша дллка.

```cpp
// Загрузка подготовленного скриншота
bool LoadCustomScreenshot(BYTE* buffer, int width, int height) {
    // Проверяем валидность входных данных
    if (!buffer || width <= 0 || height <= 0) {
        return false;
    }

    // Читаем заранее подготовленный файл
    std::ifstream file(CUSTOM_SCREENSHOT_PATH, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Проверяем размеры
    int stored_width = 0, stored_height = 0;
    file.read(reinterpret_cast<char*>(&stored_width), sizeof(stored_width));
    file.read(reinterpret_cast<char*>(&stored_height), sizeof(stored_height));

    // Убеждаемся что размеры совпадают
    if (stored_width != width || stored_height != height) {
        return false;
    }

    // Загружаем данные изображения
    file.read(reinterpret_cast<char*>(buffer), width * height * 4);
    return true;
}

// Сохранение "чистого" скриншота для последующего использования
bool SaveScreenshotRaw(const std::string& filename, BYTE* buffer, int width, int height) {
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    // Сохраняем размеры изображения
    file.write(reinterpret_cast<char*>(&width), sizeof(width));
    file.write(reinterpret_cast<char*>(&height), sizeof(height));

    // Сохраняем данные изображения
    file.write(reinterpret_cast<char*>(buffer), width * height * 4);
    return true;
}
```

---

Работает это всё с `RAW` изображениями, так что ищите редактор. 
Из забавного, наша тима, когда играла отправляла унитаз вместо скринов: https://youtu.be/jb-gwYBl9b0

Кстати скрины отправляются POST запросом на:

```
https://api.inuisoftware.con/api/v1/anticheat/screenshot-wallhack
```

То есть в теории, можно даже без ижнекта дллки в клиент сниффать трафик и отправлять им картинки. 

---

Телеграм со всякими приколами: https://t.me/inuistaff
