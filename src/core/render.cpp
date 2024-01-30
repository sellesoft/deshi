#define RenderAssert(cond, ...) do { if(!cond){ LogE("scene", __FUNCTION__, ": assertion failed `", STRINGIZE(cond), "`: ", __VA_ARGS__); Assert(0); } } while(0); 
#define RenderError(...) LogE("scene", __VA_ARGS__)

RenderGlobal __deshi__g_scene;
RenderGlobal* g_scene = &__deshi__g_scene;

void 
render_init(){DPZoneScoped;
	*g_scene = {};
	memory_pool_init(g_scene->pools.camera, 4);
	memory_pool_init(g_scene->pools.voxel_chunks, 8);
	array_init(g_scene->model_draw_commands, 4, deshi_allocator);
	
	ShaderStages stages = {};
	stages.vertex = assets_shader_load_from_source(str8l("voxel.vert"), baked_shader_voxel_vert, ShaderType_Vertex);
	array_init(stages.vertex->resources, 2, deshi_allocator);
	array_push_value(stages.vertex->resources, ShaderResourceType_UBO);
	array_push_value(stages.vertex->resources, ShaderResourceType_PushConstant);
	stages.fragment = assets_shader_load_from_source(str8l("voxel.frag"), baked_shader_voxel_frag, ShaderType_Fragment);
	
	ShaderResource* voxel_resources = array_create(ShaderResource, 2, deshi_temp_allocator);
	ShaderResource* voxel_ubo_resource = array_push(voxel_resources);
	voxel_ubo_resource->type = ShaderResourceType_UBO;
	voxel_ubo_resource->name_in_shader = "CameraInfo";
	voxel_ubo_resource->ubo = g_assets->base_ubo_handle;
	ShaderResource* voxel_pc_resource = array_push(voxel_resources);
	voxel_pc_resource->type = ShaderResourceType_PushConstant;
	voxel_pc_resource->name_in_shader = "PushConstant";
	voxel_pc_resource->push_constant_size = sizeof(mat4);
	
	g_scene->voxel_material = assets_material_create(str8l("<scene> voxels"), stages, voxel_resources);
}

void
render_update(){DPZoneScoped;
	RenderAssert(g_scene->active.window, "need a window to render to.");
	RenderAssert(g_scene->active.camera, "need a camera to render from.");
	
	auto win   = g_scene->active.window;
	auto cam   = g_scene->active.camera;
	auto frame = graphics_current_present_frame_of_window(win);
	auto pass  = frame->render_pass;
	
	GraphicsPipeline* last_pipeline = 0;
	
	// TODO(sushi) we need a way to do this only when specific things change
	// TODO(sushi) that we do this is kinda weird, cause camera is a scene thing
	//             but since assets controls the base ubo of shaders used with it
	//             we have to do this like this for now.
	assets_update_camera_view(&g_scene->active.camera->view);
	assets_update_camera_projection(&g_scene->active.camera->proj);
	
	{ using namespace graphics::cmd;
		begin_render_pass(win, pass, frame);
		
		set_viewport(win, vec2::ZERO, win->dimensions.toVec2());
		set_scissor(win, vec2::ZERO, win->dimensions.toVec2());
		
		forI(array_count(g_scene->model_draw_commands)){
			auto cmd = g_scene->model_draw_commands[i];
			bind_vertex_buffer(win, cmd.model->mesh->vertex_buffer);
			bind_index_buffer(win, cmd.model->mesh->index_buffer);
			forI(array_count(cmd.model->batch_array)){
				auto b = cmd.model->batch_array[i];
				if(!b.index_count) continue;
				if(b.material->pipeline != last_pipeline){
					last_pipeline = b.material->pipeline;
					bind_pipeline(win, last_pipeline);
					bind_descriptor_set(win, 0, g_assets->view_proj_ubo);
				}
				push_constant(win, GraphicsShaderStage_Vertex, cmd.transform, 0, sizeof(mat4));
				bind_descriptor_set(win, 1, b.material->descriptor_set);
				draw_indexed(win, b.index_count, b.index_offset, 0);
			}
		}	
		
		bind_pipeline(win, g_scene->voxel_material->pipeline);
		bind_descriptor_set(win, 0, g_assets->view_proj_ubo);
		for_pool(g_scene->pools.voxel_chunks){
			if(!it->vertex_buffer || !it->index_buffer){
				break;
			}
			
			if(!it->hidden){
				it->transform = mat4::TransformationMatrix(it->position, it->rotation, vec3::ONE);
				bind_vertex_buffer(win, it->vertex_buffer);
				bind_index_buffer(win, it->index_buffer);
				push_constant(win, GraphicsShaderStage_Vertex, &it->transform, 0, sizeof(mat4));
				draw_indexed(win, it->index_count, 0, 0);
			}
		}
		
		end_render_pass(win);
		
		if(g_scene->temp.render_pass){
			g_scene->temp.camera_ubo.proj = g_scene->active.camera->proj;
			g_scene->temp.camera_ubo.view = g_scene->active.camera->view;
			CopyMemory(graphics_buffer_mapped_data(g_scene->temp.camera_buffer), &g_scene->temp.camera_ubo, sizeof(g_scene->temp.camera_ubo));
			
			begin_render_pass(win, g_scene->temp.render_pass, frame);
			
			if(g_scene->temp.wireframe.vertex_count){
				bind_vertex_buffer(win, g_scene->temp.wireframe.vertex_buffer);
				bind_index_buffer(win, g_scene->temp.wireframe.index_buffer);
				bind_pipeline(win, g_scene->temp.wireframe.pipeline);
				bind_descriptor_set(win, 0, g_scene->temp.descriptor_set);
				draw_indexed(win, g_scene->temp.wireframe.index_count, 0, 0);
			}		
			
			if(g_scene->temp.filled.vertex_count){
				bind_vertex_buffer(win, g_scene->temp.filled.vertex_buffer);
				bind_index_buffer(win, g_scene->temp.filled.index_buffer);
				bind_pipeline(win, g_scene->temp.filled.pipeline);
				draw_indexed(win, g_scene->temp.filled.index_count, 0, 0);
			}
			end_render_pass(win);
		}
	}
	
	array_clear(g_scene->model_draw_commands);
}

