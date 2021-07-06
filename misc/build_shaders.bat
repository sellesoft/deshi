@echo off
pushd ..\data\shaders

for %%i in (*.vert) do (%VULKAN_SDK%\Bin32\glslc.exe %%i -o %%i.spv)
for %%i in (*.frag) do (%VULKAN_SDK%\Bin32\glslc.exe %%i -o %%i.spv)
for %%i in (*.geom) do (%VULKAN_SDK%\Bin32\glslc.exe %%i -o %%i.spv)

popd