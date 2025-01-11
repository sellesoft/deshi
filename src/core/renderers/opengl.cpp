/* deshi OpenGL Render Submodule
Notes:
- The minimum supported OpenGL version is 3.3 (most graphics cards after 2007 and laptops after 2012 should support this)

Logging Levels:
0: warnings and errors
1: happens on demand
2: happens reactively
3: happens every frame

Index:
@gl_types
@gl_vars
@gl_utils
@graphics_context
@graphics_device
@graphics_buffer
@graphics_image
@graphics_descriptor
@graphics_shader
@graphics_pipeline
@graphics_renderpass
@graphics_framebuffer
@graphics_commands

TODOs:
mipmaps
- https://www.khronos.org/opengl/wiki/Texture#Mip_maps
- https://learnopengl.com/Getting-started/Textures
multisampling
- https://www.khronos.org/opengl/wiki/Multisampling
- https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing
anistropic filtering
- https://www.khronos.org/opengl/wiki/Sampler_Object#Anisotropic_filtering
pipeline binarys (pipeline caching)
- https://registry.khronos.org/OpenGL/extensions/ARB/ARB_get_program_binary.txt
specialization constants (spir-v)
- https://www.khronos.org/opengl/wiki/SPIR-V/Compilation
*/


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_types


typedef struct WindowRenderInfoGl{
	GraphicsCommandBuffer* command_buffer;
	GraphicsFramebuffer* framebuffer;
}WindowRenderInfoGl;


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_vars


local void* opengl_context = 0;

local GLint opengl_success = 0;
local GLenum opengl_error = 0;

local int opengl_version = 0;
local int backend_version = 0;

#define OPENGL_INFOLOG_SIZE 512
local char opengl_infolog[OPENGL_INFOLOG_SIZE];

//NOTE(delle) the target we bind to mostly doesn't matter
#define OPENGL_GRAPHICS_BUFFER_TARGET GL_ARRAY_BUFFER


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_utils


#define LogGl(level, ...) if(g_graphics->logging_level >= level){ logger_push_indent(level); Log("opengl", __VA_ARGS__); logger_pop_indent(level); }(void)0
#define LogWGl(...) LogW("opengl", __func__, "(): ", __VA_ARGS__)
#define LogEGl(...) LogE("opengl", __func__, "(): ", __VA_ARGS__); Assert(!g_graphics->break_on_error)
#define GL_VERSION_TEST(major,minor,...) (opengl_version >= GLAD_MAKE_VERSION(major,minor))


#if DESHI_WINDOWS
local void
WGLDebugPostCallback(void *ret, const char *name, GLADapiproc apiproc, s32 len_args, ...){DPZoneScoped;
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
	LogEGl("WGL ERROR_",(u32)error_code," '",error_flag,"' on ",name,"(); Reason: ",error_msg);
}
#endif //#if DESHI_WINDOWS


#if DESHI_LINUX
local void
GLXDebugPostCallback(void *ret, const char *name, GLADapiproc apiproc, s32 len_args, ...){DPZoneScoped;
	u32 error_code = 0; //glx error code?
	const char* error_flag = 0;
	const char* error_msg  = 0;
	switch(error_code){
		default:{
			error_flag = "?";
			error_msg  = "?";
		}break;
	}
	LogEGl("GLX ERROR_",(u32)error_code," '",error_flag,"' on ",name,"(); Reason: ",error_msg);
}
#endif //#if DESHI_LINUX


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
		LogEGl("ERROR_",opengl_error," '",error_flag,"' on ",name,"(); Reason: ",error_msg);
	}
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_context


void
graphics_init(Window* window){DPZoneScoped;
	AssertAlways(window);
	DeshiStageInitStart(DS_GRAPHICS, DS_MEMORY|DS_PLATFORM, "Attempted to initialize the OpenGL Graphics module before initializing the Memory or Platform modules.");
	Log("opengl","Starting OpenGL graphics backend initialization.");
	
	graphics_init_shared(window);
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup WGL and glad
#if DESHI_WINDOWS
	//!ref: https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
	
	//restore point for contexts
	HDC   prev_dc = wglGetCurrentDC();
	HGLRC prev_rc = wglGetCurrentContext();
	
	//setup pixel format for dummy device context
	PIXELFORMATDESCRIPTOR temp_pfd{sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER};
	temp_pfd.cColorBits = 32;
	temp_pfd.cDepthBits = 24;
	temp_pfd.cStencilBits = 8;
	int temp_format = ::ChoosePixelFormat((HDC)window_helper.context, &temp_pfd);
	if(!::SetPixelFormat((HDC)window_helper.context, temp_format, &temp_pfd)){
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
	
	//load wgl
	backend_version = gladLoaderLoadWGL((HDC)window_helper.context);
	if(backend_version == 0){
		LogEGl("Failed to load WGL.");
		return;
	}
#if BUILD_INTERNAL
	gladInstallWGLDebug();
	gladSetWGLPostCallback(WGLDebugPostCallback);
#endif //#if BUILD_INTERNAL
	Logf("opengl","Loaded WGL %d.%d", GLAD_VERSION_MAJOR(backend_version), GLAD_VERSION_MINOR(backend_version));
	
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
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		//WGL_TRANSPARENT_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
		0 //terminate
	};
	if(wglChoosePixelFormatARB((HDC)window->context, format_attributes, 0, 1, &format, &format_count) == FALSE){
		win32_log_last_error("wglChoosePixelFormatARB", g_graphics->break_on_error);
		return;
	}
	if(format == 0){
		win32_log_last_error("ChoosePixelFormatARB", g_graphics->break_on_error);
		return;
	}
	if(!::DescribePixelFormat((HDC)window->context, format, sizeof(pfd), &pfd)){
		win32_log_last_error("DescribePixelFormat", g_graphics->break_on_error);
		return;
	}
	if(!::SetPixelFormat((HDC)window->context, format, &pfd)){
		win32_log_last_error("SetPixelFormat", g_graphics->break_on_error);
		return;
	}
	
	//create and enable the actual render context
	int context_attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
#if BUILD_INTERNAL
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
#else //#if BUILD_INTERNAL
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#endif //#else //#if BUILD_INTERNAL
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0 //terminate
	};
	opengl_context = wglCreateContextAttribsARB((HDC)window->context, 0, context_attributes);
	if(!opengl_context){
		win32_log_last_error("wglCreateContextAttribsARB", g_graphics->break_on_error);
		return;
	}
	wglMakeCurrent((HDC)window->context, (HGLRC)opengl_context);
	
#elif DESHI_LINUX //#if DESHI_WINDOWS
	//!ref: https://www.khronos.org/opengl/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib
	
	//print a nice message since loading opengl currently takes long enough for us to be able to see it :D
	XFlush(linux.x11.display);
	XDrawString(linux.x11.display, (X11Window)window->handle, (GC)window->context, 50, 50, "Loading OpenGL...", 17);
	
	//load glx
	backend_version = gladLoaderLoadGLX(linux.x11.display, linux.x11.screen);
	if(backend_version == 0){
		LogEGl("Failed to load GLX.");
		return;
	}
#if BUILD_INTERNAL
	gladInstallGLXDebug();
	gladSetGLXPostCallback(GLXDebugPostCallback);
#endif //#if BUILD_INTERNAL
	Logf("opengl","Loaded GLX %d.%d", GLAD_VERSION_MAJOR(backend_version), GLAD_VERSION_MINOR(backend_version));
	
	//get all framebuffer configs that support these attributes
	int attributes_config[] = {
		GLX_X_RENDERABLE, GL_TRUE,
		GLX_DOUBLEBUFFER, GL_TRUE,
		//GLX_TRANSPARENT_TYPE, GLX_TRANSPARENT_RGB,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
		0, //terminate
	};
	int framebuffer_configs_count;
	GLXFBConfig* framebuffer_configs = glXChooseFBConfig(linux.x11.display, linux.x11.screen, attributes_config, &framebuffer_configs_count);
	if(!framebuffer_configs || framebuffer_configs_count <= 0){
		LogEGl("Cannot find an appropriate framebuffer configuration with glXChooseFBConfig.");
		return;
	}
	
	//choose the best framebuffer config and visual info for the above attributes
	//NOTE(delle) "best" config meaning the with the most samples
	int best_framebuffer_config_index = 0;
	int best_framebuffer_config_samples = -1;
	XVisualInfo* best_visual_info = 0;
	for(int samples, sample_buffers, i = 1; i < framebuffer_configs_count; i += 1){
		XVisualInfo* visual_info = glXGetVisualFromFBConfig(linux.x11.display, framebuffer_configs[i]);
		if(visual_info){
			glXGetFBConfigAttrib(linux.x11.display, framebuffer_configs[i], GLX_SAMPLES, &samples);
			glXGetFBConfigAttrib(linux.x11.display, framebuffer_configs[i], GLX_SAMPLE_BUFFERS, &sample_buffers);
			if(sample_buffers > 0 && samples > best_framebuffer_config_samples){
				best_framebuffer_config_index = i;
				best_framebuffer_config_samples = samples;
				if(best_visual_info){
					XFree(best_visual_info);
				}
				best_visual_info = visual_info;
			}else{
				XFree(visual_info);
			}
		}
	}
	GLXFBConfig best_framebuffer_config = framebuffer_configs[best_framebuffer_config_index];
	XFree(framebuffer_configs);
	if(!best_visual_info){
		LogEGl("Cannot find an appropriate visual for the given attributes.");
		return;
	}
	
	//create the colormap
	Colormap cm = XCreateColormap(linux.x11.display, (X11Window)window->handle, best_visual_info->visual, AllocNone);
	XFree(best_visual_info);
	
	//create the context
	int attributes_context[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
#if BUILD_INTERNAL
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | GLX_CONTEXT_DEBUG_BIT_ARB,
#else //#if BUILD_INTERNAL
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif //#else //#if BUILD_INTERNAL
		0 //terminate
	};
	GLXContext context = glXCreateContextAttribsARB(linux.x11.display, best_framebuffer_config, 0, True, attributes_context);
	if(!glXMakeCurrent(linux.x11.display, (X11Window)window->handle, context)){
		LogEGl("Failed to create the GLX context.");
		return;
	}
	
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#   error "unhandled platform"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	//load opengl
	opengl_version = gladLoaderLoadGL();
	if(opengl_version == 0){
		LogEGl("Failed to load OpenGL.");
		return;
	}
	if(!GL_VERSION_TEST(3,3)){
		LogEGl("The OpenGL graphics backend of deshi requires at least version 3.3 which is not supported on this device.");
		return;
	}
#if BUILD_INTERNAL
	gladInstallGLDebug();
	gladSetGLPostCallback(GladDebugPostCallback);
#endif //#if BUILD_INTERNAL
	Logf("opengl","Loaded OpenGL %d.%d", GLAD_VERSION_MAJOR(opengl_version), GLAD_VERSION_MINOR(opengl_version));
	
#if DESHI_WINDOWS
	UpdateWindow((HWND)window->handle);
#elif DESHI_LINUX //#if DESHI_WINDOWS
	XFlush(linux.x11.display);
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#   error "unhandled platform"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup per-window info
	WindowRenderInfoGl* window_info = (WindowRenderInfoGl*)g_graphics->allocators.primary->reserve(sizeof(WindowRenderInfoGl));
	window->render_info = window_info;
	
	//setup the window's default command buffer
	window_info->command_buffer = graphics_command_buffer_allocate();
	window_info->command_buffer->debug_name = str8_lit("<graphics> default command buffer");
	array_init(window_info->command_buffer->commands, GRAPHICS_INITIAL_COMMAND_BUFFER_SIZE, g_graphics->allocators.primary);
	
	//setup the window's default framebuffer
	//NOTE(delle) OpenGL creation of this framebuffer occurred above during context creation
	window_info->framebuffer = graphics_framebuffer_allocate();
	window_info->framebuffer->debug_name = str8_lit("<graphics> default framebuffer");
	window_info->framebuffer->render_pass = graphics_render_pass_allocate();
	window_info->framebuffer->render_pass->debug_name = str8_lit("<graphics> default framebuffer's render pass");
	window_info->framebuffer->render_pass->use_color_attachment = true;
	window_info->framebuffer->render_pass->color_attachment.format = GraphicsFormat_R8G8B8A8_SRGB;
	window_info->framebuffer->render_pass->color_attachment.load_op = GraphicsLoadOp_Clear;
	window_info->framebuffer->render_pass->color_attachment.store_op = GraphicsStoreOp_Store;
	window_info->framebuffer->render_pass->color_attachment.stencil_load_op = GraphicsLoadOp_Dont_Care;
	window_info->framebuffer->render_pass->color_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	window_info->framebuffer->render_pass->color_attachment.initial_layout = GraphicsImageLayout_Undefined;
	window_info->framebuffer->render_pass->color_attachment.final_layout = GraphicsImageLayout_Present;
	window_info->framebuffer->render_pass->depth_attachment.format = GraphicsFormat_Depth24_UNorm_Stencil8_UInt;
	window_info->framebuffer->render_pass->depth_attachment.load_op = GraphicsLoadOp_Clear;
	window_info->framebuffer->render_pass->depth_attachment.store_op = GraphicsStoreOp_Store;
	window_info->framebuffer->render_pass->depth_attachment.stencil_load_op = GraphicsLoadOp_Clear;
	window_info->framebuffer->render_pass->depth_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	window_info->framebuffer->render_pass->depth_attachment.initial_layout = GraphicsImageLayout_Undefined;
	window_info->framebuffer->render_pass->depth_attachment.final_layout = GraphicsImageLayout_Depth_Stencil_Attachment_Optimal;
	window_info->framebuffer->render_pass->color_clear_values = vec4{0.0f,0.0f,0.0f,0.0f};
	window_info->framebuffer->render_pass->depth_clear_values.depth = 1.0f;
	window_info->framebuffer->render_pass->depth_clear_values.stencil = 0;
	window_info->framebuffer->width = window->width;
	window_info->framebuffer->height = window->height;
	window_info->framebuffer->color_image_view = graphics_image_view_allocate();
	window_info->framebuffer->color_image_view->debug_name = str8_lit("<graphics> default framebuffer's color image view");
	window_info->framebuffer->color_image_view->format = GraphicsFormat_R8G8B8A8_SRGB;
	window_info->framebuffer->color_image_view->aspect_flags = GraphicsImageViewAspectFlags_Color;
	window_info->framebuffer->color_image_view->image = graphics_image_allocate();
	window_info->framebuffer->color_image_view->image->debug_name = str8_lit("<graphics> default framebuffer's color image");
	window_info->framebuffer->color_image_view->image->format = GraphicsFormat_R8G8B8A8_SRGB;
	window_info->framebuffer->color_image_view->image->usage = GraphicsImageUsage_Color_Attachment;
	window_info->framebuffer->color_image_view->image->samples = GraphicsSampleCount_1;
	window_info->framebuffer->color_image_view->image->linear_tiling = false;
	window_info->framebuffer->color_image_view->image->extent = vec3i{(s32)window->width, (s32)window->height, 0};
	window_info->framebuffer->color_image_view->image->memory_properties = GraphicsMemoryPropertyFlag_DeviceLocal;
	window_info->framebuffer->color_image_view->image->mip_levels = 0;
	window_info->framebuffer->depth_image_view = graphics_image_view_allocate();
	window_info->framebuffer->depth_image_view->debug_name = str8_lit("<graphics> default framebuffer's depth image view");
	window_info->framebuffer->depth_image_view->format = GraphicsFormat_Depth24_UNorm_Stencil8_UInt;
	window_info->framebuffer->depth_image_view->aspect_flags = GraphicsImageViewAspectFlags_Depth;
	window_info->framebuffer->depth_image_view->image = graphics_image_allocate();
	window_info->framebuffer->depth_image_view->image->debug_name = str8_lit("<graphics> default framebuffer's depth image");
	window_info->framebuffer->depth_image_view->image->format = GraphicsFormat_Depth24_UNorm_Stencil8_UInt;
	window_info->framebuffer->depth_image_view->image->usage = GraphicsImageUsage_Depth_Stencil_Attachment;
	window_info->framebuffer->depth_image_view->image->samples = GraphicsSampleCount_1;
	window_info->framebuffer->depth_image_view->image->linear_tiling = false;
	window_info->framebuffer->depth_image_view->image->extent = vec3i{(s32)window->width, (s32)window->height, 0};
	window_info->framebuffer->depth_image_view->image->memory_properties = GraphicsMemoryPropertyFlag_DeviceLocal;
	window_info->framebuffer->depth_image_view->image->mip_levels = 0;
	window_info->framebuffer->__internal.handle = 0; //NOTE(delle) OpenGL default framebuffer handle is zero
	
	g_graphics->initialized = true;
	DeshiStageInitEnd(DS_GRAPHICS);
}


