#include "stat_reader.h"

namespace transport::detail::stat {

std::pair<std::string_view, std::string_view> ParseRequest(std::string_view string) {
    const auto start = string.find_first_not_of(' '); // Находит первый не пробельный символ
    if (start == string.npos) {
        return { "", "" }; // Если строка пустая
    }

    const auto end = string.find(' ', start); // Находит первый пробел после команды

    if (end == string.npos) {
        return { string.substr(start), "" }; // Если нет пробелов, возвращает только команду
    }

    // Команда от начала до первого пробела, аргумент - от первого пробела до конца
    return { string.substr(start, end - start), string.substr(end + 1) };
}

void ParseAndPrintStat(const Catalogue& transport_catalogue, std::string_view request, std::ostream& output) {
    std::string_view comand = ParseRequest(request).first;
    std::string_view name = ParseRequest(request).second;

    if (comand == "Bus") {
        detail::bus::Bus* bus = transport_catalogue.FindBus(name);
        if (bus) {
            bus::Info info = transport_catalogue.GetBusInfo(bus);
            output << "Bus " << bus->name << ": "
                << info.total_stops << " stops on route, "
                << info.unique_stops << " unique stops, "
                << std::fixed << std::setprecision(6) << info.route_length << " route length"
                << std::endl;
        }
        else {
            output << "Bus " << name << ": not found" << std::endl;
        }
    }
    else {
        Stop* stop = transport_catalogue.FindStop(name);
        if (stop) {
            auto buses = transport_catalogue.GetBusesByStop(stop->name);
            if (!buses.empty()) {
                output << "Stop " << stop->name << ": buses";
                for (const auto& bus_name : buses) {
                    output << " " << bus_name->name; 
                }
                output << std::endl;
            }
            else {
                output << "Stop " << stop->name << ": no buses" << std::endl;
            }
        }
        else {
            output << "Stop " << name << ": not found" << std::endl;
        }
    }
}

} // end namespace transport::detail::stat