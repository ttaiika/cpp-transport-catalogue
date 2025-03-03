#include "json_reader.h"

using namespace std;

int main() {
    transport::Catalogue catalogue;

    json::Array json;
    json::Dict json_render_settings;

    auto doc = json::Load(std::cin).GetRoot().AsMap();

    transport::JSONReader json_reader(catalogue);

    // Заполнение базы данных
    if (doc.find("base_requests"s) != doc.end()) {
        json = doc.at("base_requests"s).AsArray();
        json_reader.FillDataBase(json);
    }

    // Заполнение настроек визуализации карты
    if (doc.find("render_settings"s) != doc.end()) {
        json_render_settings = doc.at("render_settings"s).AsMap();
        json_reader.ReadRenderSettings(json_render_settings);
    }

    // Обработка запросов и печать результатов
    if (doc.find("stat_requests"s) != doc.end()) {
        json = doc.at("stat_requests"s).AsArray();
        json_reader.ProcessQueries(json, std::cout);
    }
}