void
graphics_update(Window* window){DPZoneScoped;
	Stopwatch update_stopwatch = start_stopwatch();
	WindowRenderInfoGl* window_info = (WindowRenderInfoGl*)window->render_info;
	g_graphics->stats = {};
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// handle if the window resized
	if(window->resized){
		LogGl(2,"Window resized; Updating the viewport.");
		window_info->framebuffer->color_image_view->image->extent = vec3i{(s32)window->width, (s32)window->height, 0};
		window_info->framebuffer->depth_image_view->image->extent = vec3i{(s32)window->width, (s32)window->height, 0};
		if(window->width <= 0 || window->height <= 0){
			return;
		}
		glViewport(0, 0, (GLsizei)window->width, (GLsizei)window->height);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// pre-draw setup
	//NOTE(delle) GL_MAP_PERSISTENT_BIT requires OpenGL4.4 so we have to unmap before drawing and remap after drawing to simulate
	if(!GL_VERSION_TEST(4,4, GL_MAP_PERSISTENT_BIT)){
		for_pool(g_graphics->pools.buffers){
			if(it->__internal.buffer_handle && it->__internal.mapping_behavior == GraphicsMemoryMapping_Persistent){
				glBindBuffer(OPENGL_GRAPHICS_BUFFER_TARGET, (GLuint)(u64)it->__internal.buffer_handle);
				if(!opengl_error){
					glUnmapBuffer(OPENGL_GRAPHICS_BUFFER_TARGET);
				}
			}
		}
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// execute commands
	LogGl(3,"Starting a command buffer[",window_info->command_buffer->debug_name,"].");
	
	GraphicsRenderPass* active_render_pass = 0;
	GraphicsPipeline* active_pipeline = 0;
	forX_array(cmd, window_info->command_buffer->commands){
		switch(cmd->type){
			case GraphicsCommandType_Begin_Render_Pass:{
				if(!cmd->begin_render_pass.pass){
					LogEGl("Null renderpass specified in a GraphicsCommandType_Begin_Render_Pass.");
					continue;
				}
				if(active_render_pass){
					LogEGl("Attempted to begin a renderpass[",cmd->begin_render_pass.pass->debug_name,"] while another renderpass[",active_render_pass->debug_name,"] is already in progress.");
					continue;
				}
				LogGl(3,"Beginning a renderpass[",cmd->begin_render_pass.pass->debug_name,"].");
				
				glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)(u64)cmd->begin_render_pass.frame->__internal.handle);
				
				GLenum clear_bits = 0;
				if(cmd->begin_render_pass.pass->use_color_attachment){
					glClearColor(cmd->begin_render_pass.pass->color_clear_values.r,
								 cmd->begin_render_pass.pass->color_clear_values.g,
								 cmd->begin_render_pass.pass->color_clear_values.r,
								 cmd->begin_render_pass.pass->color_clear_values.a);
					if(cmd->begin_render_pass.pass->color_attachment.load_op == GraphicsLoadOp_Clear){
						clear_bits |= GL_COLOR_BUFFER_BIT;
					}
				}
				
				if(cmd->begin_render_pass.pass->use_depth_attachment){
					//NOTE(delle) (1 - depth) because openGL Z direction is opposite deshi's
					glClearDepth(1.0 - (f64)cmd->begin_render_pass.pass->depth_clear_values.depth);
					glClearStencil(cmd->begin_render_pass.pass->depth_clear_values.stencil);
					if(cmd->begin_render_pass.pass->depth_attachment.load_op == GraphicsLoadOp_Clear){
						clear_bits |= GL_DEPTH_BUFFER_BIT;
					}
					if(cmd->begin_render_pass.pass->color_attachment.stencil_load_op == GraphicsLoadOp_Clear){
						clear_bits |= GL_STENCIL_BUFFER_BIT;
					}
				}
				
				if(clear_bits != 0){
					glClear(clear_bits);
				}
				
				active_render_pass = cmd->begin_render_pass.pass;
			}break;
			
			case GraphicsCommandType_End_Render_Pass:{
				if(!active_render_pass){
					LogEGl("Attempted to end a renderpass when one is not in progress.");
					continue;
				}
				LogGl(3,"Ending a renderpass[",active_render_pass->debug_name,"].");
				
				active_render_pass = 0;
			}break;
			
			case GraphicsCommandType_Bind_Pipeline:{
				if(!cmd->bind_pipeline.handle){
					LogEGl("Null pipeline specified in a GraphicsCommandType_Bind_Pipeline.");
					continue;
				}
				if(!cmd->bind_pipeline.handle->layout){
					LogEGl("Pipeline[",cmd->bind_pipeline.handle->debug_name,"] specified in a GraphicsCommandType_Bind_Pipeline has a null layout.");
					continue;
				}
				LogGl(3,"Binding a pipeline[",cmd->bind_pipeline.handle->debug_name,"].");
				
				//disable previous pipeline's vertex attributes array
				if(active_pipeline && active_pipeline->vertex_input_attributes){
					forI(array_count(active_pipeline->vertex_input_attributes)){
						glDisableVertexAttribArray(i);
					}
				}
				
				glUseProgram((GLuint)(u64)cmd->bind_pipeline.handle->__internal.handle);
				
				if(!cmd->bind_pipeline.handle->dynamic_viewport){
					glViewport((GLint)cmd->bind_pipeline.handle->viewport_offset.x, (GLint)cmd->bind_pipeline.handle->viewport_offset.y,
							   (GLsizei)cmd->bind_pipeline.handle->viewport_extent.x, (GLsizei)cmd->bind_pipeline.handle->viewport_extent.y);
				}
				
				if(!cmd->bind_pipeline.handle->dynamic_scissor){
					glEnable(GL_SCISSOR_TEST);
					glScissor((GLint)cmd->bind_pipeline.handle->scissor_offset.x, (GLint)cmd->bind_pipeline.handle->scissor_offset.y,
							  (GLsizei)cmd->bind_pipeline.handle->scissor_extent.x, (GLsizei)cmd->bind_pipeline.handle->scissor_extent.y);
				}else{
					glDisable(GL_SCISSOR_TEST);
				}
				
				switch(cmd->bind_pipeline.handle->front_face){
					case GraphicsFrontFace_CW:{
						glFrontFace(GL_CW);
					}break;
					case GraphicsFrontFace_CCW:{
						glFrontFace(GL_CCW);
					}break;
					default:{
						LogEGl("Unhandled GraphicsFrontFace when setting the front face for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->front_face,".");
					}break;
				}
				
				switch(cmd->bind_pipeline.handle->culling){
					case GraphicsPipelineCulling_None:{
						glDisable(GL_CULL_FACE);
					}break;
					case GraphicsPipelineCulling_Front:{
						glEnable(GL_CULL_FACE);
						glCullFace(GL_FRONT);
					}break;
					case GraphicsPipelineCulling_Back:{
						glEnable(GL_CULL_FACE);
						glCullFace(GL_BACK);
					}break;
					case GraphicsPipelineCulling_Front_Back:{
						glEnable(GL_CULL_FACE);
						glCullFace(GL_FRONT_AND_BACK);
					}break;
					default:{
						LogEGl("Unhandled GraphicsPipelineCulling when setting the cull face for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->culling,".");
					}break;
				}
				
				if(!cmd->bind_pipeline.handle->dynamic_line_width){
					glLineWidth((GLfloat)cmd->bind_pipeline.handle->line_width);
				}
				
				if(cmd->bind_pipeline.handle->depth_test){
					glEnable(GL_DEPTH_TEST);
					glDepthMask((cmd->bind_pipeline.handle->depth_writes) ? GL_TRUE : GL_FALSE);
					switch(cmd->bind_pipeline.handle->depth_compare_op){
						case GraphicsCompareOp_Never:{
							glDepthFunc(GL_NEVER);
						}break;
						case GraphicsCompareOp_Less:{
							glDepthFunc(GL_LESS);
						}break;
						case GraphicsCompareOp_Equal:{
							glDepthFunc(GL_EQUAL);
						}break;
						case GraphicsCompareOp_Less_Or_Equal:{
							glDepthFunc(GL_LEQUAL);
						}break;
						case GraphicsCompareOp_Greater:{
							glDepthFunc(GL_GREATER);
						}break;
						case GraphicsCompareOp_Not_Equal:{
							glDepthFunc(GL_NOTEQUAL);
						}break;
						case GraphicsCompareOp_Greater_Or_Equal:{
							glDepthFunc(GL_GEQUAL);
						}break;
						case GraphicsCompareOp_Always:{
							glDepthFunc(GL_ALWAYS);
						}break;
						default:{
							LogEGl("Unhandled GraphicsCompareOp when setting the depth test function for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->depth_compare_op,".");
						}break;
					}
				}else{
					glDisable(GL_DEPTH_TEST);
				}
				
				if(cmd->bind_pipeline.handle->depth_bias){
					switch(cmd->bind_pipeline.handle->polygon_mode){
						case GraphicsPolygonMode_Point:{
							glEnable(GL_POLYGON_OFFSET_POINT);
							glDisable(GL_POLYGON_OFFSET_LINE);
							glDisable(GL_POLYGON_OFFSET_FILL);
						}break;
						case GraphicsPolygonMode_Line:{
							glDisable(GL_POLYGON_OFFSET_POINT);
							glEnable(GL_POLYGON_OFFSET_LINE);
							glDisable(GL_POLYGON_OFFSET_FILL);
						}break;
						case GraphicsPolygonMode_Fill:{
							glDisable(GL_POLYGON_OFFSET_POINT);
							glDisable(GL_POLYGON_OFFSET_LINE);
							glEnable(GL_POLYGON_OFFSET_FILL);
						}break;
						default:{
							LogEGl("Unhandled GraphicsPolygonMode when setting the depth bias for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->polygon_mode,".");
						}break;
					}
					
					if(!cmd->bind_pipeline.handle->dynamic_depth_bias){
						glPolygonOffset(cmd->bind_pipeline.handle->depth_bias_slope, cmd->bind_pipeline.handle->depth_bias_constant);
					}
				}
				
				if(cmd->bind_pipeline.handle->color_blend){
					glEnable(GL_BLEND);
					
					GLenum color_blend_equation = GL_FUNC_ADD;
					GLenum alpha_blend_equation = GL_FUNC_ADD;
					switch(cmd->bind_pipeline.handle->color_blend_op){
						case GraphicsBlendOp_Add: color_blend_equation = GL_FUNC_ADD; break;
						case GraphicsBlendOp_Sub: color_blend_equation = GL_FUNC_SUBTRACT; break;
						case GraphicsBlendOp_Reverse_Sub: color_blend_equation = GL_FUNC_REVERSE_SUBTRACT; break;
						case GraphicsBlendOp_Min: color_blend_equation = GL_MIN; break;
						case GraphicsBlendOp_Max: color_blend_equation = GL_MAX; break;
						default: LogEGl("Unhandled GraphicsBlendOp when setting the color blend equation for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->polygon_mode,"."); break;
					}
					switch(cmd->bind_pipeline.handle->alpha_blend_op){
						case GraphicsBlendOp_Add: alpha_blend_equation = GL_FUNC_ADD; break;
						case GraphicsBlendOp_Sub: alpha_blend_equation = GL_FUNC_SUBTRACT; break;
						case GraphicsBlendOp_Reverse_Sub: alpha_blend_equation = GL_FUNC_REVERSE_SUBTRACT; break;
						case GraphicsBlendOp_Min: alpha_blend_equation = GL_MIN; break;
						case GraphicsBlendOp_Max: alpha_blend_equation = GL_MAX; break;
						default: LogEGl("Unhandled GraphicsBlendOp when setting the alpha blend equation for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->polygon_mode,"."); break;
					}
					glBlendEquationSeparate(color_blend_equation, alpha_blend_equation);
					
					GLenum color_src_blend_func = GL_ONE;
					GLenum color_dst_blend_func = GL_ONE;
					GLenum alpha_src_blend_func = GL_ZERO;
					GLenum alpha_dst_blend_func = GL_ZERO;
					switch(cmd->bind_pipeline.handle->color_src_blend_factor){
						case GraphicsBlendFactor_Zero: color_src_blend_func = GL_ZERO; break;
						case GraphicsBlendFactor_One: color_src_blend_func = GL_ONE; break;
						case GraphicsBlendFactor_Source_Color: color_src_blend_func = GL_SRC_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Source_Color: color_src_blend_func = GL_ONE_MINUS_SRC_COLOR; break;
						case GraphicsBlendFactor_Destination_Color: color_src_blend_func = GL_DST_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Destination_Color: color_src_blend_func = GL_ONE_MINUS_DST_COLOR; break;
						case GraphicsBlendFactor_Source_Alpha: color_src_blend_func = GL_SRC_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Source_Alpha: color_src_blend_func = GL_ONE_MINUS_SRC_ALPHA; break;
						case GraphicsBlendFactor_Destination_Alpha: color_src_blend_func = GL_DST_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Destination_Alpha: color_src_blend_func = GL_ONE_MINUS_DST_ALPHA; break;
						case GraphicsBlendFactor_Constant_Color: color_src_blend_func = GL_CONSTANT_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Constant_Color: color_src_blend_func = GL_ONE_MINUS_CONSTANT_COLOR; break;
						case GraphicsBlendFactor_Constant_Alpha: color_src_blend_func = GL_CONSTANT_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Constant_Alpha: color_src_blend_func = GL_ONE_MINUS_CONSTANT_ALPHA; break;
						default: LogEGl("Unhandled GraphicsBlendFactor when setting the color source blend function for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->polygon_mode,"."); break;
					}
					switch(cmd->bind_pipeline.handle->color_dst_blend_factor){
						case GraphicsBlendFactor_Zero: color_dst_blend_func = GL_ZERO; break;
						case GraphicsBlendFactor_One: color_dst_blend_func = GL_ONE; break;
						case GraphicsBlendFactor_Source_Color: color_dst_blend_func = GL_SRC_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Source_Color: color_dst_blend_func = GL_ONE_MINUS_SRC_COLOR; break;
						case GraphicsBlendFactor_Destination_Color: color_dst_blend_func = GL_DST_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Destination_Color: color_dst_blend_func = GL_ONE_MINUS_DST_COLOR; break;
						case GraphicsBlendFactor_Source_Alpha: color_dst_blend_func = GL_SRC_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Source_Alpha: color_dst_blend_func = GL_ONE_MINUS_SRC_ALPHA; break;
						case GraphicsBlendFactor_Destination_Alpha: color_dst_blend_func = GL_DST_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Destination_Alpha: color_dst_blend_func = GL_ONE_MINUS_DST_ALPHA; break;
						case GraphicsBlendFactor_Constant_Color: color_dst_blend_func = GL_CONSTANT_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Constant_Color: color_dst_blend_func = GL_ONE_MINUS_CONSTANT_COLOR; break;
						case GraphicsBlendFactor_Constant_Alpha: color_dst_blend_func = GL_CONSTANT_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Constant_Alpha: color_dst_blend_func = GL_ONE_MINUS_CONSTANT_ALPHA; break;
						default: LogEGl("Unhandled GraphicsBlendFactor when setting the color destination blend function for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->polygon_mode,"."); break;
					}
					switch(cmd->bind_pipeline.handle->alpha_src_blend_factor){
						case GraphicsBlendFactor_Zero: alpha_src_blend_func = GL_ZERO; break;
						case GraphicsBlendFactor_One: alpha_src_blend_func = GL_ONE; break;
						case GraphicsBlendFactor_Source_Color: alpha_src_blend_func = GL_SRC_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Source_Color: alpha_src_blend_func = GL_ONE_MINUS_SRC_COLOR; break;
						case GraphicsBlendFactor_Destination_Color: alpha_src_blend_func = GL_DST_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Destination_Color: alpha_src_blend_func = GL_ONE_MINUS_DST_COLOR; break;
						case GraphicsBlendFactor_Source_Alpha: alpha_src_blend_func = GL_SRC_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Source_Alpha: alpha_src_blend_func = GL_ONE_MINUS_SRC_ALPHA; break;
						case GraphicsBlendFactor_Destination_Alpha: alpha_src_blend_func = GL_DST_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Destination_Alpha: alpha_src_blend_func = GL_ONE_MINUS_DST_ALPHA; break;
						case GraphicsBlendFactor_Constant_Color: alpha_src_blend_func = GL_CONSTANT_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Constant_Color: alpha_src_blend_func = GL_ONE_MINUS_CONSTANT_COLOR; break;
						case GraphicsBlendFactor_Constant_Alpha: alpha_src_blend_func = GL_CONSTANT_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Constant_Alpha: alpha_src_blend_func = GL_ONE_MINUS_CONSTANT_ALPHA; break;
						default: LogEGl("Unhandled GraphicsBlendFactor when setting the alpha source blend function for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->polygon_mode,"."); break;
					}
					switch(cmd->bind_pipeline.handle->alpha_dst_blend_factor){
						case GraphicsBlendFactor_Zero: alpha_dst_blend_func = GL_ZERO; break;
						case GraphicsBlendFactor_One: alpha_dst_blend_func = GL_ONE; break;
						case GraphicsBlendFactor_Source_Color: alpha_dst_blend_func = GL_SRC_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Source_Color: alpha_dst_blend_func = GL_ONE_MINUS_SRC_COLOR; break;
						case GraphicsBlendFactor_Destination_Color: alpha_dst_blend_func = GL_DST_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Destination_Color: alpha_dst_blend_func = GL_ONE_MINUS_DST_COLOR; break;
						case GraphicsBlendFactor_Source_Alpha: alpha_dst_blend_func = GL_SRC_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Source_Alpha: alpha_dst_blend_func = GL_ONE_MINUS_SRC_ALPHA; break;
						case GraphicsBlendFactor_Destination_Alpha: alpha_dst_blend_func = GL_DST_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Destination_Alpha: alpha_dst_blend_func = GL_ONE_MINUS_DST_ALPHA; break;
						case GraphicsBlendFactor_Constant_Color: alpha_dst_blend_func = GL_CONSTANT_COLOR; break;
						case GraphicsBlendFactor_One_Minus_Constant_Color: alpha_dst_blend_func = GL_ONE_MINUS_CONSTANT_COLOR; break;
						case GraphicsBlendFactor_Constant_Alpha: alpha_dst_blend_func = GL_CONSTANT_ALPHA; break;
						case GraphicsBlendFactor_One_Minus_Constant_Alpha: alpha_dst_blend_func = GL_ONE_MINUS_CONSTANT_ALPHA; break;
						default: LogEGl("Unhandled GraphicsBlendFactor when setting the alpha destination blend function for a pipeline[",cmd->bind_pipeline.handle->debug_name,"]: ",cmd->bind_pipeline.handle->polygon_mode,"."); break;
					}
					glBlendFuncSeparate(color_src_blend_func, color_dst_blend_func,
										alpha_src_blend_func, alpha_dst_blend_func);
					
					if(!cmd->bind_pipeline.handle->dynamic_blend_constant){
						glBlendColor((GLfloat)cmd->bind_pipeline.handle->blend_constant.r / 255.0f,
									 (GLfloat)cmd->bind_pipeline.handle->blend_constant.g / 255.0f,
									 (GLfloat)cmd->bind_pipeline.handle->blend_constant.b / 255.0f,
									 (GLfloat)cmd->bind_pipeline.handle->blend_constant.a / 255.0f);
					}
				}else{
					glDisable(GL_BLEND);
				}
				
				active_pipeline = cmd->bind_pipeline.handle;
			}break;
			
			case GraphicsCommandType_Set_Viewport:{
				LogGl(3,"Setting the viewport offset to ",cmd->set_viewport.offset," and extent to ",cmd->set_viewport.extent,".");
				
				glViewport((GLint)cmd->set_viewport.offset.x, (GLint)cmd->set_viewport.offset.y,
						   (GLsizei)cmd->set_viewport.extent.x, (GLsizei)cmd->set_viewport.extent.y);
			}break;
			
			case GraphicsCommandType_Set_Scissor:{
				LogGl(3,"Setting the scissor offset to ",cmd->set_viewport.offset," and extent to ",cmd->set_viewport.extent,".");
				
				glEnable(GL_SCISSOR_TEST);
				glScissor((GLint)cmd->set_scissor.offset.x, (GLint)cmd->set_scissor.offset.y,
						  (GLsizei)cmd->set_scissor.extent.x, (GLsizei)cmd->set_scissor.extent.y);
			}break;
			
			case GraphicsCommandType_Set_Depth_Bias:{
				LogGl(3,"Setting the depth bias slope to ",cmd->set_depth_bias.slope, " and constant to ",cmd->set_depth_bias.constant);
				
				glPolygonOffset(cmd->set_depth_bias.slope, cmd->set_depth_bias.constant);
			}break;
			
			case GraphicsCommandType_Bind_Vertex_Buffer:{
				if(!cmd->bind_vertex_buffer.handle){
					LogEGl("Null buffer specified in a GraphicsCommandType_Bind_Vertex_Buffer.");
					continue;
				}
				if(!active_pipeline){
					LogEGl("Attempted to bind a vertex buffer[",cmd->bind_vertex_buffer.handle->debug_name,"] before a pipeline has been bound this frame.");
					continue;
				}
				LogGl(3,"Binding a vertex buffer[",cmd->bind_vertex_buffer.handle->debug_name,"].");
				
				glBindVertexArray((GLuint)cmd->bind_vertex_buffer.handle->__internal.vao_handle);
				glBindBuffer(GL_ARRAY_BUFFER, (GLuint)(u64)cmd->bind_vertex_buffer.handle->__internal.buffer_handle);
			}break;
			
			case GraphicsCommandType_Bind_Index_Buffer:{
				if(!cmd->bind_index_buffer.handle){
					LogEGl("Null buffer specified in a GraphicsCommandType_Bind_Index_Buffer.");
					continue;
				}
				if(!active_pipeline){
					LogEGl("Attempted to bind an index buffer[",cmd->bind_index_buffer.handle->debug_name,"] before a pipeline has been bound this frame.");
					continue;
				}
				LogGl(3,"Binding an index buffer[",cmd->bind_index_buffer.handle->debug_name,"].");
				
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)(u64)cmd->bind_index_buffer.handle->__internal.buffer_handle);
			}break;
			
			case GraphicsCommandType_Bind_Descriptor_Set:{
				if(!cmd->bind_descriptor_set.handle){
					LogEGl("Null descriptor set specified in a GraphicsCommandType_Bind_Descriptor_Set.");
					continue;
				}
				if(!active_pipeline){
					LogEGl("Attempted to bind a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"] before a pipeline has been bound this frame.");
					continue;
				}
				LogGl(3,"Binding a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"].");
				
				GLint max_texture_units;
				glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units);
				GLint max_uniform_buffer_bindings;
				glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &max_uniform_buffer_bindings);
				
				GLuint buffer_index = 0;
				GLint texture_index = 0;
				for_array(cmd->bind_descriptor_set.handle->descriptors){
					switch(it->type){
						case GraphicsDescriptorType_Uniform_Buffer:{
							if(it->__internal.location_in_shader == 0){
								if(buffer_index >= max_uniform_buffer_bindings){
									continue;
								}
								
								if(!it->name_in_shader || *it->name_in_shader == '\0'){
									LogEGl("Empty name_in_shader field on the #",(u32)(it - cmd->bind_descriptor_set.handle->descriptors)," descriptor of a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"].");
									continue;
								}
								
								GLuint uniform_location = glGetUniformBlockIndex((GLuint)(u64)active_pipeline->__internal.handle, it->name_in_shader);
								if(uniform_location == GL_INVALID_INDEX || opengl_error){
									LogEGl("Unable to find the uniform block named '",it->name_in_shader,"' in the shader for the #",(u32)(it - cmd->bind_descriptor_set.handle->descriptors)," descriptor of a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"].");
									continue;
								}
								
								//NOTE(delle) +1 since 0 means uninit
								it->__internal.location_in_shader = (u32)(uniform_location + 1);
							}
							
							glUniformBlockBinding((GLuint)(u64)active_pipeline->__internal.handle, (GLuint)it->__internal.location_in_shader - 1, buffer_index);
							glBindBufferRange(GL_UNIFORM_BUFFER, 0, (GLuint)(u64)it->ubo.buffer->__internal.buffer_handle, (GLintptr)it->ubo.offset, (GLsizeiptr)it->ubo.range);
							
							buffer_index += 1;
						}break;
						
						case GraphicsDescriptorType_Combined_Image_Sampler:{
							if(it->__internal.location_in_shader == 0){
								if(texture_index >= max_texture_units){
									continue;
								}
								if(!it->image.view){
									LogEGl("Null image view on the #",(u32)(it - cmd->bind_descriptor_set.handle->descriptors)," descriptor of a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"].");
									continue;
								}
								if(!it->image.sampler){
									LogEGl("Null image sampler on the #",(u32)(it - cmd->bind_descriptor_set.handle->descriptors)," descriptor of a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"].");
									continue;
								}
								if(!it->name_in_shader || *it->name_in_shader == '\0'){
									LogEGl("Empty name_in_shader field on the #",(u32)(it - cmd->bind_descriptor_set.handle->descriptors)," descriptor of a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"].");
									continue;
								}
								
								GLint uniform_location = glGetUniformLocation((GLuint)(u64)active_pipeline->__internal.handle, it->name_in_shader);
								if(uniform_location == -1 || opengl_error){
									LogEGl("Unable to find the uniform named '",it->name_in_shader,"' in the shader for the #",(u32)(it - cmd->bind_descriptor_set.handle->descriptors)," descriptor of a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"].");
									continue;
								}
								
								//NOTE(delle) +1 since 0 means uninit
								it->__internal.location_in_shader = (u32)(uniform_location + 1);
							}
							
							glActiveTexture(GL_TEXTURE0 + texture_index);
							glUniform1i((GLint)it->__internal.location_in_shader - 1, texture_index);
							glBindTexture((GLenum)(u64)it->image.view->image->__internal.memory_handle, (GLuint)(u64)it->image.view->image->__internal.handle);
							glBindSampler((GLuint)texture_index, (GLuint)(u64)it->image.sampler->__internal.handle);
							
							texture_index += 1;
						}break;
						
						default:{
							LogEGl("Unhandled GraphicsDescriptorType when binding a descriptor set[",cmd->bind_descriptor_set.handle->debug_name,"]: ",it->type,".");
						}continue;
					}
				}
			}break;
			
			case GraphicsCommandType_Push_Constant:{
				if(!active_pipeline){
					LogEGl("Attempted to push constants before a pipeline has been bound this frame.");
					continue;
				}
				if(!active_pipeline->layout->push_constants){
					LogEGl("Attempted to push constants for a pipeline[",active_pipeline->debug_name,"] whose layout[",active_pipeline->layout->debug_name,"] does not specify any push constants.");
					continue;
				}
				LogGl(3,"Pushing constants of [",cmd->push_constant.size,"] bytes at [",cmd->push_constant.data,"].");
				
				for_array(active_pipeline->layout->push_constants){
					if(it->__internal.shader_buffer_handle){
						glUniformBlockBinding((GLuint)(u64)active_pipeline->__internal.handle, (GLuint)it->__internal.shader_block_index, (GLuint)it->__internal.shader_block_binding);
						glBindBufferBase(GL_UNIFORM_BUFFER, (GLuint)it->__internal.shader_block_binding, (GLuint)it->__internal.shader_buffer_handle);
						glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)cmd->push_constant.offset, (GLsizeiptr)cmd->push_constant.size, cmd->push_constant.data);
					}
				}
			}break;
			
			case GraphicsCommandType_Draw_Indexed:{
				if(!active_pipeline){
					LogEGl("Attempted to push constants before a pipeline has been bound this frame.");
					continue;
				}
				LogGl(3,"Drawing vertices using [",cmd->draw_indexed.index_count,"] indices.");
				
				//NOTE(delle) the below vertex descriptions must be set once the buffer is bound, but it uses
				// information from the active pipeline, so we have to perform it here (which sucks) since
				// Vulkan doesn't require the pipeline or vertex buffer to be bound in a specific order
				if(active_pipeline->vertex_input_attributes){
					forI(array_count(active_pipeline->vertex_input_attributes)){
						GLint size = 4;
						GLenum type = GL_FLOAT;
						GLboolean normalized = GL_FALSE;
						switch(active_pipeline->vertex_input_attributes[i].format){
							case GraphicsFormat_R32G32_Float:{
								size = 2;
								type = GL_FLOAT;
								normalized = GL_FALSE;
							}break;
							case GraphicsFormat_R32G32B32_Float:{
								size = 3;
								type = GL_FLOAT;
								normalized = GL_FALSE;
							}break;
							case GraphicsFormat_R8G8B8_SRGB:{
								size = 3;
								type = GL_UNSIGNED_BYTE;
								normalized = GL_FALSE;
							}break;
							case GraphicsFormat_R8G8B8_UNorm:{
								size = 3;
								type = GL_UNSIGNED_BYTE;
								normalized = GL_TRUE;
							}break;
							case GraphicsFormat_R8G8B8A8_SRGB:{
								size = 4;
								type = GL_UNSIGNED_BYTE;
								normalized = GL_FALSE;
							}break;
							case GraphicsFormat_R8G8B8A8_UNorm:{
								size = 4;
								type = GL_UNSIGNED_BYTE;
								normalized = GL_TRUE;
							}break;
							case GraphicsFormat_B8G8R8A8_UNorm:{
								size = GL_BGRA;
								type = GL_UNSIGNED_BYTE;
								normalized = GL_TRUE;
							}break;
							case GraphicsFormat_Depth16_UNorm:{
								size = 1;
								type = GL_UNSIGNED_SHORT;
								normalized = GL_TRUE;
							}break;
							case GraphicsFormat_Depth32_Float:{
								size = 1;
								type = GL_FLOAT;
								normalized = GL_FALSE;
							}break;
							case GraphicsFormat_Depth32_Float_Stencil8_UInt:{
								size = 1;
								type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
								normalized = GL_FALSE;
							}break;
							case GraphicsFormat_Depth24_UNorm_Stencil8_UInt:{
								size = 1;
								type = GL_UNSIGNED_INT_24_8_EXT;
								normalized = GL_TRUE;
							}break;
							default:{
								LogEGl("Unhandled GraphicsFormat when setting the vertex input attributes for a pipeline[",active_pipeline->debug_name,"]: ",active_pipeline->vertex_input_attributes[i].format,".");
							}break;
						}
						u32 binding = active_pipeline->vertex_input_attributes[i].binding;
						GLsizei stride = active_pipeline->vertex_input_bindings[binding].stride;
						const void* offset = (void*)(u64)active_pipeline->vertex_input_attributes[i].offset;
						
						glVertexAttribPointer(i, size, type, normalized, stride, offset);
						glEnableVertexAttribArray(i);
					}
				}
				
				GLsizei index_count = (GLsizei)cmd->draw_indexed.index_count;
				GLenum index_type = GL_UNSIGNED_INT;
				const void* index_offset = (void*)(u64)(cmd->draw_indexed.index_offset * sizeof(u32));
				GLint vertex_offset = (GLint)cmd->draw_indexed.vertex_offset;
				switch(active_pipeline->polygon_mode){
					case GraphicsPolygonMode_Point:{
						glDrawElementsBaseVertex(GL_POINTS, index_count, index_type, index_offset, vertex_offset);
					}break;
					case GraphicsPolygonMode_Line:{
						glDrawElementsBaseVertex(GL_LINES, index_count, index_type, index_offset, vertex_offset);
					}break;
					case GraphicsPolygonMode_Fill:{
						glDrawElementsBaseVertex(GL_TRIANGLES, index_count, index_type, index_offset, vertex_offset);
					}break;
					default:{
						LogEGl("Unhandled pipeline polygon mode: ",active_pipeline->polygon_mode,".");
					}break;
				}
			}break;
		}
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// present frame
	LogGl(3,"Presenting the frame.");
	
	glFlush(); //flush all threads' commands before calling SwapBuffers (not really necessary when single-threaded)
#if DESHI_WINDOWS
	::SwapBuffers((HDC)window->context);
#elif DESHI_LINUX //#if DESHI_WINDOWS
	glXSwapBuffers(linux.x11.display, (X11Window)window->handle);
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#   error "unhandled platform"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	array_clear(window_info->command_buffer->commands);
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// post-draw setup
	//NOTE(delle) GL_MAP_PERSISTENT_BIT requires OpenGL4.4 so we have to unmap before drawing and remap after drawing to simulate
	if(!GL_VERSION_TEST(4,4, GL_MAP_PERSISTENT_BIT)){
		for_pool(g_graphics->pools.buffers){
			if(it->__internal.buffer_handle && it->__internal.mapping_behavior == GraphicsMemoryMapping_Persistent){
				glBindBuffer(OPENGL_GRAPHICS_BUFFER_TARGET, (GLuint)(u64)it->__internal.buffer_handle);
				if(!opengl_error){
					GLintptr offset = (GLintptr)it->__internal.mapped.offset;
					GLsizeiptr size = (GLsizeiptr)it->__internal.mapped.size;
					GLenum map_bits = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT;
					if(!HasFlag(it->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostCoherent)){
						map_bits |= GL_MAP_FLUSH_EXPLICIT_BIT;
					}
					it->__internal.mapped.data = glMapBufferRange(OPENGL_GRAPHICS_BUFFER_TARGET, offset, size, map_bits);
				}
			}
		}
	}
	
	g_time->renderTime = peek_stopwatch(update_stopwatch);
}


void
graphics_cleanup(){
	LogGl(1,"Cleaning up the OpenGL graphics backend.");
	
	//TODO(delle) save pipelines in GL4.1 (glGetProgramBinary)
	//TODO(delle) save graphics settings to config
	glFinish();
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_device
//!ref: https://www.khronos.org/opengl/wiki/OpenGL_Context


GraphicsDeviceInfo*
graphics_device_infos(){
	GraphicsDeviceInfo* result = 0;
	
	if(opengl_context){
		array_init(result, 1, deshi_temp_allocator);
		
		GraphicsDeviceInfo* info = array_push(result);
		CopyMemory(info->api_name, "OpenGL", 6);
		
		int major = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		info->api_version_major = (u8)major;
		
		int minor = 0;
		glGetIntegerv(GL_MINOR_VERSION, &minor);
		info->api_version_minor = (u16)minor;
		
		const char* renderer = (const char*)glGetString(GL_RENDERER);
		if(renderer	){
			CopyMemory(info->device_name, renderer, Min(103, strlen(renderer)));
		}
	}
	
	return result;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_buffer
//!ref: https://www.khronos.org/opengl/wiki/Buffer_Object


GraphicsBuffer*
graphics_buffer_create(void* data, u64 requested_size, GraphicsBufferUsage usage, GraphicsMemoryPropertyFlags properties, GraphicsMemoryMappingBehavoir mapping){DPZoneScoped;
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return 0;
	}
	if(requested_size == 0){
		LogEGl("Must not be called with zero size.");
		return 0;
	}
	if(HasFlag(properties,GraphicsMemoryProperty_HostStreamed) && HasFlag(properties,GraphicsMemoryPropertyFlag_LazilyAllocated)){
		LogEGl("The flags GraphicsMemoryPropertyFlag_HostVisible and GraphicsMemoryPropertyFlag_LazilyAllocated are incompatible.");
		return 0;
	}
	if(HasFlag(properties,GraphicsMemoryPropertyFlag_HostCoherent) && mapping != GraphicsMemoryMapping_Persistent){
		LogEGl("OpenGL requires that GraphicsMemoryPropertyFlag_HostCoherent must use the GraphicsMemoryMapping_Persistent mapping.");
		return 0;
	}
	LogGl(1,"Creating a buffer with ",requested_size," bytes.");
	
	//create the buffer
	GLuint buffer_handle;
	glGenBuffers(1, &buffer_handle);
	if(opengl_error){
		return 0;
	}
	glBindBuffer(OPENGL_GRAPHICS_BUFFER_TARGET, buffer_handle);
	if(opengl_error){
		glDeleteBuffers(1, &buffer_handle);
		return 0;
	}
	
	//allocate and upload data to the buffer memory
	void* mapped_data = 0;
	u64 mapped_size = 0;
	u64 gl_buffer_data_usage_hints = 0;
	if(GL_VERSION_TEST(4,4, glBufferStorage)){
		GLbitfield storage_flags = 0;
		if(mapping == GraphicsMemoryMapping_Never){
			if(HasFlag(properties, GraphicsMemoryPropertyFlag_HostVisible|GraphicsMemoryPropertyFlag_HostCoherent|GraphicsMemoryPropertyFlag_HostCached)){
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_Never and GraphicsMemoryPropertyFlag_HostVisible or GraphicsMemoryPropertyFlag_HostCoherent or GraphicsMemoryPropertyFlag_HostCached.");
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}else if(mapping == GraphicsMemoryMapping_Occasional){
			if(HasFlag(properties, GraphicsMemoryPropertyFlag_HostVisible) && !HasFlag(properties,GraphicsMemoryPropertyFlag_HostCoherent)){
				storage_flags = GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT|GL_CLIENT_STORAGE_BIT; 
			}else{
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_MapWriteUnmap and not GraphicsMemoryPropertyFlag_HostVisible.");
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}else if(mapping == GraphicsMemoryMapping_Persistent){
			if(HasFlag(properties, GraphicsMemoryPropertyFlag_HostVisible)){
				storage_flags = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT;
			}else if(HasFlag(properties, GraphicsMemoryPropertyFlag_HostCoherent)){
				storage_flags = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT|GL_CLIENT_STORAGE_BIT;
			}else{
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_Persistent and not GraphicsMemoryPropertyFlag_HostVisible or GraphicsMemoryPropertyFlag_HostCoherent.");
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}
		
		glBufferStorage(OPENGL_GRAPHICS_BUFFER_TARGET, requested_size, data, storage_flags);
		if(opengl_error){
			glDeleteBuffers(1, &buffer_handle);
			return 0;
		}
	}else{
		//TODO(delle) better buffer usage determination
		if(mapping == GraphicsMemoryMapping_Never){
			gl_buffer_data_usage_hints = GL_STATIC_DRAW;
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, requested_size, data, GL_STATIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}else if(mapping == GraphicsMemoryMapping_Persistent){
			//NOTE(delle) GL_MAP_PERSISTENT_BIT requires OpenGL4.4 so we have to simulate it in graphics_update
			gl_buffer_data_usage_hints = GL_DYNAMIC_DRAW;
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, requested_size, data, GL_DYNAMIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
			
			if(HasFlag(properties, GraphicsMemoryPropertyFlag_HostCoherent)){
				mapped_size = requested_size;
				mapped_data = glMapBufferRange(OPENGL_GRAPHICS_BUFFER_TARGET, 0, requested_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);
				if(opengl_error){
					glDeleteBuffers(1, &buffer_handle);
					return 0;
				}
			}else if(HasFlag(properties, GraphicsMemoryPropertyFlag_HostVisible)){
				mapped_size = requested_size;
				mapped_data = glMapBufferRange(OPENGL_GRAPHICS_BUFFER_TARGET, 0, requested_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_FLUSH_EXPLICIT_BIT);
				if(opengl_error){
					glDeleteBuffers(1, &buffer_handle);
					return 0;
				}
			}else{
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_Persistent and not GraphicsMemoryPropertyFlag_HostVisible or GraphicsMemoryPropertyFlag_HostCoherent.");
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}else if(HasFlag(usage, GraphicsBufferUsage_UniformBuffer)){
			gl_buffer_data_usage_hints = GL_STREAM_DRAW;
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, requested_size, data, GL_STREAM_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}else if(HasFlag(usage, GraphicsBufferUsage_VertexBuffer)){
			gl_buffer_data_usage_hints = GL_STATIC_DRAW;
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, requested_size, data, GL_STATIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}else{
			gl_buffer_data_usage_hints = GL_DYNAMIC_DRAW;
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, requested_size, data, GL_DYNAMIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}
	}
	
	GLuint vao_handle = 0;
	if(HasFlag(usage, GraphicsBufferUsage_VertexBuffer)){
		glGenVertexArrays(1, &vao_handle);
		if(opengl_error){
			return 0;
		}
	}
	
	GraphicsBuffer* result = memory_pool_push(g_graphics->pools.buffers);
	result->__internal.size = requested_size;
	result->__internal.usage = usage;
	result->__internal.memory_properties = properties;
	result->__internal.mapping_behavior = mapping;
	result->__internal.mapped.data = mapped_data;
	result->__internal.mapped.size = mapped_size;
	result->__internal.buffer_handle = (void*)(u64)buffer_handle;
	result->__internal.memory_handle = (void*)gl_buffer_data_usage_hints;
	result->__internal.vao_handle = (u32)vao_handle;
	return result;
}

void
graphics_buffer_destroy(GraphicsBuffer* buffer){DPZoneScoped;
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return;
	}
	if(!buffer){
		LogEGl("The input buffer was null.");
		return;
	}
	if(!buffer->__internal.buffer_handle){
		LogEGl("The input buffer[",buffer->debug_name,"] was not properly created with graphics_buffer_create() or was previously deleted.");
		return;
	}
	LogGl(1,"Destroying a buffer[",buffer->debug_name,"] with ",buffer->__internal.size," bytes.");
	
	//unmap before deletion
	if(buffer->__internal.mapped.data){
		glUnmapBuffer(OPENGL_GRAPHICS_BUFFER_TARGET);
	}
	
	//clear the buffer
	GLuint buffer_handle = (GLuint)(u64)buffer->__internal.buffer_handle;
	if(GL_VERSION_TEST(4,3, glInvalidateBufferData)){
		glInvalidateBufferData(buffer_handle);
	}else{
		glBindBuffer(OPENGL_GRAPHICS_BUFFER_TARGET, buffer_handle);
		glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, (GLsizeiptr)buffer->__internal.size, 0, (GLenum)(u64)buffer->__internal.memory_handle);
	}
	
	//delete the buffer
	glDeleteBuffers(1, &buffer_handle);
	if(opengl_error){
		return;
	}
	
	if(HasFlag(buffer->__internal.usage, GraphicsBufferUsage_VertexBuffer)){
		GLuint vao_handles[1] = { (GLuint)buffer->__internal.vao_handle };
		glDeleteVertexArrays(1, vao_handles);
		if(opengl_error){
			return;
		}
	}
	
	memory_pool_delete(g_graphics->pools.buffers, buffer);
}


void
graphics_buffer_reallocate(GraphicsBuffer* buffer, u64 new_size){DPZoneScoped;
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return;
	}
	if(!buffer){
		LogEGl("The input buffer was null.");
		return;
	}
	if(new_size == 0){
		LogEGl("Must not be called with zero size.");
		return;
	}
	if(!buffer->__internal.buffer_handle){
		LogEGl("The input buffer[",buffer->debug_name,"] was not properly created with graphics_buffer_create() or was previously deleted.");
		return;
	}
	if(new_size < buffer->__internal.size){
		return;
	}
	LogGl(1,"Reallocating a buffer[",buffer->debug_name,"] with ",buffer->__internal.size," bytes to have ",new_size," bytes.");
	
	//unmap the old buffer before copy/deletion
	if(buffer->__internal.mapped.data){
		glUnmapBuffer(OPENGL_GRAPHICS_BUFFER_TARGET);
	}
	
	//bind the old buffer
	GLuint old_buffer_handle = (GLuint)(u64)buffer->__internal.buffer_handle;
	glBindBuffer(GL_COPY_READ_BUFFER, old_buffer_handle);
	if(opengl_error){
		return;
	}
	
	//get the old buffer size
	GLint64 buffer_size_s64;
	glGetBufferParameteri64v(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &buffer_size_s64);
	GLsizeiptr old_buffer_size = (GLsizeiptr)buffer_size_s64;
	
	//create a new buffer
	GLuint new_buffer_handle;
	glGenBuffers(1, &new_buffer_handle);
	if(opengl_error){
		return;
	}
	glBindBuffer(OPENGL_GRAPHICS_BUFFER_TARGET, new_buffer_handle);
	if(opengl_error){
		glDeleteBuffers(1, &new_buffer_handle);
		return;
	}
	
	//allocate the new buffer's memory
	u64 mapped_size = 0;
	void* mapped_data = 0;
	if(GL_VERSION_TEST(4,4, glBufferStorage)){
		GLbitfield storage_flags = 0;
		if(buffer->__internal.mapping_behavior == GraphicsMemoryMapping_Never){
			if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostVisible|GraphicsMemoryPropertyFlag_HostCoherent|GraphicsMemoryPropertyFlag_HostCached)){
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_Never and GraphicsMemoryPropertyFlag_HostVisible or GraphicsMemoryPropertyFlag_HostCoherent or GraphicsMemoryPropertyFlag_HostCached.");
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(buffer->__internal.mapping_behavior == GraphicsMemoryMapping_Occasional){
			if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostVisible) && !HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostCoherent)){
				storage_flags = GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT|GL_CLIENT_STORAGE_BIT; 
			}else{
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_MapWriteUnmap and not GraphicsMemoryPropertyFlag_HostVisible.");
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(buffer->__internal.mapping_behavior == GraphicsMemoryMapping_Persistent){
			if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostVisible)){
				storage_flags = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT;
			}else if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostCoherent)){
				storage_flags = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT|GL_CLIENT_STORAGE_BIT;
			}else{
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_Persistent and not GraphicsMemoryPropertyFlag_HostVisible or GraphicsMemoryPropertyFlag_HostCoherent.");
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}
		
		glBufferStorage(OPENGL_GRAPHICS_BUFFER_TARGET, new_size, 0, storage_flags);
		if(opengl_error){
			glDeleteBuffers(1, &new_buffer_handle);
			return;
		}
	}else{
		//TODO(delle) better buffer usage determination
		if(buffer->__internal.mapping_behavior == GraphicsMemoryMapping_Never){
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, new_size, 0, GL_STATIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(buffer->__internal.mapping_behavior == GraphicsMemoryMapping_Persistent){
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, new_size, 0, GL_DYNAMIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(HasFlag(buffer->__internal.usage, GraphicsBufferUsage_UniformBuffer)){
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, new_size, 0, GL_STREAM_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else{
			glBufferData(OPENGL_GRAPHICS_BUFFER_TARGET, new_size, 0, GL_DYNAMIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}
	}
	
	//copy the old buffer's memory to the new buffer's memory
	glCopyBufferSubData(GL_COPY_READ_BUFFER, OPENGL_GRAPHICS_BUFFER_TARGET, 0, 0, old_buffer_size);
	if(buffer->__internal.mapping_behavior == GraphicsMemoryMapping_Persistent){
		//NOTE(delle) GL_MAP_PERSISTENT_BIT requires OpenGL4.4 so we have to simulate it in graphics_update
		glUnmapBuffer(GL_COPY_READ_BUFFER);
		
		if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostCoherent)){
			mapped_size = new_size;
			mapped_data = glMapBufferRange(OPENGL_GRAPHICS_BUFFER_TARGET, 0, new_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostVisible)){
			mapped_size = new_size;
			mapped_data = glMapBufferRange(OPENGL_GRAPHICS_BUFFER_TARGET, 0, new_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_FLUSH_EXPLICIT_BIT);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else{
			LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_Persistent and not GraphicsMemoryPropertyFlag_HostVisible or GraphicsMemoryPropertyFlag_HostCoherent.");
			glDeleteBuffers(1, &new_buffer_handle);
			return;
		}
	}
	
	//delete the old buffer
	if(GL_VERSION_TEST(4,3, glInvalidateBufferData)){
		glInvalidateBufferData(old_buffer_handle);
	}else{
		glBufferData(GL_COPY_READ_BUFFER, (GLsizeiptr)buffer->__internal.size, 0, (GLenum)(u64)buffer->__internal.memory_handle);
	}
	glDeleteBuffers(1, &old_buffer_handle);
	
	//update the GraphicsBuffer
	buffer->__internal.size = new_size;
	buffer->__internal.mapped.data = mapped_data;
	buffer->__internal.mapped.size = mapped_size;
	buffer->__internal.buffer_handle = (void*)(u64)new_buffer_handle;
}


void*
graphics_buffer_map(GraphicsBuffer* buffer, u64 size, u64 offset){DPZoneScoped;
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return 0;
	}
	if(!buffer){
		LogEGl("The input buffer was null.");
		return 0;
	}
	if(!buffer->__internal.buffer_handle){
		LogEGl("The input buffer[",buffer->debug_name,"] was not properly created with graphics_buffer_create() or was previously deleted.");
		return 0;
	}
	if(buffer->__internal.mapping_behavior != GraphicsMemoryMapping_Occasional){
		LogEGl("The input buffer[",buffer->debug_name,"] cannot be mapped in the middle of its lifetime since it does not have the mapping GraphicsMemoryMapping_Occasional.");
		return 0;
	}
	if(buffer->__internal.mapped.data){
		if(buffer->__internal.mapping_behavior == GraphicsMemoryMapping_Persistent){
			LogWGl("Cannot map the persistently mapped input buffer[",buffer->debug_name,"] since it's always actively mapped.");
		}else{
			LogWGl("Cannot map the input buffer[",buffer->debug_name,"] since it's already mapped.");
		}
		return 0;
	}
	LogGl(1,"Mapping ",size," bytes at the ",offset," offset into a buffer[",buffer->debug_name,"].");
	
	glBindBuffer(OPENGL_GRAPHICS_BUFFER_TARGET, (GLuint)(u64)buffer->__internal.buffer_handle);
	if(opengl_error){
		return 0;
	}
	
	buffer->__internal.mapped.data = glMapBufferRange(OPENGL_GRAPHICS_BUFFER_TARGET, (GLintptr)offset, (GLsizeiptr)size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_FLUSH_EXPLICIT_BIT);
	if(opengl_error){
		return 0;
	}
	
	buffer->__internal.mapped.offset = offset;
	buffer->__internal.mapped.size = size;
	return buffer->__internal.mapped.data;
}


void
graphics_buffer_unmap(GraphicsBuffer* buffer, b32 flush){DPZoneScoped;
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return;
	}
	if(!buffer){
		LogEGl("The input buffer was null.");
		return;
	}
	if(!buffer->__internal.buffer_handle){
		LogEGl("The input buffer[",buffer->debug_name,"] was not properly created with graphics_buffer_create() or was previously deleted.");
		return;
	}
	if(buffer->__internal.mapping_behavior != GraphicsMemoryMapping_Occasional){
		LogEGl("Cannot unmap a buffer[",buffer->debug_name,"] in the middle of its lifetime since it does not have the mapping GraphicsMemoryMapping_Occasional.");
		return;
	}
	if(!buffer->__internal.mapped.data){
		if(buffer->__internal.mapping_behavior == GraphicsMemoryMapping_Persistent){
			LogWGl("Cannot unmap a persistently mapped buffer[",buffer->debug_name,"] since it's always actively mapped.");
		}else{
			LogWGl("The input buffer[",buffer->debug_name,"] is not actively mapped.");
		}
		return;
	}
	LogGl(1,"Unmapping",(flush ? " and flushing" : "")," a buffer[",buffer->debug_name,"].");
	
	glBindBuffer(OPENGL_GRAPHICS_BUFFER_TARGET, (GLuint)(u64)buffer->__internal.buffer_handle);
	if(opengl_error){
		return;
	}
	
	if(flush){
		glFlushMappedBufferRange(OPENGL_GRAPHICS_BUFFER_TARGET, (GLintptr)buffer->__internal.mapped.offset, (GLsizeiptr)buffer->__internal.mapped.size);
		if(opengl_error){
			return;
		}
	}
	
	glUnmapBuffer(OPENGL_GRAPHICS_BUFFER_TARGET);
	if(opengl_error){
		return;
	}
	
	buffer->__internal.mapped.data = 0;
	buffer->__internal.mapped.offset = 0;
	buffer->__internal.mapped.size = 0;
}


void
graphics_buffer_flush(GraphicsBuffer* buffer){DPZoneScoped;
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return;
	}
	if(!buffer){
		LogEGl("The input buffer was null.");
		return;
	}
	if(!buffer || !buffer->__internal.buffer_handle){
		LogEGl("The input buffer[",buffer->debug_name,"] was not properly created with graphics_buffer_create() or was previously deleted.");
		return;
	}
	if(!buffer->__internal.mapped.data){
		LogWGl("The input buffer[",buffer->debug_name,"] is not actively mapped.");
		return;
	}
	LogGl(1,"Flushing a buffer[",buffer->debug_name,"].");
	
	glBindBuffer(OPENGL_GRAPHICS_BUFFER_TARGET, (GLuint)(u64)buffer->__internal.buffer_handle);
	if(opengl_error){
		return;
	}
	
	glFlushMappedBufferRange(OPENGL_GRAPHICS_BUFFER_TARGET, (GLintptr)buffer->__internal.mapped.offset, (GLsizeiptr)buffer->__internal.mapped.size);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_image


void
graphics_image_update(GraphicsImage* image){DPZoneScoped;
	if(!g_graphics->initialized){
		LogEGl("Must not be called before graphics is init.");
		return;
	}
	if(!image){
		LogEGl("The input image was null.");
		return;
	}
	LogGl(1,"Updating an image[",image->debug_name,"].");
	
	//map the sample count
	GLsizei samples = 1;
	switch(image->samples){
		case GraphicsSampleCount_1: samples = 1; break;
		case GraphicsSampleCount_2: samples = 2; break;
		case GraphicsSampleCount_4: samples = 4; break;
		case GraphicsSampleCount_8: samples = 8; break;
		case GraphicsSampleCount_16: samples = 16; break;
		case GraphicsSampleCount_32: samples = 32; break;
		case GraphicsSampleCount_64: samples = 64; break;
	}
	
	//map the texture type
	GLenum target = GL_TEXTURE_2D;
	if(samples > 1){
		target = GL_TEXTURE_2D_MULTISAMPLE;
		
		/* //TODO(delle) texture types
		if(image->type == GraphicsImageType_2D){
			target = GL_TEXTURE_2D_MULTISAMPLE;
		}else if(image->type == GraphicsImageType_2D_Array){
			target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		}else{
			LogEGl("Invalid multisample graphics image type specified: ",image->type,".");
			return;
		}
		*/
	}else{
		/* //TODO(delle) texture types
		switch(image->type){
			case GraphicsImageType_1D: target = GL_TEXTURE_1D; break;
			case GraphicsImageType_2D: target = GL_TEXTURE_2D; break;
			case GraphicsImageType_3D: target = GL_TEXTURE_3D; break;
			case GraphicsImageType_1D_Array: target = GL_TEXTURE_1D_ARRAY; break;
			case GraphicsImageType_2D_Array: target = GL_TEXTURE_2D_ARRAY; break;
			default: LogEGl("Unhandled graphics image type: ",image->type,"."); return;
		}
		*/
	}
	
	//map the texture format
	GLint color_format = GL_RGBA8;
	GLenum pixel_format = GL_RGBA;
	GLenum pixel_type = GL_UNSIGNED_BYTE;
	switch(image->format){
		case GraphicsFormat_R32G32_Float:{
			color_format = GL_RG32F;
			pixel_format = GL_RG;
			pixel_type = GL_FLOAT;
		}break;
		case GraphicsFormat_R32G32B32_Float:{
			color_format = GL_RGB32F;
			pixel_format = GL_RGB;
			pixel_type = GL_FLOAT;
		}break;
		case GraphicsFormat_R8G8B8_SRGB:{
			color_format = GL_SRGB8;
			pixel_format = GL_RGB;
			pixel_type = GL_UNSIGNED_BYTE;
		}break;
		case GraphicsFormat_R8G8B8_UNorm:{
			color_format = GL_RGB8;
			pixel_format = GL_RGB;
			pixel_type = GL_UNSIGNED_BYTE;
		}break;
		case GraphicsFormat_R8G8B8A8_SRGB:{
			color_format = GL_SRGB8_ALPHA8;
			pixel_format = GL_RGBA;
			pixel_type = GL_UNSIGNED_BYTE;
		}break;
		case GraphicsFormat_R8G8B8A8_UNorm:{
			color_format = GL_RGBA8;
			pixel_format = GL_RGBA;
			pixel_type = GL_UNSIGNED_BYTE;
		}break;
		case GraphicsFormat_B8G8R8A8_UNorm:{
			color_format = GL_BGRA;
			pixel_format = GL_BGRA;
			pixel_type = GL_UNSIGNED_BYTE;
		}break;
		case GraphicsFormat_Depth16_UNorm:{
			color_format = GL_DEPTH_COMPONENT16;
			pixel_format = GL_DEPTH_COMPONENT;
			pixel_type = GL_UNSIGNED_SHORT;
		}break;
		case GraphicsFormat_Depth32_Float:{
			color_format = GL_DEPTH_COMPONENT32F;
			pixel_format = GL_DEPTH_COMPONENT;
			pixel_type = GL_FLOAT;
		}break;
		case GraphicsFormat_Depth32_Float_Stencil8_UInt:{
			color_format = GL_DEPTH32F_STENCIL8;
			pixel_format = GL_DEPTH_STENCIL;
			pixel_type = GL_DEPTH32F_STENCIL8;
		}break;
		case GraphicsFormat_Depth24_UNorm_Stencil8_UInt:{
			color_format = GL_DEPTH24_STENCIL8;
			pixel_format = GL_DEPTH_STENCIL;
			pixel_type = GL_DEPTH24_STENCIL8;
		}break;
		default:{
			LogEGl("Unhandled graphics texture format: ",image->format,".");
		}return;
	}
	
	//delete the previous texture
	if(image->__internal.handle){
		GLuint previous_handle = (GLuint)(u64)image->__internal.handle;
		glDeleteTextures(1, &previous_handle);
		if(opengl_error){
			return;
		}
		
		image->__internal.handle = 0;
		image->__internal.memory_handle = 0;
	}
	
	//create a new texture
	GLuint handle;
	glGenTextures(1, &handle);
	if(opengl_error){
		return;
	}
	glBindTexture(target, handle);
	if(opengl_error){
		glDeleteTextures(1, &handle);
		return;
	}
	
	//define the texture
	GLsizei width = (GLsizei)image->extent.x;
	GLsizei height = (GLsizei)image->extent.y;
	GLsizei depth = (GLsizei)image->extent.z;
	switch(target){
		case GL_TEXTURE_1D:
		case GL_PROXY_TEXTURE_1D:
		{
			glTexImage1D(target, 0, color_format, width, 0, pixel_format, pixel_type, 0);
		}break;
		case GL_TEXTURE_2D:
		case GL_PROXY_TEXTURE_2D:
		case GL_TEXTURE_1D_ARRAY:
		case GL_PROXY_TEXTURE_1D_ARRAY:
		case GL_TEXTURE_RECTANGLE:
		case GL_PROXY_TEXTURE_RECTANGLE:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		case GL_PROXY_TEXTURE_CUBE_MAP:
		{
			glTexImage2D(target, 0, color_format, width, height, 0, pixel_format, pixel_type, 0);
		}break;
		case GL_TEXTURE_2D_MULTISAMPLE:
		case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
		{
			glTexImage2DMultisample(target, samples, color_format, width, height, GL_FALSE);
		}break;
		case GL_TEXTURE_3D:
		case GL_PROXY_TEXTURE_3D:
		case GL_TEXTURE_2D_ARRAY:
		case GL_PROXY_TEXTURE_2D_ARRAY:
		{
			glTexImage3D(target, 0, color_format, width, height, depth, 0, pixel_format, pixel_type, 0);
		}break;
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
		{
			glTexImage3DMultisample(target, samples, color_format, width, height, depth, GL_FALSE);
		}break;
		default:
		{
			LogEGl("Unhandled texture target: ",target,".");
			glDeleteTextures(1, &handle);
		}return;
	}
	if(opengl_error){
		glDeleteTextures(1, &handle);
		return;
	}
	
	//update the GraphicsImage
	image->__internal.handle = (void*)(u64)handle;
	image->__internal.memory_handle = (void*)(u64)target;
}


void
graphics_image_destroy(GraphicsImage* image){DPZoneScoped;
	if(!g_graphics->pools.images){
		LogEGl("Must not be called before g_graphics->pools.images is init.");
		return;
	}
	if(!image){
		LogEGl("The input image was null.");
		return;
	}
	if(!image->__internal.handle){
		LogEGl("The input image[",image->debug_name,"] was not properly initialized with graphics_image_update() or was previously deleted.");
		return;
	}
	LogGl(1,"Destroying an image[",image->debug_name,"].");
	
	//delete the texture
	GLuint texture_handle = (GLuint)(u64)image->__internal.handle;
	glDeleteTextures(1, &texture_handle);
	if(opengl_error){
		return;
	}
	
	memory_pool_delete(g_graphics->pools.images, image);
}


void
graphics_image_write(GraphicsImage* image, u8* pixels, vec2i offset, vec2i extent){DPZoneScoped;
	if(!g_graphics->initialized){
		LogEGl("Must not be called before graphics is init.");
		return;
	}
	if(!image){
		LogEGl("The input image was null.");
		return;
	}
	if(!image->__internal.handle){
		LogEGl("The input image[",image->debug_name,"] was not properly initialized with graphics_image_update() or was previously deleted.");
		return;
	}
	LogGl(1,"Writing an image[",image->debug_name,"].");
	
	//bind the texture
	GLenum texture_target = (GLenum)(u64)image->__internal.memory_handle;
	GLuint texture_handle = (GLuint)(u64)image->__internal.handle;
	glBindTexture(texture_target, texture_handle);
	if(opengl_error){
		return;
	}
	
	//map the image format
	//TODO(delle) image formats
	GLint image_format = GL_RGBA;
	
	//upload texture data
	switch(texture_target){
		case GL_TEXTURE_1D:
		{
			glTexSubImage1D(texture_target, 0, (GLint)offset.x, (GLsizei)extent.x, image_format, GL_UNSIGNED_BYTE, pixels);
		}break;
		case GL_TEXTURE_2D:
		case GL_TEXTURE_1D_ARRAY:
		case GL_TEXTURE_RECTANGLE:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		{
			glTexSubImage2D(texture_target, 0, (GLint)offset.x, (GLint)offset.y, (GLsizei)extent.x, (GLsizei)extent.y, image_format, GL_UNSIGNED_BYTE, pixels);
		}break;
		case GL_TEXTURE_3D:
		case GL_TEXTURE_2D_ARRAY:
		{
			NotImplemented;
			//TODO(delle) change func args to vec3i
			//glTexSubImage3D(texture_target, 0, (GLint)offset.x, (GLint)offset.y, (GLint)offset.z, (GLsizei)extent.x, (GLsizei)extent.y, (GLsizei)extent.z, image_format, GL_UNSIGNED_BYTE, pixels);
		}break;
		default:
		{
			LogEGl("Graphics texture target can not be written to: ",texture_target,".");
		}return;
	}
	if(opengl_error){
		return;
	}
	
	//TODO(delle) mipmaps
}


void
graphics_image_view_update(GraphicsImageView* image_view){DPZoneScoped;
	if(!g_graphics->initialized){
		LogEGl("Must not be called before graphics is init.");
		return;
	}
	if(!image_view){
		LogEGl("The input image view was null.");
		return;
	}
	
	if(GL_VERSION_TEST(4,3, glTextureView)){
		if(!image_view->image){
			LogEGl("The input image view[",image_view->debug_name,"] has a null image.");
			return;
		}
		if(!image_view->image->__internal.handle){
			LogEGl("The input image view[",image_view->debug_name,"] has an image[",image_view->image->debug_name,"] which was not properly initialized with graphics_image_update() or was previously destroyed.");
			return;
		}
		LogGl(1,"Updating an image view[",image_view->debug_name,"].");
		
		//TODO(delle) glTextureView
	}
}


void
graphics_image_view_destroy(GraphicsImageView* image_view){DPZoneScoped;
	if(!g_graphics->pools.image_views){
		LogEGl("Must not be called before g_graphics->pools.image_views is init.");
		return;
	}
	if(!image_view){
		LogEGl("The input image view was null.");
		return;
	}
	LogGl(1,"Destroying an image view[",image_view->debug_name,"].");
	
	if(GL_VERSION_TEST(4,3, glTextureView)){
		if(!image_view->__internal.handle){
			LogEGl("The input image view was not properly initialized with graphics_image_view_update() or was previously destroyed.");
			return;
		}
		
		//TODO(delle) glTextureView
	}
	
	memory_pool_delete(g_graphics->pools.image_views, image_view);
}


void
graphics_sampler_update(GraphicsSampler* sampler){DPZoneScoped;
	if(!g_graphics->initialized){
		LogEGl("Must not be called before graphics is init.");
		return;
	}
	if(!sampler){
		LogEGl("The input sampler was null.");
		return;
	}
	LogGl(1,"Updating a sampler[",sampler->debug_name,"].");
	
	//delete the previous sampler
	if(sampler->__internal.handle){
		GLuint previous_sampler_handle = (GLuint)(u64)sampler->__internal.handle;
		glDeleteSamplers(1, &previous_sampler_handle);
		if(opengl_error){
			return;
		}
		
		sampler->__internal.handle = 0;
	}
	
	//create a new sampler
	GLuint sampler_handle;
	glGenSamplers(1, &sampler_handle);
	if(opengl_error){
		return;
	}
	
	//map the magnifying and minifying filters
	switch(sampler->mag_filter){
		case GraphicsFilter_Nearest:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}break;
		case GraphicsFilter_Linear:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}break;
		default:{
			LogEGl("Unhandled GraphicsFilter when trying to map to a GL_TEXTURE_MAG_FILTER paramater: ",sampler->mag_filter,".");
			glDeleteSamplers(1, &sampler_handle);
		}return;
	}
	if(opengl_error){
		glDeleteSamplers(1, &sampler_handle);
		return;
	}
	
	switch(sampler->min_filter){
		case GraphicsFilter_Nearest:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}break;
		case GraphicsFilter_Linear:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}break;
		/* //TODO(delle) mipmaps
		case GraphicsFilter_NearestMipmapNearest:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}break;
		case GraphicsFilter_LinearMipmapNearest:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		}break;
		case GraphicsFilter_NearestMipmapLinear:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		}break;
		case GraphicsFilter_LinearMipmapLinear:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}break;
*/
		default:{
			LogEGl("Unhandled GraphicsFilter when trying to map to a GL_TEXTURE_MIN_FILTER paramater: ",sampler->min_filter,".");
			glDeleteSamplers(1, &sampler_handle);
		}return;
	}
	if(opengl_error){
		glDeleteSamplers(1, &sampler_handle);
		return;
	}
	
	//map the texture wrapping
	switch(sampler->address_mode_u){
		case GraphicsSamplerAddressMode_Repeat:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}break;
		case GraphicsSamplerAddressMode_Mirrored_Repeat:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		}break;
		case GraphicsSamplerAddressMode_Clamp_To_Edge:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_TO_EDGE);
		}break;
		case GraphicsSamplerAddressMode_Clamp_To_Border:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		}break;
		default:{
			LogEGl("Unhandled GraphicsSamplerAddressMode when trying to map to a GL_TEXTURE_WRAP_S paramater: ",sampler->address_mode_u,".");
			glDeleteSamplers(1, &sampler_handle);
		}return;
	}
	if(opengl_error){
		glDeleteSamplers(1, &sampler_handle);
		return;
	}
	
	switch(sampler->address_mode_v){
		case GraphicsSamplerAddressMode_Repeat:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}break;
		case GraphicsSamplerAddressMode_Mirrored_Repeat:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		}break;
		case GraphicsSamplerAddressMode_Clamp_To_Edge:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_T, GL_MIRROR_CLAMP_TO_EDGE);
		}break;
		case GraphicsSamplerAddressMode_Clamp_To_Border:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		}break;
		default:{
			LogEGl("Unhandled GraphicsSamplerAddressMode when trying to map to a GL_TEXTURE_WRAP_T paramater: ",sampler->address_mode_v,".");
			glDeleteSamplers(1, &sampler_handle);
		}return;
	}
	if(opengl_error){
		glDeleteSamplers(1, &sampler_handle);
		return;
	}
	
	switch(sampler->address_mode_w){
		case GraphicsSamplerAddressMode_Repeat:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_R, GL_REPEAT);
		}break;
		case GraphicsSamplerAddressMode_Mirrored_Repeat:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
		}break;
		case GraphicsSamplerAddressMode_Clamp_To_Edge:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_R, GL_MIRROR_CLAMP_TO_EDGE);
		}break;
		case GraphicsSamplerAddressMode_Clamp_To_Border:{
			glSamplerParameteri(sampler_handle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		}break;
		default:{
			LogEGl("Unhandled GraphicsSamplerAddressMode when trying to map to a GL_TEXTURE_WRAP_R paramater: ",sampler->address_mode_w,".");
			glDeleteSamplers(1, &sampler_handle);
		}return;
	}
	if(opengl_error){
		glDeleteSamplers(1, &sampler_handle);
		return;
	}
	
	//set the border color when texture wrapping is set to GL_CLAMP_TO_BORDER
	GLfloat border_color[4];
	border_color[0] = (GLfloat)sampler->border_color.r / 255.0f;
	border_color[1] = (GLfloat)sampler->border_color.g / 255.0f;
	border_color[2] = (GLfloat)sampler->border_color.b / 255.0f;
	border_color[3] = (GLfloat)sampler->border_color.a / 255.0f;
	glSamplerParameterfv(sampler_handle, GL_TEXTURE_BORDER_COLOR, border_color);
	if(opengl_error){
		glDeleteSamplers(1, &sampler_handle);
	}
	
	//TODO(delle) anistropic filtering
	
	sampler->__internal.handle = (void*)(u64)sampler_handle;
}


