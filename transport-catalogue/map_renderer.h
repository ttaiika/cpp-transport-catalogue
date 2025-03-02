#pragma once

#include "svg.h"
#include "transport_catalogue.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <array>
#include <map>

namespace map_renderer {

struct RenderSettings {
    double width = 0;
    double height = 0;
    double padding = 0;
    double line_width = 0;
    double stop_radius = 0;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset{};
    int stop_label_font_size = 0;
    svg::Point stop_label_offset{};
    svg::Color underlayer_color{ std::string("white") };
    double underlayer_width = 0;
    std::vector<svg::Color> color_palette;
    std::string stop_label_font_family{ std::string("Verdana") };
    std::string bus_label_font_family{ std::string("Verdana") };
    svg::Color stop_label_color{ std::string("black") };
};


//Для корректного проецирования координат земной поверхности в координаты на карте
class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    // выводит координаты остановок конкретного маршрута
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding);

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer() = default;
    MapRenderer(RenderSettings render_settings, std::vector<transport::detail::bus::Bus*> buses);

    void Render(std::ostream& out) const;
private:
    const svg::Color& GetBusLineColor(size_t index) const;

    void DrawBuses(svg::Document& doc) const;
    void DrawBusesNames(svg::Document& doc) const;
    void DrawStopsCircle(svg::Document& doc) const;
    void DrawStopsNames(svg::Document& doc) const;

    struct LexicCompareByName {
        bool operator()(transport::detail::Stop* lhs, transport::detail::Stop* rhs) const;
    };

    RenderSettings render_settings_;
    std::vector<transport::detail::bus::Bus*> buses_;
    //хранит имя остановки и проецируемую точку
    std::map<transport::detail::Stop*, svg::Point, LexicCompareByName> stops_positions_;
};

template <typename Iterator>
MapRenderer RenderMap(Iterator begin, Iterator end, RenderSettings settings_) {
    std::vector<transport::detail::bus::Bus*> buses;

    while (begin != end) {
        buses.emplace_back(&(*begin));
        ++begin;
    }

    return { settings_, std::move(buses) };
}

} // end namespace map_renderer