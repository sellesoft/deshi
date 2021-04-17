@echo off

pushd ..\src

@set DEFINES=/DVULKAN

@set INCLUDES=/I..\src /IC:\src\glfw-3.3.2.bin.WIN64\include /IC:\src\OpenAL1.1\include /I%VULKAN_SDK%\include /IC:\src\glm

@set SOURCES=*.cpp core\*.cpp external\imgui\*.cpp game\*.cpp game\components\*.cpp game\systems\*.cpp scene\*.cpp

@set LIBS=/LIBPATH:C:\src\glfw-3.3.2.bin.WIN64\lib-vc2019 /LIBPATH:C:\src\OpenAL1.1\libs\Win64 /libpath:%VULKAN_SDK%\lib glfw3.lib OpenAL32.lib opengl32.lib gdi32.lib shell32.lib vulkan-1.lib shaderc_combined.lib

@set OUT_EXE=deshi.exe

IF [%1]==[] GOTO DEBUG
IF [%1]==[-i] GOTO ONE_FILE
IF [%1]==[-l] GOTO LINK_ONLY
IF [%1]==[-r] GOTO RELEASE





REM ______________ DEBUG (compiles without optimization)

:DEBUG
ECHO [47m[94m%DATE% %TIME% Debug[0m[0m
@set OUT_DIR="..\build\Debug"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /EHsc /nologo /Zi /MD /MP /std:c++17 %DEFINES% %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LIBS%
GOTO DONE

REM ______________ ONE FILE (compiles just one file with debug options, links with previosly created .obj files)

:ONE_FILE
ECHO [47m[94m%DATE% %TIME% One File (Debug)[0m[0m
IF [%~2]==[] ECHO "Place the .cpp path after using -i"; GOTO DONE;
ECHO [93mWarning: debugging might not work with one-file compilation[0m

@set OUT_DIR="..\build\Debug"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /c /EHsc /nologo /Zi /MD /std:c++17 %DEFINES% %INCLUDES% %~2 /Fo%OUT_DIR%/
pushd ..\build\Debug
link /nologo *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM ______________ LINK ONLY (links with previosly created .obj files)

:LINK_ONLY
ECHO [47m[94m%DATE% %TIME% Link Only (Debug)[0m[0m
pushd ..\build\Debug
link /nologo *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM ______________ RELEASE (compiles with optimization)

:RELEASE
ECHO [47m[94m%DATE% %TIME% Release[0m[0m
@set OUT_DIR="..\build\Release"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /EHsc /nologo /Zi /MD /O2 /Oi /MP /std:c++17 %DEFINES% %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LIBS%
GOTO DONE

:DONE
popd