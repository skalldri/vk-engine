# vk-engine

A simple engine built in C++, initially following [this tutorial](https://vulkan-tutorial.com/) but being refactored into a more general purpose engine with native support for VR.

## Compatiblity
Intended to compile on any platform, this project tries to avoid taking OS-specific dependencies.

It is primarily developed on Windows, and the build system is currently only tested on Windows 10.

## Prerequisites
- Visual Studio 2019 be installed (2022 might also work?)
- Install the [LunarG Vulkan 1.2 SDK](https://vulkan.lunarg.com/sdk/home)
- Run `git submodule update --init --recursive`
- Install the extensions recommended in `extensions.json`

## Build
- Open the folder in VS Code
- Configure using a suggested Visual Studio 2019 amd64 toolchain
- Build using VS Code

Future versions will include build instructions for the Android NDK.

