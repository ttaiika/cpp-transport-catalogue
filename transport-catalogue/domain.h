#pragma once

#include <string>
#include <vector>

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
    Bus(std::string bus_name, std::vector<Stop*> bus_stops, std::vector<Stop*> fin_stops, bool is_round);
    bool operator<(Bus& other);

    std::string name;
    std::vector<Stop*> stops; 
    std::vector<Stop*> final_stops; 
    bool is_roundtrip;
};

struct Info {
    size_t total_stops{};
    size_t unique_stops{};
    double route_length{};
    double curvature{};
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
} // end namespace transport