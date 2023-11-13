#!/usr/bin/python
#_____________________________________________________________________________________________________
#                                            Usage
#_____________________________________________________________________________________________________
# build.py <command> [arguments...]
#
# Commands:
#   (empty)       Compile and link (default)
#   compile       Compile only
#   link          Link with previously generated .obj/.o
#   one <file>    Compile one file and link with previously generated .obj/.o
#
# Arguments:
#   --v    Echo build commands to the console
#   --d    Build with    debug info and without optimization (default)
#   --r    Build without debug info and with    optimization
#   --p    Enable Tracy profiling
#   --pw   Enable Tracy profiling and force the program to wait for a connection to start running
#   --sa   Enable static analysis
#   --ba   Enable build analysis (currently only works with clang with ClangBuildAnalyzer installed)
#   --nd   Disable building deshi's sources
#   --ad   Automatically decide to build deshi based on whether or not it has source files newer than the last time it was built. If --nd is used, this is ignored.
#   --pch  Generate a precompiled header from a selection of headers in the 'precompiled'. If one of these files is found to be newer than its pch, it will be regenerated
#   --cc   Generate compile_commands.json for use with clangd (requires Build EAR and clang)
#
#   -platform <win32,mac,linux>           Build for specified OS: win32, mac, linux (default: builder's OS)
#   -graphics <vulkan,opengl,directx>     Build for specified Graphics API (default: vulkan)
#   -compiler <cl,gcc,clang,clang-cl>     Build using the specified compiler (default: cl on Windows, gcc on Mac and Linux)
#   -linker <link,ld,lld,lld-link>        Build using the specified linker (default: link on Windows, ld on Mac and Linux)
#   -vulkan_path <path_to_vulkan>         Override the default $VULKAN_SDK path with this path

# TODO(sushi) we need to regenerate pch stuff if the graphics api changes, not sure how to properly detect that though

import os,sys,subprocess,platform,time
from datetime import datetime
from threading import Thread

app_name = "assets_example"

config = {
    "verbose": False, 
    "time": False,
    "buildmode": "debug",
    "profiling": "off", # "off", "on", "on and wait"
    "static_analysis": False,
    "build_analysis": False,
    "build_deshi": True,
    "auto_build_deshi": False,
    "use_pch": False,
    "gen_compcmd": False,

    "platform": "unknown",
    "compiler": "unknown",
    "linker":   "unknown",
    "graphics": "vulkan",
}

match platform.system():
    case 'Windows': 
        config["platform"] = "win32"
        config["compiler"] = "cl"
        config["linker"] = "link"
    case 'Linux': 
        config["platform"] = "linux"
        config["compiler"] = "clang++"
        config["linker"] = "ld"
    case _:
        # TODO(sushi) if we ever setup cross compiling somehow, we need to remove this
        print(f"unsupported platform: {platform.system()}")

#
#  gather command line arguments
#______________________________________________________________________________________________________________________

i = 0
while i < len(sys.argv):
    match sys.argv[i]:
        case "--v":    config["verbose"] = True
        case "--time": config["time"] = True
        case "--d":    continue # dont do anything cause debug is the default
        case "--r":    config["buildmode"] = "release"
        case "--p":    config["profiling"] = "on"
        case "--pw":   config["profiling"] = "on and wait"
        case "--sa":   config["static_analysis"] = True
        case "--ba":   config["build_analysis"] = True
        case "--nd":   config["build_deshi"] = False
        case "--ad":   config["auto_build_deshi"] = True
        case "--pch":  config["use_pch"] = True
        case "--cc":   config["gen_compcmd"] = True

        case "-platform":
            if i != len(sys.argv) - 1:
                i += 1
                config["platform"] = sys.argv[i]
                if config["platform"] not in ("win32", "linux", "mac"):
                    print(f"unknown platform: {sys.argv[i]}, expected one of (win32, linux, mac).")
                    quit()
            else:
                print("expected a platform (win32, linux, max) after switch '-platform'")

        case "-graphics":
            if i != len(sys.argv[i]):
                i += 1
                config["graphics"] = sys.argv[i]
                if config["graphics"] not in ("vulkan", "opengl", "directx"):
                    print(f"unknown api backend: {sys.argv[i]}, expected one of (vulkan, opengl, directx).")
                    quit()
            else:
                print("expected a graphics api (vulkan, opengl, directx) after switch '-graphics'")

        case "-compiler":
            if i != len(sys.argv[i]):
                i += 1
                config["compiler"] = sys.argv[i]
                if config["compiler"] not in ("cl", "gcc", "clang", "clang-cl"):
                    print(f"unknown compiler: {sys.argv[i]}, expected one of (cl, gcc, clang, clang-cl).")
                    quit()
            else:
                print("expected a compiler (cl, gcc, clang, clang-cl) after switch '-compiler'")

        case "-linker":
            if i != len(sys.argv[i]):
                i += 1
                config["linker"] = sys.argv[i]
                if config["linker"] not in ("link", "ld", "lld", "lld-link"):
                    print(f"unknown linker: {sys.argv[i]}, expected one of (link, ld, lld, lld-link).")
                    quit()
            else:
                print("expected a linker (cl, gcc, clang, clang-cl) after switch '-linker'")
        case _:
            if sys.argv[i].startswith("-"):
                print(f"unknown switch: {sys.argv[i]}")
                quit()
    i += 1
