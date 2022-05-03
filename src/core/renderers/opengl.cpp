/* deshi OpenGL Render Submodule
Index:
@gl_types
@gl_vars

*/


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_types
struct ShaderGl{
	str8 filename;
	u32 handle;
	ShaderStage stage;
};

struct ProgramGl{
	u32 handle;
	u32 shader_count;
	u32 shaders[5];
};

struct TextureGl{
	Texture* base;
	GLenum format;
	GLenum type;
	u32 handle;
};

struct MaterialGl{
	Material* base;
	ProgramGl* program;
	u32 texture_count;
	u32 textures[4];
};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_vars
#define INDEX_TYPE_GL_TWOD GL_UNSIGNED_INT
#define INDEX_TYPE_GL_TEMP GL_UNSIGNED_SHORT
#define INDEX_TYPE_GL_MESH GL_UNSIGNED_INT

local array<RenderMesh> glMeshes(deshi_allocator);
local array<ShaderGl>   glShaders(deshi_allocator);
local array<TextureGl>  glTextures(deshi_allocator);
local array<MaterialGl> glMaterials(deshi_allocator);

local s32  width  = 0;
local s32  height = 0;
local b32  initialized  = false;
local b32  remake_window = false;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_vars_instance
local void* opengl_module = 0;
local void* opengl_context = 0;
local s32 opengl_success = 0;
local int opengl_version = 0;
local int wgl_version = 0;
#define OPENGL_INFOLOG_SIZE 512
local char opengl_infolog[OPENGL_INFOLOG_SIZE] = {};

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_vars_buffers
local struct{
	u32 vao_handle;
	u32 vbo_handle;
	u32 ibo_handle;
	u32 vbo_size;
	u32 vbo_alloc;
	u32 ibo_size;
	u32 ibo_alloc;
} meshBuffers{};

local struct{
	u32 vao_handle;
	u32 vbo_handle;
	u32 ibo_handle;
	u32 vbo_size;
	u32 vbo_alloc;
	u32 ibo_size;
	u32 ibo_alloc;
} uiBuffers{};

local struct{
	u32 vao_handle;
	u32 vbo_handle;
	u32 ibo_handle;
	u32 vbo_alloc;
	u32 ibo_alloc;
} tempBuffers{};

local struct{ //vertex shader uniform buffer
	u32 handle;
	u32 binding = 1;
	
	struct{ //416 bytes
		mat4 view;        //camera view matrix
		mat4 proj;        //camera projection matrix
		vec4 lights[10];  //lights
		vec4 viewPos;     //camera pos
		vec2 screen;      //screen dimensions
		vec2 mousepos;    //mouse screen pos
		vec3 mouseWorld;  //point casted out from mouse 
		f32  time;        //total time
		mat4 lightVP;     //first light's view projection matrix
		s32  enablePCF;   //whether to blur shadow edges //TODO(delle,ReVu) convert to specialization constant
		s32  padding[3];
	} values;
} uboVS{};

local struct{ //vertex shader push constant
	u32 handle;
	u32 binding = 2;
	
	struct{ //64 bytes
		mat4 matrix;
	} values;
} pushVS{};

local struct{ //twod vertex shader push constant
	u32 handle;
	u32 binding = 3;
	
	struct{ //32 bytes
		vec2 scale;
		vec2 translate;
		s32  font_idx;
		s32  padding[3];
	} values;
} push2D{};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_vars_programs
local struct{
	union{
		ProgramGl arr[16];
		struct{
			ProgramGl null;
			//game shaders
			ProgramGl flat;
			ProgramGl phong;
			ProgramGl pbr;
			ProgramGl lavalamp;
			ProgramGl twod;
			ProgramGl ui;
			//development shaders
			ProgramGl base;
			ProgramGl wireframe;
			ProgramGl wireframe_depth;
			ProgramGl selected;
			ProgramGl collider;
			ProgramGl testing0;
			ProgramGl testing1;
			ProgramGl offscreen;
			//debug shaders
			ProgramGl normals_debug;
			ProgramGl shadowmap_debug;
		};
	};
} programs{};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_funcs_utils
#define PrintGl(level, ...) if(renderSettings.loggingLevel >= level){ logger_push_indent(level); Log("opengl", __VA_ARGS__); logger_pop_indent(level); }(void)0


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_funcs_init
#if DESHI_WINDOWS
local void
WGLDebugPostCallback(void *ret, const char *name, GLADapiproc apiproc, s32 len_args, ...){
	DWORD error_code = GetLastError();
	const char* error_flag = 0;
	const char* error_msg  = 0;
	switch(error_code){
		case ERROR_INVALID_VERSION_ARB:{
			error_flag = STRINGIZE(ERROR_INVALID_VERSION_ARB);
			error_msg  = "If attributes WGL_CONTEXT_MAJOR_VERSION_ARB and WGL_CONTEXT_MINOR_VERSION_ARB, when considered together with WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, specify an OpenGL version and feature set that are not defined. Read: https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt";
		}break;
		case ERROR_INVALID_PROFILE_ARB:{
			error_flag = STRINGIZE(ERROR_INVALID_PROFILE_ARB);
			error_msg  = "If attribute WGL_CONTEXT_PROFILE_MASK_ARB has no bits set; has any bits set other than WGL_CONTEXT_CORE_PROFILE_BIT_ARB and WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB; has more than one of these bits set; or if the implementation does not supported the requested profile. Read: https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt";
		}break;
		default:{
			error_flag = "?";
			error_msg  = "?";
		}break;
	}
	LogE("wgl","ERROR_",(u32)error_code," '",error_flag,"' on ",name,"(); Reason: ",error_msg);
	if(renderSettings.crashOnError) Assert(!"crashing because of error in opengl");
}
#endif //DESHI_WINDOWS

