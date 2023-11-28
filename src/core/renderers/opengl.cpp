/* deshi OpenGL Render Submodule
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
@graphics_buffer
@graphics_image
@graphics_descriptor
@graphics_shader
@graphics_pipeline
@graphics_renderpass
@graphics_framebuffer
@graphics_commands
*/


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_types


typedef struct WindowRenderInfoGl{
	GraphicsCommandBuffer* command_buffer;
}WindowRenderInfoGl;


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_vars


local void* opengl_context = 0;

local s32 opengl_success = 0;
local GLenum opengl_error = 0;

local int opengl_version = 0;
local int backend_version = 0;

#define OPENGL_INFOLOG_SIZE 512
local char opengl_infolog[OPENGL_INFOLOG_SIZE] = {};


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @gl_utils


#define LogGl(level, ...) if(g_graphics->logging_level >= level){ logger_push_indent(level); Log("opengl", __VA_ARGS__); logger_pop_indent(level); }(void)0
#define LogWGl(...) LogW("opengl", __func__, "(): ", __VA_ARGS__)
#define LogEGl(...) LogE("opengl", __func__, "(): ", __VA_ARGS__); Assert(!g_graphics->break_on_error)
#define GL_VERSION_TEST(major,minor,...) (opengl_version >= GLAD_MAKE_VERSION(major,minor))

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


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_context


void
graphics_init(Window* window){DPZoneScoped;
	AssertAlways(window);
	DeshiStageInitStart(DS_RENDER, DS_PLATFORM, "Attempted to reinitialize the Graphics module or initialzie it before initializing the Platform module.");
	Log("opengl","Starting OpenGL graphics backend initialization.");
	
	graphics_init_shared(window);
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup WGL and glad
#if DESHI_WINDOWS
	//restore point for contexts
	HDC   prev_dc = wglGetCurrentDC();
	HGLRC prev_rc = wglGetCurrentContext();
	
	//setup pixel format for dummy device context
	PIXELFORMATDESCRIPTOR temp_pfd{sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER};
	temp_pfd.cColorBits = 32; temp_pfd.cDepthBits = 24; temp_pfd.cStencilBits = 8;
	int temp_format = ChoosePixelFormat((HDC)window_helper.context, &temp_pfd);
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
	if(!::DescribePixelFormat((HDC)window->context, format, sizeof(pfd), &pfd)){
		win32_log_last_error("DescribePixelFormat", g_graphics->break_on_error);
		return;
	}
	if(format == 0){
		win32_log_last_error("ChoosePixelFormatARB", g_graphics->break_on_error);
		return;
	}
	if(!::SetPixelFormat((HDC)window->context, format, &pfd)){
		win32_log_last_error("SetPixelFormat", g_graphics->break_on_error);
		return;
	}
	
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
		LogEGl("Cannot find an appropriate framebuffer configuration with glXChooseFBConfig.");
		return;
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
		LogEGl("Cannot find an appropriate visual for the given attributes.");
		return;
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
		LogEGl("Failed to set the glx context.");
		return;
	}
