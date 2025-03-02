#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

#include <algorithm>

namespace transport {
namespace filler {
	// ������ ���� ������ ���������� ���������
	void FillDataBase(Catalogue& catalogue, json::Array& data); 

} // namespace filler

namespace renderer {
	// ��������� ��������� ������������ �����
	map_renderer::RenderSettings ReadRenderSettings(const json::Dict& data);

} // namespace renderer

namespace printer {
	// ������������ ������� � ������� ���������� �� �����
	void ProcessQueries(Catalogue& catalogue, json::Array& data, map_renderer::RenderSettings render_setting, std::ostream& out);

} // namespace printer
} // end namespace transport