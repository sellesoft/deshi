GraphicsGlobal  __deshi_graphics;
GraphicsGlobal* g_graphics;


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @buffer


u64
graphics_buffer_device_size(GraphicsBuffer* x) {
	return GRAPHICS_INTERNAL(x).size;
}

void*
graphics_buffer_mapped_data(GraphicsBuffer* x) {
	return GRAPHICS_INTERNAL(x).mapped.data;
}

u64
graphics_buffer_mapped_size(GraphicsBuffer* x) {
	return GRAPHICS_INTERNAL(x).mapped.size;
}

u64
graphics_buffer_mapped_offset(GraphicsBuffer* x) {
	return GRAPHICS_INTERNAL(x).mapped.offset;
}

void 
graphics_cmd_begin_render_pass(Window* window, GraphicsRenderPass* render_pass, GraphicsFramebuffer* frame) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Push_Constant;
	c->begin_render_pass.pass = render_pass;
	c->begin_render_pass.frame = frame;
}

void 
graphics_cmd_end_render_pass(Window* window) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_End_Render_Pass;
}

void 
graphics_cmd_bind_pipeline(Window* window, GraphicsPipeline* pipeline) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Bind_Pipeline;
	c->bind_pipeline.handle = pipeline;
}

void 
graphics_cmd_bind_vertex_buffer(Window* window, GraphicsBuffer* buffer) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Bind_Vertex_Buffer;
	c->bind_vertex_buffer.handle = buffer;
}

void 
graphics_cmd_bind_index_buffer(Window* window, GraphicsBuffer* buffer) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Bind_Index_Buffer;
	c->bind_index_buffer.handle = buffer;
}

void 
graphics_cmd_bind_descriptor_set(Window* window, u32 set_index, GraphicsDescriptorSet* descriptor_set) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Bind_Descriptor_Set;
	c->bind_descriptor_set.handle = descriptor_set;
	c->bind_descriptor_set.set_index = set_index;
}

void 
graphics_cmd_push_constant(Window* window, void* data, GraphicsPushConstant info) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Push_Constant;
	c->push_constant.data = data;
	c->push_constant.info = info;
}

void 
graphics_cmd_draw_indexed(Window* window, u32 index_count, u32 index_offset, u32 vertex_offset) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Draw_Indexed;
	c->draw_indexed.index_count = index_count;
	c->draw_indexed.index_offset = index_offset;
	c->draw_indexed.vertex_offset = vertex_offset;
}

void 
graphics_cmd_set_viewport(Window* window, vec2 offset, vec2 extent) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Set_Viewport;
	c->set_viewport.offset = offset;
	c->set_viewport.extent = extent;
}

void 
graphics_cmd_set_scissor(Window* window, vec2 offset, vec2 extent) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Set_Scissor;
	c->set_scissor.offset = offset;
	c->set_scissor.extent = extent;
}

void
graphics_cmd_set_depth_bias(Window* window, f32 constant, f32 clamp, f32 slope) {
	auto c = array_push(graphics_command_buffer_of_window(window)->commands);
	c->type = GraphicsCommandType_Set_Depth_Bias;
	c->set_depth_bias.constant = constant;
	c->set_depth_bias.clamp = clamp;
	c->set_depth_bias.slope = slope;
}