void
graphics_sampler_destroy(GraphicsSampler* sampler){DPZoneScoped;
	if(!g_graphics->pools.samplers){
		LogEGl("Must not be called before g_graphics->pools.samplers is init.");
		return;
	}
	if(!sampler){
		LogEGl("The input sampler was null.");
		return;
	}
	if(!sampler->__internal.handle){
		LogEGl("The input sampler[",sampler->debug_name,"] was not properly initialized with graphics_sampler_update() or was previously deleted.");
		return;
	}
	LogGl(1,"Destroying a sampler[",sampler->debug_name,"].");
	
	GLuint sampler_handle = (GLuint)(u64)sampler->__internal.handle;
	glDeleteSamplers(1, &sampler_handle);
	if(opengl_error){
		return;
	}
	
	memory_pool_delete(g_graphics->pools.samplers, sampler);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_descriptor


void
graphics_descriptor_set_layout_update(GraphicsDescriptorSetLayout* descriptor_set_layout){DPZoneScoped;
	//do nothing
}


void
graphics_descriptor_set_layout_destroy(GraphicsDescriptorSetLayout* descriptor_set_layout){DPZoneScoped;
	if(!g_graphics->pools.descriptor_set_layouts){
		LogEGl("Must not be called before g_graphics->pools.descriptor_set_layout is init.");
		return;
	}
	if(!descriptor_set_layout){
		LogEGl("The input descriptor set layout was null.");
		return;
	}
	LogGl(1,"Destroying a descriptor set layout[",descriptor_set_layout->debug_name,"].");
	
	memory_pool_delete(g_graphics->pools.descriptor_set_layouts, descriptor_set_layout);
}


void
graphics_descriptor_set_update(GraphicsDescriptorSet* descriptor_set){DPZoneScoped;
	//do nothing
}


void
graphics_descriptor_set_destroy(GraphicsDescriptorSet* descriptor_set){DPZoneScoped;
	if(!g_graphics->pools.descriptor_sets){
		LogEGl("Must not be called before g_graphics->pools.descriptor_sets is init.");
		return;
	}
	if(!descriptor_set){
		LogEGl("The input descriptor set was null.");
		return;
	}
	LogGl(1,"Destroying a descriptor set[",descriptor_set->debug_name,"].");
	
	memory_pool_delete(g_graphics->pools.descriptor_sets, descriptor_set);
}


void
graphics_descriptor_set_write(GraphicsDescriptorSet* descriptor_set){DPZoneScoped;
	//do nothing
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_shader


void
graphics_shader_update(GraphicsShader* shader){DPZoneScoped;
	if(!g_graphics->initialized){
		LogEGl("Must not be called before graphics is init.");
		return;
	}
	if(!shader){
		LogEGl("The input shader was null.");
		return;
	}
	if(!str8_valid(shader->source)){
		LogEGl("The input shader's source was empty.");
		return;
	}
	LogGl(1,"Updating a shader[",shader->debug_name,"].");
	
	//delete the old shader
	//NOTE(delle) the shader will actually be deleted once it's no longer attached to a program
	if(shader->__internal.handle){
		glDeleteShader((GLuint)(u64)shader->__internal.handle);
		if(opengl_error){
			return;
		}
		
		shader->__internal.handle = 0;
	}
	
	//map the shader stage
	GLenum shader_type = 0;
	switch(shader->shader_stage){
		case GraphicsShaderStage_Vertex:{
			shader_type = GL_VERTEX_SHADER;
		}break;
		case GraphicsShaderStage_Geometry:{
			shader_type = GL_GEOMETRY_SHADER;
		}break;
		case GraphicsShaderStage_Fragment:{
			shader_type = GL_FRAGMENT_SHADER;
		}break;
		case GraphicsShaderStage_Compute:{
			if(!GL_VERSION_TEST(4,3, GL_COMPUTE_SHADER)){
				LogEGl("Unhandled shader stage: ",shader->shader_stage,".");
				return;
			}
			shader_type = GL_COMPUTE_SHADER;
		}break;
		default:{
			LogEGl("Unhandled shader stage: ",shader->shader_stage,".");
		}return;
	}
	
	//create the shader
	GLuint shader_handle = glCreateShader(shader_type);
	if(opengl_error || shader_handle == 0){
		return;
	}
	
	//compile the shader
	GLchar* string = (GLchar*)shader->source.str;
	GLint length = shader->source.count;
	glShaderSource(shader_handle, 1, &string, &length);
	glCompileShader(shader_handle);
	glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &opengl_success);
	if(opengl_success != GL_TRUE){
		glGetShaderInfoLog(shader_handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
		LogEGl("Failed to compile a shader[",shader->debug_name,"] because:\n", opengl_infolog);
		glDeleteShader(shader_handle);
		return;
	}
	
	//update the GraphicsShader
	shader->__internal.handle = (void*)(u64)shader_handle;
}


void
graphics_shader_destroy(GraphicsShader* shader){DPZoneScoped;
	if(!g_graphics->pools.shaders){
		LogEGl("Must not be called before g_graphics->pools.shaders is init.");
		return;
	}
	if(!shader){
		LogEGl("The input shader was null.");
		return;
	}
	if(!shader->__internal.handle){
		LogEGl("The input shader was not properly initialized with graphics_shader_update() or was previously destroyed.");
		return;
	}
	LogGl(1,"Destroying a shader[",shader->debug_name,"].");
	
	//NOTE(delle) the shader will actually be deleted once it's no longer attached to a program
	glDeleteShader((GLuint)(u64)shader->__internal.handle);
	if(opengl_error){
		return;
	}
	
	memory_pool_delete(g_graphics->pools.shaders, shader);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_pipeline


void
graphics_pipeline_layout_update(GraphicsPipelineLayout* pipeline_layout){DPZoneScoped;
	//do nothing
}


void
graphics_pipeline_layout_destroy(GraphicsPipelineLayout* pipeline_layout){DPZoneScoped;
	if(!g_graphics->pools.pipeline_layouts){
		LogEGl("Must not be called before g_graphics->pools.pipeline_layouts is init.");
		return;
	}
	if(!pipeline_layout){
		LogEGl("The input pipeline layout was null.");
		return;
	}
	LogGl(1,"Destroying a pipeline layout[",pipeline_layout->debug_name,"].");
	
	memory_pool_delete(g_graphics->pools.pipeline_layouts, pipeline_layout);
}


void
graphics_pipeline_update(GraphicsPipeline* pipeline){DPZoneScoped;
	if(!g_graphics->initialized){
		LogEGl("Must not be called before graphics is init.");
		return;
	}
	if(!pipeline){
		LogEGl("The input pipeline was null.");
		return;
	}
	if(!pipeline->vertex_shader && !pipeline->geometry_shader && !pipeline->fragment_shader){
		LogEGl("The input pipeline was has no shaders specified.");
		return;
	}
	if(!pipeline->layout){
		LogEGl("The input pipeline's layout was null.");
		return;
	}
	LogGl(1,"Updating a pipeline[",pipeline->debug_name,"].");
	
	//delete the old program
	if(pipeline->__internal.handle){
		//NOTE(delle) the program will actually be deleted once it's no longer in use
		GLuint old_program_handle = (GLuint)(u64)(pipeline->__internal.handle);
		glDeleteProgram(old_program_handle);
		if(opengl_error){
			return;
		}
		
		//stop using the program if it's currently in use
		GLint current_program = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
		if((GLuint)current_program == old_program_handle){
			glUseProgram(0);
		}
		
		pipeline->__internal.handle = 0;
	}
	
	//create the program
	GLuint program_handle = glCreateProgram();
	if(opengl_error || program_handle == 0){
		return;
	}
	
	//attach the shaders
	if(pipeline->vertex_shader){
		glAttachShader(program_handle, (GLuint)(u64)pipeline->vertex_shader->__internal.handle);
		if(opengl_error){
			glDeleteProgram(program_handle);
			return;
		}
	}
	if(pipeline->geometry_shader){
		glAttachShader(program_handle, (GLuint)(u64)pipeline->geometry_shader->__internal.handle);
		if(opengl_error){
			glDeleteProgram(program_handle);
			return;
		}
	}
	if(pipeline->fragment_shader){
		glAttachShader(program_handle, (GLuint)(u64)pipeline->fragment_shader->__internal.handle);
		if(opengl_error){
			glDeleteProgram(program_handle);
			return;
		}
	}
	
	//link the program
	glLinkProgram(program_handle);
	glGetProgramiv(program_handle, GL_LINK_STATUS, &opengl_success);
	if(opengl_success != GL_TRUE){
		glGetProgramInfoLog(program_handle, OPENGL_INFOLOG_SIZE, 0, opengl_infolog);
		LogEGl("Failed to link the shaders of a pipeline[",pipeline->debug_name,"] because:\n", opengl_infolog);
		glDeleteProgram(program_handle);
		return;
	}
	
	GLint max_texture_units;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units);
	GLint max_uniform_buffer_bindings;
	glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &max_uniform_buffer_bindings);
	
	//specify descriptors (uniforms)
	u32 max_descriptor_buffer_count = 0;
	u32 max_descriptor_buffer_set_layout_index = 0;
	if(pipeline->layout->descriptor_layouts){
		b32 error_found = false;
		forX_array(descriptor_layout_it, pipeline->layout->descriptor_layouts){
			GraphicsDescriptorSetLayout* layout = *descriptor_layout_it;
			if(layout->bindings){
				u32 descriptor_texture_count = 0;
				u32 descriptor_buffer_count = 0;
				forX_array(binding, layout->bindings){
					switch(binding->type){
						case GraphicsDescriptorType_Uniform_Buffer:{
							if(descriptor_buffer_count >= max_uniform_buffer_bindings){
								LogEGl("A descriptor set layout[",layout->debug_name,"] of the pipeline layout[",pipeline->layout->debug_name,"] attempted to bind to more than GL_MAX_UNIFORM_BUFFER_BINDINGS(",max_uniform_buffer_bindings,") on this device.");
								error_found = true;
							}else{
								descriptor_buffer_count += 1;
							}
						}break;
						
						case GraphicsDescriptorType_Combined_Image_Sampler:{
							if(descriptor_texture_count >= max_texture_units){
								LogEGl("A descriptor set layout[",layout->debug_name,"] of the pipeline layout[",pipeline->layout->debug_name,"] attempted to bind to more than GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS(",max_texture_units,") on this device.");
								error_found = true;
							}else{
								descriptor_texture_count += 1;
							}
						}break;
						
						default:{
							LogEGl("Unhandled GraphicsDescriptorType when updating a pipeline[",pipeline->debug_name,"]: ",binding->type,".");
							error_found = true;
						}break;
					}
				}
				
				if(descriptor_buffer_count > max_descriptor_buffer_count){
					max_descriptor_buffer_count = descriptor_buffer_count;
					max_descriptor_buffer_set_layout_index = (u32)(descriptor_layout_it - pipeline->layout->descriptor_layouts);
				}
			}
		}
		if(error_found){
			glDeleteProgram(program_handle);
			return;
		}
	}
	
	//create push constant buffers
	if(pipeline->layout->push_constants){
		b32 error_found = false;
		for_array(pipeline->layout->push_constants){
			if(!it->name_in_shader || *it->name_in_shader == '\0'){
				LogEGl("Push constant #",it-pipeline->layout->push_constants," of a pipeline[",pipeline->debug_name,"] has an empty source_block_name field which is required for OpenGL.");
				error_found = true;
				continue;
			}
			
			GLuint block_index = glGetUniformBlockIndex(program_handle, (const GLchar*)it->name_in_shader);
			if(block_index == GL_INVALID_INDEX){
				LogEGl("Push constant #",it-pipeline->layout->push_constants," of a pipeline[",pipeline->debug_name,"] has the name '",it->name_in_shader,"' which was not found in any of the shaders.");
				error_found = true;
				continue;
			}
			
			GLuint buffer_handle;
			glGenBuffers(1, &buffer_handle);
			if(opengl_error){
				error_found = true;
				continue;
			}
			glBindBuffer(GL_UNIFORM_BUFFER, buffer_handle);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				error_found = true;
				continue;
			}
			glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)it->size, 0, GL_DYNAMIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				error_found = true;
				continue;
			}
			
			//NOTE(delle) offsetting from the max buffer binding for the push constants to avoid binding collision with regular buffers
			GLuint block_binding = (GLuint)(max_uniform_buffer_bindings - 1 - (it - pipeline->layout->push_constants));
			if(block_binding < max_descriptor_buffer_count){
				LogEGl("The number of descriptor buffers(",max_descriptor_buffer_count,") on the #",it-pipeline->layout->push_constants," descriptor set layout plus the number of push constants(",array_count(pipeline->layout->push_constants),") is greater than GL_MAX_UNIFORM_BUFFER_BINDINGS(",max_uniform_buffer_bindings,") on the current device for a pipeline[",pipeline->debug_name,"].");
				glDeleteBuffers(1, &buffer_handle);
				error_found = true;
				continue;
			}
			
			it->__internal.shader_block_index = (u32)block_index;
			it->__internal.shader_block_binding = (u32)block_binding;
			it->__internal.shader_buffer_handle = (u32)buffer_handle;
		}
		if(error_found){
			for_array(pipeline->layout->push_constants){
				if(it->__internal.shader_buffer_handle){
					GLuint handle = (GLuint)(u64)it->__internal.shader_buffer_handle;
					glDeleteBuffers(1, &handle);
				}
			}
			glDeleteProgram(program_handle);
			return;
		}
	}
	
	//detach the shaders
	if(pipeline->vertex_shader){
		glDetachShader(program_handle, (GLuint)(u64)pipeline->vertex_shader->__internal.handle);
	}
	if(pipeline->geometry_shader){
		glDetachShader(program_handle, (GLuint)(u64)pipeline->geometry_shader->__internal.handle);
	}
	if(pipeline->fragment_shader){
		glDetachShader(program_handle, (GLuint)(u64)pipeline->fragment_shader->__internal.handle);
	}
	
	//update the GraphicsPipeline
	pipeline->__internal.handle = (void*)(u64)program_handle;
}