#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
#  error "unhandled platform"
#endif //#else //#elif DESHI_LINUX //#if DESHI_WINDOWS
	
	//load glad extensions
	opengl_version = gladLoaderLoadGL();
	if(opengl_version == 0){
		LogEGl("Failed to load OpenGL.");
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
	//// require OpenGL3.0 minimum
	if(!GL_VERSION_TEST(3,0)){
		LogEGl("The OpenGL graphics backend of deshi requires at least version 3.0 which is not supported on this device.");
		return;
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// setup per-window info
	WindowRenderInfoGl* window_info = (WindowRenderInfoGl*)g_graphics->allocators.primary->reserve(sizeof(WindowRenderInfoGl));
	window->render_info = window_info;
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
		LogGl(2,"Window resized; Updating the viewport.");
		if(g_window->width <= 0 || g_window->height <= 0){
			return;
		}
		glViewport(0,0, g_window->width,g_window->height);
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// execute commands
	
	LogGl(3,"Starting the command buffer [",window_info->command_buffer->debug_name,"].");
	
	GraphicsRenderPass* current_render_pass = false;
	forX_array(cmd, window_info->command_buffer->commands){
		switch(cmd->type){
			case GraphicsCommandType_Begin_Render_Pass:{
				if(!cmd->begin_render_pass.pass){
					LogEGl("Null renderpass specified in a GraphicsCommandType_Begin_Render_Pass.");
					continue;
				}
				if(current_render_pass){
					LogEGl("Attempted to begin the renderpass [",cmd->begin_render_pass.pass->debug_name,"] while the renderpass [",current_render_pass->debug_name,"] is already in progress.");
					continue;
				}
				LogGl(3,"Beginning the renderpass [",cmd->begin_render_pass.pass->debug_name,"].");
				
				current_render_pass = cmd->begin_render_pass.pass;
				
				if(cmd->begin_render_pass.pass->use_color_attachment){
					glClearColor(cmd->begin_render_pass.pass->color_clear_values.r,
								 cmd->begin_render_pass.pass->color_clear_values.g,
								 cmd->begin_render_pass.pass->color_clear_values.r,
								 cmd->begin_render_pass.pass->color_clear_values.a);
				}
				
				if(cmd->begin_render_pass.pass->use_depth_attachment){
					//NOTE(delle) (1 - depth) because openGL Z direction is opposite ours
					glClearDepth(1.0 - (f64)cmd->begin_render_pass.pass->depth_clear_values.depth);
					glClearStencil(cmd->begin_render_pass.pass->depth_clear_values.stencil);
				}
				
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
			}break;
			
			case GraphicsCommandType_End_Render_Pass:{
				if(!current_render_pass){
					LogEGl("Attempted to end renderpass when one is not in progress.");
					continue;
				}
				LogGl(3,"Ending the renderpass [",current_render_pass->debug_name,"].");
				
				current_render_pass = 0;
			}break;
			
			case GraphicsCommandType_Bind_Pipeline:{
				if(!cmd->bind_pipeline.handle){
					LogEGl("Null pipeline specified in a GraphicsCommandType_Bind_Pipeline.");
					continue;
				}
				LogGl(3,"Binding the index buffer [",cmd->bind_pipeline.handle->debug_name,"].");
				
				//TODO pipelines
				//glUseProgram(programs.flat.handle);
			}break;
			
			case GraphicsCommandType_Set_Viewport:{
				LogGl(3,"Setting the viewport offset to ",cmd->set_viewport.offset," and extent to ",cmd->set_viewport.extent,".");
				
				//!TestMe that the offset.y doesn't need to be inverted
				glViewport(cmd->set_viewport.offset.x, cmd->set_viewport.offset.y,
						   cmd->set_viewport.extent.x, cmd->set_viewport.extent.y);
			}break;
			
			case GraphicsCommandType_Set_Scissor:{
				LogGl(3,"Setting the scissor offset to ",cmd->set_viewport.offset," and extent to ",cmd->set_viewport.extent,".");
				
				//!TestMe that the offset.y doesn't need to be inverted
				glScissor(cmd->set_scissor.offset.x, cmd->set_scissor.offset.y,
						  cmd->set_scissor.extent.x, cmd->set_scissor.extent.y);
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
				LogGl(3,"Binding the vertex buffer [",cmd->bind_vertex_buffer.handle->debug_name,"].");
				
				glBindBuffer(GL_ARRAY_BUFFER, (GLuint)(u64)cmd->bind_vertex_buffer.handle->__internal.buffer_handle);
			}break;
			
			case GraphicsCommandType_Bind_Index_Buffer:{
				if(!cmd->bind_index_buffer.handle){
					LogEGl("Null buffer specified in a GraphicsCommandType_Bind_Index_Buffer.");
					continue;
				}
				LogGl(3,"Binding the index buffer [",cmd->bind_index_buffer.handle->debug_name,"].");
				
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)(u64)cmd->bind_index_buffer.handle->__internal.buffer_handle);
			}break;
			
			case GraphicsCommandType_Bind_Descriptor_Set:{
				if(!cmd->bind_descriptor_set.handle){
					LogEGl("Null descriptor set specified in a GraphicsCommandType_Bind_Descriptor_Set.");
					continue;
				}
				LogGl(3,"Binding the descriptor set [",cmd->bind_descriptor_set.handle->debug_name,"].");
				
				//TODO textures
				//glActiveTexture(GL_TEXTURE0 + descriptor_index);
				//glBindTexture(glTextures[0].type, glTextures[0].handle);
			}break;
			
			case GraphicsCommandType_Push_Constant:{
				
				
				//TODO push constant UBO
				//glBindBuffer(GL_UNIFORM_BUFFER, pushVS.handle);
				//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &matrix);
			}break;
			
			case GraphicsCommandType_Draw_Indexed:{
				
				
				//TODO draw
				//glDrawElements(GL_TRIANGLES, it->index_count, INDEX_TYPE_GL_MESH, 0);
			}break;
		}
	}
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// present frame
	
	LogGl(3,"Presenting the frame.");
	
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
	LogGl(1,"Cleaning up the OpenGL graphics backend.");
	
	//TODO(delle) save pipelines in GL4
	//TODO(delle) save graphics settings to config
	glFinish();
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_buffer
//!ref: https://www.khronos.org/opengl/wiki/Buffer_Object


