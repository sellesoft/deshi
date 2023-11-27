/* deshi OpenGL Render Submodule
Index:
@gl_types
@gl_vars
@gl_utils
@gl_funcs_init
@gl_funcs_shaders
@gl_funcs_programs
@graphics_context
@graphics_buffer
@graphics_image
@graphics_descriptor
@graphics_shader
@graphics_pipeline
@graphics_renderpass
@graphics_framebuffer
@graphics_commands
@graphics_misc
*/


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_types
typedef struct WindowRenderInfoGl{
	
}WindowRenderInfoGl;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_vars
local void* opengl_context = 0;

local s32 opengl_success = 0;
local GLenum opengl_error = 0;

local int opengl_version = 0;
local int backend_version = 0;

#define OPENGL_INFOLOG_SIZE 512
local char opengl_infolog[OPENGL_INFOLOG_SIZE] = {};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_utils
#define LogGl(level, ...) if(g_graphics->logging_level >= level){ logger_push_indent(level); Log("opengl", __VA_ARGS__); logger_pop_indent(level); }(void)0
#define LogWGl(...) LogW("opengl", __func__, "(): ", __VA_ARGS__)
#define LogEGl(...) LogE("opengl", __func__, "(): ", __VA_ARGS__)
#define GL_VERSION_TEST(major,minor) (opengl_version >= GLAD_MAKE_VERSION(major,minor))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_funcs_init
#if DESHI_WINDOWS
local void
WGLDebugPostCallback(void *ret, const char *name, GLADapiproc apiproc, s32 len_args, ...){
	DWORD error_code = ::GetLastError();
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
	LogE("render-wgl","ERROR_",(u32)error_code," '",error_flag,"' on ",name,"(); Reason: ",error_msg);
	if(g_graphics->break_on_error) Assert(!"crashing because of error in opengl"); //TODO(delle) remove this in favor of logger assert on error
}
#endif //#if DESHI_WINDOWS