void
graphics_pipeline_destroy(GraphicsPipeline* pipeline){DPZoneScoped;
	if(!g_graphics->pools.pipelines){
		LogEGl("Must not be called before g_graphics->pools.pipelines is init.");
		return;
	}
	if(!pipeline){
		LogEGl("The input pipeline was null.");
		return;
	}
	if(!pipeline->__internal.handle){
		LogEGl("The input pipeline was not properly initialized with graphics_pipeline_update() or was previously destroyed.");
		return;
	}
	LogGl(1,"Destroying a pipeline[",pipeline->debug_name,"].");
	
	//delete the program
	//NOTE(delle) the program will actually be deleted once it's no longer in use
	GLuint program_handle = (GLuint)(u64)(pipeline->__internal.handle);
	glDeleteProgram(program_handle);
	if(opengl_error){
		return;
	}
	
	//stop using the program if it's currently in use
	GLint current_program = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
	if((GLuint)current_program == program_handle){
		glUseProgram(0);
	}
	
	memory_pool_delete(g_graphics->pools.pipelines, pipeline);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_renderpass


void
graphics_render_pass_update(GraphicsRenderPass* render_pass){DPZoneScoped;
	//do nothing
}


void
graphics_render_pass_destroy(GraphicsRenderPass* render_pass){DPZoneScoped;
	if(!g_graphics->pools.render_passes){
		LogEGl("Must not be called before g_graphics->pools.render_passes is init.");
		return;
	}
	if(!render_pass){
		LogEGl("The input render pass was null.");
		return;
	}
	LogGl(1,"Destroying a render pass[",render_pass->debug_name,"].");
	
	memory_pool_delete(g_graphics->pools.render_passes, render_pass);
}


GraphicsRenderPass*
graphics_render_pass_of_window_presentation_frames(Window* window){DPZoneScoped;
	return ((WindowRenderInfoGl*)window->render_info)->framebuffer->render_pass;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_framebuffer


void
graphics_framebuffer_update(GraphicsFramebuffer* framebuffer){DPZoneScoped;
	if(!g_graphics->initialized){
		LogEGl("Must not be called before graphics is init.");
		return;
	}
	if(!framebuffer){
		LogEGl("The input framebuffer was null.");
		return;
	}
	if(!framebuffer->render_pass){
		LogEGl("The input framebuffer has no render_pass.");
		return;
	}
	if(framebuffer->render_pass->use_color_attachment){
		if(framebuffer->color_image_view->aspect_flags != GraphicsImageViewAspectFlags_Color){
			LogEGl("A color image view[",framebuffer->color_image_view->debug_name,"] attached to a framebuffer[",framebuffer->debug_name,"] does not have the GraphicsImageViewAspectFlags_Color aspect_flags.");
			return;
		}
		if(framebuffer->color_image_view->format < GraphicsFormat_COLOR_FIRST || framebuffer->color_image_view->format > GraphicsFormat_COLOR_LAST){
			LogEGl("A color image view[",framebuffer->color_image_view->debug_name,"] attached to a framebuffer[",framebuffer->debug_name,"] does not have a valid color GraphicsFormat: ",framebuffer->color_image_view->format,".");
			return;
		}
		if(!framebuffer->color_image_view->image){
			LogEGl("A color image view[",framebuffer->color_image_view->debug_name,"] attached to a framebuffer[",framebuffer->debug_name,"] has a null image.");
			return;
		}
		if(!framebuffer->color_image_view->image->__internal.handle){
			LogEGl("A color image view[",framebuffer->color_image_view->debug_name,"] attached to a framebuffer[",framebuffer->debug_name,"] has an image[",framebuffer->color_image_view->image->debug_name,"] which was not properly initialized with graphics_image_update() or was previously destroyed.");
			return;
		}
	}
	if(framebuffer->render_pass->use_depth_attachment){
		if(!HasFlag(framebuffer->depth_image_view->aspect_flags, GraphicsImageViewAspectFlags_Depth|GraphicsImageViewAspectFlags_Stencil)){
			LogEGl("A depth image view[",framebuffer->depth_image_view->debug_name,"] attached to a framebuffer[",framebuffer->debug_name,"] does not have the GraphicsImageViewAspectFlags_Depth or GraphicsImageViewAspectFlags_Stencil aspect_flags.");
			return;
		}
		if(framebuffer->depth_image_view->format < GraphicsFormat_DEPTH_FIRST || framebuffer->depth_image_view->format > GraphicsFormat_DEPTH_LAST){
			LogEGl("A depth image view[",framebuffer->depth_image_view->debug_name,"] attached to a framebuffer[",framebuffer->debug_name,"] does not have a valid depth/stencil GraphicsFormat: ",framebuffer->depth_image_view->format,".");
			return;
		}
		if(!framebuffer->depth_image_view->image){
			LogEGl("A depth image view[",framebuffer->depth_image_view->debug_name,"] attached to a framebuffer[",framebuffer->debug_name,"] has a null image.");
			return;
		}
		if(!framebuffer->depth_image_view->image->__internal.handle){
			LogEGl("A depth image view[",framebuffer->depth_image_view->debug_name,"] attached to a framebuffer[",framebuffer->debug_name,"] has an image[",framebuffer->depth_image_view->image->debug_name,"] which was not properly initialized with graphics_image_update() or was previously destroyed.");
			return;
		}
	}
	LogGl(1,"Updating a framebuffer[",framebuffer->debug_name,"].");
	
	//delete the old framebuffer
	if(framebuffer->__internal.handle){
		GLuint old_framebuffer_handle = (GLuint)(u64)framebuffer->__internal.handle;
		glDeleteFramebuffers(1, &old_framebuffer_handle);
		if(opengl_error){
			return;
		}
		
		framebuffer->__internal.handle = 0;
	}
	
	//create a new framebuffer
	GLuint framebuffer_handle;
	glGenFramebuffers(1, &framebuffer_handle);
	if(opengl_error){
		return;
	}
	
	//bind the framebuffer for read/write
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_handle);
	if(opengl_error){
		glDeleteFramebuffers(1, &framebuffer_handle);
		return;
	}
	
	
	//attach the color texture
	if(framebuffer->render_pass->use_color_attachment){
		//TODO(delle) glFramebufferRenderbuffer if GraphicsMemoryPropertyFlag_DeviceLocal
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, (GLuint)(u64)framebuffer->color_image_view->image->__internal.handle, 0);
		if(opengl_error){
			glDeleteFramebuffers(1, &framebuffer_handle);
			return;
		}
	}
	
	//attach the depth/stencil texture
	if(framebuffer->render_pass->use_depth_attachment){
		//TODO(delle) glFramebufferRenderbuffer if GraphicsMemoryPropertyFlag_DeviceLocal
		if(HasFlag(framebuffer->depth_image_view->aspect_flags, GraphicsImageViewAspectFlags_Depth)){
			if(HasFlag(framebuffer->depth_image_view->aspect_flags, GraphicsImageViewAspectFlags_Stencil)){
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, (GLuint)(u64)framebuffer->depth_image_view->image->__internal.handle, 0);
			}else{
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint)(u64)framebuffer->depth_image_view->image->__internal.handle, 0);
			}
		}else{
			glFramebufferTexture(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, (GLuint)(u64)framebuffer->depth_image_view->image->__internal.handle, 0);
		}
		if(opengl_error){
			glDeleteFramebuffers(1, &framebuffer_handle);
			return;
		}
	}
	
	GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(framebuffer_status != GL_FRAMEBUFFER_COMPLETE){
		const char* error_flag = 0;
		const char* error_msg  = 0;
		switch(framebuffer_status){
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:{
				error_flag = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
				error_msg  = "one of the framebuffer attachment points are framebuffer incomplete";
			}break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:{
				error_flag = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
				error_msg  = "the framebuffer does not have at least one image attached to it";
			}break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:{
				error_flag = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
				error_msg  = "the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for one of the color attachment point(s) named by GL_DRAW_BUFFERi";
			}break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:{
				error_flag = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
				error_msg  = "GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER";
			}break;
			case GL_FRAMEBUFFER_UNSUPPORTED:{
				error_flag = "GL_FRAMEBUFFER_UNSUPPORTED";
				error_msg  = "the combination of internal formats of the attached images violates an implementation-dependent set of restrictions";
			}break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:{
				error_flag = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
				error_msg  = "the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES. Or, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures";
			}break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:{
				error_flag = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
				error_msg  = "one of the framebuffer attachments is layered and another populated attachment is not layered, or all populated color attachments are not from textures of the same target";
			}break;
		}
		LogEGl("Failed to update a framebuffer[",framebuffer->debug_name,"] because ",error_msg," (",error_flag,").");
		glDeleteFramebuffers(1, &framebuffer_handle);
		return;
	}
	
	framebuffer->__internal.handle = (void*)(u64)framebuffer_handle;
}


