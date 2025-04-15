#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h"
#include "graph.h"
#include "request_handler.h"

#include <algorithm>

namespace transport::reader {

class JSONReader {
public:
	JSONReader(Catalogue& catalogue);

	// ������ ���� ������ ���������� ���������
	void FillDataBase(json::Array& data); 
	// ��������� ��������� ������������ �����
	renderer::RenderSettings ReadRenderSettings(const json::Dict& data);
	// ��������� bus_wait_time � bus_velocity
	router::RoutingSettings ReadRoutingSettings(const json::Dict& data);
	// ������������ ������� � ������� ���������� �� �����
	void ProcessQueries(json::Array& data, RequestHandler& handler, std::ostream& out) const;

private:
	void AddStopsToDataBase(json::Array& data, std::map<std::string, json::Dict>& distances);
	void AddDistancesToDataBase(std::map<std::string, json::Dict>& distances);
	void AddRoutesToDataBase(json::Array& data);

	void PrintStops(const RequestHandler& handler, std::string& name,
		json::Builder& builder) const;
	void PrintBuses(const RequestHandler& handler, std::string& name,
		json::Builder& builder) const;
	void PrintMap(const RequestHandler& handler, json::Builder& builder) const;
	void PrintRoute(const RequestHandler& handler, std::string& from,
		std::string& to, json::Builder& builder) const;

	Catalogue& catalogue_;
};

} // end namespace transport::reader