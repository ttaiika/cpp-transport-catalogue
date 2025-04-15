#include "request_handler.h"
#include <sstream>

RequestHandler::RequestHandler(const transport::Catalogue& catalogue,
    const renderer::MapRenderer& renderer,
    const transport::router::TransportRouter& router)
    : catalogue_(catalogue)
    , renderer_(renderer)
    , router_(router) {}

std::optional<transport::detail::bus::Info>
RequestHandler::GetBusInfo(const std::string_view& bus_name) const {
    auto bus = catalogue_.FindBus(bus_name);

    return bus ? std::make_optional(catalogue_.GetBusInfo(bus)) : std::nullopt;
}

std::optional<const std::set<transport::detail::bus::Bus*, transport::detail::bus::PtrComparator>>
RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    auto stop = catalogue_.FindStop(stop_name);

    return stop ? std::make_optional(catalogue_.GetBusesByStop(stop->name)) : std::nullopt;
}

std::string RequestHandler::RenderMap() const {
    std::ostringstream render;
    renderer_.Render(render);

    return render.str();
}

std::optional<transport::router::RouteInfo> RequestHandler::FindRoute(std::string_view stop_name_from,
    std::string_view stop_name_to) const {
    const transport::detail::Stop* from = catalogue_.FindStop(stop_name_from);
    const transport::detail::Stop* to = catalogue_.FindStop(stop_name_to);
    if (from != nullptr && to != nullptr) {
        return router_.FindRoute(from, to);
    }
    return std::nullopt;
}