void
render_set_active_window(Window* window){DPZoneScoped;
	g_scene->active.window = window;
}

void
render_set_active_camera(Camera* camera){DPZoneScoped;
	g_scene->active.camera = camera;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @camera


Camera*
render_camera_create(){DPZoneScoped;
	auto out = memory_pool_push(g_scene->pools.camera);
	return out;
}

void
render_camera_update_view(Camera* camera){DPZoneScoped;
	RenderAssert(camera, "passed null Camera pointer.");
	camera->right = vec3::UP.cross(camera->forward).normalized();
	camera->up = camera->forward.cross(camera->right).normalized();
	camera->view = Math::LookAtMatrix(camera->position, camera->position + camera->forward).Inverse();
}

void
render_camera_update_perspective_projection(Camera* camera, u32 width, u32 height, f32 fov, f32 near_z, f32 far_z){DPZoneScoped;
	RenderAssert(camera, "passed null Camera pointer.");
	camera->proj = Math::PerspectiveProjectionMatrix(width, height, fov, near_z, far_z);
#if DESHI_VULKAN
	camera->proj.arr[5] *= -1;
#endif //#if DESHI_VULKAN
}

void
render_camera_update_orthographic_projection(Camera* camera, f32 right, f32 left, f32 top, f32 bottom, f32 far, f32 near_){DPZoneScoped;
	RenderAssert(camera, "passed null Camera pointer.");
	f32 A =  2 / (right - left);
	f32 B =  2 / (top - bottom);
	f32 C = -2 / (far - near_);
	f32 D = -(right + left) / (right - left);
	f32 E = -(top + bottom) / (top - bottom);
	f32 F = -(far + near_) / (far - near_);
	camera->proj = mat4(A, 0, 0, D,
						0, B, 0, E,
						0, 0, C, F,
						0, 0, 0, 1);
}

void
render_camera_destroy(Camera* camera){DPZoneScoped;
	memory_pool_delete(g_scene->pools.camera, camera);
}

void
render_camera_draw_frustrum(Camera* camera){DPZoneScoped;
	// TODO(sushi) implement by transforming 4 points out based on the proj and view matrices
	NotImplemented;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @model


void 
render_draw_model(Model* model, mat4* transform){DPZoneScoped;
	RenderAssert(model, "passed null Model pointer.");
	auto cmd = array_push(g_scene->model_draw_commands);
	cmd->model = model;
	cmd->transform = transform;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @temp


void
render_temp_init(Window* window, u32 v){DPZoneScoped;
	if(v > MAX_U32/3){
		RenderError("the given vertex count (", v, ") is too large to allow 3*vertex_count indexes. The max amount of supported vertexes is ", MAX_U32 / 3);
		return;
	}
	
	g_scene->temp.filled = {};
	g_scene->temp.filled.vertex_buffer = graphics_buffer_create(0, v * sizeof(RenderTempVertex), GraphicsBufferUsage_VertexBuffer, GraphicsMemoryPropertyFlag_HostCoherent, GraphicsMemoryMapping_Persistent);
	g_scene->temp.filled.index_buffer  = graphics_buffer_create(0, v * sizeof(RenderTempIndex), GraphicsBufferUsage_IndexBuffer, GraphicsMemoryPropertyFlag_HostCoherent, GraphicsMemoryMapping_Persistent);
	g_scene->temp.wireframe = {};
	g_scene->temp.wireframe.vertex_buffer = graphics_buffer_create(0, v * sizeof(RenderTempVertex), GraphicsBufferUsage_VertexBuffer, GraphicsMemoryPropertyFlag_HostCoherent, GraphicsMemoryMapping_Persistent);
	g_scene->temp.wireframe.index_buffer = graphics_buffer_create(0, v * sizeof(RenderTempIndex), GraphicsBufferUsage_IndexBuffer, GraphicsMemoryPropertyFlag_HostCoherent, GraphicsMemoryMapping_Persistent);
	
	auto rp = g_scene->temp.render_pass = graphics_render_pass_allocate();
	rp->debug_name = str8l("<render> temp render pass");
	rp->color_clear_values = vec4::ZERO;
	rp->depth_clear_values = {1.f, 0};
	rp->use_color_attachment = true;
	rp->color_attachment.          format = graphics_format_of_presentation_frames(window);
	rp->color_attachment.         load_op = GraphicsLoadOp_Load;
	rp->color_attachment.        store_op = GraphicsStoreOp_Store;
	rp->color_attachment. stencil_load_op = GraphicsLoadOp_Dont_Care;
	rp->color_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	rp->color_attachment.  initial_layout = GraphicsImageLayout_Present;
	rp->color_attachment.    final_layout = GraphicsImageLayout_Present;
	rp->use_depth_attachment = true;
	rp->depth_attachment.          format = GraphicsFormat_Depth32_Float;
	rp->depth_attachment.         load_op = GraphicsLoadOp_Load;
	rp->depth_attachment.        store_op = GraphicsStoreOp_Store;
	rp->depth_attachment. stencil_load_op = GraphicsLoadOp_Clear;
	rp->depth_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	rp->depth_attachment.  initial_layout = GraphicsImageLayout_Depth_Stencil_Attachment_Optimal;
	rp->depth_attachment.    final_layout = GraphicsImageLayout_Depth_Stencil_Attachment_Optimal;
	graphics_render_pass_update(rp);
	
	auto pl = g_scene->temp.filled.pipeline = graphics_pipeline_allocate();
	pl->            debug_name = str8l("<render> temp pipeline");
	pl->            front_face = GraphicsFrontFace_CCW;
	pl->               culling = GraphicsPipelineCulling_None;
	pl->          polygon_mode = GraphicsPolygonMode_Fill;
	pl->            depth_test = true;
	pl->          depth_writes = true;
	pl->      depth_compare_op = GraphicsCompareOp_Less;
	pl->            depth_bias = false;
	pl->            line_width = 1.f;
	pl->           color_blend = true;
	pl->        color_blend_op = GraphicsBlendOp_Add;
	pl->color_src_blend_factor = GraphicsBlendFactor_Source_Alpha;
	pl->color_dst_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
	pl->        alpha_blend_op = GraphicsBlendOp_Add;
	pl->alpha_src_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
	pl->alpha_dst_blend_factor = GraphicsBlendFactor_Zero;
	pl->        blend_constant = color(10,10,10,255);
	pl->           render_pass = rp;
	pl->      dynamic_viewport = true;
	pl->       dynamic_scissor = true;
	
	auto temp_vertex_shader = graphics_shader_allocate();
	temp_vertex_shader->debug_name = str8l("<scene> temp vertex shader");
	temp_vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
	temp_vertex_shader->source = baked_shader_temp_vert;
	graphics_shader_update(temp_vertex_shader);	
	
	auto temp_fragment_shader = graphics_shader_allocate();
	temp_fragment_shader->debug_name = str8l("<scene> temp fragment shader");
	temp_fragment_shader->shader_stage = GraphicsShaderStage_Fragment;
	temp_fragment_shader->source = baked_shader_temp_frag;
	graphics_shader_update(temp_fragment_shader);
	
	pl->vertex_shader = temp_vertex_shader;
	pl->fragment_shader = temp_fragment_shader;
	
	array_init_with_elements(pl->vertex_input_bindings, {
								 {0, sizeof(RenderTempVertex)}
							 });
	array_init_with_elements(pl->vertex_input_attributes, {
								 {0, 0, GraphicsFormat_R32G32B32_Float, offsetof(RenderTempVertex, pos)},
								 {1, 0, GraphicsFormat_R8G8B8A8_UNorm,  offsetof(RenderTempVertex, color)}
							 });
	
	g_scene->temp.camera_buffer = graphics_buffer_create(&g_scene->temp.camera_ubo, sizeof(g_scene->temp.camera_ubo), GraphicsBufferUsage_UniformBuffer, GraphicsMemoryPropertyFlag_HostCoherent, GraphicsMemoryMapping_Persistent);
	
	auto tl = graphics_pipeline_layout_allocate();
	tl->debug_name = str8l("<render> temp pipeline layout");
	auto dl = graphics_descriptor_set_layout_allocate();
	array_init_with_elements(dl->bindings, {
								 {GraphicsDescriptorType_Uniform_Buffer, GraphicsShaderStage_Vertex, 0}
							 });
	graphics_descriptor_set_layout_update(dl);
	array_init_with_elements(tl->descriptor_layouts, {dl});
	graphics_pipeline_layout_update(tl);
	
	pl->layout = tl;
	graphics_pipeline_update(pl);
	
	auto wpl = g_scene->temp.wireframe.pipeline = graphics_pipeline_duplicate(pl);
	wpl->debug_name = str8l("<render> temp wireframe pipeline");
	wpl->polygon_mode = GraphicsPolygonMode_Line;
	wpl->depth_compare_op = GraphicsCompareOp_Less_Or_Equal;
	wpl->color_blend_op = GraphicsBlendOp_Max;
	graphics_pipeline_update(wpl);
	
	GraphicsDescriptor* descriptors = array_create(GraphicsDescriptor, 1, deshi_allocator);
	GraphicsDescriptor* descriptor = array_push(descriptors);
	descriptor->type = GraphicsDescriptorType_Uniform_Buffer;
	descriptor->name_in_shader = "CameraInfo";
	descriptor->ubo = {g_scene->temp.camera_buffer, 0, sizeof(g_scene->temp.camera_ubo)};
	
	auto ds = g_scene->temp.descriptor_set = graphics_descriptor_set_allocate();
	ds->debug_name = str8l("<render> temp descriptor set");
	ds->descriptors = descriptors;
	ds->layouts = array_copy(pl->layout->descriptor_layouts).ptr;
	graphics_descriptor_set_update(ds);
	graphics_descriptor_set_write(ds);
}

void 
render_temp_clear(){DPZoneScoped;
	g_scene->temp.wireframe.vertex_count = 0;
	g_scene->temp.wireframe.index_count = 0;
	g_scene->temp.filled.vertex_count = 0;
	g_scene->temp.filled.index_count = 0;
}

void 
render_temp_update_camera(vec3 position, vec3 target){DPZoneScoped;
	g_scene->temp.camera_ubo.view = Math::LookAtMatrix(position, target).Inverse();
	CopyMemory(graphics_buffer_mapped_data(g_scene->temp.camera_buffer), &g_scene->temp.camera_ubo.view, sizeof(mat4));
}

void
render_temp_set_camera_projection(mat4 proj){DPZoneScoped;
	g_scene->temp.camera_ubo.proj = proj;
	CopyMemory((u8*)graphics_buffer_mapped_data(g_scene->temp.camera_buffer) + sizeof(mat4), &g_scene->temp.camera_ubo.proj, sizeof(mat4));
}

void 
render_temp_line(vec3 start, vec3 end, color c){DPZoneScoped;
	render_temp_line_gradient(start, end, c, c);
}

void
render_temp_line_gradient(vec3 start, vec3 end, color start_color, color end_color){DPZoneScoped;
	auto vp = (RenderTempVertex*)graphics_buffer_mapped_data(g_scene->temp.wireframe.vertex_buffer) + g_scene->temp.wireframe.vertex_count;
	auto ip = (RenderTempIndex*)graphics_buffer_mapped_data(g_scene->temp.wireframe.index_buffer) + g_scene->temp.wireframe.index_count;
	
	u32 first = g_scene->temp.wireframe.vertex_count;
	
	ip[0] = first;
	ip[1] = first+1;
	vp[0] = {start, start_color.rgba};
	vp[1] = {end,   end_color.rgba};
	
	g_scene->temp.wireframe.index_count  += 2;
	g_scene->temp.wireframe.vertex_count += 2;
}

void 
render_temp_triangle(vec3 p0, vec3 p1, vec3 p2, color c){DPZoneScoped;
	auto vp = (RenderTempVertex*)graphics_buffer_mapped_data(g_scene->temp.wireframe.vertex_buffer) + g_scene->temp.wireframe.vertex_count;
	auto ip = (RenderTempIndex*)graphics_buffer_mapped_data(g_scene->temp.wireframe.index_buffer) + g_scene->temp.wireframe.index_count;
	
	u32 first = g_scene->temp.wireframe.vertex_count;
	
	ip[0] = first;
	ip[1] = first+1;
	ip[2] = first+2;
	vp[0] = {p0, c.rgba};
	vp[1] = {p1, c.rgba};
	vp[2] = {p2, c.rgba};
	
	g_scene->temp.wireframe.vertex_count += 3;
	g_scene->temp.wireframe.index_count += 3;
}

void 
render_temp_triangle_filled(vec3 p0, vec3 p1, vec3 p2, color c){DPZoneScoped;
	auto vp = (RenderTempVertex*)graphics_buffer_mapped_data(g_scene->temp.filled.vertex_buffer) + g_scene->temp.filled.vertex_count;
	auto ip = (RenderTempIndex*)graphics_buffer_mapped_data(g_scene->temp.filled.index_buffer) + g_scene->temp.filled.index_count;
	
	u32 first = g_scene->temp.filled.vertex_count;
	
	ip[0] = first;
	ip[1] = first+1;
	ip[2] = first+2;
	vp[0] = {p0, c.rgba};
	vp[1] = {p1, c.rgba};
	vp[2] = {p2, c.rgba};
	
	g_scene->temp.filled.vertex_count += 3;
	g_scene->temp.filled.index_count += 3;
}

void 
render_temp_quad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c){DPZoneScoped;
	render_temp_line(p0, p1, c);
	render_temp_line(p1, p2, c);
	render_temp_line(p2, p3, c);
	render_temp_line(p3, p0, c);
}

void 
render_temp_quad_filled(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c){DPZoneScoped;
	render_temp_triangle_filled(p0, p1, p2, c);
	render_temp_triangle_filled(p0, p2, p3, c);
}

void 
render_temp_poly(vec3* points, color c){DPZoneScoped;
	auto p = array_from(points);
	if(p.count() < 3){
		LogE("render", "render_temp_poly(): points array only contains ", p.count(), " points, but 3 are required for this function.");
		return;
	}
	
	forI(p.count()-1){
		render_temp_line(p[i], p[i+1], c);
	}
	render_temp_line(p[p.count()-1], p[0], c);
}

void 
render_temp_poly_filled(vec3* points, color c){DPZoneScoped;
	auto p = array_from(points);
	if(p.count() < 3){
		LogE("render", "render_temp_poly_filled(): points array only contains ", p.count(), " points, but 3 are required for this function.");
		return;
	}
	
	for(s32 i = 2; i < p.count() - 1; i++){
		render_temp_triangle_filled(p[i-2], p[i-1], p[i], c);
	}
	render_temp_triangle_filled(p[p.count()-3], p[p.count()-2], p[p.count()-1], c);
}

void 
render_temp_circle(vec3 pos, vec3 rot, f32 radius, u32 subdivisions_int, color c){DPZoneScoped;
	mat4 transform = mat4::TransformationMatrix(pos, rot, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i+1)*M_2PI) / subdivisions;
		f32 a1 = (f32(i  )*M_2PI) / subdivisions;
		f32 x0 = radius * cosf(a0),
		x1 = radius * cosf(a1),
		y0 = radius * sinf(a0),
		y1 = radius * sinf(a1);
		vec3 xaxis0 = (Vec4(x0, y0, 0, 1) * transform).toVec3(),
		xaxis1 = (Vec4(x1, y1, 0, 1) * transform).toVec3();
		render_temp_line(xaxis0, xaxis1, c);
	}
}