# end of cli arg collection

# determines what compilers and linkers are compatible with each platform
# so that extending this is easy
compatibility = {
    "win32": {
        "compiler": ["cl", "clang-cl"],
        "linker": ["link"]
    },
    "linux":{
        "compiler": ["clang", "gcc", "clang++"],
        "linker": ["ld", "lld", "lld-link", "clang++", "clang"]
    }
}

# setup folders
# this assumes that the build script is in a misc folder that is in the root of the repo
folders = {}
folders["root"] = os.path.dirname(__file__)
folders["build"] = f"{folders['root']}/build/{config['buildmode']}"

if not os.path.exists(folders["build"]):
    os.makedirs(folders["build"])

os.chdir(folders["root"])

# check if deshi has any source files newer than the last time it was built
def check_newer_files():
    if os.path.exists(f"{folders['build']}/deshi.o"):
        lasttime = os.path.getmtime(f"{folders['build']}/deshi.o")
        for root, dirs, files in os.walk(f"{folders['root']}/deshi/src"):
            for file in files:
                if os.path.getmtime(f"{root}/{file}") > lasttime:
                    return True
    else:
        return True # if deshi.o does not exist then I guess we have to build it..?
    return False

# if --ad is used and deshi is found to have newer sources that its object file, then we rebuild it
if config["auto_build_deshi"] and config["build_deshi"] and not check_newer_files():
    config["build_deshi"] = False

#
#  data
#______________________________________________________________________________________________________________________

includes = (
    "-I../../src "
    "-I../../src/external "
)

if config["graphics"] == "vulkan":
    includes += f''

sources = {
    "deshi": "../../src/deshi.cpp",
    "app": "main.cpp"
}

