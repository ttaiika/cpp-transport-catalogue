#include "json_reader.h"
#include <sstream>

namespace transport {

JSONReader::JSONReader(Catalogue& catalogue)
	: catalogue_(catalogue)
{
}

void JSONReader::AddStopsToDataBase(json::Array& data, std::map<std::string, json::Dict>& distances) {
	using namespace std::string_literals;

	for (const json::Node& node : data) {
		json::Dict req_map = node.AsDict();
		std::string req_type = req_map.at("type"s).AsString();

		if (req_type == "Stop"s) {
			std::string name = req_map.at("name"s).AsString();
			double latitude = req_map.at("latitude"s).AsDouble();
			double longitude = req_map.at("longitude"s).AsDouble();
			distances[name] = req_map.at("road_distances"s).AsDict();

			catalogue_.AddStop({ name, latitude, longitude });
		}
	}
}

void JSONReader::AddDistancesToDataBase(std::map<std::string, json::Dict>& distances) {
	for (const auto& [stop_a, map_distances] : distances) {
		auto a = catalogue_.FindStop(stop_a);
		for (const auto& [stop_b, distance] : map_distances) {
			auto b = catalogue_.FindStop(stop_b);
			auto stops = std::make_pair(a, b);
			catalogue_.SetDistance(stops, distance.AsInt());
		}
	}
}

void JSONReader::AddRoutesToDataBase(json::Array& data) {
	using namespace std::string_literals;

	for (const json::Node& node : data) {
		json::Dict req_map = node.AsDict();
		std::string req_type = req_map.at("type"s).AsString();

		if (req_type == "Bus"s) {
			std::string name = req_map.at("name"s).AsString();
			//std::cout << "Bus " << name << std::endl;
			json::Array stops = req_map.at("stops"s).AsArray();
			bool is_roundtrip = req_map.at("is_roundtrip").AsBool();

			std::vector<detail::Stop*> route;
			std::vector<detail::Stop*> final_route;

			for (const json::Node& stop : stops) {
				route.push_back(catalogue_.FindStop(stop.AsString()));
			}

			// Определение конечных остановок
			final_route.push_back(route.front());
			if (route.front() != route.back()) {
				final_route.push_back(route.back());
			}

			// Приведение маршрута вида "stop1 - stop2 - ... stopN" 
			// к виду "stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1"
			if (!is_roundtrip) {
				route.insert(route.end(), route.rbegin() + 1, route.rend());
			}
			catalogue_.AddBus({ name, route, final_route, is_roundtrip });		
		}
	}
}

// Строит базу данных автобусных маршрутов
void JSONReader::FillDataBase(json::Array& data) {
	std::map<std::string, json::Dict> distances;

	// добавление остановок в базу данных
	AddStopsToDataBase(data, distances);
	// добавление расстояний в базу данных
	AddDistancesToDataBase(distances);
	// добавление автобусов (маршрутов) в базу данных
	AddRoutesToDataBase(data);
}

svg::Point ReadPoint(const json::Array& arr) {
	return { arr[0].AsDouble(), arr[1].AsDouble() };
}

svg::Color ReadColor(const json::Node& node) {
	if (node.IsString()) {
		return node.AsString();
	}
	else if (node.IsArray()) {
		json::Array arr = node.AsArray();

		if (arr.size() == 3) {
			return svg::Rgb(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt());
		}
		else if (arr.size() == 4) {
			return svg::Rgba(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt(), arr[3].AsDouble());
		}
	}
	return svg::NoneColor;
}

std::vector<svg::Color> ReadColorPalette(const json::Array& arr) {
	std::vector<svg::Color> colors;
	colors.reserve(arr.size());

	for (const auto& el : arr) {
		colors.emplace_back(ReadColor(el));
	}

	return colors;
}

// Заполняет настройки визуализации карты
void JSONReader::ReadRenderSettings(const json::Dict& data) {
	using namespace std::string_literals;

	render_settings_.width = data.at("width"s).AsDouble();
	render_settings_.height = data.at("height"s).AsDouble();
	render_settings_.padding = data.at("padding"s).AsDouble();
	render_settings_.line_width = data.at("line_width"s).AsDouble();
	render_settings_.stop_radius = data.at("stop_radius"s).AsDouble();
	render_settings_.bus_label_font_size = data.at("bus_label_font_size"s).AsInt();
	render_settings_.bus_label_offset = ReadPoint(data.at("bus_label_offset"s).AsArray());
	render_settings_.stop_label_font_size = data.at("stop_label_font_size"s).AsInt();
	render_settings_.stop_label_offset = ReadPoint(data.at("stop_label_offset"s).AsArray());
	render_settings_.underlayer_color = ReadColor(data.at("underlayer_color"s));
	render_settings_.underlayer_width = data.at("underlayer_width"s).AsDouble();
	render_settings_.color_palette = ReadColorPalette(data.at("color_palette"s).AsArray());
}

json::Dict JSONReader::PrintStops(const std::string& name) const {
	using namespace std::string_literals;

	json::Dict result;

	if (!catalogue_.FindStop(name)) {
		result["error_message"s] = { "not found"s };
		return result;
	}

	const auto& buses = catalogue_.GetBusesByStop(name);
	json::Array buses_array;

	for (const auto& bus : buses) {
		buses_array.push_back(bus->name);
	}

	result["buses"s] = buses_array;

	return result;
}

json::Dict JSONReader::PrintBuses(const std::string& name) const {
	using namespace std::string_literals;

	json::Dict result;

	detail::bus::Bus* bus = catalogue_.FindBus(name);
	if (!bus) {
		result["error_message"s] = { "not found"s };
		return result;
	}

	detail::bus::Info info = catalogue_.GetBusInfo(bus);
	result["curvature"s] = info.curvature;
	result["route_length"s] = info.route_length;
	result["stop_count"s] = static_cast<int>(info.total_stops);
	result["unique_stop_count"s] = static_cast<int>(info.unique_stops);

	return result;
}

json::Dict JSONReader::PrintMap() const {
	using namespace std::string_literals;

	auto buses = catalogue_.GetBuses();
	map_renderer::MapRenderer map_renderer(map_renderer::RenderMap(buses.begin(), buses.end(), render_settings_));

	json::Dict result;
	std::ostringstream render;

	map_renderer.Render(render);
	result["map"] = render.str();

	return result;
}

void JSONReader::ProcessQueries(json::Array& data, std::ostream& out) const {
    using namespace std::string_literals;

    json::Array result_json;

    for (const json::Node& request : data) {
        auto map_req = request.AsDict();
        int request_id = map_req.at("id"s).AsInt();
        std::string type = map_req.at("type"s).AsString();

        json::Builder builder;
        builder.StartDict().Key("request_id").Value(request_id);

        if (type == "Stop"s) {
            std::string name = map_req.at("name"s).AsString();
            json::Dict stop_info = PrintStops(name);

            if (stop_info.count("error_message"s)) {
                builder.Key("error_message").Value("not found"s);
            } else {
                builder.Key("buses").Value(stop_info.at("buses").AsArray());
            }
        } 
        else if (type == "Bus"s) {
            std::string name = map_req.at("name"s).AsString();
            json::Dict bus_info = PrintBuses(name);

            if (bus_info.count("error_message"s)) {
                builder.Key("error_message").Value("not found"s);
            } else {
                builder.Key("curvature").Value(bus_info.at("curvature").AsDouble())
                       .Key("route_length").Value(bus_info.at("route_length").AsDouble())
                       .Key("stop_count").Value(bus_info.at("stop_count").AsInt())
                       .Key("unique_stop_count").Value(bus_info.at("unique_stop_count").AsInt());
            }
        } 
        else if (type == "Map"s) {
            builder.Key("map").Value(PrintMap().at("map").AsString());
        }
		result_json.push_back(json::Node(json::Dict(builder.EndDict().Build().AsDict())));
    }

    json::Print(json::Document(result_json), out);
}

} // end namespace transport