void 
render_temp_circle_filled(vec3 pos, vec3 rot, f32 radius, u32 subdivisions_int, color c){DPZoneScoped;
	mat4 transform = mat4::TransformationMatrix(pos, rot, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	auto vp = (RenderTempVertex*)graphics_buffer_mapped_data(g_scene->temp.filled.vertex_buffer) + g_scene->temp.filled.vertex_count;
	auto ip = (RenderTempIndex*)graphics_buffer_mapped_data(g_scene->temp.filled.index_buffer) + g_scene->temp.filled.index_count;
	
	u32 start = g_scene->temp.filled.vertex_count;
	
	vp[0] = {pos, c.rgba};
	f32 multiple = M_2PI/subdivisions;
	
	for(u32 i = 0; i < subdivisions_int; i += 1){
		f32 a = i * multiple;
		f32 x = radius * cosf(a),
		y = radius * sinf(a);
		vec3 p = (Vec4(x, y, 0, 1) * transform).toVec3();
		vp[i+1] = {p, c.rgba};
		ip[i*3 + 0] = start+0;
		ip[i*3 + 1] = start+i+1;
		ip[i*3 + 2] = start+i+2;
	}
	ip[3*subdivisions_int-1] = start+1;
	g_scene->temp.filled.vertex_count += subdivisions_int + 1;
	g_scene->temp.filled.index_count += 3*subdivisions_int;
}

void 
render_temp_box(mat4 transform, color c){DPZoneScoped;
	// really awful idk
	vec3 v[8] = {
		(Vec4( 0.5,  0.5,  0.5, 1)*transform).toVec3(),
		(Vec4(-0.5,  0.5,  0.5, 1)*transform).toVec3(),
		(Vec4(-0.5,  0.5, -0.5, 1)*transform).toVec3(),
		(Vec4( 0.5,  0.5, -0.5, 1)*transform).toVec3(),
		(Vec4( 0.5, -0.5,  0.5, 1)*transform).toVec3(),
		(Vec4(-0.5, -0.5,  0.5, 1)*transform).toVec3(),
		(Vec4(-0.5, -0.5, -0.5, 1)*transform).toVec3(),
		(Vec4( 0.5, -0.5, -0.5, 1)*transform).toVec3(),
	};
	
	render_temp_line(v[0], v[1], c);
	render_temp_line(v[1], v[2], c);
	render_temp_line(v[2], v[3], c);
	render_temp_line(v[3], v[0], c);
	render_temp_line(v[4], v[5], c);
	render_temp_line(v[5], v[6], c);
	render_temp_line(v[6], v[7], c);
	render_temp_line(v[7], v[4], c);
	render_temp_line(v[0], v[4], c);
	render_temp_line(v[1], v[5], c);
	render_temp_line(v[2], v[6], c);
	render_temp_line(v[3], v[7], c);
}

void render_temp_box_filled(mat4 transform, color c){DPZoneScoped;
	auto vp = (RenderTempVertex*)graphics_buffer_mapped_data(g_scene->temp.filled.vertex_buffer) + g_scene->temp.filled.vertex_count;
	auto ip = (RenderTempIndex*)graphics_buffer_mapped_data(g_scene->temp.filled.index_buffer) + g_scene->temp.filled.index_count;
	
	u32 start = g_scene->temp.filled.vertex_count;
	
	vp[0] = {(Vec4( 0.5,  0.5,  0.5, 1)*transform).toVec3(),c.rgba};
	vp[1] = {(Vec4(-0.5,  0.5,  0.5, 1)*transform).toVec3(),c.rgba};
	vp[2] = {(Vec4(-0.5,  0.5, -0.5, 1)*transform).toVec3(),c.rgba};
	vp[3] = {(Vec4( 0.5,  0.5, -0.5, 1)*transform).toVec3(),c.rgba};
	vp[4] = {(Vec4( 0.5, -0.5,  0.5, 1)*transform).toVec3(),c.rgba};
	vp[5] = {(Vec4(-0.5, -0.5,  0.5, 1)*transform).toVec3(),c.rgba};
	vp[6] = {(Vec4(-0.5, -0.5, -0.5, 1)*transform).toVec3(),c.rgba};
	vp[7] = {(Vec4( 0.5, -0.5, -0.5, 1)*transform).toVec3(),c.rgba};
	
	ip[ 0] = start+0; ip[ 1] = start+5; ip[ 2] = start+1;
	ip[ 3] = start+0; ip[ 4] = start+4; ip[ 5] = start+5;
	ip[ 6] = start+1; ip[ 7] = start+6; ip[ 8] = start+2;
	ip[ 9] = start+1; ip[10] = start+5; ip[11] = start+6;
	ip[12] = start+2; ip[13] = start+6; ip[14] = start+7;
	ip[15] = start+2; ip[16] = start+7; ip[17] = start+3;
	ip[18] = start+3; ip[19] = start+7; ip[20] = start+4;
	ip[21] = start+3; ip[22] = start+4; ip[23] = start+0;
	ip[24] = start+0; ip[25] = start+1; ip[26] = start+3;
	ip[27] = start+1; ip[28] = start+2; ip[29] = start+3;
	ip[30] = start+4; ip[31] = start+7; ip[32] = start+5;
	ip[33] = start+5; ip[34] = start+7; ip[35] = start+6;
	
	g_scene->temp.filled.vertex_count += 8;
	g_scene->temp.filled.index_count += 36;
}

void 
render_temp_sphere(vec3 pos, f32 radius, u32 segments, u32 rings, color c){DPZoneScoped;
	render_temp_circle(pos, Vec3( 0, 0, 0), radius, segments, c);
	render_temp_circle(pos, Vec3( 0,90, 0), radius, segments, c);
	render_temp_circle(pos, Vec3(90, 0, 0), radius, segments, c);
	
	// bleh. do later
	
	//	f32 dtheta = M_2PI / rings;
	//	f32 dphi = M_2PI / segments;
	//
	//	forX(r_, rings){
	//		auto r = r_ + 1; 
	//		f32 theta0 = r * dtheta,
	//			theta1 = (r + 1) * dtheta;
	//		f32 y0 = radius * cosf(theta0),
	//			y1 = radius * cosf(theta1);
	//		f32 pre0 = radius * sinf(theta0),
	//			pre1 = radius * sinf(theta1);
	//		forX(s_, segments){
	//			auto s = s_ + 1;
	//			f32 phi0 = (  s  ) * dphi,
	//				phi1 = (s + 1) * dphi;
	//			f32 x0 = pre0 * cosf(phi0),
	//				x1 = pre1 * cosf(phi1),
	//				z0 = pre0 * sinf(phi0),
	//				z1 = pre1 * sinf(phi1);
	//			Log("", Vec3(x0, y0, z0), " ", Vec3(x1, y1, z1));
	//			render_temp_line(Vec3(x0, y0, z0), Vec3(x1, y0, z1), c);
	//		}
	//	}
}

void 
render_temp_sphere_filled(vec3 pos, f32 radius, u32 segments, u32 rings, color c){DPZoneScoped;
	NotImplemented;
}

void 
render_temp_frustrum(vec3 position, vec3 target, f32 aspect_ratio, f32 fov, f32 near_z, f32 far_z, color c){DPZoneScoped;
	f32 y = tanf(Radians(fov) / 2.f);
	f32 x = y * aspect_ratio;
	f32 near_x = x * near_z;
	f32 far_x  = x * far_z;
	f32 near_y = y * near_z;
	f32 far_y  = y * far_z;
	
	vec4 faces[8] = {
		{ near_x,  near_y, near_z, 1},
		{-near_x,  near_y, near_z, 1},
		{ near_x, -near_y, near_z, 1},
		{-near_x, -near_y, near_z, 1},
		
		{ far_x,  far_y, far_z, 1},
		{-far_x,  far_y, far_z, 1},
		{ far_x, -far_y, far_z, 1},
		{-far_x, -far_y, far_z, 1},
	};
	
	mat4 mat = Math::LookAtMatrix(position, target);
	vec3 v[8] = {};
	forI(8){
		vec4 temp = faces[i] * mat;
		v[i].x = temp.x / temp.w;
		v[i].y = temp.y / temp.w;
		v[i].z = temp.z / temp.w;
	}
	
	render_temp_line(v[0], v[1], c);
	render_temp_line(v[0], v[2], c);
	render_temp_line(v[3], v[1], c);
	render_temp_line(v[3], v[2], c);
	render_temp_line(v[4], v[5], c);
	render_temp_line(v[4], v[6], c);
	render_temp_line(v[7], v[5], c);
	render_temp_line(v[7], v[6], c);
	render_temp_line(v[0], v[4], c);
	render_temp_line(v[1], v[5], c);
	render_temp_line(v[2], v[6], c);
	render_temp_line(v[3], v[7], c);
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @voxel

enum{ //voxel face order
	render_voxel_face_posx = 0,
	render_voxel_face_posy = 1,
	render_voxel_face_posz = 2,
	render_voxel_face_negx = 3,
	render_voxel_face_negy = 4,
	render_voxel_face_negz = 5,
};

enum{ //voxel face vertex order (when looking at the front of the face)
	render_voxel_face_vertex_bl = 0,
	render_voxel_face_vertex_tl = 1,
	render_voxel_face_vertex_tr = 2,
	render_voxel_face_vertex_br = 3,
};

local RenderVoxelType* render_voxel_types;
local u64 render_voxel_types_count;
local RenderVoxelChunk* render_voxel_chunk_pool;
local u32 render_voxel_voxel_size; //width, height, and depth of a voxel in the world

local vec3 render_voxel_unit_vertex_offsets[6][4] = { //unit vertex offsets by face
	{ { 1,0,0 }, { 1,1,0 }, { 1,1,1 }, { 1,0,1 } }, //render_voxel_face_posx
	{ { 0,1,0 }, { 0,1,1 }, { 1,1,1 }, { 1,1,0 } }, //render_voxel_face_posy
	{ { 1,0,1 }, { 1,1,1 }, { 0,1,1 }, { 0,0,1 } }, //render_voxel_face_posz
	{ { 0,0,1 }, { 0,1,1 }, { 0,1,0 }, { 0,0,0 } }, //render_voxel_face_negx
	{ { 0,0,1 }, { 0,0,0 }, { 1,0,0 }, { 1,0,1 } }, //render_voxel_face_negy
	{ { 0,0,0 }, { 0,1,0 }, { 1,1,0 }, { 1,0,0 } }, //render_voxel_face_negz
};

local vec3 render_voxel_face_normals[6] = {
	vec3_RIGHT(),
	vec3_LEFT(),
	vec3_UP(),
	vec3_DOWN(),
	vec3_FORWARD(),
	vec3_BACK(),
};

//NOTE(delle) voxels are linearly laid out like x-y plane at z0, x-y plane at z1, ... with x being the major axis in the x-y plane
#define render_voxel_linear(dims,x,y,z)  (((z) * (dims) * (dims)) + ((y) * (dims)) + (x))
#define render_voxel_right(dims,linear)  ((linear) + 1)
#define render_voxel_left(dims,linear)   ((linear) - 1)
#define render_voxel_above(dims,linear)  ((linear) + (dims))
#define render_voxel_below(dims,linear)  ((linear) - (dims))
#define render_voxel_front(dims,linear)  ((linear) + ((dims)*(dims)))
#define render_voxel_behind(dims,linear) ((linear) - ((dims)*(dims)))

void
render_voxel_init(RenderVoxelType* types, u64 count, u32 voxel_size){DPZoneScoped;
	render_voxel_types = types;
	render_voxel_types_count = count;
	memory_pool_init(render_voxel_chunk_pool, 128);
	for_pool(render_voxel_chunk_pool) it->hidden = true;
	render_voxel_voxel_size = voxel_size;
}


void
render_voxel_make_face_mesh(int direction, RenderVoxelChunk* chunk, RenderVoxel* voxel, MeshVertex* vertex_array, u64* vertex_count, MeshIndex* index_array, u64* index_count){DPZoneScoped;
	vec3 voxel_position = chunk->position + Vec3(voxel->x, voxel->y, voxel->z);
	mat4 transform = mat4::TransformationMatrix(voxel_position, chunk->rotation, vec3_ONE());
	
	vertex_array[*vertex_count+0] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_bl] * transform,
		Vec2(0,0),
		ByteSwap32(render_voxel_types[voxel->type].color.rgba),
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+1] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_tl] * transform,
		Vec2(0,1),
		ByteSwap32(render_voxel_types[voxel->type].color.rgba),
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+2] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_tr] * transform,
		Vec2(1,1),
		ByteSwap32(render_voxel_types[voxel->type].color.rgba),
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+3] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_br] * transform,
		Vec2(1,0),
		ByteSwap32(render_voxel_types[voxel->type].color.rgba),
		render_voxel_face_normals[direction] * transform
	};
	
	index_array[*index_count+0] = *vertex_count+0;
	index_array[*index_count+1] = *vertex_count+1;
	index_array[*index_count+2] = *vertex_count+2;
	index_array[*index_count+3] = *vertex_count+0;
	index_array[*index_count+4] = *vertex_count+2;
	index_array[*index_count+5] = *vertex_count+3;
	
	*vertex_count += 4;
	*index_count  += 6;
}