parts = {
    "link":{ # various things handed to the linker
        "win32": { 
            "always": ["gdi32", "shell32", "ws2_32", "winmm"],
            "vulkan": ["vulkan-1", "shaderc_combined"],
            "opengl": ["opengl32"],
            "paths": []
        },
        "linux": {
            "always": ["X11", "Xrandr", "Xcursor"],
            "vulkan": ["vulkan", "shaderc_shared"],
            "opengl": ["GL"],
            "paths": []
        },
        "flags":{
            **dict.fromkeys(["link", "lld-link"], ( # NOTE(sushi) this is just assigning this value to both link and lld-link, so it doesnt appear twice
                "-nologo "         # prevents microsoft copyright banner
                "-opt:ref "        # doesn't link functions and data that are never used 
                "-incremental:no " # relink everything
            )),
            **dict.fromkeys(["ld", "lld"], "")
        },
        "prefix":{
            **dict.fromkeys(["link", "lld-link"], {
                "path": "-libpath:",
                "file": "",
            }),
            **dict.fromkeys(["ld", "lld"], {
                "path": "-L",
                "file": "-l",
            }),
        }
    },


    "defines":{ 
        "buildmode": {
            "release": "-DBUILD_INTERNAL=0 -DBUILD_SLOW=0 -DBUILD_RELEASE=1 ",
            "debug": "-DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_RELEASE=0 -DDESHI_DISABLE_ASSERT_ON_ERROR=1 -DRENDER_REWRITE ",
        },
        "platform":{
            "win32": "-DDESHI_WINDOWS=1 -DDESHI_MAC=0 -DDESHI_LINUX=0 ",
            "linux": "-DDESHI_WINDOWS=0 -DDESHI_MAC=0 -DDESHI_LINUX=1 ",
            "mac":   "-DDESHI_WINDOWS=0 -DDESHI_MAC=1 -DDESHI_LINUX=0 ",
        },
        "graphics":{
            "vulkan":  "-DDESHI_VULKAN=1 -DDESHI_OPENGL=0 -DDESHI_DIRECTX12=0 ",
            "opengl":  "-DDESHI_VULKAN=0 -DDESHI_OPENGL=1 -DDESHI_DIRECTX12=0 ",
            "directx": "-DDESHI_VULKAN=0 -DDESHI_OPENGL=0 -DDESHI_DIRECTX12=1 ",
        },
        "profiling":{
            "on":          "-DTRACY_ENABLE ",
            "on and wait": "-DTRACY_ENABLE -DDESHI_WAIT_FOR_TRACY_CONNECTION ",
            "off": "",
        },
    },


    "compiler_flags":{

        "cl": {
            "always":( # flags always applied if this compiler is chosen
                "-diagnostics:column " # prints diagnostics on one line, giving column number
                "-EHsc "               # enables C++ exception handling and defaults 'extern "C"' to nothrow
                "-nologo "             # prevents initial banner from displaying
                "-MD "                 # create a multithreaded dll
                "-Oi "                 # generates intrinsic functions
                "-GR "                 # enables runtime type information
                "-std:c++17 "          # use C++17
                "-utf-8 "              # read source as utf8
                "-MP "                 # builds multiple source files concurrently
                "-W1"                  # warning level 1
                "-wd4100"              # unused function parameter
                "-wd4189"              # unused local variables
                "-wd4201"              # nameless unions and structs
                "-wd4311"              # pointer truncation
                "-wd4706"              # assignment within conditional expression
            ),
            "release": (
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug": (
                "-Z7 " # generates C 7.0-compatible debugging information
                "-Od " # disables optimization complete
            )
        },

        "clang-cl": {
            "always":( # flags always applied if this compiler is chosen
                "-diagnostics:column "  # prints diagnostics on one line, giving column number
                "-EHsc "                # enables C++ exception handling and defaults 'extern "C"' to nothrow
                "-nologo "              # prevents initial banner from displaying
                "-MD "                  # create a multithreaded dll
                "-Oi "                  # generates intrinsic functions
                "-GR "                  # enables runtime type information
                "-std:c++17 "           # use C++17
                "-utf-8 "               # read source as utf8
                "-msse3 "               # enables SSE
                "-Wno-unused-value "                # 
                "-Wno-writable-strings "            # conversion from string literals to other things
                "-Wno-implicitly-unsigned-literal " # 
                "-Wno-nonportable-include-path "    # 
                "-Wno-unused-function "             #
                "-Wno-unused-variable "             #
                "-Wno-undefined-inline "            #
            ),
            "release": (
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug": (
                "-Z7 " # generates C 7.0-compatible debugging information
                "-Od " # disables optimization completely
            ),
            "analyze": "--analyze "
        },

        "gcc": {
            "not implemented yet"
        },

        "clang++":{
            "always": ( # flags always applied if this compiler is chosen
                "-std=c++17 "         # use the c++17 standard
                "-fexceptions "       # enable exception handling
                "-fcxx-exceptions "   # enable c++ exceptions
                "-finline-functions " # inlines suitable functions
                "-pipe "              # use pipes between commands when possible
                "-msse3 "             # enables sse
                "-Wno-unused-value "
                "-Wno-implicitly-unsigned-literal "
                "-Wno-nonportable-include-path "
                "-Wno-writable-strings "
                "-Wno-unused-function "
                "-Wno-unused-variable "
                "-Wno-undefined-inline "
                "-Wno-return-type-c-linkage "
                "-Wno-switch "
                "-Wno-deprecated-anon-enum-enum-conversion "
                "-fno-caret-diagnostics "
            ),
            "release":(
                "-O2 " # maximizes speed (O1 minimizes size)
            ),
            "debug":(
                "-fdebug-macro " # output macro information
                "-ggdb3 " # output debug information for gdb
                "-O0 " # disable optimization completely
            ),
            "analyze": {
                True: "--analyze ",
                False: ""
            }
        }
    }
}

