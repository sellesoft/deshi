@echo off
Rem pushd C:\github\VulkanTest\shaders

@echo on
C:/VulkanSDK/1.2.162.1/Bin32/glslc.exe shader.vert -o vert.spv
C:/VulkanSDK/1.2.162.1/Bin32/glslc.exe shader.frag -o frag.spv

@echo off
Rem pause
Rem popd