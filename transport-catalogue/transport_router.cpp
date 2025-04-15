#include "transport_router.h"

namespace transport::router {

TransportRouter::TransportRouter(RoutingSettings settings, const Catalogue& catalogue)
	: settings_(settings) {
	const auto& stops = catalogue.GetStops();
	const size_t vertex_count = stops.size() * 2;  // По две вершины на остановку
	graph_ = graph::DirectedWeightedGraph<double>(vertex_count);
	vertexes_.resize(vertex_count);

	AddStopsToGraph(catalogue);
	AddBusesToGraph(catalogue);

	router_ = std::make_unique<graph::Router<double>>(graph_);
}

std::optional<RouteInfo>
TransportRouter::FindRoute(const detail::Stop* from, const detail::Stop* to) const {
	const graph::VertexId vertex_from = stops_vertex_ids_.at(from).out;
	const graph::VertexId vertex_to = stops_vertex_ids_.at(to).out;
	const auto route = router_->BuildRoute(vertex_from, vertex_to);

	if (!route) {
		return std::nullopt;
	}

	RouteInfo route_info;
	route_info.total_time = route->weight;
	route_info.items.reserve(route->edges.size());

	for (const auto edge_id : route->edges) {
		const auto& edge = graph_.GetEdge(edge_id);
		const auto& bus_edge_info = edges_[edge_id];

		if (bus_edge_info.has_value()) {
			route_info.items.emplace_back(RouteInfo::BusItem{
			  bus_edge_info->bus,
			  edge.weight,
			  bus_edge_info->span_count,
			});
		}
		else {
			const graph::VertexId vertex_id = edge.from;
			route_info.items.emplace_back(RouteInfo::WaitItem{
				vertexes_[vertex_id],
				static_cast<int>(edge.weight),
			});
		}
	}
	return route_info;
}

void TransportRouter::AddStopsToGraph(const Catalogue& cat) {
	graph::VertexId vertex_id = 0;
	const auto& stops = cat.GetStops();

	for (const auto& stop : stops) {
		auto& vertex_ids = stops_vertex_ids_[&stop];

		vertex_ids.in = vertex_id++;
		vertex_ids.out = vertex_id++;
		vertexes_[vertex_ids.in] = &stop;
		vertexes_[vertex_ids.out] = &stop;

		edges_.emplace_back(std::nullopt);
		graph_.AddEdge({
		  vertex_ids.out,
		  vertex_ids.in,
		  static_cast<double> (settings_.bus_wait_time)
		});
	}
}

void TransportRouter::AddBusesToGraph(const Catalogue& cat) {
	const auto& buses = cat.GetBuses();

	for (const auto& bus : buses) {
		const auto& bus_stops = bus.stops;
		const size_t stop_count = bus_stops.size();

		if (stop_count <= 1) {
			continue;
		}

		auto compute_distance_from = [&cat, &bus_stops](size_t stop_idx) {
			return cat.GetDistance(bus_stops[stop_idx], bus_stops[stop_idx + 1]);
			};

		for (size_t start = 0; start < stop_count - 1; ++start) {
			const graph::VertexId begin = stops_vertex_ids_.at(bus_stops[start]).in;
			size_t total_distance = 0;

			for (size_t end = start + 1; end < stop_count; ++end) {
				total_distance += compute_distance_from(end - 1);
				edges_.emplace_back(BusEdge{
					&bus,
					end - start,
				});

				graph_.AddEdge({
					begin,
					stops_vertex_ids_.at(bus_stops[end]).out,
					static_cast<double>(total_distance)
					  / (settings_.bus_velocity * 1000.0 / 60)
				});
			}
		}
	}
}

} // namespace transport::router