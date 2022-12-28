#!/bin/bash
#_____________________________________________________________________________________________________
#                                            Usage
#_____________________________________________________________________________________________________
# build.sh <example> <command> [arguments...]
#
# Commands:
#   (empty)       Compile and link (default)
#   compile       Compile only
#   link          Link with previously generated .obj/.o
#   one <file>    Compile one file and link with previously generated .obj/.o
#
# Arguments:
#   --v    Echo build commands to the console
#   --time Time the script (this relies on GNU awk)
#   --d    Build with    debug info and without optimization (default)
#   --r    Build without debug info and with    optimization
#   --s    Build certain modules as shared libraries for code implementation reloading
#   --p    Enable Tracy profiling
#   --pw   Enable Tracy profiling and force the program to wait for a connection to start running
#   --sa   Enable static analysis
#
#   -platform <win32,mac,linux>           Build for specified OS: win32, mac, linux (default: builder's OS)
#   -graphics <vulkan,opengl,directx>     Build for specified Graphics API (default: vulkan)
#   -compiler <cl,gcc,clang,clang-cl>     Build using the specified compiler (default: cl on Windows, gcc on Mac and Linux)
#   -linker <link,ld,lld,lld-link>        Build using the specified linker (default: link on Windows, ld on Mac and Linux)
#   -vulkan_path <path_to_vulkan>         Override the default $VULKAN_SDK path with this path
#
# Notes:
#   >For some reason, the argument count is wrong if you don't add sh.exe before build.sh even if 
#      it's the default program. Windows usage: "sh build.sh <command> [arguments...]"
#
# Examples:
#   >sh.exe build.sh voxels --d --time -graphics vulkan
#
# TODOs:
#   >support the non-default commands
#   >echo out when we successfully build things
#   >early out when we don't successfully build
#   > eventually support gcc in the code base when someone really wants it
#       but push for them to use clang instead :)
#   > make this script easy to use without deshi
#_____________________________________________________________________________________________________
#                                           Builder Vars
#_____________________________________________________________________________________________________
#### Determine builder's OS ####
builder_platform="unknown"
builder_compiler="unknown"
builder_linker="unknown"
if [ "$OSTYPE" == "linux-gnu"* ]; then
  builder_platform="linux"
  builder_compiler="gcc"
  builder_linker="ld"
elif [ "$OSTYPE" == "darwin"* ]; then
  builder_platform="mac"
  builder_compiler="gcc"
  builder_linker="ld"
elif [ "$OSTYPE" == "cygwin" ]; then
  builder_platform="win32"
  builder_compiler="cl"
  builder_linker="link"
elif [ "$OSTYPE" == "msys" ]; then
  builder_platform="win32"
  builder_compiler="cl"
  builder_linker="link"
elif [ "$OSTYPE" == "win32" ]; then
  builder_platform="win32"
  builder_compiler="cl"
  builder_linker="link"
elif [ "$OSTYPE" == "freebsd"* ]; then
  builder_platform="linux"
  builder_compiler="gcc"
  builder_linker="ld"
else
  echo "Unhandled development platform: $OSTYPE"
  exit 1
fi
#_____________________________________________________________________________________________________
#                                       Command Line Args
#_____________________________________________________________________________________________________
build_cmd=""
build_cmd_one_file=""
build_example=""
build_dir="debug"
build_verbose=0
build_release=0
build_shared=0
build_time=0
build_profiler=""
build_static_analysis=0
build_platform=$builder_platform
build_graphics="vulkan"
build_compiler="$builder_compiler"
build_linker="$builder_linker"
build_object=""
vulkan_override=0


