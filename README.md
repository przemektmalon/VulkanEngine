# VulkanEngine
Rendering &amp; Game Engine Using Modern Vulkan API

![Engine Image](https://github.com/przemektmalon/VulkanEngine/blob/master/engineimg.png)

# Major features
* Deferred HDR renderer using half-z tiled light culling
* PBR lighting model
* Ambient Occlusion
* Model LOD system
* Logarithmic depth buffer
* Dynamic lighting and shadows (spot, point, directional)
* Materials system
* Multi-threaded design
* Smooth runtime asset loading pipeline
* Partial bullet physics library integration
* Chaiscript scripting, integrated with in-engine text console
* Multi-threaded profiler
* Windows and Linux support

# Future
Once the core rendering and game specific features are implemented I plan to create a GUI world editor.

Some features that still need to be implemented include:
* Entity-Component-System
* Global illumination
* Skeletal animation
* Particle system
* Custom memory allocators
* Audio library integration

# Dependencies

Most dependancies are automatically downloaded by the prepare_deps.py script and linked in CMakeLists. The LunarG Vulkan SDK has to be downloaded and installed manually, it is linked automatically. Bullet physics currently has to be downloaded, compiled, and linked manually in CMake.

* LunarG Vulkan SDK
* libshaderc (distributed with the LunarG Vulkan SDK)
* [VDU](https://github.com/przemektmalon/VulkanDevUtility) (my personal project to provide abstractions and interfaces for Vulkan functionality)
* Assimp
* Bullet Physics
* FreeType
* GLM
* rapidxml
* stb image (load and write)
