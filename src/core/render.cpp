//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_status
RenderStats    renderStats{};
RenderStage    renderStage = RENDERERSTAGE_NONE;
RenderSettings renderSettings;
local ConfigMapItem  renderConfigMap[] = {
	{str8_lit("#render settings config file"),0,0},
	
	{str8_lit("\n#    //// REQUIRES RESTART ////"),  ConfigValueType_PADSECTION,(void*)21},
	{str8_lit("debugging"),            ConfigValueType_B32, &renderSettings.debugging},
	{str8_lit("printf"),               ConfigValueType_B32, &renderSettings.printf},
	{str8_lit("texture_filtering"),    ConfigValueType_B32, &renderSettings.textureFiltering},
	{str8_lit("anistropic_filtering"), ConfigValueType_B32, &renderSettings.anistropicFiltering},
	{str8_lit("msaa_level"),           ConfigValueType_U32, &renderSettings.msaaSamples},
	{str8_lit("recompile_all_shaders"),ConfigValueType_B32, &renderSettings.recompileAllShaders},
	
	{str8_lit("\n#    //// RUNTIME VARIABLES ////"), ConfigValueType_PADSECTION,(void*)15},
	{str8_lit("logging_level"),  ConfigValueType_U32, &renderSettings.loggingLevel},
	{str8_lit("crash_on_error"), ConfigValueType_B32, &renderSettings.crashOnError},
	{str8_lit("vsync_type"),     ConfigValueType_U32, &renderSettings.vsync},
	
	{str8_lit("\n#shaders"),                         ConfigValueType_PADSECTION,(void*)17},
	{str8_lit("optimize_shaders"), ConfigValueType_B32, &renderSettings.optimizeShaders},
	
	{str8_lit("\n#shadows"),                         ConfigValueType_PADSECTION,(void*)20},
	{str8_lit("shadow_pcf"),          ConfigValueType_B32, &renderSettings.shadowPCF},
	{str8_lit("shadow_resolution"),   ConfigValueType_U32, &renderSettings.shadowResolution},
	{str8_lit("shadow_nearz"),        ConfigValueType_F32, &renderSettings.shadowNearZ},
	{str8_lit("shadow_farz"),         ConfigValueType_F32, &renderSettings.shadowFarZ},
	{str8_lit("depth_bias_constant"), ConfigValueType_F32, &renderSettings.depthBiasConstant},
	{str8_lit("depth_bias_slope"),    ConfigValueType_F32, &renderSettings.depthBiasSlope},
	{str8_lit("show_shadow_map"),     ConfigValueType_B32, &renderSettings.showShadowMap},
	
	{str8_lit("\n#colors"),                          ConfigValueType_PADSECTION,(void*)15},
	{str8_lit("clear_color"),    ConfigValueType_FV4, &renderSettings.clearColor},
	{str8_lit("selected_color"), ConfigValueType_FV4, &renderSettings.selectedColor},
	{str8_lit("collider_color"), ConfigValueType_FV4, &renderSettings.colliderColor},
	
	{str8_lit("\n#filters"),                         ConfigValueType_PADSECTION,(void*)15},
	{str8_lit("wireframe_only"), ConfigValueType_B32, &renderSettings.wireframeOnly},
	
	{str8_lit("\n#overlays"),                        ConfigValueType_PADSECTION,(void*)17},
	{str8_lit("mesh_wireframes"),  ConfigValueType_B32, &renderSettings.meshWireframes},
	{str8_lit("mesh_normals"),     ConfigValueType_B32, &renderSettings.meshNormals},
	{str8_lit("light_frustrums"),  ConfigValueType_B32, &renderSettings.lightFrustrums},
	{str8_lit("temp_mesh_on_top"), ConfigValueType_B32, &renderSettings.tempMeshOnTop},
};


void
render_save_settings(){
	config_save(str8_lit("data/cfg/render.cfg"), renderConfigMap, ArrayCount(renderConfigMap));
}

void
render_load_settings(){
	config_load(str8_lit("data/cfg/render.cfg"), renderConfigMap, ArrayCount(renderConfigMap));
}

RenderSettings*
render_get_settings(){
	return &renderSettings;
}

RenderStats*
render_get_stats(){
	return &renderStats;
}

