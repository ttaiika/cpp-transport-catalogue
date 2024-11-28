#pragma once

#include <iosfwd>
#include <string_view>
#include <utility>

#include "transport_catalogue.h"

namespace transport::detail::stat {

std::pair<std::string_view, std::string_view> ParseRequest(std::string_view string);

void ParseAndPrintStat(const Catalogue& tansport_catalogue, std::string_view request,
    std::ostream& output);

} // end namespace transport::detail::stat