void
graphics_framebuffer_destroy(GraphicsFramebuffer* framebuffer){DPZoneScoped;
	if(!g_graphics->pools.framebuffers){
		LogEGl("Must not be called before g_graphics->pools.framebuffers is init.");
		return;
	}
	if(!framebuffer){
		LogEGl("The input framebuffer was null.");
		return;
	}
	if(!framebuffer->__internal.handle){
		LogEGl("The input framebuffer was not properly initialized with graphics_framebuffer_update() or was previously deleted.");
		return;
	}
	LogGl(1,"Destroying a framebuffer[",framebuffer->debug_name,"].");
	
	GLuint framebuffer_handle = (GLuint)(u64)framebuffer->__internal.handle;
	glDeleteFramebuffers(1, &framebuffer_handle);
	if(opengl_error){
		return;
	}
	
	memory_pool_delete(g_graphics->pools.framebuffers, framebuffer);
}


GraphicsFramebuffer*
graphics_current_present_frame_of_window(Window* window){DPZoneScoped;
	return ((WindowRenderInfoGl*)window->render_info)->framebuffer;
}


GraphicsFormat
graphics_format_of_presentation_frames(Window* window){DPZoneScoped;
	return ((WindowRenderInfoGl*)window->render_info)->framebuffer->color_image_view->format;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_commands


void
graphics_command_buffer_update(GraphicsCommandBuffer* command_buffer){DPZoneScoped;
	//do nothing
}


void
graphics_command_buffer_destroy(GraphicsCommandBuffer* command_buffer){DPZoneScoped;
	if(!g_graphics->pools.command_buffers){
		LogEGl("Must not be called before g_graphics->pools.command_buffers is init.");
		return;
	}
	if(!command_buffer){
		LogEGl("The input command buffer was null.");
		return;
	}
	LogGl(1,"Destroying a command buffer[",command_buffer->debug_name,"].");
	
	memory_pool_delete(g_graphics->pools.command_buffers, command_buffer);
}


GraphicsCommandBuffer*
graphics_command_buffer_of_window(Window* window){DPZoneScoped;
	return ((WindowRenderInfoGl*)window->render_info)->command_buffer;
}