RenderStage*
render_get_stage(){
	return &renderStage;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_buffer
RenderBuffer* deshi__render_buffer_pool;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_surface
#define MAX_SURFACES 2
u32 renderActiveSurface = 0;

u32
render_max_surface_count(){
	return MAX_SURFACES;
}

//-///////////////////////////////////////////////////////////////////////////////////////////////
// @render_shared_temp

void 
render_temp_clear(){
	g_render.temp_wireframe.vertex_count =
		g_render.temp_wireframe.index_count =
		g_render.temp_filled.vertex_count =
		g_render.temp_filled.index_count = 0;
}

void 
render_temp_update_camera(vec3 position, vec3 target){
	g_render.temp_camera_ubo.view = Math::LookAtMatrix(position, target).Inverse();
	CopyMemory(g_render.temp_camera_buffer->mapped_data, &g_render.temp_camera_ubo.view, sizeof(mat4));
}

void
render_temp_set_camera_projection(mat4 proj){
	g_render.temp_camera_ubo.proj = proj;
	CopyMemory((u8*)g_render.temp_camera_buffer->mapped_data + sizeof(mat4), &g_render.temp_camera_ubo.proj, sizeof(mat4));
}

void 
render_temp_line(vec3 start, vec3 end, color c){
	auto vp = (RenderTempVertex*)g_render.temp_wireframe.vertex_buffer->mapped_data + g_render.temp_wireframe.vertex_count;
	auto ip = (RenderTempIndex*)g_render.temp_wireframe.index_buffer->mapped_data + g_render.temp_wireframe.index_count;
	
	u32 first = g_render.temp_wireframe.vertex_count;
	
	ip[0] = first;
	ip[1] = first+1;
	ip[2] = first;
	vp[0] = {start, c.rgba};
	vp[1] = {end,   c.rgba};
	
	g_render.temp_wireframe.index_count  += 3;
	g_render.temp_wireframe.vertex_count += 2;
}

void 
render_temp_triangle(vec3 p0, vec3 p1, vec3 p2, color c){
	auto vp = (RenderTempVertex*)g_render.temp_wireframe.vertex_buffer->mapped_data + g_render.temp_wireframe.vertex_count;
	auto ip = (RenderTempIndex*)g_render.temp_wireframe.index_buffer->mapped_data + g_render.temp_wireframe.index_count;
	
	u32 first = g_render.temp_wireframe.vertex_count;
	
	ip[0] = first;
	ip[1] = first+1;
	ip[2] = first+2;
	vp[0] = {p0, c.rgba};
	vp[1] = {p1, c.rgba};
	vp[2] = {p2, c.rgba};
	
	g_render.temp_wireframe.vertex_count += 3;
	g_render.temp_wireframe.index_count += 3;
}

void 
render_temp_triangle_filled(vec3 p0, vec3 p1, vec3 p2, color c){
	auto vp = (RenderTempVertex*)g_render.temp_filled.vertex_buffer->mapped_data + g_render.temp_filled.vertex_count;
	auto ip = (RenderTempIndex*)g_render.temp_filled.index_buffer->mapped_data + g_render.temp_filled.index_count;
	
	u32 first = g_render.temp_filled.vertex_count;
	
	ip[0] = first;
	ip[1] = first+1;
	ip[2] = first+2;
	vp[0] = {p0, c.rgba};
	vp[1] = {p1, c.rgba};
	vp[2] = {p2, c.rgba};
	
	g_render.temp_filled.vertex_count += 3;
	g_render.temp_filled.index_count += 3;
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
	auto vp = (RenderTempVertex*)g_render.temp_filled.vertex_buffer->mapped_data + g_render.temp_filled.vertex_count;
	auto ip = (RenderTempIndex*)g_render.temp_filled.index_buffer->mapped_data + g_render.temp_filled.index_count;
	
	u32 start = g_render.temp_filled.vertex_count;
	
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
	g_render.temp_filled.vertex_count += subdivisions_int + 1;
	g_render.temp_filled.index_count += 3*subdivisions_int;
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
	auto vp = (RenderTempVertex*)g_render.temp_filled.vertex_buffer->mapped_data + g_render.temp_filled.vertex_count;
	auto ip = (RenderTempIndex*)g_render.temp_filled.index_buffer->mapped_data + g_render.temp_filled.index_count;
	
	u32 start = g_render.temp_filled.vertex_count;
	
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
	
	g_render.temp_filled.vertex_count += 8;
	g_render.temp_filled.index_count += 36;
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

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_draw_3d
#define MAX_TEMP_VERTICES 0xFFFFF //max u16: 65535
#define MAX_TEMP_INDICES 3*MAX_TEMP_VERTICES
RenderTempIndex  renderTempWireframeVertexCount = 0;
RenderTempIndex  renderTempWireframeIndexCount  = 0;
MeshVertex       renderTempWireframeVertexArray[MAX_TEMP_VERTICES];
RenderTempIndex  renderTempWireframeIndexArray[MAX_TEMP_INDICES];
RenderTempIndex  renderTempFilledVertexCount    = 0;
RenderTempIndex  renderTempFilledIndexCount     = 0;
MeshVertex       renderTempFilledVertexArray[MAX_TEMP_VERTICES];
RenderTempIndex  renderTempFilledIndexArray[MAX_TEMP_INDICES];
RenderTempIndex  renderDebugWireframeVertexCount = 0;
RenderTempIndex  renderDebugWireframeIndexCount  = 0;
MeshVertex       renderDebugWireframeVertexArray[MAX_TEMP_VERTICES];
RenderTempIndex  renderDebugWireframeIndexArray[MAX_TEMP_INDICES];
RenderTempIndex  renderDebugFilledVertexCount    = 0;
RenderTempIndex  renderDebugFilledIndexCount     = 0;
MeshVertex       renderDebugFilledVertexArray[MAX_TEMP_VERTICES];
RenderTempIndex  renderDebugFilledIndexArray[MAX_TEMP_INDICES];

#define MAX_MODEL_CMDS 10000 
RenderModelIndex renderModelCmdCount = 0;
RenderModelCmd   renderModelCmdArray[MAX_MODEL_CMDS];


void
render_line3(vec3 start, vec3 end, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32 rgba = c.rgba;
	MeshVertex*    vp = renderTempWireframeVertexArray + renderTempWireframeVertexCount;
	RenderTempIndex* ip = renderTempWireframeIndexArray  + renderTempWireframeIndexCount;
	
	ip[0] = renderTempWireframeVertexCount; 
	ip[1] = renderTempWireframeVertexCount+1; 
	ip[2] = renderTempWireframeVertexCount;
	vp[0].pos = start; vp[0].color = rgba;
	vp[1].pos = end;   vp[1].color = rgba;
	
	renderTempWireframeVertexCount += 2;
	renderTempWireframeIndexCount  += 3;
}

void
render_triangle3(vec3 p0, vec3 p1, vec3 p2, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32 rgba = c.rgba;
	MeshVertex*    vp = renderTempWireframeVertexArray + renderTempWireframeVertexCount;
	RenderTempIndex* ip = renderTempWireframeIndexArray  + renderTempWireframeIndexCount;
	
	ip[0] = renderTempWireframeVertexCount; 
	ip[1] = renderTempWireframeVertexCount+1; 
	ip[2] = renderTempWireframeVertexCount+2;
	vp[0].pos = p0; vp[0].color = rgba;
	vp[1].pos = p1; vp[1].color = rgba;
	vp[2].pos = p2; vp[2].color = rgba;
	
	renderTempWireframeVertexCount += 3;
	renderTempWireframeIndexCount  += 3;
}

void
render_triangle_filled3(vec3 p0, vec3 p1, vec3 p2, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32 rgba = c.rgba;
	MeshVertex*    vp = renderTempFilledVertexArray + renderTempFilledVertexCount;
	RenderTempIndex* ip = renderTempFilledIndexArray  + renderTempFilledIndexCount;
	
	ip[0] = renderTempFilledVertexCount; 
	ip[1] = renderTempFilledVertexCount+1; 
	ip[2] = renderTempFilledVertexCount+2;
	vp[0].pos = p0; vp[0].color = rgba;
	vp[1].pos = p1; vp[1].color = rgba;
	vp[2].pos = p2; vp[2].color = rgba;
	
	renderTempFilledVertexCount += 3;
	renderTempFilledIndexCount  += 3;
}

void
render_quad3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c){DPZoneScoped;
	if(c.a == 0) return;
	render_line3(p0, p1, c);
	render_line3(p1, p2, c);
	render_line3(p2, p3, c);
	render_line3(p3, p0, c);
}

void
render_quad_filled3(vec3 p0, vec3 p1, vec3 p2, vec3 p3, color c){DPZoneScoped;
	if(c.a == 0) return;
	render_triangle_filled3(p0, p1, p2, c);
	render_triangle_filled3(p0, p2, p3, c);
}

void
render_poly3(vec3* points, u64 count, color c){DPZoneScoped;
	if(c.a == 0) return;
	if(count < 2){
		LogE("render","render_poly_filled3() was passed less than 3 points.");
		return;
	}
	
	for(s32 i=1; i < count-1; ++i) render_line3(points[i-1], points[i], c);
	render_line3(points[count-2], points[count-1], c);
}

void
render_poly_filled3(vec3* points, u64 count, color c){DPZoneScoped;
	if(c.a == 0) return;
	if(count < 3){
		LogE("render","render_poly_filled3() was passed less than 3 points.");
		return;
	}
	
	for(s32 i=2; i < count-1; ++i) render_triangle_filled3(points[i-2], points[i-1], points[i], c);
	render_triangle_filled3(points[count-3], points[count-2], points[count-1], c);
}

void
render_circle3(vec3 position, vec3 rotation, f32 radius, u32 subdivisions_int, color c){DPZoneScoped;
	if(c.a == 0) return;
	mat4 transform = mat4::TransformationMatrix(position, rotation, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i-1)*M_2PI) / subdivisions;
		f32 a1 = (f32(i  )*M_2PI) / subdivisions;
		f32 x0 = radius*cosf(a0); f32 x1 = radius*cosf(a1);
		f32 y0 = radius*sinf(a0); f32 y1 = radius*sinf(a1);
		vec3 xaxis0 = vec3{0, y0, x0} * transform; vec3 xaxis1 = vec3{0, y1, x1} * transform;
		render_line3(xaxis0, xaxis1, c);
	}
}

