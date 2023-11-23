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