# make sure that all the chosen options are compatible
if config["compiler"] not in compatibility[config["platform"]]["compiler"]:
    print(f"compiler {config['compiler']} is not compatible with the platform {config['platform']}")
    quit()
if config["linker"] not in compatibility[config["platform"]]["linker"]: 
    print(f"linker {config['linker']} is not compatible with the platform {config['platform']}")
    quit()

if config["build_analysis"]:
    if config["compiler"] != "clang++":
        print("Build analysis (--ba) is only available with clang.")
        quit()
    import shutil
    if shutil.which("ClangBuildAnalyzer") is None:
        print("Build analysis (--ba) is enabled, but ClangBuildAnalyzer is not installed.")
        quit()
    parts["compiler_flags"]["clang++"]["always"] += "-ftime-trace"

#
#  construct compiler commands
#______________________________________________________________________________________________________________________

header = f'{datetime.now().strftime("%a, %h %d %Y, %H:%M:%S")} ({config["compiler"]}/{config["buildmode"]}/{config["graphics"]}) [{app_name}]'
print(header)
print('-'*len(header))

# special vulkan case where we need to grab the sdk path
if config["graphics"] == "vulkan":
    vpath = os.getenv("VULKAN_SDK", None)
    if vpath == None:
        print("the chosen graphics API is Vulkan, but the environment variable VULKAN_SDK is not set")
    parts["link"][config["platform"]]["paths"].append(f'{vpath}lib')
    includes += f'-I{vpath}include '

link = {
    "flags": "",
    "libs": "",
    "paths": ""
}

defines = (
    parts["defines"]["buildmode"][config["buildmode"]] +
    parts["defines"]["platform"][config["platform"]] +
    parts["defines"]["graphics"][config["graphics"]] +
    parts["defines"]["profiling"][config["profiling"]]
)

link_names = (
    parts["link"][config["platform"]]["always"] +
    parts["link"][config["platform"]][config["graphics"]]
)

nameprefix = parts['link']['prefix'][config['linker']]['file']
for ln in link_names:
    link["libs"] += f"{nameprefix}{ln} "

link_paths = parts["link"][config["platform"]]["paths"]
pathprefix = parts['link']['prefix'][config['linker']]['path']
for lp in link_paths:
    link["paths"] += f"{pathprefix}{lp} "

shared = (
    f'{defines} '
    f'{includes} '
    f'{parts["compiler_flags"][config["compiler"]]["always"]} '
    f'{parts["compiler_flags"][config["compiler"]][config["buildmode"]]} '
    f'{parts["compiler_flags"][config["compiler"]]["analyze"][config["static_analysis"]]} '
)

# TODO(sushi) look more into precompiling
# cmd = f"clang -c -xc++-header test.h -o test.h.pch {shared}"
# print(cmd)
# subprocess.Popen(cmd.split(' ')).wait()

if config["use_pch"]:
    f = open("src/pch.h", "r")
    buff = f.read()
    f.close()

    pchlasttime = 0
    if os.path.exists(f"{folders['build']}/pch.h.pch"):
        pchlasttime = os.path.getmtime(f"{folders['build']}/pch.h.pch")

    buff = buff.split("*/")[1]
    includes = list(filter(len, [a.strip().strip('"') for a in buff.split("#include")]))
    def regen_pch():
        for include in includes:
            if os.path.getmtime(include) > pchlasttime:
                return True
        return False
    if os.path.getmtime("src/pch.h") > pchlasttime or regen_pch():
        subprocess.Popen(f"clang -c -xc++-header -I. src/pch.h -o {folders['build']}/pch.h.pch {shared}".split(' ')).wait()

    shared += f"-include-pch {folders['build']}/pch.h.pch "

full_deshi = (
    f'{config["compiler"]} -c '
    f'{sources["deshi"]} ' + shared + 
    f'-o {folders["build"]}/deshi.o'
) 

# dream on it 

