/* deshi OpenGL Render Submodule
Index:
@gl_types
@gl_vars
@gl_vars_instance
@gl_vars_buffers
@gl_vars_programs
@gl_funcs_utils
@gl_funcs_init
@gl_funcs_shaders
@gl_funcs_programs
@gl_funcs_imgui
@render_init
@render_update
@render_reset
@render_cleanup
@render_loading
@render_draw_3d
@render_draw_2d
@render_surface
@render_light
@render_camera
@render_shaders
@render_other
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
#define INDEX_TYPE_GL_TEMP GL_UNSIGNED_INT
#define INDEX_TYPE_GL_MESH GL_UNSIGNED_INT
StaticAssertAlways(sizeof(RenderTwodIndex)  == 4);
StaticAssertAlways(sizeof(RenderTempIndex)  == 4);
StaticAssertAlways(sizeof(RenderModelIndex) == 4);

local arrayT<RenderMesh> glMeshes(deshi_allocator);
local arrayT<ShaderGl>   glShaders(deshi_allocator);
local arrayT<TextureGl>  glTextures(deshi_allocator);
local arrayT<MaterialGl> glMaterials(deshi_allocator);

local s32  width  = 0;
local s32  height = 0;
local b32  initialized  = false;
local b32  remake_window = false;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_vars_instance
local void* opengl_context = 0;
local s32 opengl_success = 0;
local int opengl_version = 0;
local int backend_version = 0;
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
load_shader(str8 name, str8 source, ShaderStage stage){
	if(!source){
		//TODO(delle) proper error
		Assert(!"Failed to load shader");
		return -1;
	}
	Assert(stage > ShaderStage_NONE && stage < ShaderStage_COUNT);
	
	Stopwatch t_s = start_stopwatch();
	PrintGl(4, "Compiling shader: ",name);
	
	ShaderGl sgl{};
	sgl.filename = str8_copy(name, deshi_allocator);
	sgl.stage    = stage;
	
	//create shader
	switch(stage){ //TODO(delle) opengl4 shader stages
		case ShaderStage_Vertex:   sgl.handle = glCreateShader(GL_VERTEX_SHADER);   break;
		case ShaderStage_TessCtrl: Assert(!"not implemented yet REQUIRES OPENGL4"); break;
		case ShaderStage_TessEval: Assert(!"not implemented yet REQUIRES OPENGL4"); break;
		case ShaderStage_Geometry: sgl.handle = glCreateShader(GL_GEOMETRY_SHADER); break;
		case ShaderStage_Fragment: sgl.handle = glCreateShader(GL_FRAGMENT_SHADER); break;
		case ShaderStage_Compute:  Assert(!"not implemented yet REQUIRES OPENGL4"); break;
	}
	
	//compile shader
	const char* str = (const char*)source.str; int len = (int)source.count;
	glShaderSource(sgl.handle, 1, &str, &len);
	glCompileShader(sgl.handle);
	
	//check for errors
	glGetShaderiv(sgl.handle, GL_COMPILE_STATUS, &opengl_success);
	if(opengl_success != GL_TRUE){
		glGetShaderInfoLog(sgl.handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
		PrintGl(0, "Failed to compile shader '",(char*)name.str,"':\n",opengl_infolog);
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
	dstr8 prog_shaders;
	dstr8_init(&prog_shaders, str8_lit("|"), deshi_temp_allocator);
	
	//create program, attach shaders and link
	pgl.handle = glCreateProgram();
	forI(shader_count){ 
		glAttachShader(pgl.handle, glShaders[shader_indexes[i]].handle); 
		pgl.shaders[i] = shader_indexes[i]; 
		dstr8_append(&prog_shaders, glShaders[shader_indexes[i]].filename);
		dstr8_append(&prog_shaders, str8_lit("|"));
	}
	glLinkProgram(pgl.handle);
	
	//check for errors
	glGetProgramiv(pgl.handle, GL_LINK_STATUS, &opengl_success);
	if(opengl_success != GL_TRUE){
		glGetProgramInfoLog(pgl.handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
		PrintGl(0,"Failed to link program '",(char*)prog_shaders.str,"':\n",opengl_infolog);
		
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


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_init
void
render_init(){DPZoneScoped;
	DeshiStageInitStart(DS_RENDER, DS_PLATFORM, "Attempted to initialize OpenGL module before initializing Platform module");
	Log("opengl","Starting opengl renderer initialization");
	
	//create the shaders directory if it doesn't exist already
	file_create(str8_lit("data/shaders/"));
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// load render settings
	render_load_settings();
	if(renderSettings.debugging && renderSettings.printf) renderSettings.loggingLevel = 4;
	
	width  = DeshWindow->width;
	height = DeshWindow->height;
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup WGL and glad
	{ 
#if DESHI_WINDOWS
		//restore point for contexts
		HDC   prev_dc = wglGetCurrentDC();
		HGLRC prev_rc = wglGetCurrentContext();
		
		//setup pixel format for dummy device context
		PIXELFORMATDESCRIPTOR temp_pfd{sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER};
		temp_pfd.cColorBits = 32; temp_pfd.cDepthBits = 24; temp_pfd.cStencilBits = 8;
		int temp_format = ChoosePixelFormat((HDC)window_helper.context, &temp_pfd);
		if(!SetPixelFormat((HDC)window_helper.context, temp_format, &temp_pfd)){ win32_log_last_error("SetPixelFormat", renderSettings.crashOnError); return; }
		
		//create and enable dummy render context
		HGLRC temp_context = wglCreateContext((HDC)window_helper.context);
		if(!temp_context){ win32_log_last_error("wglCreateContext", renderSettings.crashOnError); return; }
		wglMakeCurrent((HDC)window_helper.context, temp_context);
		
		//load wgl extensions
		backend_version = gladLoaderLoadWGL((HDC)window_helper.context);
		if(backend_version == 0){ LogE("opengl","Failed to load OpenGL"); return; }
		Logf("opengl","Loaded WGL %d.%d", GLAD_VERSION_MAJOR(backend_version), GLAD_VERSION_MINOR(backend_version));
		gladInstallWGLDebug();
		gladSetWGLPostCallback(WGLDebugPostCallback);
		
		//delete dummy context
		wglMakeCurrent(prev_dc, prev_rc);
		wglDeleteContext(temp_context);
		
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
		wglChoosePixelFormatARB((HDC)DeshWindow->context, format_attributes, 0, 1, &format, &format_count); //https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
		if(!DescribePixelFormat((HDC)DeshWindow->context, format, sizeof(pfd), &pfd)){  win32_log_last_error("DescribePixelFormat", renderSettings.crashOnError); return;  }
		if(format == 0){ win32_log_last_error("ChoosePixelFormatARB", renderSettings.crashOnError); return; }
		if(!SetPixelFormat((HDC)DeshWindow->context, format, &pfd)){ win32_log_last_error("SetPixelFormat", renderSettings.crashOnError); return; }
		
		//set the desired OpenGL version and render context settings
		int context_attributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 2,
#  if BUILD_INTERNAL
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
#  else
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#  endif
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};
		
		//create actual render context and delete temporary one
		opengl_context = wglCreateContextAttribsARB((HDC)DeshWindow->context, 0, context_attributes); //https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
		if(!opengl_context){ win32_log_last_error("wglCreateContextAttribsARB", renderSettings.crashOnError); return; }
		wglMakeCurrent((HDC)DeshWindow->context, (HGLRC)opengl_context);
		
		//load glad extensions
		opengl_version = gladLoaderLoadGL();
		
#elif DESHI_LINUX 
		// following this tutorial: https://www.khronos.org/opengl/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib
		
		XFlush(linux.x11.display);
		XDrawString(linux.x11.display, (X11Window)DeshWindow->handle, (GC)DeshWindow->context, 50, 50, "Loading OpenGL...", 17);

		backend_version = gladLoaderLoadGLX(linux.x11.display, linux.x11.screen);
		Logf("opengl","Loaded GLX %d.%d", GLAD_VERSION_MAJOR(backend_version), GLAD_VERSION_MINOR(backend_version));
		
		gladInstallGLXDebug();

		// get restore points 
		Display* prev_display = glXGetCurrentDisplay();
		GLXContext prev_context = glXGetCurrentContext();
		
		// list of attributes to ask of GLX
		int attributes[] = {
			GLX_RGBA,            // true color
			GLX_DEPTH_SIZE, 24,  // 24 bit depth
			GLX_STENCIL_SIZE, 8, // 8 bit stencil size
			GLX_DOUBLEBUFFER,    // use a double buffer
			GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB,
			0,
		};
		
		int n_elem;
		GLXFBConfig* config = glXChooseFBConfig(linux.x11.display, linux.x11.screen, attributes, &n_elem);
		
		if(!config || !n_elem) {
			LogE("opengl", "Cannot find an appropriate framebuffer configuration with glXChooseFBConfig");
			Assert(0);
		}

		GLXFBConfig fbconfig = config[0];
		XFree(config);

		// get the best visual for our chosen attributes
		XVisualInfo* vi = glXChooseVisual(linux.x11.display, linux.x11.screen, attributes);
		if(!vi) {
			LogE("opengl", "Cannot find an appropriate visual for the given attributes");
			Assert(0);
		}
		
		Colormap cm = XCreateColormap(linux.x11.display, (X11Window)DeshWindow->handle, vi->visual, AllocNone);
		
		int contextAttribs[] = {
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3, 
			GLX_CONTEXT_MINOR_VERSION_ARB, 2, 
#if BUILD_INTERNAL
			GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | GLX_CONTEXT_DEBUG_BIT_ARB,
#else
			GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, 
#endif
			0 // Terminate the attribute list
		};

		GLXContext context = glXCreateContextAttribsARB(linux.x11.display, fbconfig, 0, True, contextAttribs);
		if(!glXMakeCurrent(linux.x11.display, (X11Window)DeshWindow->handle, context)) {
			Log("", "unable to set glx context");
		}
		
		opengl_version = gladLoaderLoadGL();
#else
#  error "unhandled platform"
#endif
		
		if(opengl_version == 0){ LogE("opengl","Failed to load OpenGL"); return; }
		Logf("opengl","Loaded OpenGL %d.%d", GLAD_VERSION_MAJOR(opengl_version), GLAD_VERSION_MINOR(opengl_version));
		gladInstallGLDebug();
		gladSetGLPostCallback(GladDebugPostCallback);
		
#if DESHI_WINDOWS
		UpdateWindow((HWND)DeshWindow->handle);
#elif DESHI_LINUX
		XFlush(linux.x11.display);
#else
#  error "unhandled platform"
#endif
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
			shaders[0] = load_shader(STR8("base.vert"), baked_shader_base_vert, ShaderStage_Vertex);
			shaders[1] = load_shader(STR8("base.frag"), baked_shader_base_frag, ShaderStage_Fragment);
			shader_count = 2;
			programs.base = CreateProgram(shaders, shader_count);
			
			{//selected
				
			}
		}
		
		{//null
			shaders[0] = load_shader(STR8("null.vert"), baked_shader_null_vert, ShaderStage_Vertex);
			shaders[1] = load_shader(STR8("null.frag"), baked_shader_null_frag, ShaderStage_Fragment);
			shader_count = 2;
			programs.null = CreateProgram(shaders, shader_count);
		}
		
		{//flat
			shaders[0] = load_shader(STR8("flat.vert"), baked_shader_flat_vert, ShaderStage_Vertex);
			shaders[1] = load_shader(STR8("flat.frag"), baked_shader_flat_frag, ShaderStage_Fragment);
			shader_count = 2;
			programs.flat = CreateProgram(shaders, shader_count);
		}
		
		{//phong
			shaders[0] = load_shader(STR8("phong.vert"), baked_shader_phong_vert, ShaderStage_Vertex);
			shaders[1] = load_shader(STR8("phong.frag"), baked_shader_phong_frag, ShaderStage_Fragment);
			shader_count = 2;
			programs.phong = CreateProgram(shaders, shader_count);
		}
		
		{//pbr
			shaders[0] = load_shader(STR8("pbr.vert"), baked_shader_pbr_vert, ShaderStage_Vertex);
			shaders[1] = load_shader(STR8("pbr.frag"), baked_shader_pbr_frag, ShaderStage_Fragment);
			shader_count = 2;
			programs.pbr = CreateProgram(shaders, shader_count);
		}
		
		{//2d
			shaders[0] = load_shader(STR8("twod.vert"), baked_shader_twod_vert, ShaderStage_Vertex);
			shaders[1] = load_shader(STR8("twod.frag"), baked_shader_twod_frag, ShaderStage_Fragment);
			shader_count = 2;
			programs.twod = CreateProgram(shaders, shader_count, true);
			
			{//ui
				shaders[0] = load_shader(STR8("ui.vert"), baked_shader_ui_vert, ShaderStage_Vertex);
				shaders[1] = load_shader(STR8("ui.frag"), baked_shader_ui_frag, ShaderStage_Fragment);
				shader_count = 2;
				programs.ui = CreateProgram(shaders, shader_count, true);
			}
		}
		
		{//wireframe
			shaders[0] = load_shader(STR8("wireframe.vert"), baked_shader_wireframe_vert, ShaderStage_Vertex);
			shaders[1] = load_shader(STR8("wireframe.frag"), baked_shader_wireframe_frag, ShaderStage_Fragment);
			shader_count = 2;
			programs.wireframe = CreateProgram(shaders, shader_count);
			
			{//wireframe with depth test
				
			}
		}
		
		{//offscreen
			shaders[0] = load_shader(STR8("offscreen.vert"), baked_shader_offscreen_vert, ShaderStage_Vertex);
			shader_count = 1;
			programs.offscreen = CreateProgram(shaders, shader_count);
		}
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup shared render
	{
		memory_pool_init(deshi__render_buffer_pool, 16);
	}
	
	initialized = true;
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
		width  = DeshWindow->width;
		height = DeshWindow->height;
		if(width <= 0 || height <= 0){ return; }
		glViewport(0,0,width,height);
		remake_window = false;
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// update uniform buffers
	{
		uboVS.values.screen     = Vec2((f32)width, (f32)height);
		uboVS.values.mousepos   = input_mouse_position();
		uboVS.values.mouseWorld = Math::ScreenToWorld(input_mouse_position(), uboVS.values.proj, uboVS.values.view, Vec2(DeshWindow->width,DeshWindow->height));
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
		u64 temp_wire_vb_size = renderTempWireframeVertexCount*sizeof(MeshVertex);
		u64 temp_fill_vb_size = renderTempFilledVertexCount*sizeof(MeshVertex);
		u64 temp_wire_ib_size = renderTempWireframeIndexCount*sizeof(RenderTempIndex);
		u64 temp_fill_ib_size = renderTempFilledIndexCount*sizeof(RenderTempIndex);
		u64 temp_vb_size = temp_wire_vb_size+temp_fill_vb_size;
		u64 temp_ib_size = temp_wire_ib_size+temp_fill_ib_size;
		temp_vb_size = Max(1000*sizeof(MeshVertex),   temp_vb_size);
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
			glVertexAttribPointer(0, 3,  GL_FLOAT,         GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex,pos));
			glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex,uv));
			glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(MeshVertex), (void*)offsetof(MeshVertex,color));
			glVertexAttribPointer(3, 3,  GL_FLOAT,         GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex,normal));
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
								 (void*)(renderModelCmdArray[i].indexOffset*sizeof(MeshIndex)), renderModelCmdArray[i].vertexOffset);
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
		//     for each layer here, if it's worse tell me and ill fix it.
		forX(layer, TWOD_LAYERS){
			forX(cmd_idx, renderTwodCmdCounts[renderActiveSurface][layer]){
				if(renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx].indexCount == 0) continue;
				RenderTwodCmd cmd = renderTwodCmdArrays[renderActiveSurface][layer][cmd_idx];
				glScissor(GLint(cmd.scissorOffset.x), GLint((height - cmd.scissorOffset.y) - (cmd.scissorExtent.y)), GLsizei(cmd.scissorExtent.x), GLsizei(cmd.scissorExtent.y));
				glBindTexture(glTextures[(u64)cmd.handle].type, glTextures[(u64)cmd.handle].handle);
				glUniform1i(glGetUniformLocation(programs.ui.handle, "tex"), 0);
				glDrawElementsBaseVertex(GL_TRIANGLES, cmd.indexCount, INDEX_TYPE_GL_TWOD, (void*)(cmd.indexOffset * sizeof(RenderTwodIndex)), 0);
			}
		}
		
		glScissor(0, 0, width, height);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// present frame
	{
#if DESHI_LINUX
		glXSwapBuffers(linux.x11.display, (X11Window)DeshWindow->handle);
#else 
		window_swap_buffers(DeshWindow);
#endif
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// reset commands
	{
		//// twod commands ////
		forI(TWOD_LAYERS+1){
			ZeroMemory(renderTwodCmdArrays[renderActiveSurface][i], renderTwodCmdCounts[renderActiveSurface][i]*sizeof(RenderTwodCmd));
			renderTwodCmdCounts[renderActiveSurface][i] = 0;
		}
		renderTwodVertexCount = 0;
		renderTwodIndexCount  = 0;
		
		//// temp commands ////
		renderTempWireframeVertexCount = 0;
		renderTempWireframeIndexCount  = 0;
		renderTempFilledVertexCount = 0;
		renderTempFilledIndexCount  = 0;
		
		//// model commands ////
		renderModelCmdCount = 0;

#ifdef BUILD_INTERNAL
		renderBookKeeperCount = 0;
#endif
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
	
	u64 mesh_vb_size   = mesh->vertexCount*sizeof(MeshVertex);
	u64 mesh_ib_size   = mesh->indexCount*sizeof(MeshIndex);
	u64 total_vb_size  = meshBuffers.vbo_size + mesh_vb_size;
	u64 total_ib_size  = meshBuffers.ibo_size + mesh_ib_size;
	total_vb_size = Max(1024*sizeof(MeshVertex), total_vb_size); //minimum of 1024 vertexes to avoid early growths
	total_ib_size = Max(4096*sizeof(MeshIndex),  total_ib_size); //minimum of 4096 indexes to avoid early growths
	
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
	glVertexAttribPointer(0, 3,  GL_FLOAT,         GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex,pos));
	glVertexAttribPointer(1, 2,  GL_FLOAT,         GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex,uv));
	glVertexAttribPointer(2, 4,  GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(MeshVertex), (void*)offsetof(MeshVertex,color));
	glVertexAttribPointer(3, 3,  GL_FLOAT,         GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex,normal));
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3);
	
	//update buffer sizes
	meshBuffers.vbo_alloc = total_vb_size;
	meshBuffers.ibo_alloc = total_ib_size;
	meshBuffers.vbo_size = meshBuffers.vbo_size + mesh_vb_size;
	meshBuffers.ibo_size = meshBuffers.ibo_size + mesh_ib_size;
	
	mesh->render_idx = glMeshes.count;
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
	
	texture->render_idx = glTextures.count;
	glTextures.add(tgl);
}

void
render_load_material(Material* material){DPZoneScoped;
	//!Incomplete
}

void
render_unload_mesh(Mesh* mesh){
	//!NotImplemented
}

void
render_unload_texture(Texture* texture){
	//!NotImplemented
}

void
render_unload_material(Material* material){
	//!NotImplemented
}

void
render_update_material(Material* material){DPZoneScoped;
	NotImplemented; //!Incomplete
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_draw_3d
void
render_model(Model* model, mat4* matrix){DPZoneScoped;
	Assert(renderModelCmdCount + arrlenu(model->batchArray) < MAX_MODEL_CMDS, "attempted to draw more than the global maximum number of batches");
	RenderModelCmd* cmd = renderModelCmdArray + renderModelCmdCount;
	forI(arrlenu(model->batchArray)){
		if(!model->batchArray[i].indexCount) continue;
		cmd[i].vertexOffset = glMeshes[model->mesh->render_idx].vertexOffset;
		cmd[i].indexOffset  = glMeshes[model->mesh->render_idx].indexOffset + model->batchArray[i].indexOffset;
		cmd[i].indexCount   = model->batchArray[i].indexCount;
		cmd[i].material     = model->batchArray[i].material->render_idx;
		cmd[i].name         = model->name;
		cmd[i].matrix       = *matrix;
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
deshi__render_start_cmd2(str8 file, u32 line, u32 layer, Texture* texture, vec2 scissorOffset, vec2 scissorExtent){DPZoneScoped;
	renderActiveLayer = layer;
	if(   (renderTwodCmdCounts[renderActiveSurface][layer] == 0)
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].handle        != (void*)((texture) ? (u64)texture->render_idx : 1))
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].scissorOffset != scissorOffset)
	   || (renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]-1].scissorExtent != scissorExtent)){
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].handle        = (void*)((texture) ? (u64)texture->render_idx : 1);
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].indexOffset   = renderTwodIndexCount;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].scissorOffset = scissorOffset;
		renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]].scissorExtent = scissorExtent;
		renderTwodCmdCounts[renderActiveSurface][layer] += 1;
	}
#if BUILD_INTERNAL
	RenderBookKeeper keeper; 
	keeper.type = RenderBookKeeper_Cmd;
	keeper.cmd = &renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer]];
	keeper.file = file;
	keeper.line = line;
	renderBookKeeperArray[renderBookKeeperCount++] = keeper;
#endif
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_buffer
RenderBuffer*
render_buffer_create(void* data, u64 size, RenderBufferUsageFlags usage, RenderMemoryPropertyFlags properties, RenderMemoryMappingType mapping){
	NotImplemented; //!Incomplete: https://www.khronos.org/opengl/wiki/Buffer_Object
	return 0;
}

void
render_buffer_delete(RenderBuffer* buffer){
	NotImplemented; //!Incomplete
}

void
render_buffer_map(RenderBuffer* buffer, u64 offset, u64 size){
	NotImplemented; //!Incomplete
}

void
render_buffer_unmap(RenderBuffer* buffer, b32 flush){
	NotImplemented; //!Incomplete
}

void
render_buffer_flush(RenderBuffer* buffer){
	NotImplemented; //!Incomplete
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_surface
void 
render_register_surface(Window* window){DPZoneScoped;
	Assert(DeshiModuleLoaded(DS_RENDER), "Attempted to register a surface for a window without initializaing Render module first");
	Assert(window->index < MAX_SURFACES);
	NotImplemented; //!Incomplete
}

void
render_set_active_surface(Window* window){DPZoneScoped;
	Assert(window->index != -1, "Attempt to set draw target to a window who hasnt been registered to the renderer");
	renderActiveSurface = window->index;
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
	uboVS.values.lights[idx] = Vec4(position.x, position.y, position.z, brightness);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_camera
void
render_update_camera_position(vec3 position){DPZoneScoped;
	uboVS.values.viewPos = Vec4(position.x, position.y, position.z, 1.f);
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

void 
render_update_texture(Texture* texture, vec2i offset, vec2i size){
	TextureGl* gltex = &glTextures[texture->render_idx];

	switch(texture->type){
		case TextureType_2D:{
			glBindTexture(GL_TEXTURE_2D, gltex->handle); 
			glTexSubImage2D(GL_TEXTURE_2D, 0, offset.x, offset.y, size.x, size.y, gltex->format, GL_UNSIGNED_BYTE, texture->pixels);
			glBindTexture(GL_TEXTURE_2D, 0);
			
		}break;
		default: NotImplemented;
	}
}