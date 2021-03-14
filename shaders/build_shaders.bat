

@echo on
%VULKAN_SDK%\Bin32\glslc.exe default.vert -o default.vert.spv
%VULKAN_SDK%\Bin32\glslc.exe default.frag -o default.frag.spv

%VULKAN_SDK%\Bin32\glslc.exe wireframe.vert -o wireframe.vert.spv
%VULKAN_SDK%\Bin32\glslc.exe wireframe.frag -o wireframe.frag.spv
@echo off
