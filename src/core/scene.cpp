#define SceneAssert(cond, ...) do { if(!cond) { LogE("scene", __FUNCTION__, ": assertion failed `", STRINGIZE(cond), "`: ", __VA_ARGS__); Assert(0); } } while(0); 
#define SceneError(...) LogE("scene", __VA_ARGS__)

SceneGlobal __deshi__g_scene;
SceneGlobal* g_scene = &__deshi__g_scene;

void 
scene_init() {
	*g_scene = {};
	memory_pool_init(g_scene->pools.camera, 4);
	array_init(g_scene->model_draw_commands, 4, deshi_allocator);
}

void
scene_render() {
	SceneAssert(g_scene->active.window, "need a window to render to.");
	SceneAssert(g_scene->active.camera, "need a camera to render from.");
	
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
	
		forI(array_count(g_scene->model_draw_commands)) {
			auto cmd = g_scene->model_draw_commands[i];
			bind_vertex_buffer(win, cmd.model->mesh->vertex_buffer);
			bind_index_buffer(win, cmd.model->mesh->index_buffer);
			forI(arrlenu(cmd.model->batch_array)) {
				auto b = cmd.model->batch_array[i];
				if(!b.index_count) continue;
				if(b.material->pipeline != last_pipeline) {
					last_pipeline = b.material->pipeline;
					bind_pipeline(win, last_pipeline);
					set_viewport(win, vec2::ZERO, win->dimensions.toVec2());
					set_scissor(win, vec2::ZERO, win->dimensions.toVec2());
					bind_descriptor_set(win, 0, g_assets->view_proj_ubo);
				}
				push_constant(win, cmd.transform, {GraphicsShaderStage_Vertex, sizeof(mat4), 0});
				bind_descriptor_set(win, 1, b.material->descriptor_set);
				draw_indexed(win, b.index_count, b.index_offset, 0);
			}
		}	

		end_render_pass(win);
	
		if(g_scene->temp.render_pass) {
			g_scene->temp.camera_ubo.proj = g_scene->active.camera->proj;
			g_scene->temp.camera_ubo.view = g_scene->active.camera->view;
			CopyMemory(graphics_buffer_mapped_data(g_scene->temp.camera_buffer), &g_scene->temp.camera_ubo, sizeof(g_scene->temp.camera_ubo));

			begin_render_pass(win, g_scene->temp.render_pass, frame);

			if(g_scene->temp.wireframe.vertex_count) {
				bind_vertex_buffer(win, g_scene->temp.wireframe.vertex_buffer);
				bind_index_buffer(win, g_scene->temp.wireframe.index_buffer);
				bind_pipeline(win, g_scene->temp.wireframe.pipeline);
				bind_descriptor_set(win, 0, g_scene->temp.descriptor_set);
				set_viewport(win, vec2::ZERO, win->dimensions.toVec2());
				set_scissor(win, vec2::ZERO, win->dimensions.toVec2());
				draw_indexed(win, g_scene->temp.wireframe.index_count, 0, 0);
			}		

			if(g_scene->temp.filled.vertex_count) {
				bind_vertex_buffer(win, g_scene->temp.wireframe.vertex_buffer);
				bind_index_buffer(win, g_scene->temp.wireframe.index_buffer);
				bind_pipeline(win, g_scene->temp.wireframe.pipeline);
				bind_descriptor_set(win, 0, g_scene->temp.descriptor_set);
				set_viewport(win, vec2::ZERO, win->dimensions.toVec2());
				set_scissor(win, vec2::ZERO, win->dimensions.toVec2());
				draw_indexed(win, g_scene->temp.wireframe.index_count, 0, 0);
			}
			end_render_pass(win);
		}

	}



	array_clear(g_scene->model_draw_commands);
}

void
scene_set_active_window(Window* window) {
	g_scene->active.window = window;
}