local void
GladDebugPostCallback(void *ret, const char *name, GLADapiproc apiproc, s32 len_args, ...){DPZoneScoped;
	opengl_error = glad_glGetError();
	if(opengl_error != GL_NO_ERROR){
		const char* error_flag = 0;
		const char* error_msg  = 0;
		switch(opengl_error){
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
		LogEGl("ERROR_",opengl_error," '",error_flag,"' on ",name,"(); Reason: ",error_msg,"; Info: http://docs.gl/gl3/",name);
		if(g_graphics->break_on_error) Assert(!"crashing because of error in opengl"); //TODO(delle) remove this in favor of logger assert on error
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
	LogGl(4, "Compiling shader: ",name);
	
	ShaderGl sgl{};
	sgl.filename = str8_copy(name, deshi_allocator);
	sgl.stage	 = stage;
	
	//create shader
	switch(stage){ //TODO(delle) opengl4 shader stages
		case ShaderStage_Vertex:   sgl.handle = glCreateShader(GL_VERTEX_SHADER);	break;
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
		LogGl(0, "Failed to compile shader '",(char*)name.str,"':\n",opengl_infolog);
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
		LogGl(0,"Failed to link program '",(char*)prog_shaders.str,"':\n",opengl_infolog);
		
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
			LogGl(0,"Failed to find UniformBufferObject in vertex shader of program '",prog_shaders.str,"'");
			if(g_graphics->break_on_error) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
		
		local_ubi = glGetUniformBlockIndex(pgl.handle,"PushConsts");
		if(local_ubi != -1){ 
			glUniformBlockBinding(pgl.handle, local_ubi, pushVS.binding);
		}else{
			LogGl(0,"Failed to find PushConsts in vertex shader of program '",prog_shaders.str,"'");
			if(g_graphics->break_on_error) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
	}else{
		u32 local_ubi = glGetUniformBlockIndex(pgl.handle,"PushConsts");
		if(local_ubi != -1){ 
			glUniformBlockBinding(pgl.handle, local_ubi, push2D.binding);
		}else{
			LogGl(0,"Failed to find PushConsts in vertex shader of program '",prog_shaders.str,"'");
			if(g_graphics->break_on_error) Assert(false);
			glDeleteProgram(pgl.handle);
			return {};
		}
	}
	
	//detach shaders
	forI(shader_count){ glDetachShader(pgl.handle, glShaders[shader_indexes[i]].handle); }
	
	return pgl;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_context


void
graphics_init(Window* window){DPZoneScoped;
	AssertAlways(window);
	DeshiStageInitStart(DS_RENDER, DS_PLATFORM, "Attempted to reinitialize the Graphics module or initialzie it before initializing the Platform module");
	Log("opengl","Starting opengl renderer initialization");
	
	graphics_init_shared(window);
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup WGL and glad
#if DESHI_WINDOWS
	//restore point for contexts
	HDC   prev_dc = wglGetCurrentDC();
	HGLRC prev_rc = wglGetCurrentContext();
	
	//setup pixel format for dummy device context
	PIXELFORMATDESCRIPTOR temp_pfd{sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER};
	temp_pfd.cColorBits = 32; temp_pfd.cDepthBits = 24; temp_pfd.cStencilBits = 8;
	int temp_format = ChoosePixelFormat((HDC)window_helper.context, &temp_pfd);
	if(!SetPixelFormat((HDC)window_helper.context, temp_format, &temp_pfd)){
		win32_log_last_error("SetPixelFormat", g_graphics->break_on_error);
		return;
	}
	
	//create and enable dummy render context
	HGLRC temp_context = wglCreateContext((HDC)window_helper.context);
	if(!temp_context){
		win32_log_last_error("wglCreateContext", g_graphics->break_on_error);
		return;
	}
	wglMakeCurrent((HDC)window_helper.context, temp_context);
	
	//load wgl extensions
	backend_version = gladLoaderLoadWGL((HDC)window_helper.context);
	if(backend_version == 0){
		LogEGl("Failed to load OpenGL");
		return;
	}
	Logf("opengl","Loaded WGL %d.%d", GLAD_VERSION_MAJOR(backend_version), GLAD_VERSION_MINOR(backend_version));
	
#if BUILD_INTERNAL
	gladInstallWGLDebug();
	gladSetWGLPostCallback(WGLDebugPostCallback);
#endif //#if BUILD_INTERNAL
	
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
		WGL_DOUBLE_BUFFER_ARB,	GL_TRUE,
		WGL_PIXEL_TYPE_ARB,		WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,		32,
		WGL_DEPTH_BITS_ARB,		24,
		WGL_STENCIL_BITS_ARB,	8,
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
		0
	};
	wglChoosePixelFormatARB((HDC)window->context, format_attributes, 0, 1, &format, &format_count); //https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
	if(!DescribePixelFormat((HDC)window->context, format, sizeof(pfd), &pfd)){
		win32_log_last_error("DescribePixelFormat", g_graphics->break_on_error);
		return;
	}
	if(format == 0){
		win32_log_last_error("ChoosePixelFormatARB", g_graphics->break_on_error);
		return;
	}
	if(!SetPixelFormat((HDC)window->context, format, &pfd)){ win32_log_last_error("SetPixelFormat", g_graphics->break_on_error); return; }
	
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
	opengl_context = wglCreateContextAttribsARB((HDC)window->context, 0, context_attributes); //https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
	if(!opengl_context){
		win32_log_last_error("wglCreateContextAttribsARB", g_graphics->break_on_error);
		return;
	}
	wglMakeCurrent((HDC)window->context, (HGLRC)opengl_context);
#elif DESHI_LINUX //#if DESHI_WINDOWS
	// following this tutorial: https://www.khronos.org/opengl/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib
	
	XFlush(linux.x11.display);
	// print a nice message since loading opengl currently takes long enough for us to be able to see it :D
	XDrawString(linux.x11.display, (X11Window)window->handle, (GC)window->context, 50, 50, "Loading OpenGL...", 17);
	
	backend_version = gladLoaderLoadGLX(linux.x11.display, linux.x11.screen);
	Logf("opengl","Loaded GLX %d.%d", GLAD_VERSION_MAJOR(backend_version), GLAD_VERSION_MINOR(backend_version));
	
	gladInstallGLXDebug();
	
	// get restore points 
	Display* prev_display = glXGetCurrentDisplay();
	GLXContext prev_context = glXGetCurrentContext();
	
	// list of attributes to ask of GLX
	int attributes_config[] = {
		GLX_DEPTH_SIZE, 24,  // 24 bit depth
		GLX_STENCIL_SIZE, 8, // 8 bit stencil size
		GLX_DOUBLEBUFFER,	 // use a double buffer
		0,
	};
	
	int n_elem;
	GLXFBConfig* config = glXChooseFBConfig(linux.x11.display, linux.x11.screen, attributes_config, &n_elem);
	if(!config || !n_elem) {
		LogEGl("Cannot find an appropriate framebuffer configuration with glXChooseFBConfig");
		Assert(false);
	}
	
	GLXFBConfig fbconfig = config[0];
	XFree(config);
	
	int attributes_visual[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 24,  // 24 bit depth
		GLX_STENCIL_SIZE, 8, // 8 bit stencil size
		GLX_DOUBLEBUFFER,	 // use a double buffer
		0,
	};
	
	// get the best visual for our chosen attributes
	XVisualInfo* vi = glXChooseVisual(linux.x11.display, linux.x11.screen, attributes_visual);
	if(!vi) {
		LogEGl("Cannot find an appropriate visual for the given attributes");
		Assert(0);
	}
	
	Colormap cm = XCreateColormap(linux.x11.display, (X11Window)window->handle, vi->visual, AllocNone);
	
	int contextAttribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3, 
		GLX_CONTEXT_MINOR_VERSION_ARB, 2, 
#if BUILD_INTERNAL
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | GLX_CONTEXT_DEBUG_BIT_ARB,
#else //#if BUILD_INTERNAL
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, 
#endif //#else //#if BUILD_INTERNAL
		0 // Terminate the attribute list
	};
	
	GLXContext context = glXCreateContextAttribsARB(linux.x11.display, fbconfig, 0, True, contextAttribs);
	if(!glXMakeCurrent(linux.x11.display, (X11Window)window->handle, context)) {
		LogEGl("Failed to set glx context");
	}
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  error "unhandled platform"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	//load glad extensions
	opengl_version = gladLoaderLoadGL();
	if(opengl_version == 0){
		LogEGl("Failed to load OpenGL");
		return;
	}
	Logf("opengl","Loaded OpenGL %d.%d", GLAD_VERSION_MAJOR(opengl_version), GLAD_VERSION_MINOR(opengl_version));
	
#if BUILD_INTERNAL
	gladInstallGLDebug();
	gladSetGLPostCallback(GladDebugPostCallback);
#endif //#if BUILD_INTERNAL
	
#if DESHI_WINDOWS
	UpdateWindow((HWND)window->handle);
#elif DESHI_LINUX //#if DESHI_WINDOWS
	XFlush(linux.x11.display);
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  error "unhandled platform"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup per-window info
	WindowRenderInfoGl* window_info = window->render_info = memory_allocT(WindowRenderInfoGl);
	window_info->command_buffer = graphics_command_buffer_allocate();
	array_init(window_info->command_buffer->commands, 1, g_graphics->allocators.primary);
	
	g_graphics->initialized = true;
	DeshiStageInitEnd(DS_RENDER);
}


void
graphics_update(Window* window){DPZoneScoped;
	Stopwatch update_stopwatch = start_stopwatch();
	WindowRenderInfoGl* window_info = (WindowRenderInfoGl*)window->render_info;
	g_graphics->stats = {};
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// handle if the window resized
	if(g_window->resized){
		if(g_window->width <= 0 || g_window->height <= 0){
			return;
		}
		glViewport(0,0, g_window->width,g_window->height);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// execute commands
	for_array(window_info->command_buffer->commands){
		
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// present frame
#if DESHI_WINDOWS
	::SwapBuffers((HDC)window->context);
#elif DESHI_LINUX //#if DESHI_WINDOWS
	glXSwapBuffers(linux.x11.display, (X11Window)window->handle);
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  error "unhandled platform"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	g_time->renderTime = peek_stopwatch(update_stopwatch);
}

void
graphics_cleanup(){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_buffer


GraphicsBuffer*
graphics_buffer_create(void* data, u64 requested_size, GraphicsBufferUsage usage, GraphicsMemoryPropertyFlags properties, GraphicsMemoryMappingBehavoir mapping_behavoir){
	DontCompile;
}

void
graphics_buffer_destroy(GraphicsBuffer* x){
	DontCompile;
}

void
graphics_buffer_reallocate(GraphicsBuffer* x, u64 new_size){
	DontCompile;
}

void*
graphics_buffer_map(GraphicsBuffer* x, u64 size, u64 offset){
	DontCompile;
}

void
graphics_buffer_unmap(GraphicsBuffer* x, b32 flush){
	DontCompile;
}

void
graphics_buffer_flush(GraphicsBuffer* x){
	DontCompile;
}

u64
graphics_buffer_device_size(GraphicsBuffer* x){
	DontCompile;
}

void*
graphics_buffer_mapped_data(GraphicsBuffer* x){
	DontCompile;
}

u64
graphics_buffer_mapped_size(GraphicsBuffer* x){
	DontCompile;
}

u64
graphics_buffer_mapped_offset(GraphicsBuffer* x){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_image


void
graphics_image_update(GraphicsImage* x){
	DontCompile;
}

void
graphics_image_destroy(GraphicsImage* x){
	DontCompile;
}

void
graphics_image_write(GraphicsImage* x, u8* pixels, vec2i offset, vec2i extent){
	DontCompile;
}

void
graphics_image_view_update(GraphicsImageView* x){
	DontCompile;
}

void
graphics_image_view_destroy(GraphicsImageView* x){
	DontCompile;
}

void
graphics_sampler_update(GraphicsSampler* x){
	DontCompile;
}

void
graphics_sampler_destroy(GraphicsSampler* x){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_descriptor


void
graphics_descriptor_set_layout_update(GraphicsDescriptorSetLayout* x){
	DontCompile;
}

void
graphics_descriptor_set_layout_destroy(GraphicsDescriptorSetLayout* x){
	DontCompile;
}

void
graphics_descriptor_set_update(GraphicsDescriptorSet* x){
	DontCompile;
}

void
graphics_descriptor_set_destroy(GraphicsDescriptorSet* x){
	DontCompile;
}

void
graphics_descriptor_set_write(GraphicsDescriptorSet* x, u32 binding, GraphicsDescriptor descriptor){
	DontCompile;
}

void
graphics_descriptor_set_write_array(GraphicsDescriptorSet* x, GraphicsDescriptor* descriptors){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_shader


void
graphics_shader_update(GraphicsShader* x){
	DontCompile;
}

void
graphics_shader_destroy(GraphicsShader* x){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_pipeline


void
graphics_pipeline_layout_update(GraphicsPipelineLayout* x){
	DontCompile;
}

void
graphics_pipeline_layout_destroy(GraphicsPipelineLayout* x){
	DontCompile;
}

void
graphics_pipeline_update(GraphicsPipeline* x){
	DontCompile;
}

void
graphics_pipeline_destroy(GraphicsPipeline* x){
	DontCompile;
}

GraphicsPipeline*
graphics_pipeline_duplicate(GraphicsPipeline* x){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_renderpass


void
graphics_render_pass_update(GraphicsRenderPass* x){
	DontCompile;
}

void
graphics_render_pass_destroy(GraphicsRenderPass* x){
	DontCompile;
}

GraphicsRenderPass*
graphics_render_pass_of_window_presentation_frames(Window* window){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_framebuffer


void
graphics_framebuffer_update(GraphicsFramebuffer* x){
	DontCompile;
}

void
graphics_framebuffer_destroy(GraphicsFramebuffer* x){
	DontCompile;
}

GraphicsFramebuffer*
graphics_current_present_frame_of_window(Window* window){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_commands


void
graphics_command_buffer_update(GraphicsCommandBuffer* x){
	DontCompile;
}

void
graphics_command_buffer_destroy(GraphicsCommandBuffer* x){
	DontCompile;
}

GraphicsCommandBuffer*
graphics_command_buffer_of_window(Window* window){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_misc


GraphicsFormat
graphics_format_of_presentation_frames(Window* window){
	DontCompile;
}