RenderVoxelChunk*
render_voxel_chunk_create(vec3 position, vec3 rotation, u32 dimensions, RenderVoxel* voxels, u64 voxels_count){DPZoneScoped;
	Assert(dimensions != 0, "Dimensions can not be zero!");
	Assert(voxels != 0 && voxels_count != 0, "Don't call this with an invalid voxels array!");
	
	//alloc and init chunk
	RenderVoxelChunk* chunk = memory_pool_push(g_scene->pools.voxel_chunks);
	chunk->position    = position;
	chunk->rotation    = rotation;
	chunk->dimensions  = dimensions;
	chunk->modified    = false;
	chunk->hidden      = false;
	chunk->voxel_count = voxels_count;
	
	//calculate some chunk creation info
	upt dimensions_cubed    = dimensions * dimensions * dimensions;
	upt dimensions_stride_x = 1;
	upt dimensions_stride_y = dimensions;
	upt dimensions_stride_z = dimensions * dimensions;
	
	//alloc an arena for chunk creation
	upt array_header_size = sizeof(stbds_array_header);
	upt voxels_array_size = dimensions_cubed * sizeof(RenderVoxel*);
	upt max_vertices_size = dimensions_cubed * 24 * sizeof(MeshVertex);
	upt max_indices_size  = dimensions_cubed * 36 * sizeof(MeshIndex);
	chunk->arena = memory_create_arena(voxels_array_size + max_vertices_size + max_indices_size);
	
	//init voxels array
	chunk->voxels = (RenderVoxel**)memory_arena_push(chunk->arena,voxels_array_size);
	ZeroMemory(chunk->voxels, dimensions_cubed * sizeof(RenderVoxel*));
	for(RenderVoxel* it = voxels; it < voxels+voxels_count; ++it){
		chunk->voxels[render_voxel_linear(dimensions, it->x, it->y, it->z)] = it;
	}
	
	//generate chunk's mesh
	//TODO(delle) combine faces across the chunk where possible
	MeshVertex* vertex_array = (MeshVertex*)memory_arena_push(chunk->arena,max_vertices_size);
	MeshIndex*  index_array  =  (MeshIndex*)memory_arena_push(chunk->arena,max_indices_size);
	u32 dimensions_minus_one = dimensions-1;
	forI(dimensions_cubed){
		if(chunk->voxels[i] == 0) continue; //skip empty voxels
		
		if((chunk->voxels[i]->x == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_x] == 0)){
			render_voxel_make_face_mesh(render_voxel_face_posx, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		}
		if((chunk->voxels[i]->x == 0)                    || (chunk->voxels[i - dimensions_stride_x] == 0)){
			render_voxel_make_face_mesh(render_voxel_face_negx, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		}
		if((chunk->voxels[i]->y == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_y] == 0)){
			render_voxel_make_face_mesh(render_voxel_face_posy, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		}
		if((chunk->voxels[i]->y == 0)                    || (chunk->voxels[i - dimensions_stride_y] == 0)){
			render_voxel_make_face_mesh(render_voxel_face_negy, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		}
		if((chunk->voxels[i]->z == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_z] == 0)){
			render_voxel_make_face_mesh(render_voxel_face_posz, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		}
		if((chunk->voxels[i]->z == 0)                    || (chunk->voxels[i - dimensions_stride_z] == 0)){
			render_voxel_make_face_mesh(render_voxel_face_negz, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		}
	}
	
	//shift the index_array to the end of the vertex_array
	upt actual_vertices_size = chunk->vertex_count*sizeof(MeshVertex);
	upt actual_indices_size  =  chunk->index_count*sizeof(MeshIndex);
	MeshIndex* new_index_array = (MeshIndex*)((u8*)vertex_array + actual_vertices_size);
	CopyMemory(new_index_array, index_array, actual_indices_size);
	index_array = new_index_array;
	
	//fit the arena to its actually used size
	chunk->arena->used = voxels_array_size + actual_vertices_size + actual_indices_size;
	memory_arena_fit(chunk->arena);
	
	//create the vertex/index GPU buffers and upload the vertex/index data to them
	chunk->vertex_buffer = graphics_buffer_create(vertex_array, actual_vertices_size, GraphicsBufferUsage_VertexBuffer,
												  GraphicsMemoryProperty_DeviceMappable, GraphicsMemoryMapping_Occasional);
	chunk->index_buffer  = graphics_buffer_create(index_array,  actual_indices_size,  GraphicsBufferUsage_IndexBuffer,
												  GraphicsMemoryProperty_DeviceMappable, GraphicsMemoryMapping_Occasional);
	
	return chunk;
}


void
render_voxel_delete_chunk(RenderVoxelChunk* chunk){DPZoneScoped;
	//dealloc GPU buffers
	graphics_buffer_destroy(chunk->vertex_buffer);
	graphics_buffer_destroy(chunk->index_buffer);
	
	//dealloc chunk arena
	memory_delete_arena(chunk->arena);
	
	//delete the chunk (and set it to hidden since for_pool() doesn't skip deleted chunks)
	memory_pool_delete(render_voxel_chunk_pool, chunk);
	chunk->hidden = true;
}
