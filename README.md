# VulkanEngine
Rendering &amp; Game Engine Using Modern Vulkan API

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
* [VDU](https://github.com/przemektmalon/VulkanDevUtility) (Vulkan Development Utilities)
* Assimp
* Bullet Physics
* FreeType
* rapidxml
* GLM
* stb image (load and write)
* libshaderc (as distributed with the LunarG Vulkan SDK)