void
render_box(mat4* transform, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	vec3 p{0.5f, 0.5f, 0.5f};
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] += vec3::ONE*0.5; points[i] = points[i] * (*transform); }
	render_line3(points[3], points[1], c); render_line3(points[3], points[2], c); render_line3(points[3], points[7], c);
	render_line3(points[0], points[1], c); render_line3(points[0], points[2], c); render_line3(points[0], points[4], c);
	render_line3(points[5], points[1], c); render_line3(points[5], points[4], c); render_line3(points[5], points[7], c);
	render_line3(points[6], points[2], c); render_line3(points[6], points[4], c); render_line3(points[6], points[7], c);
}

void
render_box_filled(mat4* transform, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	vec3 p{0.5f, 0.5f, 0.5f};
	vec3 points[8] = {
		{-p.x, p.y, p.z},{-p.x,-p.y, p.z},{-p.x, p.y,-p.z},{-p.x,-p.y,-p.z},
		{ p.x, p.y, p.z},{ p.x,-p.y, p.z},{ p.x, p.y,-p.z},{ p.x,-p.y,-p.z},
	};
	forI(8){ points[i] += vec3::ONE*0.5; points[i] = points[i] * (*transform); }
	render_triangle_filled3(points[4], points[2], points[0], c); render_triangle_filled3(points[4], points[6], points[2], c);
	render_triangle_filled3(points[2], points[7], points[3], c); render_triangle_filled3(points[2], points[6], points[7], c);
	render_triangle_filled3(points[6], points[5], points[7], c); render_triangle_filled3(points[6], points[4], points[5], c);
	render_triangle_filled3(points[1], points[7], points[5], c); render_triangle_filled3(points[1], points[3], points[7], c);
	render_triangle_filled3(points[0], points[3], points[1], c); render_triangle_filled3(points[0], points[2], points[3], c);
	render_triangle_filled3(points[4], points[1], points[5], c); render_triangle_filled3(points[4], points[0], points[1], c);
}

void
render_sphere(vec3 position, vec3 rotation, f32 radius, u32 subdivisions_int, color cx, color cy, color cz){DPZoneScoped;
	mat4 transform = mat4::TransformationMatrix(position, rotation, vec3::ONE);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (f32(i-1)*M_2PI) / subdivisions;
		f32 a1 = (f32(i  )*M_2PI) / subdivisions;
		f32 x0 = radius*cosf(a0); f32 x1 = radius*cosf(a1);
		f32 y0 = radius*sinf(a0); f32 y1 = radius*sinf(a1);
		vec3 xaxis0 = vec3{0, y0, x0} * transform; vec3 xaxis1 = vec3{0, y1, x1} * transform;
		vec3 yaxis0 = vec3{x0, 0, y0} * transform; vec3 yaxis1 = vec3{x1, 0, y1} * transform;
		vec3 zaxis0 = vec3{x0, y0, 0} * transform; vec3 zaxis1 = vec3{x1, y1, 0} * transform;
		render_line3(xaxis0, xaxis1, cx); render_line3(yaxis0, yaxis1, cy); render_line3(zaxis0, zaxis1, cz);
	}
}

void
render_frustrum(vec3 position, vec3 target, f32 aspectRatio, f32 fovx, f32 nearZ, f32 farZ, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	f32 y = tanf(Radians(fovx / 2.0f));
	f32 x = y * aspectRatio;
	f32 nearX = x * nearZ;
	f32 farX  = x * farZ;
	f32 nearY = y * nearZ;
	f32 farY  = y * farZ;
	
	vec4 faces[8] = {
		//near face
		{ nearX,  nearY, nearZ, 1},
		{-nearX,  nearY, nearZ, 1},
		{ nearX, -nearY, nearZ, 1},
		{-nearX, -nearY, nearZ, 1},
		
		//far face
		{ farX,  farY, farZ, 1},
		{-farX,  farY, farZ, 1},
		{ farX, -farY, farZ, 1},
		{-farX, -farY, farZ, 1},
	};
	
	mat4 mat = Math::LookAtMatrix(position, target);
	vec3 v[8];
	forI(8){
		vec4 temp = faces[i] * mat;
		v[i].x = temp.x / temp.w;
		v[i].y = temp.y / temp.w;
		v[i].z = temp.z / temp.w;
	}
	
	render_line3(v[0], v[1], c);
	render_line3(v[0], v[2], c);
	render_line3(v[3], v[1], c);
	render_line3(v[3], v[2], c);
	render_line3(v[4], v[5], c);
	render_line3(v[4], v[6], c);
	render_line3(v[7], v[5], c);
	render_line3(v[7], v[6], c);
	render_line3(v[0], v[4], c);
	render_line3(v[1], v[5], c);
	render_line3(v[2], v[6], c);
	render_line3(v[3], v[7], c);
}

void
render_clear_debug(){DPZoneScoped;
	renderDebugWireframeVertexCount = 0;
	renderDebugWireframeIndexCount  = 0;
	renderDebugFilledVertexCount = 0;
	renderDebugFilledIndexCount  = 0;
}

