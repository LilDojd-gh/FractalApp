// SettingsStorage.h — хранение параметров сеанса и пользовательских палитр в текстовых файлах.
#pragma once
#include <string>
#include <vector>

#include "ColorPalette.h"
#include "FractalParams.h"

namespace fractal {

/// Палитра с названием, загруженная из файла пользователя.
struct NamedPalette {
    std::string name;
    ColorPalette palette;
};

/**
 * Формат settings.ini — строки «ключ=значение», строки с # игнорируются.
 * Формат palettes.txt — строки «Название | pos r g b | pos r g b | ...».
 * Пути передаются в UTF-8. Чтение устойчиво к повреждённым файлам:
 * неизвестные или некорректные значения заменяются значениями по умолчанию.
 */
class SettingsStorage {
public:
    static bool save(const FractalParams& p, const std::string& path);
    /// Читает параметры; ok (если задан) = true, когда файл удалось открыть.
    static FractalParams load(const std::string& path, bool* ok = nullptr);
    static std::vector<NamedPalette> loadPalettes(const std::string& path);
};

}  // namespace fractal
