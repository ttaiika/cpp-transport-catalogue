#pragma once
#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"

#include <optional>
#include <string_view>
#include <unordered_set>

class RequestHandler {
public:
    RequestHandler(const transport::Catalogue& catalogue,
        const renderer::MapRenderer& renderer,
        const transport::router::TransportRouter& router);

    // Возвращает информацию о маршруте (запрос Bus)
    [[nodiscard]] std::optional<transport::detail::bus::Info>
        GetBusInfo(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через остановку (запрос Stop)
    [[nodiscard]] std::optional<const std::set<transport::detail::bus::Bus*, transport::detail::bus::PtrComparator>>
        GetBusesByStop(const std::string_view& stop_name) const;

    // Рендерит карту маршрутов
    std::string RenderMap() const;

    // Возвращает описание маршрута
    [[nodiscard]] std::optional<transport::router::RouteInfo>
        FindRoute(std::string_view stop_from, std::string_view stop_to) const;

private:
    const transport::Catalogue& catalogue_;
    const renderer::MapRenderer& renderer_;
    const transport::router::TransportRouter& router_;
};