void
render_debug_line3(vec3 start, vec3 end, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32            color = c.rgba;
	MeshVertex*     vp = renderDebugWireframeVertexArray + renderDebugWireframeVertexCount;
	RenderTempIndex*  ip = renderDebugWireframeIndexArray + renderDebugWireframeIndexCount;
	
	ip[0] = renderDebugWireframeVertexCount; 
	ip[1] = renderDebugWireframeVertexCount+1; 
	ip[2] = renderDebugWireframeVertexCount;
	vp[0].pos = start; vp[0].color = color;
	vp[1].pos = end;   vp[1].color = color;
	
	renderDebugWireframeVertexCount += 2;
	renderDebugWireframeIndexCount  += 3;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_draw_2d

RenderTwodIndex renderTwodVertexCount = 0;
RenderTwodIndex renderTwodIndexCount  = 0;
Vertex2         renderTwodVertexArray[MAX_TWOD_VERTICES];
RenderTwodIndex renderTwodIndexArray [MAX_TWOD_INDICES];
RenderTwodIndex renderTwodCmdCounts[MAX_SURFACES][TWOD_LAYERS+1]; //these always start with 1
RenderTwodCmd   renderTwodCmdArrays[MAX_SURFACES][TWOD_LAYERS+1][MAX_TWOD_CMDS]; //different UI cmd per texture
RenderBookKeeper renderBookKeeperArray[MAX_TWOD_CMDS]; // keeps track of different kinds of allocations
u32 renderBookKeeperCount = 0;
u32 renderActiveLayer = 5;

//external buffers
#define MAX_EXTERNAL_BUFFERS 6
RenderTwodIndex renderExternalCmdCounts[MAX_EXTERNAL_BUFFERS];
RenderTwodCmd   renderExternalCmdArrays[MAX_EXTERNAL_BUFFERS][MAX_TWOD_CMDS]; 

void
deshi__render_add_vertices2(str8 file, u32 line, u32 layer, Vertex2* vertices, u32 vCount, u32* indices, u32 iCount){DPZoneScoped;
	Assert(vCount + renderTwodVertexCount < MAX_TWOD_VERTICES);
	Assert(iCount + renderTwodIndexCount  < MAX_TWOD_INDICES);
	
	Vertex2* vp         = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	CopyMemory(vp, vertices, vCount*sizeof(Vertex2));
	forI(iCount){
		Assert(indices[i] < vCount, "Index out of range of given number of vertices!\nMake sure your indices weren't overwritten by something.");
		ip[i] = renderTwodVertexCount + indices[i];
	} 
	
#ifdef BUILD_INTERNAL
	RenderBookKeeper keeper;
	keeper.type = RenderBookKeeper_Vertex;
	keeper.vertex.start = vp;
	keeper.vertex.count = vCount;
	keeper.file = file;
	keeper.line = line;
	renderBookKeeperArray[renderBookKeeperCount++] = keeper;
	
	keeper.type = RenderBookKeeper_Index;
	keeper.index.start = ip;
	keeper.index.count = iCount;
	keeper.file = file;
	keeper.line = line;
	renderBookKeeperArray[renderBookKeeperCount++] = keeper;
#endif
	
	renderTwodVertexCount += vCount;
	renderTwodIndexCount  += iCount;
	renderTwodCmdArrays[renderActiveSurface][layer][renderTwodCmdCounts[renderActiveSurface][layer] - 1].index_count += iCount;
	
	
}

u32
render_top_layer_index(){DPZoneScoped;
	return TWOD_LAYERS-1;
}

u32
render_decoration_layer_index(){DPZoneScoped;
	return TWOD_LAYERS;
}

u32
render_active_layer(){DPZoneScoped;
	return renderActiveLayer;
}

void
render_line2(vec2 start, vec2 end, color c){DPZoneScoped;
	render_line_thick2(start, end, 1, c);
}

void
render_line_thick2(vec2 start, vec2 end, f32 thickness, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32           color = c.rgba;
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	vec2 ott = end - start;
	vec2 norm = Vec2(ott.y, -ott.x).normalized();
	
	ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
	ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
	vp[0].pos = { start.x,start.y }; vp[0].uv = { 0,0 }; vp[0].color = color;
	vp[1].pos = { end.x,  end.y   }; vp[1].uv = { 0,0 }; vp[1].color = color;
	vp[2].pos = { end.x,  end.y   }; vp[2].uv = { 0,0 }; vp[2].color = color;
	vp[3].pos = { start.x,start.y }; vp[3].uv = { 0,0 }; vp[3].color = color;
	
	vp[0].pos += norm * thickness / 2.f;
	vp[1].pos += norm * thickness / 2.f;
	vp[2].pos -= norm * thickness / 2.f;
	vp[3].pos -= norm * thickness / 2.f;
	
	renderTwodVertexCount += 4;
	renderTwodIndexCount  += 6;
	renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].index_count += 6;
}

//TODO(sushi) this function needs to be made more robust as well as cleaned up. currently, if 2 line segments form a
// small enough angle, the thickness stop being preserved. this funciton also needs to be moved out to suugu and
// replaced by a more general render function that allows you to manipulate the vertex/index arrays
void
render_lines2(vec2* points, u64 count, f32 thickness, color c){DPZoneScoped;
	if(count < 2){
		LogE("render","render_lines2() was passed less than 2 points.");
		return;
	}
	if(c.a == 0 || thickness == 0) return;
	
	
	u32           color = c.rgba;
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	f32 halfthick = thickness / 2.f;
	{// first point
		
		vec2 ott = points[1] - points[0];
		vec2 norm = Vec2(ott.y, -ott.x).normalized();
		
		vp[0].pos = points[0] + norm * halfthick; vp[0].uv = { 0,0 }; vp[0].color = color;
		vp[1].pos = points[0] - norm * halfthick; vp[1].uv = { 0,0 }; vp[1].color = color;
		
		ip[0] = renderTwodVertexCount;
		ip[1] = renderTwodVertexCount + 1;
		ip[3] = renderTwodVertexCount;
		
		renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].index_count += 3;
		
		renderTwodVertexCount += 2;
		renderTwodIndexCount  += 3;
		vp += 2;
	}
	
	//in betweens
	s32 flip = -1;
	for(s32 i = 1; i < count - 1; i++, flip *= -1){
		vec2 last, curr, next, norm;
		
		last = points[i - 1];
		curr = points[i];
		next = points[i + 1];
		
		//figure out average norm
		vec2
			p01 = curr - last,
		p12 = next - curr,
		p02 = next - last,
		//norm01 = vec2{ p01.y, -p01.x } * flip, //we flip the normal everytime to keep up the pattern
		//norm12 = vec2{ p12.y, -p12.x } * flip,
		normav;//((norm01 + norm12) / 2).normalized();
		
		f32 a = p01.mag(), b = p12.mag(), c = p02.mag();
		f32 ang = Radians(Math::AngBetweenVectors(-p01, p12));
		
		//this is the critical angle where the thickness of the 2 lines cause them to overlap at small angles
		//if(fabs(ang) < 2 * atanf(thickness / (2 * p02.mag()))){
		//	ang = 2 * atanf(thickness / (2 * p02.mag()));
		//}
		
		normav = p12.normalized();
		normav = Math::vec2RotateByAngle(-Degrees(ang) / 2, normav);
		normav *= (f32)flip;
		
		//this is where we calc how wide the thickness of the inner line is meant to be
		normav = normav.normalized() * thickness / (2 * sinf(ang / 2));
		
		vec2 normavout = normav;
		vec2 normavin = -normav;
		
		normavout.clampMag(0, thickness * 2);//sqrt(2) / 2 * thickness );
		normavin.clampMag(0, thickness * 4);
		
		//set indicies by pattern
		s32 ipidx = 6 * (i - 1) + 2;
		ip[ipidx + 0] =
			ip[ipidx + 2] =
			ip[ipidx + 4] =
			ip[ipidx + 7] =
			renderTwodVertexCount;
		
		ip[ipidx + 3] =
			ip[ipidx + 5] =
			renderTwodVertexCount + 1;
		
		vp[0].pos = curr + normavout; vp[0].uv = { 0,0 }; vp[0].color = color;//PackColorU32(255, 0, 0, 255);
		vp[1].pos = curr + normavin;  vp[1].uv = { 0,0 }; vp[1].color = color;//PackColorU32(255, 0, 255, 255);
		
		renderTwodVertexCount += 2;
		renderTwodIndexCount  += 6;
		vp += 2;
		
		renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].index_count += 6;
		
	}
	
	{//last point
		vec2 ott = points[count - 1] - points[count - 2];
		vec2 norm = Vec2(ott.y, -ott.x).normalized() * (f32)flip;
		
		vp[0].pos = points[count - 1] + norm * halfthick; vp[0].uv = { 0,0 }; vp[0].color = color;//PackColorU32(255, 50, 255, 255);
		vp[1].pos = points[count - 1] - norm * halfthick; vp[1].uv = { 0,0 }; vp[1].color = color;//PackColorU32(255, 50, 100, 255);
		
		//set final indicies by pattern
		s32 ipidx = 6 * (count - 2) + 2;
		ip[ipidx + 0] = renderTwodVertexCount;
		ip[ipidx + 2] = renderTwodVertexCount;
		ip[ipidx + 3] = renderTwodVertexCount + 1;
		
		renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].index_count += 3;
		
		renderTwodVertexCount += 2;
		renderTwodIndexCount  += 3;
		vp += 2; ip += 3;
	}
}