local void
GladDebugPostCallback(void *ret, const char *name, GLADapiproc apiproc, s32 len_args, ...){DPZoneScoped;
	GLenum error_code = glad_glGetError();
	if(error_code != GL_NO_ERROR){
		const char* error_flag = 0;
		const char* error_msg  = 0;
		switch(error_code){
			case GL_INVALID_ENUM:{
				error_flag = "GL_INVALID_ENUM";
				error_msg  = "Set when an enumeration parameter is not legal.";
			}break;
			case GL_INVALID_VALUE:{
				error_flag = "GL_INVALID_VALUE";
				error_msg  = "Set when a value parameter is not legal.";
			}break;
			case GL_INVALID_OPERATION:{
				error_flag = "GL_INVALID_OPERATION";
				error_msg  = "Set when the state for a command is not legal for its given parameters.";
			}break;
			case 1283:{
				error_flag = "GL_STACK_OVERFLOW";
				error_msg  = "Set when a stack pushing operation causes a stack overflow.";
			}break;
			case 1284:{
				error_flag = "GL_STACK_UNDERFLOW";
				error_msg  = "Set when a stack popping operation occurs while the stack is at its lowest point.";
			}break;
			case GL_OUT_OF_MEMORY:{
				error_flag = "GL_OUT_OF_MEMORY";
				error_msg = "Set when a memory allocation operation cannot allocate (enough) memory.";
			}break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:{
				error_flag = "GL_INVALID_FRAMEBUFFER_OPERATION";
				error_msg = "Set when reading or writing to a framebuffer that is not complete.";
			}break;
		}
		LogE("opengl","ERROR_",error_code," '",error_flag,"' on ",name,"(); Reason: ",error_msg,"; Info: http://docs.gl/gl3/",name);
		if(renderSettings.crashOnError) Assert(!"crashing because of error in opengl");
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_funcs_shaders
local u32
CompileAndLoadShader(str8 filename, ShaderStage stage){
	Assert(stage > ShaderStage_NONE && stage < ShaderStage_COUNT);
	
	//check if already loaded
	forE(glShaders){
		if(str8_equal_lazy(it->filename, filename)){
			return u32(it-it_begin);
		}
	}
	
	ShaderGl sgl{};
	sgl.filename = str8_copy(filename, deshi_allocator);
	sgl.stage    = stage;
	
	//create shader, load file, and compile shader
	switch(stage){ //TODO(delle) opengl4 shader stages
		case ShaderStage_Vertex:   sgl.handle = glCreateShader(GL_VERTEX_SHADER);   break;
		case ShaderStage_TessCtrl: Assert(!"not implemented yet REQUIRES OPENGL4"); break;
		case ShaderStage_TessEval: Assert(!"not implemented yet REQUIRES OPENGL4"); break;
		case ShaderStage_Geometry: sgl.handle = glCreateShader(GL_GEOMETRY_SHADER); break;
		case ShaderStage_Fragment: sgl.handle = glCreateShader(GL_FRAGMENT_SHADER); break;
		case ShaderStage_Compute:  Assert(!"not implemented yet REQUIRES OPENGL4"); break;
	}
	
	str8 file_name = str8_concat(str8_lit("data/shaders/"), filename, deshi_temp_allocator);
	str8 contents = file_read_simple(file_name, deshi_temp_allocator);
	if(!contents) Assert(!"Failed to load shader");
	const char* str = (const char*)contents.str; int len = (int)contents.count;
	glShaderSource(sgl.handle, 1, &str, &len);
	glCompileShader(sgl.handle);
	
	//check for errors
	glGetShaderiv(sgl.handle, GL_COMPILE_STATUS, &opengl_success);
	if(opengl_success != GL_TRUE){
		glGetShaderInfoLog(sgl.handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
		PrintGl(0, "Failed to compile shader '",filename,"':\n",opengl_infolog);
		glDeleteShader(sgl.handle);
		return 0;
	}
	
	glShaders.add(sgl);
	return glShaders.count-1;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_funcs_programs
//TODO(delle) cleanup shaders maybe? glDeleteShader()
local ProgramGl
CreateProgram(u32 shader_indexes[], u32 shader_count, bool twod = false){
	if(shader_count == 0) return {};
	//TODO(delle) Assert(ubo's setup)
	
	ProgramGl pgl{};
	pgl.shader_count = shader_count;
	str8_builder prog_shaders;
	str8_builder_init(&prog_shaders, str8_lit("|"), deshi_temp_allocator);
	
	//create program, attach shaders and link
	pgl.handle = glCreateProgram();
	forI(shader_count){ 
		glAttachShader(pgl.handle, glShaders[shader_indexes[i]].handle); 
		pgl.shaders[i] = shader_indexes[i]; 
		str8_builder_append(&prog_shaders, glShaders[shader_indexes[i]].filename);
		str8_builder_append(&prog_shaders, str8_lit("|"));
	}
	glLinkProgram(pgl.handle);
	
	//check for errors
	glGetProgramiv(pgl.handle, GL_LINK_STATUS, &opengl_success);
	if(opengl_success != GL_TRUE){
		glGetProgramInfoLog(pgl.handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
		PrintGl(0,"Failed to link program '",prog_shaders.str,"':\n",opengl_infolog);
		
		//delete broken program
		glDeleteProgram(pgl.handle);
		return {};
	}
	
	//specify UBOs
	if(!twod){
		u32 local_ubi = glGetUniformBlockIndex(pgl.handle,"UniformBufferObject");
		if(local_ubi != -1){ 
			glUniformBlockBinding(pgl.handle, local_ubi, uboVS.binding);
		}else{
			PrintGl(0,"Failed to find UniformBufferObject in vertex shader of program '",prog_shaders.str,"'");
			if(renderSettings.crashOnError) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
		
		local_ubi = glGetUniformBlockIndex(pgl.handle,"PushConsts");
		if(local_ubi != -1){ 
			glUniformBlockBinding(pgl.handle, local_ubi, pushVS.binding);
		}else{
			PrintGl(0,"Failed to find PushConsts in vertex shader of program '",prog_shaders.str,"'");
			if(renderSettings.crashOnError) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
	}else{
		u32 local_ubi = glGetUniformBlockIndex(pgl.handle,"PushConsts");
		if(local_ubi != -1){ 
			glUniformBlockBinding(pgl.handle, local_ubi, push2D.binding);
		}else{
			PrintGl(0,"Failed to find PushConsts in vertex shader of program '",prog_shaders.str,"'");
			if(renderSettings.crashOnError) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
	}
	
	//detach shaders
	forI(shader_count){ glDetachShader(pgl.handle, glShaders[shader_indexes[i]].handle); }
	
	return pgl;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_funcs_imgui
local char iniFilepath[256] = {};
void DeshiImGui::
Init(){
	DeshiStageInitStart(DS_IMGUI, DS_RENDER, "Attempted to initialize ImGui module before initializing Render module");
	
	//Setup Dear ImGui context
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	cpystr(iniFilepath, "data/config/imgui.ini", 256);
	io.IniFilename = iniFilepath;
	
	//Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	
	//Setup Platform/Renderer backends
#if DESHI_WINDOWS
	ImGui_ImplWin32_Init((HWND)DeshWindow->handle);
#elif DESHI_LINUX
	ImGui_ImplGlfw_InitForopenGl(DeshWindow->window, true);
#elif DESHI_MAC
	ImGui_ImplGlfw_InitForopenGl(DeshWindow->window, true);
#endif
	ImGui_ImplOpenGL3_Init();
	
	DeshiStageInitEnd(DS_IMGUI);
}

void DeshiImGui::
Cleanup(){
	ImGui_ImplOpenGL3_Shutdown();
#if DESHI_WINDOWS
	ImGui_ImplWin32_Shutdown();
#elif DESHI_LINUX
	ImGui_ImplGlfw_Shutdown();
#elif DESHI_MAC
	ImGui_ImplGlfw_Shutdown();
#endif
	ImGui::DestroyContext();
}

void DeshiImGui::
NewFrame(){
	ImGui_ImplOpenGL3_NewFrame();
#if DESHI_WINDOWS
	ImGui_ImplWin32_NewFrame();
#elif DESHI_LINUX
	ImGui_ImplGlfw_Shutdown();
#elif DESHI_MAC
	ImGui_ImplGlfw_NewFrame();
#endif
	ImGui::NewFrame();
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_init
void
render_init(){DPZoneScoped;
	DeshiStageInitStart(DS_RENDER, DS_MEMORY|DS_WINDOW, "Attempted to initialize OpenGL module before initializing Memory/Window modules");
	Log("opengl","Starting opengl renderer initialization");
	logger_push_indent();
	
	//create the shaders directory if it doesn't exist already
	file_create(str8_lit("data/shaders/"));
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// load render settings
	render_load_settings();
	if(renderSettings.debugging && renderSettings.printf) renderSettings.loggingLevel = 4;
	
	DeshWindow->GetWindowSize(width, height);
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup WGL and glad
	{
		opengl_module = platform_load_module("opengl32.dll");
		if(!opengl_module){
			LogE("opengl", "Failed to load module opengl32.dll");
		}
		
#if DESHI_WINDOWS
		//restore point for contexts
		HDC   prev_dc = wglGetCurrentDC();
		HGLRC prev_rc = wglGetCurrentContext();
		
		//create dummy window to load extensions
		WNDCLASSW wc{CS_OWNDC, WndProc, 0,0, (HINSTANCE)DeshWindow->instance, 0,0,0,0, L"deshi temp"};
		if(!RegisterClassW(&wc)){ Win32LogLastError("RegisterClassW", renderSettings.crashOnError); return; }
		
		HWND temp_wnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, L"deshi temp", L"deshi opengl init window",
										WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0,0, 1,1, 0, 0, (HINSTANCE)DeshWindow->instance, 0);
		if(!temp_wnd){ Win32LogLastError("CreateWindowW", renderSettings.crashOnError); return; }
		HDC temp_dc = GetDC(temp_wnd);
		ShowWindow(temp_wnd, SW_HIDE);
		MSG msg;
		while(PeekMessageW(&msg, temp_wnd, 0, 0, PM_REMOVE)){ TranslateMessage(&msg); DispatchMessageW(&msg); }
		
		//setup pixel format for dummy device context
		PIXELFORMATDESCRIPTOR temp_pfd{sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER};
		temp_pfd.cColorBits = 32; temp_pfd.cDepthBits = 24; temp_pfd.cStencilBits = 8; temp_pfd.iLayerType = PFD_MAIN_PLANE;
		int temp_format = ChoosePixelFormat(temp_dc, &temp_pfd);
		if(!SetPixelFormat(temp_dc, temp_format, &temp_pfd)){ Win32LogLastError("SetPixelFormat", renderSettings.crashOnError); return; }
		
		//create and enable dummy render context
		HGLRC temp_context = wglCreateContext(temp_dc);
		if(!temp_context){ Win32LogLastError("wglCreateContext", renderSettings.crashOnError); return; }
		wglMakeCurrent(temp_dc, temp_context);
		
		//load wgl extensions
		wgl_version = gladLoaderLoadWGL(temp_dc);
		if(wgl_version == 0){ LogE("opengl","Failed to load OpenGL"); return; }
		Logf("opengl","Loaded WGL %d.%d", GLAD_VERSION_MAJOR(wgl_version), GLAD_VERSION_MINOR(wgl_version));
		gladInstallWGLDebug();
		gladSetWGLPostCallback(WGLDebugPostCallback);
		
		//delete dummy window and context
		wglMakeCurrent(prev_dc, prev_rc);
		wglDeleteContext(temp_context);
		DestroyWindow(temp_wnd);
		UnregisterClassW(L"deshi temp", (HINSTANCE)DeshWindow->instance);
		
		//set pixel format for device context
		int format = 0;
		UINT format_count = 0;
		PIXELFORMATDESCRIPTOR pfd;
		const int format_attributes[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
			WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB,     32,
			WGL_DEPTH_BITS_ARB,     24,
			WGL_STENCIL_BITS_ARB,   8,
			WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
			0
		};
		wglChoosePixelFormatARB((HDC)DeshWindow->dc, format_attributes, 0, 1, &format, &format_count); //https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
		if(!DescribePixelFormat((HDC)DeshWindow->dc, format, sizeof(pfd), &pfd)){  Win32LogLastError("DescribePixelFormat", renderSettings.crashOnError); return;  }
		if(format == 0){ Win32LogLastError("ChoosePixelFormatARB", renderSettings.crashOnError); return; }
		if(!SetPixelFormat((HDC)DeshWindow->dc, format, &pfd)){ Win32LogLastError("SetPixelFormat", renderSettings.crashOnError); return; }
		
		//set the desired OpenGL version and render context settings
		int context_attributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 2,
#if BUILD_INTERNAL
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
#else
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#endif
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};
		
		//create actual render context and delete temporary one
		opengl_context = wglCreateContextAttribsARB((HDC)DeshWindow->dc, 0, context_attributes); //https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
		if(!opengl_context){ Win32LogLastError("wglCreateContextAttribsARB", renderSettings.crashOnError); return; }
		wglMakeCurrent((HDC)DeshWindow->dc, (HGLRC)opengl_context);
		
		//load glad extensions
		opengl_version = gladLoaderLoadGL();
		
#else
		glfwMakeContextCurrent((HWND)DeshWindow->handle);
		opengl_version = gladLoadGL(glfwGetProcAddress);
#endif
		
		if(opengl_version == 0){ LogE("opengl","Failed to load OpenGL"); return; }
		Logf("opengl","Loaded OpenGL %d.%d", GLAD_VERSION_MAJOR(opengl_version), GLAD_VERSION_MINOR(opengl_version));
		gladInstallGLDebug();
		gladSetGLPostCallback(GladDebugPostCallback);
		
		UpdateWindow((HWND)DeshWindow->handle);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup uniform buffers
	{
		//vertex shader ubo
		glGenBuffers(1, &uboVS.handle);
		glBindBufferRange(GL_UNIFORM_BUFFER, uboVS.binding, uboVS.handle, 0, sizeof(uboVS.values));
		
		//vertex shader push constant
		glGenBuffers(1, &pushVS.handle);
		glBindBufferRange(GL_UNIFORM_BUFFER, pushVS.binding, pushVS.handle, 0, sizeof(pushVS.values));
		glBufferData(GL_UNIFORM_BUFFER, sizeof(pushVS.values), 0, GL_DYNAMIC_DRAW);
		
		//twod vertex shader push constant
		glGenBuffers(1, &push2D.handle);
		glBindBufferRange(GL_UNIFORM_BUFFER, push2D.binding, push2D.handle, 0, sizeof(push2D.values));
		glBufferData(GL_UNIFORM_BUFFER, sizeof(push2D.values), 0, GL_DYNAMIC_DRAW);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup pipelines
	{
		u32 shaders[5] = {};
		u32 shader_count = 0;
		
		{//base
			shaders[0] = CompileAndLoadShader(str8_lit("base_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("base_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.base = CreateProgram(shaders, shader_count);
			
			{//selected
				
			}
		}
		
		{//null
			shaders[0] = CompileAndLoadShader(str8_lit("null_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("null_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.null = CreateProgram(shaders, shader_count);
		}
		
		{//flat
			shaders[0] = CompileAndLoadShader(str8_lit("flat_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("flat_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.flat = CreateProgram(shaders, shader_count);
		}
		
		{//phong
			shaders[0] = CompileAndLoadShader(str8_lit("phong_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("phong_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.phong = CreateProgram(shaders, shader_count);
		}
		
		{//pbr
			shaders[0] = CompileAndLoadShader(str8_lit("pbr_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("pbr_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.pbr = CreateProgram(shaders, shader_count);
		}
		
		{//2d
			shaders[0] = CompileAndLoadShader(str8_lit("twod_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("twod_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.twod = CreateProgram(shaders, shader_count, true);
			
			{//ui
				shaders[0] = CompileAndLoadShader(str8_lit("ui_gl3.vert"), ShaderStage_Vertex);
				shaders[1] = CompileAndLoadShader(str8_lit("ui_gl3.frag"), ShaderStage_Fragment);
				shader_count = 2;
				programs.ui = CreateProgram(shaders, shader_count, true);
				
				
			}
		}
		
		{//wireframe
			shaders[0] = CompileAndLoadShader(str8_lit("wireframe_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("wireframe_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.wireframe = CreateProgram(shaders, shader_count);
			
			{//wireframe with depth test
				
			}
		}
		
		{//lavalamp
			shaders[0] = CompileAndLoadShader(str8_lit("lavalamp_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("lavalamp_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.lavalamp = CreateProgram(shaders, shader_count);
		}
		
		{//offscreen
			shaders[0] = CompileAndLoadShader(str8_lit("offscreen_gl3.vert"), ShaderStage_Vertex);
			shader_count = 1;
			programs.offscreen = CreateProgram(shaders, shader_count);
		}
		
		{//testing0
			shaders[0] = CompileAndLoadShader(str8_lit("testing0_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("testing0_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.testing0 = CreateProgram(shaders, shader_count);
		}
		
		{//testing1
			shaders[0] = CompileAndLoadShader(str8_lit("testing1_gl3.vert"), ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("testing1_gl3.frag"), ShaderStage_Fragment);
			shader_count = 2;
			programs.testing1 = CreateProgram(shaders, shader_count);
		}
		
		{//DEBUG mesh normals
			shaders[0] = CompileAndLoadShader(str8_lit("nothing_gl3.vert"),     ShaderStage_Vertex);
			shaders[1] = CompileAndLoadShader(str8_lit("normaldebug_gl3.geom"), ShaderStage_Geometry);
			shaders[2] = CompileAndLoadShader(str8_lit("nothing_gl3.frag"),     ShaderStage_Fragment);
			shader_count = 3;
			programs.normals_debug = CreateProgram(shaders, shader_count);
		}
		
		{//DEBUG shadow map
			//shaders[0] = CompileAndLoadShader(str8_lit("shadowmapDEBUG.vert"), ShaderStage_Vertex);
			//shaders[1] = CompileAndLoadShader(str8_lit("shadowmapDEBUG.frag"), ShaderStage_Fragment);
			//shader_count = 2;
			//programs.shadowmap_debug = CreateProgram(shaders, shader_count);
		}
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup commands
	forI(TWOD_LAYERS) renderTwodCmdCounts[renderActiveSurface][i] = 1;
	
	initialized = true;
	logger_pop_indent();
	DeshiStageInitEnd(DS_RENDER);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_update
void
render_update(){DPZoneScoped;
	Stopwatch update_stopwatch = start_stopwatch();
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// handle widow resize
	if(DeshWindow->resized || remake_window){
		DeshWindow->GetWindowSize(width, height);
		if(width <= 0 || height <= 0){ ImGui::EndFrame(); return; }
		glViewport(0,0,width,height);
		remake_window = false;
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// update uniform buffers
	{
		uboVS.values.screen     = vec2((f32)width, (f32)height);
		uboVS.values.mousepos   = input_mouse_position();
		uboVS.values.mouseWorld = Math::ScreenToWorld(input_mouse_position(), uboVS.values.proj, uboVS.values.view, DeshWindow->dimensions);
		uboVS.values.time       = DeshTime->totalTime;
		uboVS.values.enablePCF  = renderSettings.shadowPCF;
		
		glBindBuffer(GL_UNIFORM_BUFFER, uboVS.handle);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(uboVS.values), &uboVS.values, GL_STREAM_DRAW);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// build commands
	{
		//// ui commands ////
		u64 ui_vb_size = Max(1000*sizeof(Vertex2),         renderTwodVertexCount * sizeof(Vertex2));
		u64 ui_ib_size = Max(3000*sizeof(RenderTwodIndex), renderTwodIndexCount  * sizeof(RenderTwodIndex));
		if(ui_vb_size && ui_ib_size){
			//create vertex array object and buffers if they dont exist
			if(uiBuffers.vao_handle == 0){
				glGenVertexArrays(1, &uiBuffers.vao_handle);
				glGenBuffers(1, &uiBuffers.vbo_handle);
				glGenBuffers(1, &uiBuffers.ibo_handle);
			}
			
			//bind buffers
			glBindVertexArray(uiBuffers.vao_handle);
			glBindBuffer(GL_ARRAY_BUFFER, uiBuffers.vbo_handle);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiBuffers.ibo_handle);
			
			//specify vertex packing
			glVertexAttribPointer(0, 2,  GL_FLOAT,         GL_FALSE, sizeof(Vertex2), (void*)offsetof(Vertex2,pos));
			glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(Vertex2), (void*)offsetof(Vertex2,uv));
			glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Vertex2), (void*)offsetof(Vertex2,color));
			glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2);
			
			//resize buffers if too small and update buffer sizes
			if(uiBuffers.vbo_alloc < ui_vb_size){
				glBufferData(GL_ARRAY_BUFFER, ui_vb_size, 0, GL_STATIC_DRAW);
				uiBuffers.vbo_alloc = ui_vb_size;
			}
			if(uiBuffers.ibo_alloc < ui_ib_size){
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, ui_ib_size, 0, GL_STATIC_DRAW);
				uiBuffers.ibo_alloc = ui_ib_size;
			}
			
			//fill buffers
			glBufferSubData(GL_ARRAY_BUFFER,         0, ui_vb_size, renderTwodVertexArray);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, ui_ib_size, renderTwodIndexArray);
		}
		
		//// temp commands ////
		u64 temp_wire_vb_size = renderTempWireframeVertexCount*sizeof(Mesh::Vertex);
		u64 temp_fill_vb_size = renderTempFilledVertexCount*sizeof(Mesh::Vertex);
		u64 temp_wire_ib_size = renderTempWireframeIndexCount*sizeof(RenderTempIndex);
		u64 temp_fill_ib_size = renderTempFilledIndexCount*sizeof(RenderTempIndex);
		u64 temp_vb_size = temp_wire_vb_size+temp_fill_vb_size;
		u64 temp_ib_size = temp_wire_ib_size+temp_fill_ib_size;
		temp_vb_size = Max(1000*sizeof(Mesh::Vertex),   temp_vb_size);
		temp_ib_size = Max(3000*sizeof(RenderTempIndex), temp_ib_size);
		if(temp_vb_size && temp_ib_size){
			//create vertex array object and buffers if they dont exist
			if(tempBuffers.vao_handle == 0){
				glGenVertexArrays(1, &tempBuffers.vao_handle);
				glGenBuffers(1, &tempBuffers.vbo_handle);
				glGenBuffers(1, &tempBuffers.ibo_handle);
			}
			
			//bind buffers
			glBindVertexArray(tempBuffers.vao_handle);
			glBindBuffer(GL_ARRAY_BUFFER, tempBuffers.vbo_handle);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempBuffers.ibo_handle);
			
			//specify vertex packing
			glVertexAttribPointer(0, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,pos));
			glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,uv));
			glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,color));
			glVertexAttribPointer(3, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,normal));
			glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3);
			
			//resize buffers if too small and update buffer sizes
			if(tempBuffers.vbo_alloc < temp_vb_size){
				glBufferData(GL_ARRAY_BUFFER, temp_vb_size, 0, GL_STATIC_DRAW);
				tempBuffers.vbo_alloc = temp_vb_size;
			}
			if(tempBuffers.ibo_alloc < temp_ib_size){
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, temp_ib_size, 0, GL_STATIC_DRAW);
				tempBuffers.ibo_alloc = temp_ib_size;
			}
			
			//fill buffers
			glBufferSubData(GL_ARRAY_BUFFER, 0,                 temp_fill_vb_size, renderTempFilledVertexArray);
			glBufferSubData(GL_ARRAY_BUFFER, temp_fill_vb_size, temp_wire_vb_size, renderTempWireframeVertexArray);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,                 temp_fill_ib_size, renderTempFilledIndexArray);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, temp_fill_ib_size, temp_wire_ib_size, renderTempWireframeIndexArray);
		}
		
		//// imgui commands ////
		if(DeshiModuleLoaded(DS_IMGUI)){
			ImGui::Render();
		}
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup pipeline settings
	{
		glClearColor(renderSettings.clearColor.r, renderSettings.clearColor.g, renderSettings.clearColor.b, renderSettings.clearColor.a);
		glClearDepth(1.0f); //1 rather than 0 because openGL Z direction is opposite ours
		glFrontFace(GL_CW);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		//glEnable(GL_FRAMEBUFFER_SRGB);
		glScissor(0, 0, width, height);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// draw first renderpass
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//// draw meshes ////
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(glTextures[0].type, glTextures[0].handle);
	
	glBindVertexArray(meshBuffers.vao_handle);
	glUseProgram(programs.null.handle);
	glUniform1i(glGetUniformLocation(programs.null.handle,"nullSampler"),0);
	forI(renderModelCmdCount){ //TODO(delle) materials/textures
		if(renderModelCmdArray[i].indexCount == 0) continue;
		
		glBindBuffer(GL_UNIFORM_BUFFER, pushVS.handle);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &renderModelCmdArray[i].matrix);
		glPolygonMode(GL_FRONT_AND_BACK, (renderSettings.wireframeOnly) ? GL_LINE : GL_FILL);
		glDrawElementsBaseVertex(GL_TRIANGLES, renderModelCmdArray[i].indexCount, INDEX_TYPE_GL_MESH,
								 (void*)(renderModelCmdArray[i].indexOffset*sizeof(Mesh::Index)), renderModelCmdArray[i].vertexOffset);
	}
	
	//// draw temp ///
	if(renderTempWireframeVertexCount > 0 && renderTempWireframeIndexCount > 0){
		glBindVertexArray(tempBuffers.vao_handle);
		glBindBuffer(GL_UNIFORM_BUFFER, pushVS.handle);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &mat4::IDENTITY);
		glDisable(GL_CULL_FACE);
		if(renderSettings.tempMeshOnTop){
			glDisable(GL_DEPTH_TEST);
		}else{
			glEnable(GL_DEPTH_TEST);
		}
		
		//filled
		glUseProgram(programs.selected.handle);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElementsBaseVertex(GL_TRIANGLES, renderTempFilledIndexCount, INDEX_TYPE_GL_TEMP, 0, 0);
		
		//wireframe
		glUseProgram(programs.wireframe.handle);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElementsBaseVertex(GL_TRIANGLES, renderTempWireframeIndexCount, INDEX_TYPE_GL_TEMP,
								 (void*)(renderTempFilledIndexCount*sizeof(RenderTempIndex)), 0);
	}
	
	//// draw ui ////
	if(renderTwodVertexCount > 0 && renderTwodIndexCount > 0){
		glBindVertexArray(uiBuffers.vao_handle);
		glBindBuffer(GL_UNIFORM_BUFFER, push2D.handle);
		push2D.values.scale     = {2.0f/f32(width), -2.0f/f32(height)};
		push2D.values.translate = {-1.f, 1.f};
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(push2D.values), &push2D.values);
		
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glFrontFace(GL_CCW);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUseProgram(programs.ui.handle);
		
		//NOTE I don't think using a 2D array would be any different than having a separate for loop
		//     for each layer here, if its worse tell me and ill fix it.
		forX(layer, TWOD_LAYERS){
			forX(cmd_idx, renderTwodCmdCounts[renderActiveSurface][layer]){
				if(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].indexCount == 0) continue;
				
				glScissor(GLint(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorOffset.x),
						  GLint(  (height - renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorOffset.y)
								- (renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorExtent.y)),
						  GLsizei(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorExtent.x),
						  GLsizei(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].scissorExtent.y));
				glBindTexture(glTextures[(u32)renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].handle].type, glTextures[(u32)renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].handle].handle);
				glUniform1i(glGetUniformLocation(programs.ui.handle, "tex"), 0);
				glDrawElementsBaseVertex(GL_TRIANGLES, renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].indexCount, INDEX_TYPE_GL_TWOD,
										 (void*)(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].indexOffset * sizeof(RenderTwodIndex)), 0);
			}
		}
		
		glScissor(0, 0, width, height);
	}
	
	//// draw imgui ////
	if(DeshiModuleLoaded(DS_IMGUI)){
		if(ImDrawData* imDrawData = ImGui::GetDrawData()) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// present frame
	{
		DeshWindow->SwapBuffers();
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// reset commands
	{
		//// twod commands ////
		renderTwodVertexCount = 0;
		renderTwodIndexCount = 0;
		forX(layer, 9){
			memset(&renderTwodCmdArrays[renderActiveSurface][layer][0], 0, sizeof(RenderTwodCmd) * renderTwodCmdCounts[renderActiveSurface][layer]);
			renderTwodCmdCounts[renderActiveSurface][layer] = 1;
		}
		
		//// temp commands ////
		renderTempWireframeVertexCount = 0;
		renderTempWireframeIndexCount  = 0;
		renderTempFilledVertexCount = 0;
		renderTempFilledIndexCount  = 0;
		
		//// model commands ////
		renderModelCmdCount = 0;
	}
	
	DeshTime->renderTime = peek_stopwatch(update_stopwatch);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_reset
void
render_reset(){DPZoneScoped;
	Log("opengl","Resetting render");
	glFinish();
	//TODO(delle) delete things
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_cleanup
void
render_cleanup(){DPZoneScoped;
	render_save_settings();
	glFinish();
	//TODO(delle) save pipelines in GL4
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_loading
//TODO(delle) one large vertex/index array maybe
void
render_load_mesh(Mesh* mesh){DPZoneScoped;
	 RenderMesh mgl{};
	mgl.base = mesh;
	mgl.vertexCount = mesh->vertexCount;
	mgl.indexCount  = mesh->indexCount;
	if(glMeshes.count){
		mgl.vertexOffset = glMeshes.last->vertexOffset + glMeshes.last->vertexCount;
		mgl.indexOffset  = glMeshes.last->indexOffset  + glMeshes.last->indexCount;
	}
	
	u64 mesh_vb_size   = mesh->vertexCount*sizeof(Mesh::Vertex);
	u64 mesh_ib_size   = mesh->indexCount*sizeof(Mesh::Index);
	u64 total_vb_size  = meshBuffers.vbo_size + mesh_vb_size;
	u64 total_ib_size  = meshBuffers.ibo_size + mesh_ib_size;
	total_vb_size = Max(1024*sizeof(Mesh::Vertex), total_vb_size); //minimum of 1024 vertexes to avoid early growths
	total_ib_size = Max(4096*sizeof(Mesh::Index),  total_ib_size); //minimum of 4096 indexes to avoid early growths
	
	//create/bind vertex array object
	if(meshBuffers.vao_handle){
		glBindVertexArray(meshBuffers.vao_handle);
	}else{
		glGenVertexArrays(1, &meshBuffers.vao_handle);
		glBindVertexArray(meshBuffers.vao_handle);
	}
	
	//create/resize buffers: allocate new buffer, copy old buffer to new buffer, delete old buffer
	if(meshBuffers.vbo_alloc < total_vb_size){
		u32 old_vbo_handle = meshBuffers.vbo_handle;
		glGenBuffers(1, &meshBuffers.vbo_handle);
		glBindBuffer(GL_ARRAY_BUFFER, meshBuffers.vbo_handle);
		glBufferData(GL_ARRAY_BUFFER, total_vb_size, 0, GL_STATIC_DRAW); //TODO(delle) maybe growth rate/fit?
		if(old_vbo_handle){
			glBindBuffer(GL_COPY_READ_BUFFER, old_vbo_handle);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, 0, meshBuffers.vbo_size);
			glDeleteBuffers(1, &old_vbo_handle);
		}
	}
	if(meshBuffers.ibo_alloc < total_ib_size){
		u32 old_ibo_handle = meshBuffers.ibo_handle;
		glGenBuffers(1, &meshBuffers.ibo_handle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffers.ibo_handle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_ib_size, 0, GL_STATIC_DRAW); //TODO(delle) maybe growth rate/fit?
		if(old_ibo_handle){
			glBindBuffer(GL_COPY_READ_BUFFER, old_ibo_handle);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ELEMENT_ARRAY_BUFFER, 0, 0, meshBuffers.ibo_size);
			glDeleteBuffers(1, &old_ibo_handle);
		}
	}
	
	//copy mesh to buffers
	glBindBuffer(GL_ARRAY_BUFFER,         meshBuffers.vbo_handle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffers.ibo_handle);
	glBufferSubData(GL_ARRAY_BUFFER,         meshBuffers.vbo_size, mesh_vb_size, mesh->vertexArray);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, meshBuffers.ibo_size, mesh_ib_size, mesh->indexArray);
	
	//specify vertex packing
	glVertexAttribPointer(0, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,pos));
	glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,uv));
	glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,color));
	glVertexAttribPointer(3, 3,  GL_FLOAT,         GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex,normal));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3);
	
	//update buffer sizes
	meshBuffers.vbo_alloc = total_vb_size;
	meshBuffers.ibo_alloc = total_ib_size;
	meshBuffers.vbo_size = meshBuffers.vbo_size + mesh_vb_size;
	meshBuffers.ibo_size = meshBuffers.ibo_size + mesh_ib_size;
	
	glMeshes.add(mgl);
}

void
render_load_texture(Texture* texture){DPZoneScoped;
	TextureGl tgl{};
	tgl.base = texture;
	
	//determine image format
	switch(texture->format){ //TODO(delle) handle non RGBA formats
		case ImageFormat_BW:   tgl.format = GL_RGBA; break;
		case ImageFormat_BWA:  tgl.format = GL_RGBA; break;
		case ImageFormat_RGB:  tgl.format = GL_RGBA; break;
		case ImageFormat_RGBA: tgl.format = GL_RGBA; break;
		default: PrintGl(0,"Unhandled image format when loading texture: ", texture->name); return;
	}
	
	//determine image type
	switch(texture->type){
		case TextureType_1D:         tgl.type = GL_TEXTURE_1D; break;
		case TextureType_2D:         tgl.type = GL_TEXTURE_2D; break;
		case TextureType_3D:         tgl.type = GL_TEXTURE_3D; break;
		case TextureType_Cube:       tgl.type = GL_TEXTURE_CUBE_MAP; break;
		case TextureType_Array_1D:   tgl.type = GL_TEXTURE_1D_ARRAY; break;
		case TextureType_Array_2D:   tgl.type = GL_TEXTURE_2D_ARRAY; break;
		case TextureType_Array_Cube:{
			if(GLAD_VERSION_MAJOR(opengl_version) >= 4)
				tgl.type = GL_TEXTURE_CUBE_MAP_ARRAY;
			else{
				Assert(!"GL_TEXTURE_CUBE_MAP_ARRAY requires OpenGL4");
			}
		}break;
		default: PrintGl(0,"Uknown image type when loading texture: ", texture->name); return;
	}
	
	//create texture
	glGenTextures(1, &tgl.handle);
	glBindTexture(tgl.type, tgl.handle);
	
	//load texture to GPU
	if      (texture->type == TextureType_1D){
		glTexImage1D(tgl.type, 0, tgl.format, texture->width, 0, tgl.format, GL_UNSIGNED_BYTE, texture->pixels);
	}else if(texture->type == TextureType_2D || texture->type == TextureType_Array_1D){
		glTexImage2D(tgl.type, 0, tgl.format, texture->width, texture->height, 0, tgl.format, GL_UNSIGNED_BYTE, texture->pixels);
	}else if(texture->type == TextureType_Cube || texture->type == TextureType_Array_Cube || 
			 texture->type== TextureType_3D   || texture->type == TextureType_Array_2D){
		glTexImage3D(tgl.type, 0, tgl.format, texture->width, texture->height, texture->depth, 0, tgl.format, GL_UNSIGNED_BYTE, texture->pixels);
	}
	
	//setup texture filtering
	switch(texture->filter){
		case TextureFilter_Nearest:{
			if(texture->mipmaps > 1){
				glTexParameteri(tgl.type, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				glTexParameteri(tgl.type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glGenerateMipmap(tgl.type);
			}else{
				glTexParameteri(tgl.type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(tgl.type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
		}break;
		case TextureFilter_Linear:TextureFilter_Cubic:{
			if(texture->mipmaps > 1){
				glTexParameteri(tgl.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(tgl.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glGenerateMipmap(tgl.type);
			}else{
				glTexParameteri(tgl.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(tgl.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
		}break;
		default: LogE("opengl", "Unhandled texture filter: ", texture->filter); break;
	}
	
	//setup texture address mode
	switch(texture->uvMode){
		case TextureAddressMode_Repeat:{
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}break;
		case TextureAddressMode_MirroredRepeat:{
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		}break;
		case TextureAddressMode_ClampToEdge:{
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}break;
		case TextureAddressMode_ClampToWhite:{
			float border_color[4] = {1.f, 1.f, 1.f, 1.f};
			glTexParameterfv(tgl.type, GL_TEXTURE_BORDER_COLOR, border_color);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		}break;
		case TextureAddressMode_ClampToBlack:{
			float border_color[4] = {0.f, 0.f, 0.f, 1.f};
			glTexParameterfv(tgl.type, GL_TEXTURE_BORDER_COLOR, border_color);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		}break;
		case TextureAddressMode_ClampToTransparent:{
			float border_color[4] = {0.f, 0.f, 0.f, 0.f};
			glTexParameterfv(tgl.type, GL_TEXTURE_BORDER_COLOR, border_color);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(tgl.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		}break;
		default: LogE("vulkan", "Unhandled texture address mode: ", texture->uvMode); break;
	}
	
	//TODO(delle) EXT_texture_filter_anisotropic
	
	glTextures.add(tgl);
}

void
render_load_material(Material* material){DPZoneScoped;
	//!Incomplete
}

void
render_update_material(Material* material){DPZoneScoped;
	NotImplemented; //!Incomplete
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_3d
void
render_model(Model* model, mat4* matrix){DPZoneScoped;
	Assert(renderModelCmdCount + model->batches.count < MAX_MODEL_CMDS, "attempted to draw more than the global maximum number of batches");
	RenderModelCmd* cmd = renderModelCmdArray + renderModelCmdCount;
	forI(model->batches.count){
		if(!model->batches[i].indexCount) continue;
		cmd[i].vertexOffset = glMeshes[model->mesh->idx].vertexOffset;
		cmd[i].indexOffset = glMeshes[model->mesh->idx].indexOffset + model->batches[i].indexOffset;
		cmd[i].indexCount  = model->batches[i].indexCount;
		cmd[i].material  = model->batches[i].material;
		cmd[i].name      = model->name;
		cmd[i].matrix    = *matrix;
		renderModelCmdCount += 1;
	}
}

void
render_model_wireframe(Model* model, mat4 matrix, color color){DPZoneScoped;
	NotImplemented; //!Incomplete
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_2d
void
render_start_cmd2(u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent){DPZoneScoped;
	renderActiveLayer = layer;
	RenderTwodCmd& prevCmd = renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer] - 1];
	if(   ((u32)prevCmd.handle != texture->idx)
	   || (prevCmd.scissorOffset != scissorOffset)
	   || (prevCmd.scissorExtent != scissorExtent)){
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].scissorOffset = scissorOffset;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].scissorExtent = scissorExtent;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].handle        = (void*)(u64)texture->idx;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].indexOffset   = renderTwodIndexCount;
		renderTwodCmdCounts[renderActiveSurface][layer] += 1;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_surface
void 
render_register_surface(u32 idx, Window* window){DPZoneScoped;
	AssertDS(DS_RENDER, "Attempted to register a surface for a window without initializaing Render module first");
	Assert(idx < MAX_SURFACES);
	NotImplemented; //!Incomplete
}

void
render_set_active_surface(Window* window){DPZoneScoped;
	Assert(window->renderer_surface_index != -1, "Attempt to set draw target to a window who hasnt been registered to the renderer");
	renderActiveSurface = window->renderer_surface_index;
}

void
render_set_active_surface_idx(u32 idx){DPZoneScoped;
	Assert(idx < MAX_SURFACES, "Attempt to set draw target to a window who hasnt been registered to the renderer");
	renderActiveSurface = idx;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_light
void
render_update_light(u32 idx, vec3 position, f32 brightness){DPZoneScoped;
	uboVS.values.lights[idx] = vec4(position, brightness);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_camera
void
render_update_camera_position(vec3 position){DPZoneScoped;
	uboVS.values.viewPos = vec4(position, 1.f);
}

void
render_update_camera_view(mat4* view_matrix){DPZoneScoped;
	uboVS.values.view = *view_matrix;
}

void
render_update_camera_projection(mat4* projection_matrix){DPZoneScoped;
	uboVS.values.proj = *projection_matrix;
}

void
render_use_default_camera(){DPZoneScoped;
	uboVS.values.view = Math::LookAtMatrix(vec3::ZERO, (vec3::FORWARD * mat4::RotationMatrix(vec3::ZERO)).normalized()).Inverse();
	uboVS.values.proj = Camera::MakePerspectiveProjectionMatrix((f32)DeshWindow->width, (f32)DeshWindow->height, 90.f, 1000.f, 0.1f);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shaders
void
render_reload_shader(u32 shader){DPZoneScoped;
	NotImplemented; //!Incomplete
}

void
render_reload_all_shaders(){DPZoneScoped;
	NotImplemented; //!Incomplete
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_other
void
render_remake_offscreen(){DPZoneScoped;
	NotImplemented; //!Incomplete
}
