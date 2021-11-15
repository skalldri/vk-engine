# Vulkan Engine Design

## Overview

### Goals
- Compatible with Desktop and VR use cases
- Ability to load and manipulate models
- Ability to create UI elements using imGui
- Full compatibility with GLTF 2.0 model loading
  - Animation framework
  - PBR rendering
  - Embedded shader support?
- High performance VR rendering
 - Supports instancing
 - Supports foveated rendering
- VR and AR hand meshes and tracking
- Supports Ray Tracing
- Natively multi-threaded: work scales up to the number of available processors

### Non-goals (initially)
- ECS
- Physics

## Design

### Objects

#### `ShaderModule`
A `ShaderModule` object represents a Shader which can be combined into a `GraphicsPipeline`. The `ShaderModule` can load files from disk, compile them, and submit them to the GPU.

#### `GraphicsPipeline`
A `GraphicsPipeline` object combines various `ShaderModule`s into a sequence of graphics operations which can be executed on the GPU.


### Threading

#### Main Thread
Submits work to the Vulkan queues and manages the swapchains. Performs synchronization with the underlying XR subsystem to fetch
the head / eye pose data and expose the latest information to the worker threads.

Spawns Render Worker Threads 

#### Render Worker Thread
Creates command buffers to render parts of the scene and submits them to the main thread

#### Transfer Worker Thread
Runs in the background all the time, watching a parallel queue of requests to transfer data to the GPU (like textures). Can load data from disk and transfer it to the GPU autonomously. Executes a callback when transfer is complete.
Multiple Transfer Worker Threads can be spawned in parallel. Each Transfer Worker Thread will require it's own Vulkan Queue to submit transfers to.