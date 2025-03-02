#include "domain.h"

namespace transport {
namespace detail {

Stop::Stop(std::string stop_name, double latitude, double longitude)
    : name(std::move(stop_name)), coordinates({ latitude, longitude }) {}

namespace bus {

Bus::Bus(std::string bus_name, std::vector<Stop*> bus_stops, std::vector<Stop*> fin_stops, bool is_round)
    : name(std::move(bus_name))
    , stops(std::move(bus_stops)) 
    , final_stops(std::move(fin_stops)) 
    , is_roundtrip(is_round)
{    
}

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
} // end namespace transport