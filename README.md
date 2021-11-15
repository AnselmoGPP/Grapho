# VulkRend (Vulkan renderer)

<br>![Khronos Vulkan logo](https://raw.githubusercontent.com/AnselmoGPP/Vulkan_samples/master/files/Vulkan_logo.png)

VulkRend is a lightweight and easy to use project for rendering computer graphics with C++ and [Vulkan®](https://www.khronos.org/vulkan/), the new generation graphics and compute API from [Khronos®](https://www.khronos.org/). 

## Table of Contents
+ [VulkRend](#vulkrend)
+ [Dependencies](#dependencies)
+ [Building the project](#building-the-project)
+ [Adding a new project](#adding-a-new-project)
+ [Deeper technical details](#deeper-technical-details)
    + [Loading models system](#loading-models-system)

## VulkRend

VulkRend allows the programmer to load models and textures (OBJ or raw data), and render them in a 3D space. A single model can be rendered many times simultaneously. Loaded models, or some of their renderings (or all of them), can be removed at any time. The camera system allows to navigate through the rendering. The models are loaded in a parallel thread, so that the render loop don't suffer delays.

<h4>Main project content:</h4>

- _**projects:**_ Contains VulkRend and some example projects.
  - _**Renderer:**_ VulkRend headers and source files.
  - _**shaders**_ Shaders used (vertex & fragment).
    - **environment:** Creates and configures the core Vulkan environment.
    - **renderer:** Uses the Vulkan environment for rendering the models provided by the user.
    - **models:** The user loads his models to VulkRend through the modelData class.
    - **input:** Manages user input (keyboard, mouse...) and delivers it to VulkRend.
    - **camera:** Camera system.
    - **timer:** Time data.
    - **data:** Bonus functions (model matrix computation...).
    - **main:** Examples of how to use VulkRend.
- _**extern:**_ Dependencies (GLFW, GLM, stb_image, tinyobjloader...).
- _**files:**_ Scripts and images.
- _**models:**_ Models for loading in our projects.
- _**textures:**_ Images used as textures in our projects.

<br>![Vulkan window](https://raw.githubusercontent.com/AnselmoGPP/Vulkan_samples/master/files/window_2.png)

## Dependencies

<h4>Dependencies of these projects:</h4>

- **GLFW** (Window system and inputs)
- **GLM** (Mathematics library)
- **stb_image** (Image loader)
- **tinyobjloader** (Load vertices and faces from an OBJ file)
- **Vulkan SDK** (Set of repositories useful for Vulkan development) (installed in platform-specfic directories)
  - Vulkan loader (Khronos)
  - Vulkan validation layer (Khronos)
  - Vulkan extension layer (Khronos)
  - Vulkan tools (Khronos)
  - Vulkan tools (LunarG)
  - gfxreconstruct (LunarG)
  - glslang (Shader compiler to SPIR-V) (Khronos)
  - shaderc (C++ API wrapper around glslang) (Google)
  - SPIRV-Tools (Khronos)
  - SPIRV-Cross (Khronos)
  - SPIRV-Reflect (Khronos)

## Building the project

<h4>Steps for building this project:</h4>

The following includes the basics for setting up Vulkan and this project. For more details about setting up Vulkan, check [Setting up Vulkan](https://sciencesoftcode.wordpress.com/2021/03/09/setting-up-vulkan/).

### Ubuntu

- Update your GPU's drivers
- Get:
    1. Compiler that supports C++17
    2. Make
    3. CMake
- Install Vulkan SDK
    1. [Download](https://vulkan.lunarg.com/sdk/home) tarball in `extern\`.
    2. Run script `./files/install_vulkansdk` (modify `pathVulkanSDK` variable if necessary)
- Build project using the scripts:
    1. `sudo ./files/Ubuntu/1_build_dependencies_ubuntu`
    2. `./files/Ubuntu/2_build_project_ubuntu`

### Windows

- Update your GPU's drivers
- Install Vulkan SDK
    1. [Download](https://vulkan.lunarg.com/sdk/home) installer wherever 
    2. Execute installer
- Get:
    1. MVS
    2. CMake
- Build project using the scripts:
    1. `1_build_dependencies_Win.bat`
    2. `2_build_project_Win.bat`
- Compile project with MVS (Set as startup project & Release mode) (first glfw and glm_static, then the Vulkan projects)

## Adding a new project

  1. Copy some project from _/project_ and paste it there
  2. Include it in /project/CMakeLists.txt
  3. Modify project name in copiedProject/CMakeLists.txt
  4. Modify in-code shaders paths, if required

## Deeper technical details

### Loading models system

A parallel thread (loadModelsThread) starts running for each Renderer object after it is created, and finishes right after the render loop has finished. When the user creates a new modelData object (newModel()), a partially initialized modelData is stored in the list "waitingModels". Meanwhile, loadModelsThread checks the waitingModels list periodically. If it detects any element inside, proceeds to fully initialize them (load models and textures into Vulkan), moves them to the list "models" (used mainly for updating the model matrices and creating the command buffers), and recreates the Vulkan command buffers. Three semaphores (lock_guard) are required:

  - waitingModelsMutex: Controls access to the waitingModels list from main thread (newModel -> inserts models) and secondary thread (extracts models from that list to put them in models list).
  - modelsMutex: Controls access to the models list and command buffer from main thread (createCommandBuffers, updateUniformBuffer,recreateSwapChain, cleanupSwapChain) and secondary thread (inserts models in models list and updates command buffers).
  - queuemutex: A vkQueue object (graphicsQueue, presentQueue) cannot be used in different threads simultaneously, so this controls the access to them.

## Links

- [Setting up Vulkan](https://sciencesoftcode.wordpress.com/2021/03/09/setting-up-vulkan/)
- [Vulkan tutorials](https://sciencesoftcode.wordpress.com/2019/04/08/vulkan-tutorials/)
- [Vulkan notes](https://sciencesoftcode.wordpress.com/2021/11/08/vulkan-notes/)
- [Vulkan SDK (getting started)](https://vulkan.lunarg.com/doc/sdk/1.2.170.0/linux/getting_started.html)
- [Vulkan tutorial](https://vulkan-tutorial.com/)