skip_arg=0
for (( i=1; i<=$#; i++)); do
  #### skip argument if already handled
  if [ $skip_arg == 1 ]; then
    skip_arg=0
    continue
  fi

  #### parse <example>
  if [ $i == 1 ]; then
    if [ "${!i}" == "voxels" ]; then
      build_example="voxels"
      continue
    elif [ "${!i}" == "ui" ]; then
      build_example="ui"
      continue
    elif [ "${!i}" == "dining_philosophers" ]; then
      build_example="dining_philosophers"
      continue
    elif [ "${!i}" == "ant_sim" ]; then
      build_example="ant_sim"
      continue
    else
      echo "Unknown example: ${!i}"
      exit 1
    fi
  fi

  #### parse <command>
  if [ $i == 2 ]; then
    if [ "${!i}" == "compile" ]; then
      build_cmd="compile"
      continue
    elif [ "${!i}" == "link" ]; then
      build_cmd="link"
      continue
    elif [ "${!i}" == "one" ]; then
      build_cmd="one"
      skip_arg=1
      next_arg=$((i+1))
      build_cmd_one_file="${!next_arg}"
      continue
    fi
  fi

  #### parse [arguments...]
  if [ "${!i}" == "--v" ]; then
    build_verbose=1
  elif [ "${!i}" == "--time" ]; then
    build_time=1
  elif [ "${!i}" == "--d" ]; then
    echo "" #### do nothing since this is default (bash has to have something inside an if)
  elif [ "${!i}" == "--r" ]; then
    build_dir="release"
    build_release=1
  elif [ "${!i}" == "--s" ]; then
    build_shared=1
  elif [ "${!i}" == "--p" ]; then
    build_profiler="profile"
  elif [ "${!i}" == "--pw" ]; then
    build_profiler="wait and profile"
  elif [ "${!i}" == "--sa" ]; then
    build_static_analysis=1
  elif [ "${!i}" == "-platform" ]; then
    skip_arg=1
    next_arg=$((i+1))
    if [ "${!next_arg}" == "win32" ] || [ "${!next_arg}" == "mac" ] || [ "${!next_arg}" == "linux" ]; then
      build_platform="${!next_arg}"
    else
      echo "Unknown platform: ${!next_arg}; Valid options: win32, mac, linux"
    fi
  elif [ "${!i}" == "-graphics" ]; then
    skip_arg=1
    next_arg=$((i+1))
    if [ "${!next_arg}" == "vulkan" ] || [ "${!next_arg}" == "opengl" ] || [ "${!next_arg}" == "directx" ]; then
      build_graphics="${!next_arg}"
    else
      echo "Unknown graphics API: ${!next_arg}; Valid options: vulkan, opengl, directx"
    fi
  elif [ "${!i}" == "-compiler" ]; then
    skip_arg=1
    next_arg=$((i+1))
    if [ "${!next_arg}" == "cl" ] || [ "${!next_arg}" == "gcc" ] || [ "${!next_arg}" == "clang" ] || [ "${!next_arg}" == "clang-cl" ]; then
      build_compiler="${!next_arg}"
    else
      echo "Unknown compiler: ${!next_arg}; Valid options: cl, gcc, clang, clang-cl"
    fi
  elif [ "${!i}" == "-linker" ]; then
    skip_arg=1
    next_arg=$((i+1))
    if [ "${!next_arg}" == "link" ] || [ "${!next_arg}" == "ld" ] || [ "${!next_arg}" == "lld" ] || [ "${!next_arg}" == "lld-link" ]; then
      build_linker="${!next_arg}"
    else
      echo "Unknown linker: ${!next_arg}; Valid options: link, ld, lld, lld-link"
    fi
  elif [ "${!i}" == "-vulkan_path" ]; then
    skip_arg=1
    next_arg=$((i+1))
    vulkan_override="${!next_arg}"
  else
    echo "Unknown switch: ${!i}"
    exit 1
  fi
done


if [ "$build_compiler" == "cl" ]; then
  build_object="obj"
else
  build_object="o"
fi
#_____________________________________________________________________________________________________
#                                           Build Vars
#_____________________________________________________________________________________________________
#### Specify paths ####
examples_folder="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";
root_folder="$examples_folder/.."
vulkan_folder="$VULKAN_SDK"
tracy_folder="H:/src/tracy-0.7.8" #TODO(sushi) make this an env var

if [ $vulkan_override != 0 ]; then
  vulkan_folder=$vulkan_override
fi


#### Specify outputs ####
app_name="$build_example"
build_folder="$examples_folder/$app_name/build"


#### Specify sources ####
includes="
  -Isrc
  -Isrc/external
  -I$vulkan_folder/include
  -I$tracy_folder"
deshi_sources="src/deshi.cpp"
dll_sources="src/core/ui2.cpp"
app_sources="examples/$app_name/$app_name.cpp"


#### Specifiy libs ####
lib_paths=(
  $vulkan_folder/lib
)
libs=(
  #win32 libs
  gdi32
  shell32
  ws2_32
  winmm

  #graphics libs
  opengl32
  vulkan-1
  shaderc_combined    #required for vulkan shader compilation at runtime
)
#_____________________________________________________________________________________________________
#                                         Global Defines
#_____________________________________________________________________________________________________
defines_build=""
if [ $build_release == 0 ]; then
  defines_build="-DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_RELEASE=0"
else
  defines_build="-DBUILD_INTERNAL=0 -DBUILD_SLOW=0 -DBUILD_RELEASE=1"
fi


defines_platform=""
if [ $build_platform == "win32" ]; then
  defines_platform="-DDESHI_WINDOWS=1 -DDESHI_MAC=0 -DDESHI_LINUX=0"
elif [ $build_platform == "mac" ]; then
  defines_platform="-DDESHI_WINDOWS=0 -DDESHI_MAC=1 -DDESHI_LINUX=0" 
elif [ $build_platform == "linux" ]; then
  defines_platform="-DDESHI_WINDOWS=0 -DDESHI_MAC=0 -DDESHI_LINUX=1"
else
  echo "Platform defines not setup for platform: $build_platform"
  exit 1
fi


defines_graphics=""
if [ $build_graphics == "vulkan" ]; then
  defines_graphics="-DDESHI_VULKAN=1 -DDESHI_OPENGL=0 -DDESHI_DIRECTX12=0"
elif [ $build_graphics == "opengl" ]; then
  defines_graphics="-DDESHI_VULKAN=0 -DDESHI_OPENGL=1 -DDESHI_DIRECTX12=0"
elif [ $build_graphics == "directx" ]; then
  defines_graphics="-DDESHI_VULKAN=0 -DDESHI_OPENGL=0 -DDESHI_DIRECTX12=1"
else
  echo "Platform defines not setup for platform: $build_platform"
  exit 1
fi

defines_misc="-D_CRT_SECURE_NO_WARNINGS"
if [ "$build_profiler" == "profile" ]; then
  defines_misc="$defines_misc -DTRACY_ENABLE"
elif [ "$build_profiler" == "wait and profile" ]; then
  defines_misc="$defines_misc -DTRACY_ENABLE -DDESHI_WAIT_FOR_TRACY_CONNECTION"
fi

defines_shared=""
if [ $build_shared == 1 ]; then
  defines_shared="-DDESHI_RELOADABLE_UI=1"
else
  defines_shared="-DDESHI_RELOADABLE_UI=0"
fi


defines="$defines_build $defines_platform $defines_graphics $defines_shared $defines_misc"
#_____________________________________________________________________________________________________
#                                           Build Flags
#_____________________________________________________________________________________________________
compile_flags=""
if [ $build_compiler == "cl" ] || [ $build_compiler == "clang-cl" ]; then #__________________________________________cl
  #### -diagnostics:column (shows the file and column where the error is)
  #### -diagnostics:caret (shows the file, column, and code where the error is)
  #### -EHsc   (enables exception handling)
  #### -nologo (prevents Microsoft copyright banner showing up)
  #### -MD     (is used because vulkan's shader compilation lib requires dynamic linking with the CRT)
  #### -MP     (enables building with multiple processors)
  #### -Oi     (enables function inlining)
  #### -GR     (enables RTTI and dynamic_cast<>())
  #### -Gm-    (disables minimal rebuild (recompile all files))
  #### -std:c++17 (specifies to use the C++17 standard)
  #### -utf-8  (specifies that source files are in utf8)
  compile_flags="$compile_flags -diagnostics:column -EHsc -nologo -MD  -Oi -GR  -std:c++17 -utf-8"

  if [ $build_compiler == "clang-cl" ]; then
    #### -msse3 (enable SSE)
    #### -Wno-unused-value ()
    #### -Wno-implicitly-unsigned-literal ()
    #### -Wno-nonportable-include-path ()
    #### -Wno-writable-strings ()
    #### -Wno-unused-function ()
    #### -Wno-unused-variable ()
    #### -Wno-undefined-inline ()
    compile_flags="$compile_flags -msse3 -Wno-unused-value -Wno-implicitly-unsigned-literal -Wno-nonportable-include-path -Wno-writable-strings -Wno-unused-function -Wno-unused-variable -Wno-undefined-inline"
  else
    #### -W1 (is the warning level)
    #### -wd4100 (disables warning: unused function parameter)
    #### -wd4189 (disables warning: unused local variables)
    #### -wd4201 (disables warning: nameless union or struct)
    #### -wd4311 (disables warning: pointer truncation)
    #### -wd4706 (disables warning: assignment within conditional expression)
    compile_flags="$compile_flags -MP -Gm- -W1 -wd4100 -wd4189 -wd4201 -wd4311 -wd4706"
  fi

  if [ $build_release == 0 ]; then
    #### -Zi (produces a .pdb file containing debug information)
    #### -Od (prevents all optimization)
    compile_flags="$compile_flags -Z7 -Od"
  else
    #### -O2 (maximizes speed (O1 minimizes size))
    compile_flags="$compile_flags -O2"
  fi

  if [ $build_static_analysis == 1 ]; then
    #### -analyze (enables static analysis)
    compile_flags="$compile_flags -analyze"
  fi
elif [ $build_compiler == "gcc" ]; then #____________________________________________________________________________gcc
  #### -exceptions (enables exception handling)
  #### -std=c++17  (specifies use of the C++17 standard)
  compile_flags="$compile_flags -exceptions -std=c++17"
  
  if [ $build_release == 0 ]; then
    #### -ggdb3 (produces max debug info with extra stuff for gdb)
    #### -Og    (optimize debugging experience, -O0 also disables some debug information so its not recommended for debugging)
    #### -fpermissive ()
    compile_flags="$compile_flags -ggdb3 -Og -fpermissive"

    #### -Wno-return-type   (disables warning: no return statement)
    #### -Wno-write-strings (disables warning: converting a constant string to a char*)
    #### -Wno-pointer-arith (disables warning: NULL being used in pointer arithmetic (==NULL apparently counts as this))
    compile_flags="$compile_flags -Wno-return-type -Wno-write-strings -Wno-pointer-arith"

  else
    #### -O3 (enable all optimizations that are standard compliant, -Ofast disregards standards) TODO(sushi) consider -Ofast
    compile_flags="$compile_flags -O3"
  fi

  if [ $build_static_analysis == 1 ]; then
    #### -fanalyzer (enables static analysis) TODO(sushi) look into other analyzer settings
    compile_flags="$compile_flags -fanalysis"
  fi
elif [ $build_compiler == "clang" ]; then #__________________________________________________________________________clang
  #### -std=c++17 (specifies to use the C++17 standard)
  #### -fexceptions ()
  #### -fcxx-exceptions ()
  #### -finline-functions ()
  #### -pipe ()
  #### -msse3 ()
  compile_flags="$compile_flags 
    -std=c++17 
    -fexceptions 
    -fcxx-exceptions 
    -finline-functions 
    -pipe 
    -msse3"

  #### -Wno-unused-value ()
  #### -Wno-implicitly-unsigned-literal ()
  #### -Wno-nonportable-include-path ()
  #### -Wno-writable-strings ()
  #### -Wno-unused-function ()
  #### -Wno-unused-variable ()
  #### -Wno-undefined-inline ()
  compile_flags="$compile_flags
    -Wno-unused-value 
    -Wno-implicitly-unsigned-literal 
    -Wno-nonportable-include-path 
    -Wno-writable-strings 
    -Wno-unused-function 
    -Wno-unused-variable 
    -Wno-undefined-inline"

  if [ $build_release == 0 ]; then
    #### -ggdb3 (produces max debug info with extra stuff for gdb)
    #### -gcodeview ()
    #### -O0 ()
    compile_flags="$compile_flags -ggdb3 -gcodeview -O0"
  else
    #### -O2 ()
    compile_flags="$compile_flags -O2"
  fi
else
  echo "Compile flags not setup for compiler: $build_compiler"
  exit 1
fi


link_flags=""
link_libs=""
if [ $build_linker == "link" ] || [ $build_linker == "lld-link" ]; then
  #### -nologo (prevents Microsoft copyright banner showing up)
  #### -opt:ref (eliminates functions and data that are never used)
  #### -incremental:no (disables incremental linking (relink all files))
  link_flags="$link_flags -nologo -opt:ref -incremental:no"

  if [ $build_release == 0 ]; then
    #### -debug:full (enabled debug info and PDB generation during linking)
    link_flags="$link_flags -debug:full"
  fi

  for ((i=0; i<${#lib_paths[@]}; i++)); do
    lib_path=${lib_paths[i]}
    link_libs="$link_libs -libpath:$lib_path"
  done

  for ((i=0; i<${#libs[@]}; i++)); do
    lib_lib=${libs[i]}
    link_libs="$link_libs $lib_lib.lib"
  done
elif [ $build_linker == "ld" ] || [ $build_linker == "lld" ]; then


  for ((i=0; i<${#lib_paths[@]}; i++)); do
    lib_path=${lib_paths[i]}
    link_libs="$link_libs -L$lib_path"
  done

  for ((i=0; i<${#libs[@]}; i++)); do
    lib_lib=${libs[i]}
    link_libs="$link_libs -l$lib_lib"
  done
else
  echo "Link flags not setup for linker: $build_linker"
  exit 1
fi

if ([ $build_compiler == "cl" ] || [ $build_compiler == "clang-cl" ]) && ([ $build_linker == "ld" ] || [ $build_linker == "lld" ]); then
  echo "[31mcl/clang-cl compilers are not compatible with ld/lld linkers.[0m"
  exit 1
elif ([ $build_compiler == "gcc" ] || [ $build_compiler == "clang" ]) && ([ $build_linker == "link" ] || [ $build_linker == "lld-link" ]); then
  echo "[31mgcc/clang compilers are not compatible with link/lld-link linkers.[0m"
  exit 1
fi
#_____________________________________________________________________________________________________
#                                           Execute Commands
#_____________________________________________________________________________________________________
#### function to echo and execute commands if verbose flag is set
exe(){
  if [ $build_verbose == 1 ]; then
    echo "\$ $@"; "$@";
  else
    "$@";
  fi
}

echo "`date +'%a, %h %d %Y, %H:%M:%S'` ($build_compiler/$build_dir/$build_graphics) [$app_name]"
if [ ! -e $build_folder ]; then mkdir $build_folder; fi
if [ ! -e $build_folder/$build_dir ]; then mkdir $build_folder/$build_dir; fi
pushd $root_folder > /dev/null
build_dir="examples/$app_name/build/$build_dir"
if [ $builder_platform == "win32" ]; then #_________________________________________________________________________________win32
  if [ -e examples/ctime.exe ]; then ctime -begin examples/$app_name/$app_name.ctm; fi
  if [ $build_time == 1 ]; then start_time=$(date +%s.%3N); fi
  echo ---------------------------------
  
  if [ $build_compiler == "cl" ] || [ $build_compiler == "clang-cl" ]; then #______________________________________cl
    #### delete previous debug info
    rm $build_dir/*.pdb > /dev/null 2> /dev/null
    #echo Waiting for PDB > lock.tmp

    #### compile app (generates app_name.exe)
    exe $build_compiler $app_sources $deshi_sources $includes $compile_flags $defines -Fo"$build_dir/" -link $link_flags $link_libs -OUT:"$build_dir/$app_name.exe" -PDB:"$build_dir/$app_name.pdb"

    if [ $? == 0 ] && [ -e $build_dir/$app_name.exe ]; then
      echo "[32m  $app_name.exe[0m"
    else
      echo "[93mFailed to build: $app_name.exe[0m"
    fi
  elif [ $build_compiler == "gcc" ]; then #________________________________________________________________________gcc
    echo "Execute commands not setup for compiler: $builder_compiler"
  elif [ $build_compiler == "clang" ]; then #______________________________________________________________________clang
    #### compile app (generates app_name.exe)
    exe $build_compiler $app_sources $deshi_sources $includes $compile_flags $defines -Fo"$build_dir/" $link_flags $link_libs -o"$build_dir/$app_name.exe"

    if [ $? == 0 ] && [ -e $build_dir/$app_name.exe ]; then
      echo "[32m  $app_name.exe[0m"
    else
      echo "[93mFailed to build: $app_name.exe[0m"
    fi
  fi

  echo ---------------------------------
  if [ -e examples/ctime.exe ]; then ctime -end examples/$app_name/$app_name.ctm; fi
  if [ $build_time == 1 ]; then printf "time: %f seconds" $(awk "BEGIN {print $(date +%s.%3N) - $start_time}"); fi
elif [ $builder_platform == "mac" ]; then #_________________________________________________________________________________mac
  echo "Execute commands not setup for platform: $builder_platform"
elif [ $builder_platform == "linux" ]; then #_______________________________________________________________________________linux
  echo "Execute commands not setup for platform: $builder_platform"
else
  echo "Execute commands not setup for platform: $builder_platform"
fi
popd > /dev/null