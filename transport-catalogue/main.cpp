#include "json_reader.h"
#include "request_handler.h"

using namespace std;

int main() {
    transport::Catalogue catalogue;

    json::Array json;
    json::Dict json_render_settings;
    json::Dict json_router_settings;
    transport::router::RoutingSettings routing_settings;
    renderer::RenderSettings render_settings;

    auto doc = json::Load(std::cin).GetRoot().AsDict();

    transport::reader::JSONReader json_reader(catalogue);

    // Заполнение базы данных
    if (doc.find("base_requests"s) != doc.end()) {
        json = doc.at("base_requests"s).AsArray();
        json_reader.FillDataBase(json);
    }

    // Заполнение настроек визуализации карты
    if (doc.find("render_settings"s) != doc.end()) {
        json_render_settings = doc.at("render_settings"s).AsDict();
        render_settings = json_reader.ReadRenderSettings(json_render_settings);
    }

    // Заполнение bus_wait_time и bus_velocity
    if (doc.find("routing_settings"s) != doc.end()) {
        json_router_settings = doc.at("routing_settings"s).AsDict();
        routing_settings = json_reader.ReadRoutingSettings(json_router_settings);
    }

    // Обработка запросов и печать результатов
    if (doc.find("stat_requests"s) != doc.end()) {
        auto buses = catalogue.GetBuses();
        renderer::MapRenderer map_renderer(renderer::RenderMap(buses.begin(), buses.end(), render_settings));

        transport::router::TransportRouter transport_router(routing_settings, catalogue);

        RequestHandler handler(catalogue, map_renderer, transport_router);

        json = doc.at("stat_requests"s).AsArray();
        json_reader.ProcessQueries(json, handler, std::cout);
    }
}