#define OPENGL_RENDER_BUFFER_TARGET GL_ARRAY_BUFFER

GraphicsBuffer*
graphics_buffer_create(void* data, u64 requested_size, GraphicsBufferUsage usage, GraphicsMemoryPropertyFlags properties, GraphicsMemoryMappingBehavoir mapping){
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
	glBindBuffer(OPENGL_RENDER_BUFFER_TARGET, buffer_handle); //NOTE: the target we bind it to doesn't matter for creation
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
		
		glBufferStorage(OPENGL_RENDER_BUFFER_TARGET, requested_size, data, storage_flags);
		if(opengl_error){
			glDeleteBuffers(1, &buffer_handle);
			return 0;
		}
	}else{
		//TODO(delle) better buffer usage determination
		if(mapping == GraphicsMemoryMapping_Never){
			gl_buffer_data_usage_hints = GL_STATIC_DRAW;
			glBufferData(OPENGL_RENDER_BUFFER_TARGET, requested_size, data, GL_STATIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}else if(mapping == GraphicsMemoryMapping_Persistent){
			gl_buffer_data_usage_hints = GL_STREAM_DRAW;
			glBufferData(OPENGL_RENDER_BUFFER_TARGET, requested_size, data, GL_STREAM_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
			
			if(HasFlag(properties, GraphicsMemoryPropertyFlag_HostVisible)){
				mapped_size = requested_size;
				mapped_data = glMapBufferRange(OPENGL_RENDER_BUFFER_TARGET, 0, requested_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT);
				if(opengl_error){
					glDeleteBuffers(1, &buffer_handle);
					return 0;
				}
			}else if(HasFlag(properties, GraphicsMemoryPropertyFlag_HostCoherent)){
				mapped_size = requested_size;
				mapped_data = glMapBufferRange(OPENGL_RENDER_BUFFER_TARGET, 0, requested_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT);
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
			glBufferData(OPENGL_RENDER_BUFFER_TARGET, requested_size, data, GL_STREAM_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}else{
			gl_buffer_data_usage_hints = GL_DYNAMIC_DRAW;
			glBufferData(OPENGL_RENDER_BUFFER_TARGET, requested_size, data, GL_DYNAMIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &buffer_handle);
				return 0;
			}
		}
	}
	
	GraphicsBuffer* result = memory_pool_push(g_graphics->pools.buffers);
	result->__internal.size = requested_size;
	result->__internal.usage = usage;
	result->__internal.memory_properties = properties;
	result->__internal.mapping_behavoir = mapping;
	result->__internal.mapped.data = mapped_data;
	result->__internal.mapped.size = mapped_size;
	result->__internal.buffer_handle = (void*)(u64)buffer_handle;
	result->__internal.memory_handle = (void*)gl_buffer_data_usage_hints;
	return result;
}

void
graphics_buffer_destroy(GraphicsBuffer* buffer){
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return;
	}
	if(!buffer || !buffer->__internal.buffer_handle){
		LogEGl("The input buffer was not properly created with graphics_buffer_create() or was previously deleted.");
		return;
	}
	LogGl(1,"Destroying the buffer [",buffer->debug_name,"] with ",buffer->__internal.size," bytes.");
	
	//unmap before deletion
	if(buffer->__internal.mapped.data){
		glUnmapBuffer(OPENGL_RENDER_BUFFER_TARGET);
	}
	
	GLuint buffer_handle = (GLuint)(u64)buffer->__internal.buffer_handle;
	if(GL_VERSION_TEST(4,3, glInvalidateBufferData)){
		glInvalidateBufferData(buffer_handle);
	}else{
		glBindBuffer(OPENGL_RENDER_BUFFER_TARGET, buffer_handle);
		glBufferData(OPENGL_RENDER_BUFFER_TARGET, (GLsizeiptr)buffer->__internal.size, 0, (GLenum)(u64)buffer->__internal.memory_handle);
	}
	glDeleteBuffers(1, &buffer_handle);
	
	memory_pool_delete(g_graphics->pools.buffers, buffer);
}

void
graphics_buffer_reallocate(GraphicsBuffer* buffer, u64 new_size){
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return;
	}
	if(new_size == 0){
		LogEGl("Must not be called with zero size.");
		return;
	}
	if(!buffer || !buffer->__internal.buffer_handle){
		LogEGl("The input buffer was not properly created with graphics_buffer_create() or was previously deleted.");
		return;
	}
	if(!HasFlag(buffer->__internal.usage, GraphicsBufferUsage_TransferSource|GraphicsBufferUsage_TransferDestination)){
		//NOTE: only here for consistency as it's not a requirement in OpenGL
		LogEGl("The input buffer does not have the usage flags GraphicsBufferUsage_TransferSource and GraphicsBufferUsage_TransferDestination, so it cannot be copied.");
		return;
	}
	if(new_size < buffer->__internal.size){
		return;
	}
	LogGl(1,"Reallocating the buffer [",buffer->debug_name,"] with ",buffer->__internal.size," bytes to have ",new_size," bytes.");
	
	//unmap the old buffer before copy/deletion
	if(buffer->__internal.mapped.data){
		glUnmapBuffer(OPENGL_RENDER_BUFFER_TARGET);
	}
	
	//bind the old buffer
	GLuint old_buffer_handle = (GLuint)(u64)buffer->__internal.buffer_handle;
	glBindBuffer(GL_COPY_READ_BUFFER, old_buffer_handle);
	if(opengl_error){
		return;
	}
	
	//get the old buffer size
	GLsizeiptr old_buffer_size = buffer->__internal.size;
	if(GL_VERSION_TEST(3,2, glGetBufferParameteri64v)){
		GLint64 gl_buffer_size;
		glGetBufferParameteri64v(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &gl_buffer_size);
		old_buffer_size = (GLsizeiptr)gl_buffer_size;
	}else{
		GLint gl_buffer_size;
		glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &gl_buffer_size);
		old_buffer_size = (GLsizeiptr)gl_buffer_size;
	}
	
	//create a new buffer
	GLuint new_buffer_handle;
	glGenBuffers(1, &new_buffer_handle);
	if(opengl_error){
		return;
	}
	glBindBuffer(OPENGL_RENDER_BUFFER_TARGET, new_buffer_handle); //NOTE: the target we bind it to doesn't matter for creation
	if(opengl_error){
		glDeleteBuffers(1, &new_buffer_handle);
		return;
	}
	
	//allocate the new buffer's memory
	void* mapped_data = 0;
	u64   mapped_size = 0;
	if(GL_VERSION_TEST(4,4, glBufferStorage)){
		GLbitfield storage_flags = 0;
		if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Never){
			if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostVisible|GraphicsMemoryPropertyFlag_HostCoherent|GraphicsMemoryPropertyFlag_HostCached)){
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_Never and GraphicsMemoryPropertyFlag_HostVisible or GraphicsMemoryPropertyFlag_HostCoherent or GraphicsMemoryPropertyFlag_HostCached.");
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Occasional){
			if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostVisible) && !HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostCoherent)){
				storage_flags = GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT|GL_CLIENT_STORAGE_BIT; 
			}else{
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_MapWriteUnmap and not GraphicsMemoryPropertyFlag_HostVisible.");
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Persistent){
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
		
		glBufferStorage(OPENGL_RENDER_BUFFER_TARGET, new_size, 0, storage_flags);
		if(opengl_error){
			glDeleteBuffers(1, &new_buffer_handle);
			return;
		}
	}else{
		//TODO(delle) better buffer usage determination
		if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Never){
			glBufferData(OPENGL_RENDER_BUFFER_TARGET, new_size, 0, GL_STATIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Persistent){
			glBufferData(OPENGL_RENDER_BUFFER_TARGET, new_size, 0, GL_STREAM_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else if(HasFlag(buffer->__internal.usage, GraphicsBufferUsage_UniformBuffer)){
			glBufferData(OPENGL_RENDER_BUFFER_TARGET, new_size, 0, GL_STREAM_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}else{
			glBufferData(OPENGL_RENDER_BUFFER_TARGET, new_size, 0, GL_DYNAMIC_DRAW);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
		}
	}
	
	//copy the old buffer's memory to the new buffer's memory
	if(GL_VERSION_TEST(3,1, glCopyBufferSubData)){
		glCopyBufferSubData(GL_COPY_READ_BUFFER, OPENGL_RENDER_BUFFER_TARGET, 0, 0, old_buffer_size);
		
		if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Persistent){
			glUnmapBuffer(GL_COPY_READ_BUFFER);
			
			if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostVisible)){
				mapped_size = new_size;
				mapped_data = glMapBufferRange(OPENGL_RENDER_BUFFER_TARGET, 0, new_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT);
				if(opengl_error){
					glDeleteBuffers(1, &new_buffer_handle);
					return;
				}
			}else if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostCoherent)){
				mapped_size = new_size;
				mapped_data = glMapBufferRange(OPENGL_RENDER_BUFFER_TARGET, 0, new_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT);
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
	}else{
		//NOTE: Not sure if there is a routine for copying buffer memory before OpenGL3.1 so doing a memory mapping in order to copy
		
		void* old_mapped_data = glMapBufferRange(GL_COPY_READ_BUFFER, 0, old_buffer_size, GL_MAP_READ_BIT);
		if(opengl_error){
			glDeleteBuffers(1, &new_buffer_handle);
			return;
		}
		
		if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Persistent){
			if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostVisible)){
				mapped_size = new_size;
				mapped_data = glMapBufferRange(OPENGL_RENDER_BUFFER_TARGET, 0, new_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT);
				if(opengl_error){
					glDeleteBuffers(1, &new_buffer_handle);
					return;
				}
			}else if(HasFlag(buffer->__internal.memory_properties, GraphicsMemoryPropertyFlag_HostCoherent)){
				mapped_size = new_size;
				mapped_data = glMapBufferRange(OPENGL_RENDER_BUFFER_TARGET, 0, new_size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT);
				if(opengl_error){
					glDeleteBuffers(1, &new_buffer_handle);
					return;
				}
			}else{
				LogEGl("Called with incompatible mapping and memory flags, GraphicsMemoryMapping_Persistent and not GraphicsMemoryPropertyFlag_HostVisible or GraphicsMemoryPropertyFlag_HostCoherent.");
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
			
			CopyMemory(mapped_data, old_mapped_data, (upt)old_buffer_size);
		}else{
			void* new_mapped_data = glMapBufferRange(OPENGL_RENDER_BUFFER_TARGET, 0, old_buffer_size, GL_MAP_WRITE_BIT);
			if(opengl_error){
				glDeleteBuffers(1, &new_buffer_handle);
				return;
			}
			
			CopyMemory(new_mapped_data, old_mapped_data, (upt)old_buffer_size);
			
			glUnmapBuffer(OPENGL_RENDER_BUFFER_TARGET);
		}
		
		glUnmapBuffer(GL_COPY_READ_BUFFER);
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
graphics_buffer_map(GraphicsBuffer* buffer, u64 size, u64 offset){
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return 0;
	}
	if(!buffer || !buffer->__internal.buffer_handle){
		LogEGl("The input buffer was not properly created with graphics_buffer_create() or was previously deleted.");
		return 0;
	}
	if(buffer->__internal.mapping_behavoir != GraphicsMemoryMapping_Occasional){
		LogEGl("A buffer must have the mapping GraphicsMemoryMapping_Occasional in order to be mapped in the middle of its lifetime.");
		return 0;
	}
	if(buffer->__internal.mapped.data){
		if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Persistent){
			LogWGl("Cannot map a persistently mapped buffer since it's always actively mapped.");
		}else{
			LogWGl("Cannot map an actively mapped buffer.");
		}
		return 0;
	}
	LogGl(1,"Mapping ",size," bytes at the ",offset," offset into the buffer [",buffer->debug_name,"].");
	
	glBindBuffer(OPENGL_RENDER_BUFFER_TARGET, (GLuint)(u64)buffer->__internal.buffer_handle);
	if(opengl_error){
		return 0;
	}
	
	buffer->__internal.mapped.data = glMapBufferRange(OPENGL_RENDER_BUFFER_TARGET, (GLintptr)offset, (GLsizeiptr)size, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_FLUSH_EXPLICIT_BIT);
	if(opengl_error){
		return 0;
	}
	
	buffer->__internal.mapped.offset = offset;
	buffer->__internal.mapped.size = size;
}

