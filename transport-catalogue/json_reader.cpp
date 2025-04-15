#include "json_reader.h"
#include <sstream>

namespace transport::reader {

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
renderer::RenderSettings JSONReader::ReadRenderSettings(const json::Dict& data) {
	using namespace std::string_literals;

	renderer::RenderSettings render_settings;

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

// Заполняет bus_wait_time и bus_velocity
router::RoutingSettings JSONReader::ReadRoutingSettings(const json::Dict& data) {
	using namespace std::string_literals;

	router::RoutingSettings routing_settings;

	routing_settings.bus_wait_time = data.at("bus_wait_time"s).AsInt();
	routing_settings.bus_velocity = data.at("bus_velocity"s).AsDouble();

	return routing_settings;
}

void JSONReader::PrintStops(const RequestHandler& handler, std::string& name,
	json::Builder& builder) const {
	using namespace std::string_literals;

	auto buses = handler.GetBusesByStop(name);

	if (!buses) {
		builder.Key("error_message"s).Value("not found"s);
		return;
	}

	builder.Key("buses").StartArray();
	for (const auto& bus : *buses) {
		builder.Value(bus->name);
	}
	builder.EndArray();
}

void JSONReader::PrintBuses(const RequestHandler& handler, std::string& name,
	json::Builder& builder) const {
	using namespace std::string_literals;

	auto info = handler.GetBusInfo(name);

	if (!info) {
		builder.Key("error_message").Value("not found"s);
		return;
	}

	builder.Key("curvature").Value(info->curvature)
			.Key("route_length").Value(info->route_length)
			.Key("stop_count").Value(static_cast<int>(info->total_stops))
			.Key("unique_stop_count").Value(static_cast<int>(info->unique_stops));
}

void JSONReader::PrintMap(const RequestHandler& handler, json::Builder& builder) const {
	using namespace std::string_literals;

	builder.Key("map"s).Value(handler.RenderMap());
}

void BuildRouteItem(json::Builder& builder, const transport::router::RouteInfo::BusItem& item) {
	using namespace std::string_literals;

	builder.StartDict()
		.Key("type"s).Value("Bus"s)
		.Key("bus"s).Value(item.bus->name)
		.Key("time"s).Value(item.time)
		.Key("span_count"s).Value(static_cast<int>(item.span_count))
		.EndDict();
}

void BuildRouteItem(json::Builder& builder,
	const transport::router::RouteInfo::WaitItem& item) {
	using namespace std::string_literals;

	builder.StartDict()
		.Key("type"s).Value("Wait"s)
		.Key("stop_name"s).Value(item.stop->name)
		.Key("time"s).Value(item.time)
		.EndDict();
}

void JSONReader::PrintRoute(const RequestHandler& handler, std::string& from,
	std::string& to, json::Builder& builder) const {
	using namespace std::string_literals;

	const auto route = handler.FindRoute(from, to);
	if (!route.has_value()) {
		builder.Key("error_message"s).Value("not found"s);
		return;
	}

	builder
		.Key("total_time"s).Value(route->total_time)
		.Key("items"s).StartArray();

	// Конструирования массива элементов, каждый из которых описывает непрерывную
	// активность пассажира, требующую временных затрат
	for (const auto& item : route->items) {
		std::visit(
			[&builder](const auto& item) {
				BuildRouteItem(builder, item);
			},
			item);
	}

	builder.EndArray();
}

void JSONReader::ProcessQueries(json::Array& data, RequestHandler& handler, std::ostream& out) const {
    using namespace std::string_literals;
	using namespace std::string_view_literals;

	json::Builder builder;

	builder.StartArray();

	for (const json::Node& request : data) {
		auto map_req = request.AsDict();
		int request_id = map_req.at("id"s).AsInt();
		std::string type = map_req.at("type"s).AsString();

		builder.StartDict().Key("request_id").Value(request_id);

		if (type == "Stop"s) {
			std::string name = map_req.at("name"s).AsString();
			PrintStops(handler, name, builder);
		}
		else if (type == "Bus"s) {
			std::string name = map_req.at("name"s).AsString();
			PrintBuses(handler, name, builder);
		}
		else if (type == "Map"s) {
			PrintMap(handler, builder);
		}
		if (type == "Route"s) {
			std::string from = map_req.at("from"s).AsString();
			std::string to = map_req.at("to"s).AsString();
			PrintRoute(handler, from, to, builder);
		}
		builder.EndDict();
	}
	builder.EndArray();

    json::Print(json::Document(builder.Build()), out);
}

} // end namespace transport::reader