void
render_triangle2(vec2 p1, vec2 p2, vec2 p3, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	render_line2(p1, p2, c);
	render_line2(p2, p3, c);
	render_line2(p3, p1, c);
}

void
render_triangle_filled2(vec2 p1, vec2 p2, vec2 p3, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32           color = c.rgba;
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = color;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = color;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = color;
	
	renderTwodVertexCount += 3;
	renderTwodIndexCount  += 3;
	renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].index_count += 3;
}

void
render_quad2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	render_line2(top_left,  top_right, c);
	render_line2(top_right, bot_right, c);
	render_line2(bot_right, bot_left,  c);
	render_line2(bot_left,  top_left,  c);
}

void
render_quad_thick2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, f32 thickness, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	render_line_thick2(top_left,  top_right, thickness, c);
	render_line_thick2(top_right, bot_right, thickness, c);
	render_line_thick2(bot_right, bot_left,  thickness, c);
	render_line_thick2(bot_left,  top_left,  thickness, c);
}

void
render_quad_filled2(vec2 top_left, vec2 top_right, vec2 bot_left, vec2 bot_right, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	u32           color = c.rgba;
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
	ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
	vp[0].pos = top_left;  vp[0].uv = { 0,0 }; vp[0].color = color;
	vp[1].pos = top_right; vp[1].uv = { 0,0 }; vp[1].color = color;
	vp[2].pos = bot_right; vp[2].uv = { 0,0 }; vp[2].color = color;
	vp[3].pos = bot_left;  vp[3].uv = { 0,0 }; vp[3].color = color;
	
	renderTwodVertexCount += 4;
	renderTwodIndexCount  += 6;
	renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].index_count += 6;
}

void
render_circle2(vec2 pos, f32 rotation, f32 radius, u32 subdivisions_int, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	f32 rot_rad = Radians(rotation);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (((f32)(i-1) * M_2PI) / subdivisions) + rot_rad;
		f32 a1 = (((f32)(i-0) * M_2PI) / subdivisions) + rot_rad;
		f32 x0 = pos.x + radius * cosf(a0); f32 x1 = pos.x + radius * cosf(a1);
		f32 y0 = pos.y + radius * sinf(a0); f32 y1 = pos.y + radius * sinf(a1);
		render_line2(Vec2(x0, y0), Vec2(x1, y1), c);
	}
}

void
render_circle_filled2(vec2 pos, f32 rotation, f32 radius, u32 subdivisions_int, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	f32 rot_rad = Radians(rotation);
	f32 subdivisions = f32(subdivisions_int);
	forI(subdivisions_int){
		f32 a0 = (((f32)(i-1) * M_2PI) / subdivisions) + rot_rad;
		f32 a1 = (((f32)(i-0) * M_2PI) / subdivisions) + rot_rad;
		f32 x0 = pos.x + radius * cosf(a0); f32 x1 = pos.x + radius * cosf(a1);
		f32 y0 = pos.y + radius * sinf(a0); f32 y1 = pos.y + radius * sinf(a1);
		render_triangle_filled2(pos, Vec2(x0, y0), Vec2(x1, y1), c);
	}
}

void
render_text2(Font* font, str8 text, vec2 pos, vec2 scale, color c){DPZoneScoped;
	if(c.a == 0) return;
	
	switch(font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE:{
			while(text){
				DecodedCodepoint decoded = str8_advance(&text);
				if(decoded.codepoint == 0) break;
				
				u32           color = c.rgba;
				Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
				RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
				
				f32 w  = font->max_width  * scale.x;
				f32 h  = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx    = (f32)(decoded.codepoint - 32);
				f32 topoff = (idx * dy) + font->uv_yoffset;
				f32 botoff = topoff + dy;
				
				ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
				ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff }; vp[0].color = color; //top left
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff }; vp[1].color = color; //top right
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff }; vp[2].color = color; //bot right
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff }; vp[3].color = color; //bot left
				
				renderTwodVertexCount += 4;
				renderTwodIndexCount  += 6;
				renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer]].index_count += 6;
				pos.x += font->max_width * scale.x;
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF:{
			while(text){
				DecodedCodepoint decoded = str8_advance(&text);
				if(decoded.codepoint == 0) break;
				
				u32           color = c.rgba;
				Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
				RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
				FontAlignedQuad   q = font_aligned_quad(font, decoded.codepoint, &pos, scale);
				
				ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
				ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.u0,q.v0 }; vp[0].color = color; //top left
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.u1,q.v0 }; vp[1].color = color; //top right
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.u1,q.v1 }; vp[2].color = color; //bot right
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.u0,q.v1 }; vp[3].color = color; //bot left
				
				renderTwodVertexCount += 4;
				renderTwodIndexCount  += 6;
				renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] - 1].index_count += 6;
			}break;
			default: Assert(!"unhandled font type"); break;
		}
	}
}

void
render_texture2(Texture* texture, vec2 tl, vec2 tr, vec2 bl, vec2 br, f32 transparency){DPZoneScoped;
	if(transparency == 0) return;
	
	u32           color = PackColorU32(255, 255, 255, (u8)(255.f*transparency));
	Vertex2*         vp = renderTwodVertexArray + renderTwodVertexCount;
	RenderTwodIndex* ip = renderTwodIndexArray  + renderTwodIndexCount;
	
	ip[0] = renderTwodVertexCount; ip[1] = renderTwodVertexCount + 1; ip[2] = renderTwodVertexCount + 2;
	ip[3] = renderTwodVertexCount; ip[4] = renderTwodVertexCount + 2; ip[5] = renderTwodVertexCount + 3;
	vp[0].pos = tl; vp[0].uv = { 0,1 }; vp[0].color = color;
	vp[1].pos = tr; vp[1].uv = { 1,1 }; vp[1].color = color;
	vp[2].pos = br; vp[2].uv = { 1,0 }; vp[2].color = color;
	vp[3].pos = bl; vp[3].uv = { 0,0 }; vp[3].color = color;
	
	renderTwodVertexCount += 4;
	renderTwodIndexCount  += 6;
	
	RenderTwodCmd* cmd =  &renderTwodCmdArrays[renderActiveSurface][renderActiveLayer][renderTwodCmdCounts[renderActiveSurface][renderActiveLayer]];
	cmd->index_count += 6;
	cmd->handle = (void*)((u64)texture->render_idx);
	cmd->scissor_extent = g_window->dimensions.toVec2();
	cmd->scissor_offset = Vec2(0,0);
	renderTwodCmdCounts[renderActiveSurface][renderActiveLayer] += 1;
}

