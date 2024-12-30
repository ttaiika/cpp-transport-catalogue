#include "transport_catalogue.h"

namespace transport {
namespace detail {

Stop::Stop(std::string stop_name, double latitude, double longitude)
    : name(std::move(stop_name)), coordinates({ latitude, longitude }) {}

namespace bus {

Bus::Bus(std::string bus_name, std::vector<Stop*> bus_stops) : name(std::move(bus_name)),
    stops(std::move(bus_stops)) {}

bool Bus::operator<(Bus& other)
{
    return std::lexicographical_compare(name.begin(), name.end(),
        other.name.begin(), other.name.end());
}

bool PtrComparator::operator()(Bus* lhs, Bus* rhs) const {
    return *lhs < *rhs;
}

} //end namespace bus

size_t Hasher::operator()(const std::string_view& str) const {
    return std::hash<std::string>()(std::string(str));
}

size_t Hasher::operator()(const std::pair<Stop*, Stop*>& stops) const {
    size_t hash1 = std::hash<Stop*>()(stops.first);
    size_t hash2 = std::hash<Stop*>()(stops.second);
    return hash1 ^ (hash2 << 1); // XOR для комбинирования хешей
}

size_t Hasher::operator()(const bus::Bus* bus) const {
    return std::hash<std::string_view>()(bus->name);
}

size_t Hasher::operator()(const Stop* stop) const {
    return std::hash<std::string_view>()(stop->name);
}

} // end namespace detail

void Catalogue::AddStop(detail::Stop stop) {
     auto& ref = stops_.emplace_back(std::move(stop.name), stop.coordinates.lat, stop.coordinates.lng);
     stopname_to_stop_[ref.name] = &ref;
}

void Catalogue::AddBus(detail::bus::Bus bus) {
    // Помещение оригинала автобуса в список (постоянное хранилище)
    auto& ref = buses_.emplace_back(std::move(bus.name), bus.stops);

    // Добавление текущего автобуса ко всем остановкам, через которые он проезжает
    for (const auto& stop : bus.stops) {
        stop_to_buses_[stop->name].insert(&ref);
    }

    // Добавление автобуса в ассоциативный словарь для поиска по имени
    busname_to_bus_[ref.name] = &ref;

    detail::bus::Info info;
    double route_length = 0.0;
    std::vector<detail::Stop*>& route = ref.stops;

    // Уникальные остановки
    std::unordered_set<detail::Stop*> unique_stops(route.begin(), route.end());

    // Расчёт длины маршрута по координатам
    for (size_t i = 0; i < route.size(); ++i) {
        const auto& current_stop = route[i];
        const auto& next_stop = route[(i + 1) % route.size()]; 
        if (i < route.size() - 1 || route.size() != 2) {
            route_length += ComputeDistance(current_stop->coordinates, next_stop->coordinates);
        }
    }

    double bus_length = 0;
    // Рассчёт длины маршрута по заданным пользователем значениям
    for (size_t i = 0; i < route.size() - 1; ++i) {
        bus_length += GetDistance(route[i], route[i + 1]);
    }

    info.total_stops = route.size();
    info.unique_stops = unique_stops.size();
    info.route_length = bus_length;
    info.curvature = bus_length / route_length;

    // Добавление автобуса и информации о нем 
    bus_to_info_[&ref] = info;
}

// Получает расстояние между двумя остановками
double Catalogue::GetDistance(detail::Stop* a, detail::Stop* b) const {
    // Расстояние от А до Б
    auto it = distances_between_stops_.find({ a, b });

    if (it == distances_between_stops_.end()) {
        // Расстояние от Б до А
        it = distances_between_stops_.find({ b, a });
        if (it == distances_between_stops_.end()) {
            return -1;
        }
    }

    return it->second;
}
    
void Catalogue::SetDistance(const std::pair<detail::Stop*, detail::Stop*>& stops, double distance) {
    distances_between_stops_[stops] = distance;
}

detail::Stop* Catalogue::FindStop(const std::string_view name) const {
    auto it = stopname_to_stop_.find(name);

    if (it != stopname_to_stop_.end()) {
        return it->second;
    }

    return nullptr;
}

detail::bus::Bus* Catalogue::FindBus(const std::string_view name) const {
    auto it = busname_to_bus_.find(name);

    if (it != busname_to_bus_.end()) {
        return it->second;
    }

    return nullptr;
}

detail::bus::Info Catalogue::GetBusInfo(detail::bus::Bus* bus) const {
    return bus_to_info_.at(bus);
}

// Получает все автобусы, проходящие через остановку
const std::set<detail::bus::Bus*, detail::bus::PtrComparator>& Catalogue::GetBusesByStop(std::string_view name) const {
    static const std::set<detail::bus::Bus*, detail::bus::PtrComparator> empty;
    auto it = stop_to_buses_.find(name);

    if (it != stop_to_buses_.end()) {
        return it->second;
    }
    
    return empty;
}

} // end namespace transport