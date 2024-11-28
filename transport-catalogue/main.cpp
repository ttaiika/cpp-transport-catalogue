#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
    transport::Catalogue catalogue;

    transport::detail::input::Reader reader;
    reader.ReadAndApplyCommands(cin, catalogue);

    transport::detail::stat::ReadAndProcessStats(cin, cout, catalogue);
}