void
render_texture_flat2(Texture* texture, vec2 pos, vec2 dimensions, f32 transparency){DPZoneScoped;
	if(transparency == 0) return;
	
	vec2 tl = pos,
	tr = vec2{pos.x + dimensions.x, pos.y},
	bl = vec2{pos.x,                pos.y + dimensions.y},
	br = vec2{pos.x + dimensions.x, pos.y + dimensions.y};
	render_texture2(texture, tl, tr, bl, br, transparency);
}

void
render_texture_rotated2(Texture* texture, vec2 center, vec2 dimensions, f32 rotation, f32 transparency){DPZoneScoped;
	if(transparency == 0) return;
	
	vec2 half_dims = dimensions / 2.f,
	tl = Math::vec2RotateByAngle(rotation, center - half_dims),
	tr = Math::vec2RotateByAngle(rotation, center + half_dims.yInvert()),
	bl = Math::vec2RotateByAngle(rotation, center + half_dims.xInvert()),
	br = Math::vec2RotateByAngle(rotation, center + half_dims);
	render_texture2(texture, tl, tr, bl, br, transparency);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_make_2d
//4 verts, 6 indices
vec2i
render_make_line(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 start, vec2 end, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vec2 ott = end - start;
	vec2 norm = Vec2(ott.y, -ott.x).normalized();
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = { start.x,start.y }; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = { end.x,    end.y }; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = { end.x,    end.y }; vp[2].uv = { 0,0 }; vp[2].color = col;
	vp[3].pos = { start.x,start.y }; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	vp[0].pos += norm * thickness / 2.f;
	vp[1].pos += norm * thickness / 2.f;
	vp[2].pos -= norm * thickness / 2.f;
	vp[3].pos -= norm * thickness / 2.f;
	
	return render_make_line_counts();
}

//3 verts, 3 indices
vec2i 
render_make_filledtriangle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 p1, vec2 p2, vec2 p3, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = col;
	
	return render_make_filledtriangle_counts();
}

vec2i
render_make_triangle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vec2i sum;
	sum += render_make_line(vp, ip, {    0,    0}, p0, p1, 1, color);
	sum += render_make_line(vp, ip, {sum.x,sum.y}, p1, p2, 1, color);
	sum += render_make_line(vp, ip, {sum.x,sum.y}, p2, p0, 1, color);
	
	return sum;
	
	//TODO(sushi) this should be fixed to replace reliance on MakeLine
	//ip[0]  = offsets.y + 0; ip[1]  = offsets.y + 1; ip[2]  = offsets.y + 3;
	//ip[3]  = offsets.y + 0; ip[4]  = offsets.y + 3; ip[5]  = offsets.y + 2;
	//ip[6]  = offsets.y + 2; ip[7]  = offsets.y + 3; ip[8]  = offsets.y + 5;
	//ip[9]  = offsets.y + 2; ip[10] = offsets.y + 5; ip[11] = offsets.y + 4;
	//ip[12] = offsets.y + 4; ip[13] = offsets.y + 5; ip[14] = offsets.y + 1;
	//ip[15] = offsets.y + 4; ip[16] = offsets.y + 1; ip[17] = offsets.y + 0;
	//
	//f32 ang1 = Math::AngBetweenVectors(p1 - p0, p2 - p0)/2;
	//f32 ang2 = Math::AngBetweenVectors(p0 - p1, p2 - p1)/2;
	//f32 ang3 = Math::AngBetweenVectors(p1 - p2, p0 - p2)/2;
	//
	//vec2 p0offset = (Math::vec2RotateByAngle(-ang1, p2 - p0).normalized() * thickness / (2 * sinf(Radians(ang1)))).clampedMag(0, thickness * 2);
	//vec2 p1offset = (Math::vec2RotateByAngle(-ang2, p2 - p1).normalized() * thickness / (2 * sinf(Radians(ang2)))).clampedMag(0, thickness * 2);
	//vec2 p2offset = (Math::vec2RotateByAngle(-ang3, p0 - p2).normalized() * thickness / (2 * sinf(Radians(ang3)))).clampedMag(0, thickness * 2);
	//       
	//vp[0].pos = p0 - p0offset; vp[0].uv = { 0,0 }; vp[0].color = col;
	//vp[1].pos = p0 + p0offset; vp[1].uv = { 0,0 }; vp[1].color = col;
	//vp[2].pos = p1 + p1offset; vp[2].uv = { 0,0 }; vp[2].color = col;
	//vp[3].pos = p1 - p1offset; vp[3].uv = { 0,0 }; vp[3].color = col;
	//vp[4].pos = p2 + p2offset; vp[4].uv = { 0,0 }; vp[4].color = col;
	//vp[5].pos = p2 - p2offset; vp[5].uv = { 0,0 }; vp[5].color = col;
	
	//return vec3(6, 18);
}

vec2i
render_make_filledrect(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, vec2 size, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vec2 tl = pos;
	vec2 br = pos + size;
	vec2 bl = pos + Vec2(0, size.y);
	vec2 tr = pos + Vec2(size.x, 0);
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = tl; vp[0].uv = { 0,1 }; vp[0].color = col;
	vp[1].pos = tr; vp[1].uv = { 1,1 }; vp[1].color = col;
	vp[2].pos = br; vp[2].uv = { 1,0 }; vp[2].color = col;
	vp[3].pos = bl; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	return render_make_filledrect_counts();
}

