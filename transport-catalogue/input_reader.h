#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "transport_catalogue.h"
#include "geo.h"

namespace transport::detail {

struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;      // Название команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

namespace input {

class Reader {
public:
    /**
     * Парсит строку в структуру CommandDescription и сохраняет результат в commands_
     */
    void ParseLine(std::string_view line);

    /**
     * Наполняет данными транспортный справочник, используя команды из commands_
     */
    void ApplyCommands(Catalogue& catalogue) const;

private:
    std::vector<CommandDescription> commands_;
};

} // end namespace input
} // end namespace transport::detail