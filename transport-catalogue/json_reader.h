#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

#include <algorithm>

namespace transport {
namespace filler {
	// Строит базу данных автобусных маршрутов
	void FillDataBase(Catalogue& catalogue, json::Array& data); 

} // namespace filler

namespace renderer {
	// Заполняет настройки визуализации карты
	map_renderer::RenderSettings ReadRenderSettings(const json::Dict& data);

} // namespace renderer

namespace printer {
	// Обрабатывает запросы и выводит результаты на экран
	void ProcessQueries(Catalogue& catalogue, json::Array& data, map_renderer::RenderSettings render_setting, std::ostream& out);

} // namespace printer
} // end namespace transport