void
graphics_buffer_unmap(GraphicsBuffer* buffer, b32 flush){
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return;
	}
	if(!buffer || !buffer->__internal.buffer_handle){
		LogEGl("The input buffer was not properly created with graphics_buffer_create() or was previously deleted.");
		return;
	}
	if(buffer->__internal.mapping_behavoir != GraphicsMemoryMapping_Occasional){
		LogEGl("A buffer must have the mapping GraphicsMemoryMapping_Occasional in order to be unmapped in the middle of its lifetime.");
		return;
	}
	if(!buffer->__internal.mapped.data){
		if(buffer->__internal.mapping_behavoir == GraphicsMemoryMapping_Persistent){
			LogWGl("Cannot unmap a persistently mapped buffer since it's always actively mapped.");
		}else{
			LogWGl("The input buffer is not actively mapped.");
		}
		return;
	}
	LogGl(1,"Unmapping",(flush ? " and flushing" : "")," the buffer [",buffer->debug_name,"].");
	
	glBindBuffer(OPENGL_RENDER_BUFFER_TARGET, (GLuint)(u64)buffer->__internal.buffer_handle);
	if(opengl_error){
		return;
	}
	
	if(flush){
		glFlushMappedBufferRange(OPENGL_RENDER_BUFFER_TARGET, (GLintptr)buffer->__internal.mapped.offset, (GLsizeiptr)buffer->__internal.mapped.size);
		if(opengl_error){
			return;
		}
	}
	
	glUnmapBuffer(OPENGL_RENDER_BUFFER_TARGET);
	if(opengl_error){
		return;
	}
	
	buffer->__internal.mapped.data = 0;
	buffer->__internal.mapped.offset = 0;
	buffer->__internal.mapped.size = 0;
}

