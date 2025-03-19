#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_builder.h"

#include <algorithm>

namespace transport {

class JSONReader {
public:
	JSONReader(Catalogue& catalogue);

	// ������ ���� ������ ���������� ���������
	void FillDataBase(json::Array& data); 
	// ��������� ��������� ������������ �����
	void ReadRenderSettings(const json::Dict& data);
	// ������������ ������� � ������� ���������� �� �����
	void ProcessQueries(json::Array& data, std::ostream& out) const;

private:
	void AddStopsToDataBase(json::Array& data, std::map<std::string, json::Dict>& distances);
	void AddDistancesToDataBase(std::map<std::string, json::Dict>& distances);
	void AddRoutesToDataBase(json::Array& data);

	json::Dict PrintStops(const std::string& name) const;
	json::Dict PrintBuses(const std::string& name) const;
	json::Dict PrintMap() const;

	Catalogue catalogue_;
	map_renderer::RenderSettings render_settings_;
};

} // end namespace transport
