[P3DPGE](https://github.com/SushiSalad/P3DPGE)
===
A game engine for 3D physics and graphics using javidx9's [Pixel Game Engine](https://github.com/OneLoneCoder/olcPixelGameEngine)

### Build Dependencies:
* [Boost Libraries 1.74.0](https://www.boost.org/users/history/version_1_74_0.html)
* [GLFW 3.3.2](https://www.glfw.org/download.html)
* [VulkanSDK 1.2.162.1](https://vulkan.lunarg.com/sdk/home)
* [GLM 0.9.9.8](https://github.com/g-truc/glm/releases/tag/0.9.9.8)

### Features
* Importing Wavefront (.obj) models
* Naive 3D rendering implementation from scratch
* 3D physics collision detection and resolution (partial)
* Console and commands
* Mouse and keyboard input
<!--
### Exporting a Model From Blender and Loading it Into The Engine
1. In the export menu for Blender choose Wavefront (.obj).
2. Then on the right side open the Geometry tab and make sure that everything is unchecked except for "Triangulate Faces"
3. Export it and place it into the objects folder 
4. In the code, create a new object of type Complex in this fashion: `Complex("name_of_obj_file.obj", id, position)`
-->
### Major TODOs
* Finish collisions for the remaining physics primitives
* Component object pooling
* 2D collision detection and resolution
* 3D-2D entity conversion
* Atmospherics and fluids
* Vulkan rendering
* Lighting and shadows
* Level/scene editor

### Credits
* [dellevelled](https://github.com/DelleVelleD) Co-developer
* [SushiSalad](https://github.com/SushiSalad) Co-developer
* [olcPixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine)
* [Dear ImGui](https://github.com/ocornut/imgui)

### [License](LICENSE.txt)
Public Domain or MIT licensed -- whichever you prefer