#include <iostream>
#include <string>

#include <fmt/core.h>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace std;

int main(int argc, char** argv)
{
    cout << "Hello world" << endl;

    fmt::print("Hello world from {}\n", "fmtlib");

    CLI::App app{"Vulkan Engine"};

    std::string filename = "default";
    app.add_option("-f,--file", filename, "A help string");

    CLI11_PARSE(app, argc, argv);

    GLFWwindow* window;

    // Initialize GLFW
    if (!glfwInit()) {
        fmt::print("Failed to init GLFW\n");
        return -1;
    }

    fmt::print("GLFW initialized\n");

    if (glfwVulkanSupported())
    {
        // Vulkan is available, at least for compute
        fmt::print("GLFW detected vulkan support\n");
    } else {
        fmt::print("GLFW did not detect vulkan support\n");
    }


    return 0;
}