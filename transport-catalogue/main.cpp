#include "json_reader.h"

using namespace std;

int main() {
    transport::Catalogue catalogue;

    json::Array json;
    json::Dict json_render_settings;

    auto doc = json::Load(std::cin).GetRoot().AsMap();

    // ���������� ���� ������
    if (doc.find("base_requests"s) != doc.end()) {
        json = doc.at("base_requests"s).AsArray();
        transport::filler::FillDataBase(catalogue, json);
    }

    // ���������� �������� ������������ �����
    map_renderer::RenderSettings render_setting;
    if (doc.find("render_settings"s) != doc.end()) {
        json_render_settings = doc.at("render_settings"s).AsMap();
        render_setting = transport::renderer::ReadRenderSettings(json_render_settings);
    }

    // ��������� �������� � ������ �����������
    if (doc.find("stat_requests"s) != doc.end()) {
        json = doc.at("stat_requests"s).AsArray();
        transport::printer::ProcessQueries(catalogue, json, render_setting, std::cout);
    }
}