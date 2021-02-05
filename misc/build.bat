@echo off

set CompilerFlags=-Wall -std=c++17
set LinkerFlags=
set ExternalIncludes= -I"C:\src\stb-master" -I"C:\VulkanSDK\1.2.162.1\Include" -I"C:\src\glm" -I"C:\src\glfw-3.3.2.bin.WIN64\include" -I"C:\src\boost_1_74_0" 
set ExternalLibs= -L"C:\VulkanSDK\1.2.162.1\Lib\" -l"vulkan-1.lib" -L"C:\src\glfw-3.3.2.bin.WIN64\lib-vc2019\" -l"glfw3.lib"

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

dir

REM Engine
gcc %CompilerFlags% -o deshi.exe "..\src\*.cpp" -I"..\src" %LinkerFlags% %ExternalIncludes% %ExternalLibs%

popd
pause