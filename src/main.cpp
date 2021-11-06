#include <iostream>
#include <string>

#include <fmt/core.h>
#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

using namespace std;

int main(int argc, char** argv)
{
    cout << "Hello world" << endl;

    fmt::print("Hello world from {}\n", "fmtlib");

    CLI::App app{"Vulkan Engine"};

    std::string filename = "default";
    app.add_option("-f,--file", filename, "A help string");

    CLI11_PARSE(app, argc, argv);

    return 0;
}