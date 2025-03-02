#include "transport_catalogue.h"

namespace transport {

void Catalogue::AddStop(detail::Stop stop) {
     auto& ref = stops_.emplace_back(std::move(stop.name), stop.coordinates.lat, stop.coordinates.lng);
     stopname_to_stop_[ref.name] = &ref;
}

void Catalogue::AddBus(detail::bus::Bus bus) {
    // Помещение оригинала автобуса в список (постоянное хранилище)
    auto& ref = buses_.emplace_back(std::move(bus.name), bus.stops, bus.final_stops, bus.is_roundtrip);

    // Добавление текущего автобуса ко всем остановкам, через которые он проезжает
    for (const auto stop : bus.stops) {
        stop_to_buses_[stop->name].insert(&ref);
    }

    // Добавление автобуса в ассоциативный словарь для поиска по имени
    busname_to_bus_[ref.name] = &ref;

    detail::bus::Info info;
    double fact_route_length = 0.0;
    double line_route_length = 0.0;
    std::vector<detail::Stop*>& route = bus.stops;

    // Уникальные остановки
    std::unordered_set<detail::Stop*> unique_stops(route.begin(), route.end());

    // Расчёт длины маршрута по координатам
    for (size_t i = 0; i < route.size() - 1; ++i) {
        geo::Coordinates first = {(*route[i]).coordinates.lat, (*route[i]). coordinates.lng};
        geo::Coordinates second = {(*route[i + 1]).coordinates.lat, (*route[i+ 1]).coordinates.lng};
        line_route_length += ComputeDistance(first, second);
    }

    // Рассчёт длины маршрута по заданным пользователем значениям
    for (size_t i = 0; i < route.size() - 1; ++i) {
        fact_route_length += GetDistance(route[i], route[i + 1]);
    }

    info.total_stops = route.size();
    info.unique_stops = unique_stops.size();
    info.route_length = fact_route_length;
    info.curvature = fact_route_length / line_route_length;

    // Добавление автобуса и информации о нем 
    bus_to_info_[&ref] = info;
}

const std::deque<detail::bus::Bus>& Catalogue::GetBuses() const {
    return buses_;
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