vec2i
render_make_rect(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, vec2 size, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	// vec2 tl = pos;
	// vec2 br = pos + size;
	// vec2 bl = pos + Vec2(0, size.y);
	// vec2 tr = pos + Vec2(size.x, 0);
	
	// vec2i sum = {0};
	// sum += render_make_line(vp, ip, sum, tl,tr,thickness,color);
	// sum += render_make_line(vp, ip, sum, tr,br,thickness,color);
	// sum += render_make_line(vp, ip, sum, br,bl,thickness,color);
	// sum += render_make_line(vp, ip, sum, bl,tl,thickness,color);
	
	//TODO(sushi) test this
	vec2 tl = pos;
	vec2 br = pos + size;
	vec2 bl = vec2{br.x, tl.y};
	vec2 tr = vec2{tl.x, br.y};
	f32 t = thickness; u32 v = offsets.x;
	ip[ 0] = v+0; ip[ 1] = v+1; ip[ 2] = v+3; 
	ip[ 3] = v+0; ip[ 4] = v+3; ip[ 5] = v+2; 
	ip[ 6] = v+2; ip[ 7] = v+3; ip[ 8] = v+5; 
	ip[ 9] = v+2; ip[10] = v+5; ip[11] = v+4; 
	ip[12] = v+4; ip[13] = v+5; ip[14] = v+7; 
	ip[15] = v+4; ip[16] = v+7; ip[17] = v+6; 
	ip[18] = v+6; ip[19] = v+7; ip[20] = v+1; 
	ip[21] = v+6; ip[22] = v+1; ip[23] = v+0;
	vp[0].pos = tl;             vp[0].uv = {0,0}; vp[0].color = color.rgba;
	vp[1].pos = tl+Vec2( t, t); vp[1].uv = {0,0}; vp[1].color = color.rgba;
	vp[2].pos = tr;             vp[2].uv = {0,0}; vp[2].color = color.rgba;
	vp[3].pos = tr+Vec2(-t, t); vp[3].uv = {0,0}; vp[3].color = color.rgba;
	vp[4].pos = br;             vp[4].uv = {0,0}; vp[4].color = color.rgba;
	vp[5].pos = br+Vec2(-t,-t); vp[5].uv = {0,0}; vp[5].color = color.rgba;
	vp[6].pos = bl;             vp[6].uv = {0,0}; vp[6].color = color.rgba;
	vp[7].pos = bl+Vec2( t,-t); vp[7].uv = {0,0}; vp[7].color = color.rgba;
	
	return render_make_rect_counts();
}

vec2i
render_make_circle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	f32 subdivisions = f32(subdivisions_int);
	u32 nuindexes = subdivisions * 6;
	
	//first and last point
	vec2 last = pos + Vec2(radius, 0);
	vp[0].pos = last + Vec2(-thickness / 2, 0); vp[0].uv={0,0}; vp[0].color=col;
	vp[1].pos = last + Vec2( thickness / 2, 0); vp[1].uv={0,0}; vp[1].color=col;
	ip[0] = offsets.x + 0; ip[1] = offsets.x + 1; ip[3] = offsets.x + 0;
	ip[nuindexes - 1] = offsets.x + 0; ip[nuindexes - 2] = ip[nuindexes - 4] = offsets.x + 1;
	
	for(s32 i = 1; i < subdivisions_int; i++){
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		vec2 offset = Vec2(radius * cosf(a1), radius * sinf(a1));
		vec2 point = pos + offset;
		
		u32 idx = i * 2;
		vp[idx].pos = point - offset.normalized() * thickness / 2; vp[idx].uv = { 0,0 }; vp[idx].color = col;
		vp[idx + 1].pos = point + offset.normalized() * thickness / 2; vp[idx + 1].uv = { 0,0 }; vp[idx + 1 ].color = col;
		
		u32 ipidx1 = 6 * (i - 1) + 2;
		u32 ipidx2 = 6 * i - 1;
		ip[ipidx1] = ip[ipidx1 + 2] = ip[ipidx1 + 5] = offsets.x + idx + 1;
		ip[ipidx2] = ip[ipidx2 + 1] = ip[ipidx2 + 4] = offsets.x + idx;
	}
	
	return render_make_circle_counts(subdivisions_int);
}

vec2i 
render_make_filledcircle(Vertex2* putverts, u32* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vp[0].pos = pos; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = pos + Vec2(radius, 0); vp[1].uv = { 0,0 }; vp[1].color = col;
	u32 nuindexes = 3 * subdivisions_int;
	
	ip[1] = offsets.x + 1;
	for(s32 i = 0; i < nuindexes; i += 3) ip[i] = offsets.x;
	
	ip[nuindexes - 1] = offsets.x + 1;
	
	vec2 sum;
	f32 subdivisions = f32(subdivisions_int);
	for(u32 i = 1; i < subdivisions_int; i++){
		f32 a1 = (f32(i) * M_2PI) / subdivisions;
		vec2 offset = Vec2(radius * cosf(a1), radius * sinf(a1));
		vec2 point = pos + offset;
		
		vp[i+1].pos = point; vp[i+1].uv = { 0,0 }; vp[i+1].color = col;
		
		u32 ipidx = 3 * i - 1;
		ip[ipidx] = ip[ipidx + 2] = offsets.x + i + 1;
	}
	
	return render_make_filledcircle_counts(subdivisions_int);
}

vec2i
render_make_text(Vertex2* putverts, u32* putindices, vec2i offsets, str8 text, Font* font, vec2 pos, color color, vec2 scale){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	vec2i sum={0};
	switch (font->type){
		//// BDF (and NULL) font rendering ////
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			str8 remaining = text;
			u32 i = 0;
			while(remaining && (codepoint = str8_advance(&remaining).codepoint)){
				u32     col = color.rgba;
				Vertex2* vp = putverts + offsets.x + 4 * i;
				u32*     ip = putindices + offsets.y + 6 * i;
				
				f32 w = font->max_width * scale.x;
				f32 h = font->max_height * scale.y;
				f32 dy = 1.f / (f32)font->count;
				
				f32 idx = f32(codepoint - 32);
				f32 topoff = (idx * dy) + font->uv_yoffset;
				f32 botoff = topoff + dy;
				
				ip[0] = offsets.x+4*i; ip[1] = offsets.x+4*i + 1; ip[2] = offsets.x+4*i + 2;
				ip[3] = offsets.x+4*i; ip[4] = offsets.x+4*i + 2; ip[5] = offsets.x+4*i + 3;
				vp[0].pos = { pos.x + 0,pos.y + 0 }; vp[0].uv = { 0,topoff }; vp[0].color = col; //top left
				vp[1].pos = { pos.x + w,pos.y + 0 }; vp[1].uv = { 1,topoff }; vp[1].color = col; //top right
				vp[2].pos = { pos.x + w,pos.y + h }; vp[2].uv = { 1,botoff }; vp[2].color = col; //bot right
				vp[3].pos = { pos.x + 0,pos.y + h }; vp[3].uv = { 0,botoff }; vp[3].color = col; //bot left
				pos.x += font->max_width * scale.x;
				i += 1;
				sum+=render_make_text_counts(1);
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF:{
			u32 codepoint;
			str8 remaining = text;
			u32 i = 0;
			while(remaining && (codepoint = str8_advance(&remaining).codepoint)){
				u32     col = color.rgba;
				Vertex2* vp = putverts + offsets.x + 4 * i;
				u32*     ip = putindices + offsets.y + 6 * i;
				FontAlignedQuad q = font_aligned_quad(font, codepoint, &pos, scale);
				
				ip[0] = offsets.x+4*i; ip[1] = offsets.x+4*i + 1; ip[2] = offsets.x+4*i + 2;
				ip[3] = offsets.x+4*i; ip[4] = offsets.x+4*i + 2; ip[5] = offsets.x+4*i + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.u0,q.v0 }; vp[0].color = col; //top left
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.u1,q.v0 }; vp[1].color = col; //top right
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.u1,q.v1 }; vp[2].color = col; //bot right
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.u0,q.v1 }; vp[3].color = col; //bot left
				i += 1;
				sum+=render_make_text_counts(1);
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return sum;
}

