#pragma once

#include <deque>
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <set>

#include "domain.h"

namespace transport {

class Catalogue {
public:
    void AddStop(detail::Stop stop);
    void AddBus(detail::bus::Bus bus);

    detail::Stop* FindStop(std::string_view name) const;
    detail::bus::Bus* FindBus(std::string_view name) const;
    detail::bus::Info GetBusInfo(detail::bus::Bus* bus) const;

    const std::set<detail::bus::Bus*, detail::bus::PtrComparator>& GetBusesByStop(std::string_view name) const;
    const std::deque<detail::bus::Bus>& GetBuses() const;
    double GetDistance(detail::Stop* a, detail::Stop* b) const;
    void SetDistance(const std::pair<detail::Stop*, detail::Stop*>& stops, double distance);

private:
    std::deque<detail::bus::Bus> buses_;
    std::deque<detail::Stop> stops_;
    std::unordered_map<std::string_view, detail::bus::Bus*, detail::Hasher> busname_to_bus_;
    std::unordered_map<std::string_view, detail::Stop*, detail::Hasher> stopname_to_stop_;
    // Остановка и автобусы, проезжающие через нее
    std::unordered_map<std::string_view, std::set<detail::bus::Bus*, detail::bus::PtrComparator>> stop_to_buses_; 
    // Информация о каждом автобусе
    std::unordered_map<detail::bus::Bus*, detail::bus::Info, detail::Hasher> bus_to_info_; 
    // Расстояние между двумя остановками
    std::unordered_map<std::pair<detail::Stop*, detail::Stop*>, double, detail::Hasher> distances_between_stops_; 
};

} // end namespace transport