# f = open("deshi/src/external/stb/stb_image.h", "r")
# buff = f.read()
# startidx = buff.find("#ifdef STB_IMAGE_IMPLEMENTATION")
# endidx = buff.find("#endif // STB_IMAGE_IMPLEMENTATION")
# print(buff[idx:idx+128])

full_app = (
    f'{config["compiler"]} -c '
    f'{sources["app"]} ' + shared +
    f'-o {folders["build"]}/{app_name}.o' 
)

if config["gen_compcmd"]:
    import re
    shared_args = []
    shared_args.append(config["compiler"])
    for s in shared.split(' '):
        if s and not s.isspace():
            shared_args.append(s)
    out = "["
    cfiles = []
    hfiles = []
    with open(sources["app"], "r") as f:
        includes = re.findall(r'#\s*include\s*?["|<](.*?)["|>]', f.read())
        for include in includes:
            if include.endswith('.cpp'):
                cfiles.append(include)
            else:
                hfiles.append(include)
    with open(sources["deshi"], "r") as f:
        includes = re.findall(r'#\s*include\s*?["|<](.*?)["|>]', f.read())
        for include in includes:
            if include.endswith('.cpp'):
                cfiles.append(include)
            else:
                hfiles.append(include)
    shared_args.append("-DDESHI_IMPLEMENTATION")
    includes = ""
    for h in hfiles:
        includes += '"-include' + h + '",\n'
    for c in cfiles:
        out += '{\n"arguments": [\n'
        out += "\n".join(['"' + arg + '",' for arg in shared_args])
        out += '"-includedeshi/src/deshi.cpp",\n'
        out += includes
        out += '"-c",'
        out += '"' + os.path.abspath(c[0]) + '/' + c[1] + '"\n'
        out += '],'
        out += f'"file": "{os.path.abspath(c)}",'
        out += f'"directory": "{folders["root"]}"}},'
    out = out[0:-1]
    out += "]"
    with open('../../../compile_commands.json', "w") as f:
        f.write(out)

full_link = (
    f'{config["compiler"]} ' # NOTE(sushi) this should not be the compiler, but it is for now cause that is how it is on linux for me so fix later please
    f'{folders["build"]}/deshi.o {folders["build"]}/{app_name}.o '
    f'{link["flags"]} '
    f'{link["libs"]} '
    f'{link["paths"]} '
    f'-o {folders["build"]}/{app_name} '
)

def format_time(time):
    one_second = 1
    one_minute = 60*one_second
    if not time: return "0 seconds"
    if time > one_minute:
        n_minutes = time//one_minute
        return f"{n_minutes} minute{'s' if n_minutes != 1 else ''} {format_time(time-n_minutes*one_minute)}"
    return f"{time} second{'s' if time != 1 else ''}"

def run_proc(name, cmd):
    if config["verbose"]: print(cmd)
    start = time.time()
    dproc = subprocess.Popen(cmd.split(' '))
    dproc.communicate()
    taken = format_time(round(time.time()-start, 3))
    if not dproc.returncode:
        print(f'  \033[32m{name}\033[0m - {taken}')
    else:
        print(f'  \033[31m{name} failed to build\033[0m - {taken}')

start = time.time()
dproc = Thread(target=run_proc, args=("deshi", full_deshi))
aproc = Thread(target=run_proc, args=(app_name, full_app))

baproc = None
if config["build_analysis"]:
    subprocess.Popen(f"ClangBuildAnalyzer --start {folders['build']}/".split(' '), stdout=subprocess.PIPE).wait()

if config["build_deshi"]: 
    dproc.start()

aproc.start()

if config["build_deshi"]: 
    dproc.join()

aproc.join()

if config["build_analysis"]:
    subprocess.Popen(f"ClangBuildAnalyzer --stop {folders['build']}/ {folders['build']}/out".split(' '), stdout=subprocess.PIPE).wait()
    proc = subprocess.Popen(f"ClangBuildAnalyzer --analyze {folders['build']}/out".split(' '), stdout=subprocess.PIPE)
    analysis = proc.communicate()[0].decode()
    file = open(f"{folders['build']}/ctimeanalysis", "w")
    file.write(analysis)
    file.close()

lproc = Thread(target=run_proc, args=("exe", full_link))
lproc.start()
lproc.join()

print(f"time: {format_time(round(time.time()-start, 3))}")