vec2i 
render_make_texture(Vertex2* putverts, u32* putindices, vec2i offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx = 0, b32 flipy = 0){DPZoneScoped;
	Assert(putverts && putindices);
	if(!alpha) return{0,0};
	
	u32     col = PackColorU32(255,255,255,255.f * alpha);
	Vertex2* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	ip[3] = offsets.x; ip[4] = offsets.x + 2; ip[5] = offsets.x + 3;
	vp[0].pos = p0; vp[0].uv = { 0,1 }; vp[0].color = col;
	vp[1].pos = p1; vp[1].uv = { 1,1 }; vp[1].color = col;
	vp[2].pos = p2; vp[2].uv = { 1,0 }; vp[2].color = col;
	vp[3].pos = p3; vp[3].uv = { 0,0 }; vp[3].color = col;
	
	if(flipx){
		vec2 u0 = vp[0].uv, u1 = vp[1].uv, u2 = vp[2].uv, u3 = vp[3].uv;
		vp[0].uv = u1; vp[1].uv = u0; vp[2].uv = u3; vp[3].uv = u2;
	}
	if(flipy){
		vec2 u0 = vp[0].uv, u1 = vp[1].uv, u2 = vp[2].uv, u3 = vp[3].uv;
		vp[0].uv = u3; vp[1].uv = u2; vp[2].uv = u1; vp[3].uv = u0;
	}
	return render_make_texture_counts();
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_voxel
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


RenderVoxelType* render_voxel_types;
u64 render_voxel_types_count;
RenderVoxelChunk* render_voxel_chunk_pool;
u32 render_voxel_voxel_size; //width, height, and depth of a voxel in the world

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
render_voxel_init(RenderVoxelType* types, u64 count, u32 voxel_size){
	render_voxel_types = types;
	render_voxel_types_count = count;
	memory_pool_init(render_voxel_chunk_pool, 128);
	for_pool(render_voxel_chunk_pool) it->hidden = true;
	render_voxel_voxel_size = voxel_size;
}


void
render_voxel_make_face_mesh(int direction, RenderVoxelChunk* chunk, RenderVoxel* voxel, MeshVertex* vertex_array, u64* vertex_count, MeshIndex* index_array, u64* index_count){
	vec3 voxel_position = chunk->position + Vec3(voxel->x, voxel->y, voxel->z);
	mat4 transform = mat4::TransformationMatrix(voxel_position, chunk->rotation, vec3_ONE());
	
	vertex_array[*vertex_count+0] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_bl] * transform,
		Vec2(0,0),
		render_voxel_types[voxel->type].color,
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+1] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_tl] * transform,
		Vec2(0,1),
		render_voxel_types[voxel->type].color,
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+2] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_tr] * transform,
		Vec2(1,1),
		render_voxel_types[voxel->type].color,
		render_voxel_face_normals[direction] * transform
	};
	vertex_array[*vertex_count+3] = {
		render_voxel_unit_vertex_offsets[direction][render_voxel_face_vertex_br] * transform,
		Vec2(1,0),
		render_voxel_types[voxel->type].color,
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
render_voxel_create_chunk(vec3 position, vec3 rotation, u32 dimensions, RenderVoxel* voxels, u64 voxels_count){
	Assert(dimensions != 0, "Dimensions can not be zero!");
	Assert(voxels != 0 && voxels_count != 0, "Don't call this with an invalid voxels array!");
	
	//alloc and init chunk
	RenderVoxelChunk* chunk = memory_pool_push(render_voxel_chunk_pool);
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
		
		if((chunk->voxels[i]->x == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_x] == 0))
			render_voxel_make_face_mesh(render_voxel_face_posx, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->x == 0)                    || (chunk->voxels[i - dimensions_stride_x] == 0))
			render_voxel_make_face_mesh(render_voxel_face_negx, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->y == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_y] == 0))
			render_voxel_make_face_mesh(render_voxel_face_posy, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->y == 0)                    || (chunk->voxels[i - dimensions_stride_y] == 0))
			render_voxel_make_face_mesh(render_voxel_face_negy, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->z == dimensions_minus_one) || (chunk->voxels[i + dimensions_stride_z] == 0))
			render_voxel_make_face_mesh(render_voxel_face_posz, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
		if((chunk->voxels[i]->z == 0)                    || (chunk->voxels[i - dimensions_stride_z] == 0))
			render_voxel_make_face_mesh(render_voxel_face_negz, chunk, chunk->voxels[i], vertex_array, &chunk->vertex_count, index_array, &chunk->index_count);
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
	chunk->vertex_buffer = render_buffer_create(vertex_array, actual_vertices_size, RenderBufferUsage_VertexBuffer,
												RenderMemoryProperty_DeviceMappable, RenderMemoryMapping_MapWriteUnmap);
	chunk->index_buffer  = render_buffer_create(index_array,  actual_indices_size,  RenderBufferUsage_IndexBuffer,
												RenderMemoryProperty_DeviceMappable, RenderMemoryMapping_MapWriteUnmap);
	
	renderStats.totalVoxels += voxels_count;
	renderStats.totalVoxelChunks += 1;
	return chunk;
}


void
render_voxel_delete_chunk(RenderVoxelChunk* chunk){
	Assert(renderStats.totalVoxelChunks > 0);
	
	//dealloc GPU buffers
	render_buffer_delete(chunk->vertex_buffer);
	render_buffer_delete(chunk->index_buffer);
	
	//dealloc chunk arena
	memory_delete_arena(chunk->arena);
	
	//delete the chunk (and set it to hidden since for_pool() doesn't skip deleted chunks)
	memory_pool_delete(render_voxel_chunk_pool, chunk);
	chunk->hidden = true;
	
	renderStats.totalVoxels -= chunk->voxel_count;
	renderStats.totalVoxelChunks -= 1;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render_shared_other
void
render_display_stats(){
	FixMe;
    // using namespace UI;
    // BeginRow(str8_lit("renderstatsaligned"), 2, 0, UIRowFlags_AutoSize);{
    //     RowSetupColumnAlignments({{0,0.5},{1,0.5}});
    //     TextF(str8_lit("total triangles: %d"), renderStats.totalTriangles);
    //     TextF(str8_lit("total vertices: %d"),  renderStats.totalVertices);
    //     TextF(str8_lit("total indices: %d"),   renderStats.totalIndices);
    //     TextF(str8_lit("drawn triangles: %d"), renderStats.drawnTriangles);
    //     TextF(str8_lit("drawn indicies: %d"),  renderStats.drawnIndices);
    //     TextF(str8_lit("render time: %g"),     renderStats.renderTimeMS);
    // }EndRow();
}


