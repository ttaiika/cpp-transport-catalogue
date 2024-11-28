#pragma once

#include <deque>
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <set>

#include "geo.h"

namespace transport {
namespace detail {

struct Stop {
    Stop() = default;
    Stop(std::string stop_name, double latitude, double longitude);

    std::string name;
    geo::Coordinates coordinates;
};

namespace bus {

struct Bus {
    Bus() = default;
    Bus(std::string bus_name, std::vector<Stop*> bus_stops);
    bool operator<(Bus& other);

    std::string name;
    std::vector<Stop*> stops; 
};

struct Info {
    size_t total_stops{};
    size_t unique_stops{};
    double route_length{};
};

struct PtrComparator {
    bool operator()(Bus* lhs, Bus* rhs) const;
};

} //end namespace bus

struct Hasher {
    size_t operator()(const std::string_view& str) const;
    size_t operator()(const std::pair<Stop*, Stop*>& stops) const;
    size_t operator()(const bus::Bus* bus) const;
    size_t operator()(const Stop* stop) const;
};

} // end namespace detail

class Catalogue {
public:
    void AddStop(detail::Stop stop);
    void AddBus(detail::bus::Bus bus);
    detail::Stop* FindStop(std::string_view name) const;
    detail::bus::Bus* FindBus(std::string_view name) const;
    detail::bus::Info GetBusInfo(detail::bus::Bus* bus) const;

    const std::set<detail::bus::Bus*, detail::bus::PtrComparator>& GetBusesByStop(std::string_view name) const;
    double GetDistance(detail::Stop* a, detail::Stop* b) const;

private:
    std::deque<detail::bus::Bus> buses_;
    std::deque<detail::Stop> stops_;
    std::unordered_map<std::string_view, detail::bus::Bus*, detail::Hasher> busname_to_bus_;
    std::unordered_map<std::string_view, detail::Stop*, detail::Hasher> stopname_to_stop_;

    std::unordered_map<std::string_view, std::set<detail::bus::Bus*, detail::bus::PtrComparator>> stop_to_buses_; // Остановка и автобусы, проезжающие через нее
    std::unordered_map<detail::bus::Bus*, detail::bus::Info, detail::Hasher> bus_to_info_; // Информация о каждом автобусе
    std::unordered_map<std::pair<detail::Stop*, detail::Stop*>, double, detail::Hasher> distances_between_stops_; // Расстояние между двумя остановками
};

} // end namespace transport