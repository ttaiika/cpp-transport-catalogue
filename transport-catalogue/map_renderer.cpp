#include "map_renderer.h"
#include <algorithm>
#include <iostream>

namespace map_renderer { 

const svg::Color& MapRenderer::GetBusLineColor(size_t index) const {
    using namespace std::string_literals;

    static const svg::Color default_color = svg::Color{ "black"s };
    const auto& palette = render_settings_.color_palette;

    return !palette.empty() ? palette[index % palette.size()] : default_color;
}

MapRenderer::MapRenderer(const RenderSettings& render_settings, const std::vector<transport::detail::bus::Bus*>& buses)
    : render_settings_(std::move(render_settings))
    , buses_(std::move(buses)) {
    std::sort(buses_.begin(), buses_.end(), [](transport::detail::bus::Bus* lhs, transport::detail::bus::Bus* rhs) {
        return lhs->name < rhs->name;
        });

    // заполняет мн-во всеми остановками
    std::unordered_set<transport::detail::Stop*> stops;
    for (const auto& bus : buses_) {
        stops.insert(bus->stops.begin(), bus->stops.end());
    }

    std::vector<geo::Coordinates> positions;
    positions.reserve(stops.size());

    for (const auto& stop : stops) {
        positions.emplace_back(stop->coordinates);
    }

    SphereProjector sphere_projector(positions.begin(), positions.end(),
        render_settings_.width, render_settings_.height, render_settings_.padding);

    for (const auto& stop : stops) {
        stops_positions_[stop] = sphere_projector(stop->coordinates);
    }

}

bool MapRenderer::LexicCompareByName::operator()(transport::detail::Stop* lhs, transport::detail::Stop* rhs) const {
    return lhs->name < rhs->name;
}

void MapRenderer::DrawBuses(svg::Document& doc) const {
    int color_index = 0;

    for (const auto& bus : buses_) {
        auto& stops = bus->stops;

        if (stops.empty()) {
            continue;  // Пропускаем автобусы без остановок
        }

        svg::Polyline polyline;
        polyline.SetStrokeColor(GetBusLineColor(color_index++))
            .SetStrokeWidth(render_settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
            .SetFillColor(svg::NoneColor);

        for (const auto& stop : stops) {
            polyline.AddPoint(stops_positions_.at(stop));
        }

        doc.Add(std::move(polyline));
    }
}

void MapRenderer::DrawBusesNames(svg::Document& doc) const {
    using namespace std::string_literals;
    int color_index = 0;

    for (const auto& bus : buses_) {
        if (bus->stops.empty()) {
            continue;  // Пропускаем автобусы без остановок
        }

        //1 конечная остановка
        auto stop = bus->final_stops.front();
        auto& bus_color = GetBusLineColor(color_index);

        auto base = svg::Text()
            .SetPosition(stops_positions_.at(stop))
            .SetOffset(render_settings_.bus_label_offset)
            .SetFontSize(render_settings_.bus_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetFontWeight("bold"s)
            .SetData(bus->name);

        doc.Add(svg::Text{ base }
            .SetFillColor(render_settings_.underlayer_color)
            .SetStrokeColor(render_settings_.underlayer_color)
            .SetStrokeWidth(render_settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));

        doc.Add(svg::Text{ base }.SetFillColor(bus_color));

        //2 конечная остановка
        if (!bus->is_roundtrip && stop != bus->final_stops.back()) {
            auto stop2 = bus->final_stops.back();
            auto& bus_color2 = GetBusLineColor(color_index);

            auto base2 = svg::Text()
                .SetPosition(stops_positions_.at(stop2))
                .SetOffset(render_settings_.bus_label_offset)
                .SetFontSize(render_settings_.bus_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFontWeight("bold"s)
                .SetData(bus->name);

            doc.Add(svg::Text{ base2 }
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));

            doc.Add(svg::Text{ base2 }.SetFillColor(bus_color2));
            
        }
        color_index++;
    }
}

void MapRenderer::DrawStopsCircle(svg::Document& doc) const {
    using namespace std::string_literals;

    for (const auto& [key, position] : stops_positions_) {
        svg::Circle circle;
        circle.SetCenter(position)
            .SetRadius(render_settings_.stop_radius)
            .SetFillColor("white"s);

        doc.Add(std::move(circle));
    }
}

void MapRenderer::DrawStopsNames(svg::Document& doc) const {
    using namespace std::string_literals;

    for (const auto& [key, position] : stops_positions_) {
        auto base = svg::Text()
            .SetPosition(position)
            .SetOffset(render_settings_.stop_label_offset)
            .SetFontSize(render_settings_.stop_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetData(key->name);

        doc.Add(svg::Text{ base }
            .SetFillColor(render_settings_.underlayer_color)
            .SetStrokeColor(render_settings_.underlayer_color)
            .SetStrokeWidth(render_settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));

        doc.Add(svg::Text{ base }.SetFillColor("black"s));
    }
}

void MapRenderer::Render(std::ostream& out) const {
    svg::Document doc;
    DrawBuses(doc);
    DrawBusesNames(doc);
    DrawStopsCircle(doc);
    DrawStopsNames(doc);
    doc.Render(out);
}

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
    double max_width, double max_height, double padding)
    : padding_(padding)
{
    if (points_begin == points_end) {
        return;
    }

    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    // Вычисляем коэффициенты масштабирования
    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    }
    else if (width_zoom) {
        zoom_coeff_ = *width_zoom;
    }
    else if (height_zoom) {
        zoom_coeff_ = *height_zoom;
    }
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    double x = (coords.lng - min_lon_) * zoom_coeff_ + padding_;
    double y = (max_lat_ - coords.lat) * zoom_coeff_ + padding_;

    return { x, y };
}

} // end namespace map_renderer