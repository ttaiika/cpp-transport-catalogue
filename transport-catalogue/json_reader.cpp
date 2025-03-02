#include "json_reader.h"

namespace transport {
namespace filler {

void AddStopsToDataBase(Catalogue& catalogue, json::Array& data, std::map<std::string, json::Dict>& distances) {
	using namespace std::string_literals;

	for (const json::Node& node : data) {
		json::Dict req_map = node.AsMap();
		std::string req_type = req_map.at("type"s).AsString();

		if (req_type == "Stop"s) {
			std::string name = req_map.at("name"s).AsString();
			double latitude = req_map.at("latitude"s).AsDouble();
			double longitude = req_map.at("longitude"s).AsDouble();
			distances[name] = req_map.at("road_distances"s).AsMap();

			catalogue.AddStop({ name, latitude, longitude });
		}		
	}
}

void AddDistancesToDataBase(Catalogue& catalogue, std::map<std::string, json::Dict>& distances) {
	for (const auto& [stop_a, map_distances] : distances) {
		auto a = catalogue.FindStop(stop_a);
		for (const auto& [stop_b, distance] : map_distances) {
			auto b = catalogue.FindStop(stop_b);
			auto stops = std::make_pair(a, b);
			catalogue.SetDistance(stops, distance.AsInt());
		}
	}
}

void AddRoutesToDataBase(Catalogue& catalogue, json::Array& data) {
	using namespace std::string_literals;

	for (const json::Node& node : data) {
		json::Dict req_map = node.AsMap();
		std::string req_type = req_map.at("type"s).AsString();

		if (req_type == "Bus"s) {
			std::string name = req_map.at("name"s).AsString();
			json::Array stops = req_map.at("stops"s).AsArray();
			bool is_roundtrip = req_map.at("is_roundtrip").AsBool();
			
			std::vector<detail::Stop*> route;
			std::vector<detail::Stop*> final_route;

			for (const json::Node& stop : stops) {
				route.push_back(catalogue.FindStop(stop.AsString()));
			}
			
			// определение конечных остановок
			final_route.push_back(route.front());
			if (route.front() != route.back()) {
				final_route.push_back(route.back());
			}

			// Приведение маршрута вида "stop1 - stop2 - ... stopN" 
			// к виду "stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1"
			if (!is_roundtrip) {
				route.insert(route.end(), route.rbegin() + 1, route.rend());
			}
			
			catalogue.AddBus({ name, route, final_route, is_roundtrip});
		}
	}
}

// строим базу данных автобусных маршрутов
void FillDataBase(Catalogue& catalogue, json::Array& data) { //принимаем ЧТО будем заполнять и ЧЕМ заполнять 
	 std::map<std::string, json::Dict> distances;

	 // добавление остановок в базу данных
	 AddStopsToDataBase(catalogue, data, distances);
	 // добавление расстояний в базу данных
	 AddDistancesToDataBase(catalogue, distances);
	 // добавление автобусов (маршрутов) в базу данных
	 AddRoutesToDataBase(catalogue, data);
}

} // namespace filler

namespace renderer {
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

// заполняем настройки визуализации карты
map_renderer::RenderSettings ReadRenderSettings(const json::Dict& data) {
	using namespace std::string_literals;

	map_renderer::RenderSettings render_settings;

	render_settings.width = data.at("width"s).AsDouble();
	render_settings.height = data.at("height"s).AsDouble();
	render_settings.padding = data.at("padding"s).AsDouble();
	render_settings.line_width = data.at("line_width"s).AsDouble();
	render_settings.stop_radius = data.at("stop_radius"s).AsDouble();
	render_settings.bus_label_font_size = data.at("bus_label_font_size"s).AsInt();
	render_settings.bus_label_offset = ReadPoint(data.at("bus_label_offset"s).AsArray());
	render_settings.stop_label_font_size = data.at("stop_label_font_size"s).AsInt();
	render_settings.stop_label_offset = ReadPoint(data.at("stop_label_offset"s).AsArray());
	render_settings.underlayer_color = ReadColor(data.at("underlayer_color"s));
	render_settings.underlayer_width = data.at("underlayer_width"s).AsDouble();
	render_settings.color_palette = ReadColorPalette(data.at("color_palette"s).AsArray());

	return render_settings;
}

} // namespace renderer

namespace printer {

json::Dict PrintStops(Catalogue& catalogue, const std::string& name) {
	using namespace std::string_literals;

	json::Dict result;

	if (!catalogue.FindStop(name)) {
		result["error_message"s] = { "not found"s };
		return result;
	}

	const auto& buses = catalogue.GetBusesByStop(name);
	json::Array buses_array;

	for (const auto& bus : buses) {
		buses_array.push_back(bus->name);
	}

	result["buses"s] = buses_array;

	return result;
}

json::Dict PrintBuses(Catalogue& catalogue, const std::string& name) {
	using namespace std::string_literals;

	json::Dict result;

	detail::bus::Bus* bus = catalogue.FindBus(name);
	if (!bus) {
		result["error_message"s] = { "not found"s };
		return result;
	}

	detail::bus::Info info = catalogue.GetBusInfo(bus);
	result["curvature"s] = info.curvature;
	result["route_length"s] = info.route_length;
	result["stop_count"s] = static_cast<int>(info.total_stops);
	result["unique_stop_count"s] = static_cast<int>(info.unique_stops);

	return result;
}

json::Dict PrintMap(Catalogue& catalogue, map_renderer::RenderSettings render_setting) {
	using namespace std::string_literals;

	auto buses = catalogue.GetBuses();
	map_renderer::MapRenderer map_renderer = map_renderer::RenderMap(buses.begin(), buses.end(), render_setting);

	json::Dict result;
	std::ostringstream render;

	map_renderer.Render(render);
	result["map"] = render.str();

	return result;
}

// Обрабатывает запросы и выводит результаты на экран 
void ProcessQueries(Catalogue& catalogue, json::Array& data, map_renderer::RenderSettings render_setting, std::ostream& out) {
	using namespace std::string_literals;

	json::Array result_json;

	// Обработка массива запросов
	for (const json::Node& request : data) {
		json::Dict response;

		auto map_req = request.AsMap();
		std::string type = map_req.at("type").AsString();
		std::string name;

		response["request_id"s] = map_req.at("id"s).AsInt();

		if (type == "Stop"s) {
			name = map_req.at("name"s).AsString();
			response.merge(PrintStops(catalogue, name));
		}
		else if (type == "Bus"s) {
			name = map_req.at("name"s).AsString();
			response.merge(PrintBuses(catalogue, name));
		}
		else if (type == "Map"s) {
			response.merge(PrintMap(catalogue, render_setting));
		}
		result_json.push_back(response);
	}
	json::Print(json::Document(result_json), out);
}

} // namespace printer
} // end namespace transport