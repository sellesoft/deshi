@echo off

REM remove this call somehow
REM call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

pushd C:\github\deshi\src

@set INCLUDES=/I..\src /IC:\src\glfw-3.3.2.bin.WIN64\include /IC:\src\OpenAL1.1\include /I%VULKAN_SDK%\include /IC:\src\glm /IC:\src\stb-master /IC:\src\boost_1_74_0

@set SOURCES=*.cpp utils\*.cpp systems\*.cpp math\*.cpp external\imgui\*.cpp core\*.cpp components\*.cpp animation\*.cpp

@set LIBS=/LIBPATH:C:\src\glfw-3.3.2.bin.WIN64\lib-vc2019 /LIBPATH:C:\src\OpenAL1.1\libs\Win64 /libpath:%VULKAN_SDK%\lib glfw3.lib OpenAL32.lib opengl32.lib gdi32.lib shell32.lib vulkan-1.lib



@set OUT_EXE=deshi

IF [%1]==[] GOTO DEBUG
IF [%1]==[-i] GOTO ONE_FILE
IF [%1]==[-r] GOTO RELEASE

:DEBUG
@set OUT_DIR="..\build\Debug"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /EHsc /nologo /Zi /MD /std:c++17 %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
GOTO DONE

:ONE_FILE
IF [%~2]==[] ECHO "Place the .cpp path after using -i" GOTO DONE 

@set OUT_DIR="..\build\Debug"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /c /EHsc /nologo /Zi /MD /std:c++17 %INCLUDES% %~2 /Fo%OUT_DIR%/
pushd ..\build\Debug
link /nologo *.obj %LIBS% /OUT:%OUT_EXE%.exe 
popd
GOTO DONE

:RELEASE
@set OUT_DIR="..\build\Release"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /EHsc /nologo /Zi /MD /Ox /Oi /std:c++17 %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
GOTO DONE

:DONE
popd