void
scene_set_active_camera(Camera* camera) {
	g_scene->active.camera = camera;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @camera


Camera*
scene_camera_create() {
	auto out = memory_pool_push(g_scene->pools.camera);
	return out;
}

void
scene_camera_update_view(Camera* camera) {
	SceneAssert(camera, "passed null Camera pointer.");
	camera->right = camera->forward.cross(vec3::UP).normalized();
	camera->up = camera->forward.cross(camera->right).normalized();
	camera->view = Math::LookAtMatrix(camera->position, camera->position + camera->forward);
}

void
scene_camera_update_perspective_projection(Camera* camera, u32 width, u32 height, f32 fov, f32 near_z, f32 far_z) {
	SceneAssert(camera, "passed null Camera pointer.");
	camera->proj = Math::PerspectiveProjectionMatrix(width, height, fov, near_z, far_z);
}

void
scene_camera_update_orthographic_projection(Camera* camera, f32 right, f32 left, f32 top, f32 bottom, f32 far, f32 near_) {
	SceneAssert(camera, "passed null Camera pointer.");
	f32 A =  2 / (right - left),
		B =  2 / (top - bottom),
		C = -2 / (far - near_),
		D = -(right + left) / (right - left),
		E = -(top + bottom) / (top - bottom),
		F = -(far + near_) / (far - near_);
	camera->proj = mat4(
		A, 0, 0, D,
		0, B, 0, E,
		0, 0, C, F,
		0, 0, 0, 1
	);
}

void
scene_camera_destroy(Camera* camera) {
	memory_pool_delete(g_scene->pools.camera, camera);
}

void
scene_camera_draw_frustrum(Camera* camera) {
	// TODO(sushi) implement by transforming 4 points out based on the proj and view matrices
	NotImplemented;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @model


void 
scene_draw_model(Model* model, mat4* transform) {
	SceneAssert(model, "passed null Model pointer.");
	auto cmd = array_push(g_scene->model_draw_commands);
	cmd->model = model;
	cmd->transform = transform;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @temp


void
render_temp_init(Window* window, u32 v) {
	if(v > MAX_U32/3) {
		SceneError("the given vertex count (", v, ") is too large to allow 3*vertex_count indexes. The max amount of supported vertexes is ", MAX_U32 / 3);
		return;
	}

	g_scene->temp.filled = {};
	g_scene->temp.filled.vertex_buffer = graphics_buffer_create(0, v * sizeof(RenderTempVertex), GraphicsBufferUsage_VertexBuffer, GraphicsMemoryPropertyFlag_HostCoherent, GraphicsMemoryMapping_Persistent);
	g_scene->temp.filled.index_buffer  = graphics_buffer_create(0, v * sizeof(RenderTempIndex), GraphicsBufferUsage_IndexBuffer, GraphicsMemoryPropertyFlag_HostCoherent, GraphicsMemoryMapping_Persistent);
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
	rp->depth_attachment.         load_op = GraphicsLoadOp_Clear;
	rp->depth_attachment.        store_op = GraphicsStoreOp_Store;
	rp->depth_attachment. stencil_load_op = GraphicsLoadOp_Clear;
	rp->depth_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	rp->depth_attachment.  initial_layout = GraphicsImageLayout_Undefined;
	rp->depth_attachment.    final_layout = GraphicsImageLayout_Depth_Stencil_Attachment_Optimal;
	graphics_render_pass_update(rp);

	auto pl = g_scene->temp.filled.pipeline = graphics_pipeline_allocate();
	pl->            debug_name = str8l("<render> temp pipeline");
	pl->            front_face = GraphicsFrontFace_CCW;
	pl->               culling = GraphicsPipelineCulling_Back;
	pl->          polygon_mode = GraphicsPolygonMode_Fill;
	pl->            depth_test = true;
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
	array_init_with_elements(pl->shader_stages, {
			{
				GraphicsShaderStage_Vertex,
				str8l("temp.vert"),
				file_read_simple(str8l("data/shaders/temp.vert"), deshi_temp_allocator)
			},
			{
				GraphicsShaderStage_Fragment,
				str8l("temp.frag"),
				file_read_simple(str8l("data/shaders/temp.frag"), deshi_temp_allocator)
			}});
	array_init_with_elements(pl->vertex_input_bindings, {
			{0, sizeof(RenderTempVertex)}});
	array_init_with_elements(pl->vertex_input_attributes, {
			{0, 0, GraphicsFormat_R32G32B32_Float, offsetof(RenderTempVertex, pos)},
			{1, 0, GraphicsFormat_R8G8B8A8_UNorm,  offsetof(RenderTempVertex, color)}});
	
	g_scene->temp.camera_buffer = graphics_buffer_create(&g_scene->temp.camera_ubo, sizeof(g_scene->temp.camera_ubo), GraphicsBufferUsage_UniformBuffer, GraphicsMemoryPropertyFlag_HostCoherent, GraphicsMemoryMapping_Persistent);
	
	auto tl = graphics_pipeline_layout_allocate();
	tl->debug_name = str8l("<render> temp pipeline layout");
	auto dl = graphics_descriptor_set_layout_allocate();
	array_init_with_elements(dl->bindings, {{
				RenderDescriptorType_Uniform_Buffer,
				RenderShaderStage_Vertex,
				0
			}});
	graphics_descriptor_set_layout_update(dl);
	array_init_with_elements(tl->descriptor_layouts, {dl});
	graphics_pipeline_layout_update(tl);

	pl->layout = tl;
	graphics_pipeline_update(pl);
	
	auto wpl = g_scene->temp.wireframe.pipeline = graphics_pipeline_duplicate(pl);
	wpl->debug_name = str8l("<render> temp wireframe pipeline");
	wpl->polygon_mode = GraphicsPolygonMode_Line;
	wpl->culling = GraphicsPipelineCulling_None;
	wpl->depth_test = false;
	wpl->color_blend = false;
	graphics_pipeline_update(wpl);

	auto ds = g_scene->temp.descriptor_set = graphics_descriptor_set_allocate();
	ds->debug_name = str8l("<render> temp descriptor set");
	ds->layouts = array_copy(pl->layout->descriptor_layouts).ptr;
	graphics_descriptor_set_update(ds);

	GraphicsDescriptor descriptor = {};
	descriptor.type = GraphicsDescriptorType_Uniform_Buffer;
	descriptor.ubo = {
		g_scene->temp.camera_buffer,
		0,
		sizeof(g_scene->temp.camera_ubo)
	};
	graphics_descriptor_set_write(ds, 0, descriptor);
}

void 
render_temp_clear(){
	g_scene->temp.wireframe.vertex_count =
		g_scene->temp.wireframe.index_count =
		g_scene->temp.filled.vertex_count =
		g_scene->temp.filled.index_count = 0;
}

void 
render_temp_update_camera(vec3 position, vec3 target){
	g_scene->temp.camera_ubo.view = Math::LookAtMatrix(position, target).Inverse();
	CopyMemory(graphics_buffer_mapped_data(g_scene->temp.camera_buffer), &g_scene->temp.camera_ubo.view, sizeof(mat4));
}

void
render_temp_set_camera_projection(mat4 proj){
	g_scene->temp.camera_ubo.proj = proj;
	CopyMemory((u8*)graphics_buffer_mapped_data(g_scene->temp.camera_buffer) + sizeof(mat4), &g_scene->temp.camera_ubo.proj, sizeof(mat4));
}

void 
render_temp_line(vec3 start, vec3 end, color c){
	auto vp = (RenderTempVertex*)graphics_buffer_mapped_data(g_scene->temp.wireframe.vertex_buffer) + g_scene->temp.wireframe.vertex_count;
	auto ip = (RenderTempIndex*)graphics_buffer_mapped_data(g_scene->temp.wireframe.index_buffer) + g_scene->temp.wireframe.index_count;
	
	u32 first = g_scene->temp.wireframe.vertex_count;
	
	ip[0] = first;
	ip[1] = first+1;
	ip[2] = first;
	vp[0] = {start, c.rgba};
	vp[1] = {end,   c.rgba};
	
	g_scene->temp.wireframe.index_count  += 3;
	g_scene->temp.wireframe.vertex_count += 2;
}

void 
render_temp_triangle(vec3 p0, vec3 p1, vec3 p2, color c){
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
render_temp_triangle_filled(vec3 p0, vec3 p1, vec3 p2, color c){
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
render_temp_quad(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c){
	render_temp_line(p0, p1, c);
	render_temp_line(p1, p2, c);
	render_temp_line(p2, p3, c);
	render_temp_line(p3, p0, c);
}

void 
render_temp_quad_filled(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c){
	render_temp_triangle_filled(p0, p1, p2, c);
	render_temp_triangle_filled(p0, p2, p3, c);
}

void 
render_temp_poly(vec3* points, color c){
	auto p = array_from(points);
	if(p.count() < 3){
		LogE("render", "render_temp_poly(): points array only contains ", p.count(), " points, but 3 are required for this function.");
		return;
	}
	
	forI(p.count()-1) 
		render_temp_line(p[i], p[i+1], c);
	render_temp_line(p[p.count()-1], p[0], c);
}

void 
render_temp_poly_filled(vec3* points, color c){
	auto p = array_from(points);
	if(p.count() < 3){
		LogE("render", "render_temp_poly_filled(): points array only contains ", p.count(), " points, but 3 are required for this function.");
		return;
	}
	
	for(s32 i = 2; i < p.count() - 1; i++)  {
		render_temp_triangle_filled(p[i-2], p[i-1], p[i], c);
	}
	render_temp_triangle_filled(p[p.count()-3], p[p.count()-2], p[p.count()-1], c);
}

void 
render_temp_circle(vec3 pos, vec3 rot, f32 radius, u32 subdivisions_int, color c){
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
render_temp_circle_filled(vec3 pos, vec3 rot, f32 radius, u32 subdivisions_int, color c){
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
render_temp_box(mat4 transform, color c){
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
void render_temp_box_filled(mat4 transform, color c){
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
render_temp_sphere(vec3 pos, f32 radius, u32 segments, u32 rings, color c){
	NotImplemented;
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
render_temp_sphere_filled(vec3 pos, f32 radius, u32 segments, u32 rings, color c){
	NotImplemented;
}

void 
render_temp_frustrum(vec3 position, vec3 target, f32 aspect_ratio, f32 fov, f32 near_z, f32 far_z, color c){
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


#define VoxelError(...) LogE("scene.voxels", __FUNCTION__, "(): ", __VA_ARGS__)
#define VoxelWarning(...) LogW("scene.voxels", __FUNCTION__, "(): ", __VA_ARGS__)

namespace __deshi_voxels {

SceneVoxelChunk* chunk_pool;
array<SceneVoxelType> types;
u32 size;

// stupid people polluting my damn namespace
#undef index
FORCE_INLINE SceneVoxel* 
index(SceneVoxelChunk* chunk, vec3i pos) { 
	if(pos.x > chunk->dimensions || pos.y > chunk->dimensions || pos.z > chunk->dimensions) return 0;
	return chunk->voxels[pos.z * chunk->dimensions * chunk->dimensions + pos.y * chunk->dimensions + pos.x]; 
}

FORCE_INLINE SceneVoxel* right(SceneVoxelChunk* chunk, vec3i pos)   { return index(chunk, vec3i{pos.x + 1, pos.y, pos.z}); }
FORCE_INLINE SceneVoxel* left(SceneVoxelChunk* chunk, vec3i pos)    { return index(chunk, vec3i{pos.x - 1, pos.y, pos.z}); }
FORCE_INLINE SceneVoxel* above(SceneVoxelChunk* chunk, vec3i pos)   { return index(chunk, vec3i{pos.x, pos.y + 1, pos.z}); }
FORCE_INLINE SceneVoxel* below(SceneVoxelChunk* chunk, vec3i pos)   { return index(chunk, vec3i{pos.x, pos.y - 1, pos.z}); }
FORCE_INLINE SceneVoxel* forward(SceneVoxelChunk* chunk, vec3i pos) { return index(chunk, vec3i{pos.x, pos.y, pos.z + 1}); }
FORCE_INLINE SceneVoxel* behind(SceneVoxelChunk* chunk, vec3i pos)  { return index(chunk, vec3i{pos.x, pos.y, pos.z - 1}); }

} // namespace __deshi_voxels
#define voxel __deshi_voxels

void
scene_voxel_init(SceneVoxelType* types, u32 voxel_size) {
	voxel::types = array_from(types);
	if(!voxel::types.count()) {
		LogE("scene.voxels", "given types array is empty");
		return;
	}

	memory_pool_init(voxel::chunk_pool, 128);
	for_pool(voxel::chunk_pool) it->hidden = true;
	
	voxel::size = voxel_size;
}

SceneVoxelChunk*
scene_voxel_chunk_create(vec3 pos, vec3 rot, u32 dimensions, SceneVoxel* voxels) {
	if(!dimensions) {
		VoxelError("given dimensions is zero. Chunks are statically sized, so this does not make sense.");
		return 0;
	}

	const u64 dimensions_cubed = dimensions * dimensions * dimensions;
	
	auto chunk = memory_pool_push(voxel::chunk_pool);
	chunk->position = pos;
	chunk->rotation = rot;
	chunk->dimensions = dimensions;
	chunk->modified = false;
	chunk->hidden = false;

	NotImplemented;
	return 0;
}






#undef voxels
