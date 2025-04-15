#pragma once

#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

namespace transport::router {

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0;
};

struct RouteInfo {
    double total_time;

    struct WaitItem {
        const detail::Stop* stop;
        int time = 0;
    };

    struct BusItem {
        const detail::bus::Bus* bus;
        double time = 0;
        size_t span_count = 0;
    };

    using Item = std::variant<BusItem, WaitItem>;
    std::vector<Item> items;
};

class TransportRouter {
public:
    struct StopVertexIds {
        graph::VertexId in;
        graph::VertexId out;
    };

    struct BusEdge {
        const detail::bus::Bus* bus;
        size_t span_count;
    };

    TransportRouter() = default;
    TransportRouter(RoutingSettings settings, const Catalogue& catalogue);

    std::optional<RouteInfo> FindRoute(const detail::Stop* from, const detail::Stop* to) const;

private:
    void AddStopsToGraph(const Catalogue& catalogue);
    void AddBusesToGraph(const Catalogue& catalogue);

    RoutingSettings settings_;
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::unordered_map<const detail::Stop*, StopVertexIds, detail::Hasher> stops_vertex_ids_;
    std::vector<const detail::Stop*> vertexes_;
    std::vector<std::optional<BusEdge>> edges_;
};

}  // namespace transport::router