void
graphics_buffer_flush(GraphicsBuffer* buffer){
	if(!g_graphics->pools.buffers){
		LogEGl("Must not be called before g_graphics->pools.buffers is init.");
		return;
	}
	if(!buffer || !buffer->__internal.buffer_handle){
		LogEGl("The input buffer was not properly created with graphics_buffer_create() or was previously deleted.");
		return;
	}
	if(!buffer->__internal.mapped.data){
		LogWGl("The input buffer is not actively mapped.");
		return;
	}
	LogGl(1,"Flushing the buffer [",buffer->debug_name,"].");
	
	glFlushMappedBufferRange(OPENGL_RENDER_BUFFER_TARGET, (GLintptr)buffer->__internal.mapped.offset, (GLsizeiptr)buffer->__internal.mapped.size);
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_image


void
graphics_image_update(GraphicsImage* image){
	DontCompile;
}

void
graphics_image_destroy(GraphicsImage* image){
	DontCompile;
}

void
graphics_image_write(GraphicsImage* image, u8* pixels, vec2i offset, vec2i extent){
	DontCompile;
}

void
graphics_image_view_update(GraphicsImageView* image_view){
	DontCompile;
}

void
graphics_image_view_destroy(GraphicsImageView* image_view){
	DontCompile;
}

void
graphics_sampler_update(GraphicsSampler* sampler){
	DontCompile;
}

void
graphics_sampler_destroy(GraphicsSampler* sampler){
	DontCompile;
}

GraphicsFormat
graphics_format_of_presentation_frames(Window* window){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_descriptor


void
graphics_descriptor_set_layout_update(GraphicsDescriptorSetLayout* descriptor_set_layout){
	DontCompile;
}

void
graphics_descriptor_set_layout_destroy(GraphicsDescriptorSetLayout* descriptor_set_layout){
	DontCompile;
}

void
graphics_descriptor_set_update(GraphicsDescriptorSet* descriptor_set){
	DontCompile;
}

void
graphics_descriptor_set_destroy(GraphicsDescriptorSet* descriptor_set){
	DontCompile;
}

void
graphics_descriptor_set_write(GraphicsDescriptorSet* descriptor_set, u32 binding, GraphicsDescriptor descriptor){
	DontCompile;
}

void
graphics_descriptor_set_write_array(GraphicsDescriptorSet* descriptor_set, GraphicsDescriptor* descriptors){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_shader


void
graphics_shader_update(GraphicsShader* shader){
	DontCompile;
}

void
graphics_shader_destroy(GraphicsShader* shader){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_pipeline


void
graphics_pipeline_layout_update(GraphicsPipelineLayout* pipeline_layout){
	DontCompile;
}

void
graphics_pipeline_layout_destroy(GraphicsPipelineLayout* pipeline_layout){
	DontCompile;
}

void
graphics_pipeline_update(GraphicsPipeline* pipeline){
	DontCompile;
}

void
graphics_pipeline_destroy(GraphicsPipeline* pipeline){
	DontCompile;
}

GraphicsPipeline*
graphics_pipeline_duplicate(GraphicsPipeline* pipeline){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_renderpass


void
graphics_render_pass_update(GraphicsRenderPass* renderpass){
	DontCompile;
}

void
graphics_render_pass_destroy(GraphicsRenderPass* renderpass){
	DontCompile;
}

GraphicsRenderPass*
graphics_render_pass_of_window_presentation_frames(Window* window){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_framebuffer


void
graphics_framebuffer_update(GraphicsFramebuffer* framebuffer){
	DontCompile;
}

void
graphics_framebuffer_destroy(GraphicsFramebuffer* framebuffer){
	DontCompile;
}

GraphicsFramebuffer*
graphics_current_present_frame_of_window(Window* window){
	DontCompile;
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @graphics_commands


void
graphics_command_buffer_update(GraphicsCommandBuffer* command_buffer){
	DontCompile;
}

void
graphics_command_buffer_destroy(GraphicsCommandBuffer* command_buffer){
	DontCompile;
}

GraphicsCommandBuffer*
graphics_command_buffer_of_window(Window* window){
	DontCompile;
}

