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

### Threading

#### Main Thread
Submits work to the Vulkan queues and manages the swapchains. Performs synchronization with the underlying XR subsystem to fetch
the head / eye pose data and expose the latest information to the worker threads. 

#### Worker Thread
Creates command buffers to render parts of the scene and submits them to the main thread.