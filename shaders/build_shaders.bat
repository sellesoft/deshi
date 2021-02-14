@echo off
pushd C:\github\deshi\shaders

@echo on
%VULKAN_SDK%\Bin32\glslc.exe phong.vert -o phong.vert.spv
%VULKAN_SDK%\Bin32\glslc.exe phong.frag -o phong.frag.spv

%VULKAN_SDK%\Bin32\glslc.exe wireframe.vert -o wireframe.vert.spv
%VULKAN_SDK%\Bin32\glslc.exe wireframe.frag -o wireframe.frag.spv

@echo off
REM pause
popd