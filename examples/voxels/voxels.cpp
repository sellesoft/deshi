#include "deshi.h"
#include "core/camera.h"
#include <random>

int main(int args_count, char** args){
	deshi_init();
	
	//init ui
	{
	g_ui->base.style.font        = assets_font_create_from_file_bdf(STR8("gohufont-11.bdf"));
	g_ui->base.style.font_height = 11;
	g_ui->base.style.text_color  = Color_White;
	}
	
	//init camera
	Camera camera;
	{
		camera.position = Vec3(4,    3, -4);
		camera.rotation = Vec3(28, -45,  0);
		camera.nearZ    = .01f;
	camera.farZ     = 10000.f;
	camera.fov      = 90.f;
	camera.forward  = vec3_FORWARD();
	camera.right    = vec3_RIGHT();
	camera.up       = vec3_UP();
	camera.viewMat  = Math::LookAtMatrix(vec3_ZERO(), vec3_FORWARD()).Inverse();
		camera.projMat  = Math::PerspectiveProjectionMatrix(g_window->width, g_window->height, camera.fov, camera.nearZ, camera.farZ);
		window_cursor_mode(g_window, CursorMode_FirstPerson);
	}
	
	//init voxels
	enum{
	VoxelType_Empty,
	VoxelType_Rock,
	VoxelType_Ice,
	VoxelType_Core,
	VoxelType_COUNT
};
RenderVoxelType voxel_types[VoxelType_COUNT] = {
	{ 0 },
	{ PackColorU32( 12, 12, 12,255) },
	{ PackColorU32( 92,166,181,255) },
	{ PackColorU32(214,107,  0,255) },
};
	render_voxel_init(voxel_types, VoxelType_COUNT, 1);
	
	//generate voxel astroids
	RenderVoxel* asteroid_voxels = 0;
	arrsetcap(asteroid_voxels, ASTEROIDS_SIZE_RADIUS_MAX*ASTEROIDS_COUNT*sizeof(RenderVoxel));
	{
		#define ASTEROIDS_RING_RADIUS_INNER_SQ 1000*1000
#define ASTEROIDS_RING_RADIUS_OUTER_SQ 1500*1500
#define ASTEROIDS_RING_HEIGHT_MAX 64
		#define ASTEROIDS_SIZE_RADIUS_MIN 8
#define ASTEROIDS_SIZE_RADIUS_MAX 32
		#define ASTEROIDS_COUNT 8192
		
		//setup rng
		std::random_device rng_device;
		std::default_random_engine rng_engine(rng_device());
		std::uniform_real_distribution<f32> rng_dist_alpha(0, 1);
		std::uniform_real_distribution<f32> rng_dist_ring_angle(0, M_TAU);
		std::uniform_real_distribution<f32> rng_dist_ring_height(-ASTEROIDS_RING_HEIGHT_MAX, ASTEROIDS_RING_HEIGHT_MAX);
		std::uniform_real_distribution<s32> rng_dist_size_radius(-ASTEROIDS_SIZE_RADIUS_MIN, ASTEROIDS_SIZE_RADIUS_MAX);
		
		forI(ASTEROIDS_COUNT){
			//asteroid position
			f32 ring_theta  = rng_dist_ring_angle(rng_engine);
			f32 ring_radius_alpha = rng_dist_alpha(rng_engine);
			f32 ring_radius = Sqrt(((1.0f - ring_radius_alpha) * ASTEROIDS_RING_RADIUS_INNER_SQ) + (ring_radius_alpha * ASTEROIDS_RING_RADIUS_OUTER_SQ));
			f32 ring_height = rng_dist_ring_height(rng_engine);
			vec3 position = Vec3(ring_radius * cos(ring_theta), ring_height, ring_radius * sin(ring_theta));
			position = vec3_floor(position);
			
			//asteroid rotation
			f32 rotation_x = Degrees(rng_dist_ring_angle(rng_engine));
			f32 rotation_y = Degrees(rng_dist_ring_angle(rng_engine));
			f32 rotation_z = Degrees(rng_dist_ring_angle(rng_engine));
				vec3 rotation = Vec3(rotation_x, rotation_y, rotation_z);
			
			//asteroid size
			s32 radius = rng_dist_size_radius(rng_engine);
			//s32 radius_x = rng_dist_size_radius(rng_engine);
			 //s32 radius_y = rng_dist_size_radius(rng_engine);
			//s32 radius_z = rng_dist_size_radius(rng_engine);
			
			//draw voxel sphere
			{ //!ref: https://web.archive.org/web/20120422045142/https://banu.com/blog/7/drawing-circles/
				s32 x, y, z;
			s32 r_sq, z_sq, y_sq;
			s32 z_sq_new, y_sq_new;
			s32 test_z, test_y;
			s32 iteration_count = (radius * 185363) >> 18; //radius * cos(pi/4) = 185363 / 2^18 (approx)
			
			 y = radius; //at x=0, y=radius
				r_sq = y_sq = y * y;
				test_y = (2 * y) - 1;
				y_sq_new = r_sq + 3;
			
			for(y = 0; y <= iteration_count; ++y){
					y_sq_new -= (2 * x) - 3;
					if((y_sq - y_sq_new) >= test_y){
						y_sq -= test_y;
						y -= 1;
						test_y -= 2;
					}
					
				for(x = 0; x <= iteration_count; ++x){
					
					
					arrput(asteroid_voxels, RenderVoxel{VoxelType_Rock,  x,  y,  z});
					arrput(asteroid_voxels, RenderVoxel{VoxelType_Rock,  x,  y, -z});
					arrput(asteroid_voxels, RenderVoxel{VoxelType_Rock, -x,  y,  z});
					arrput(asteroid_voxels, RenderVoxel{VoxelType_Rock, -x,  y, -z});
					
					arrput(asteroid_voxels, RenderVoxel{VoxelType_Rock,  z,  y,  x});
					arrput(asteroid_voxels, RenderVoxel{VoxelType_Rock,  z,  y, -x});
					arrput(asteroid_voxels, RenderVoxel{VoxelType_Rock, -z,  y,  x});
					arrput(asteroid_voxels, RenderVoxel{VoxelType_Rock, -z,  y, -x});
				}
				}
			}
		}
	}
	
	deshi_loop_start();{
	//update camera
	if(!g_window->minimized){
		vec3 inputs = vec3_ZERO();
		if(key_down(Key_W))     inputs += camera.forward;
		if(key_down(Key_S))     inputs -= camera.forward;
		if(key_down(Key_D))     inputs += camera.right;
		if(key_down(Key_A))     inputs -= camera.right;
		if(key_down(Key_SPACE)) inputs += camera.up;
		if(key_down(Key_LCTRL)) inputs -= camera.up;
		if     (input_lshift_down()){ camera.position += inputs * 32.f * (g_time->deltaTime / 1000); }
		else if(input_lalt_down())  { camera.position += inputs * 4.f  * (g_time->deltaTime / 1000); }
		else                        { camera.position += inputs * 8.f  * (g_time->deltaTime / 1000); }
		
		camera.rotation.y += (g_input->mouseX - (f32)g_window->center.x) * .075f;
		camera.rotation.x += (g_input->mouseY - (f32)g_window->center.y) * .075f;
		camera.rotation.x = Clamp(camera.rotation.x, -89.0f, 89.0f);
		if(camera.rotation.y >  1440.f) camera.rotation.y -= 1440.f;
		if(camera.rotation.y < -1440.f) camera.rotation.y += 1440.f;
		
		camera.forward = vec3_normalized(vec3_FORWARD() * mat4::RotationMatrix(camera.rotation));
		camera.right   = vec3_normalized(vec3_cross(vec3_UP(), camera.forward));
		camera.up      = vec3_normalized(vec3_cross(camera.forward, camera.right));
		
		camera.viewMat = Math::LookAtMatrix(camera.position, camera.position + camera.forward).Inverse();
		if(g_window->resized){
			camera.projMat = Math::PerspectiveProjectionMatrix(g_window->width, g_window->height, camera.fov, camera.nearZ, camera.farZ);
		}
		
		render_update_camera_view(&camera.viewMat);
		}
		
		//draw grid
		{
			int lines = 100;
		f32 xp = floor(camera.position.x) + lines;
		f32 xn = floor(camera.position.x) - lines;
		f32 zp = floor(camera.position.z) + lines;
		f32 zn = floor(camera.position.z) - lines;
		
		color color(50, 50, 50);
		for(int i = 0; i < lines * 2 + 1; i++){
			vec3 v1 = Vec3(xn + i, 0, zn);
			vec3 v2 = Vec3(xn + i, 0, zp);
			vec3 v3 = Vec3(xn, 0, zn + i);
			vec3 v4 = Vec3(xp, 0, zn + i);
			
			if(xn + i != 0) render_line3(v1, v2, color);
			if(zn + i != 0) render_line3(v3, v4, color);
		}
		
		render_line3(Vec3(-1000,0,0), Vec3(1000,0,0), Color_Red);
		render_line3(Vec3(0,-1000,0), Vec3(0,1000,0), Color_Green);
		render_line3(Vec3(0,0,-1000), Vec3(0,0,1000), Color_Blue);
		}
	
	//update ui
	uiImmediateB();{
		persist Type anchor = anchor_top_left;
		uiItem* window = uiItemB();
		if(Math::PointInRectangle(input_mouse_position(),window->spos,window->size)) anchor = (anchor+1) % (anchor_bottom_left+1);
		window->style.sizing           = size_auto;
		window->style.background_color = Color_Black;
		window->style.positioning      = pos_absolute;
		window->style.anchor           = anchor;
		window->id                     = STR8("voxels.info_window");
		uiTextM(ToString8(deshi_temp_allocator, (int)F_AVG(100,1000/DeshTime->deltaTime),        " fps"));
			uiTextM(ToString8(deshi_temp_allocator, (int)F_AVG(100,render_get_stats()->renderTimeMS)," ms render"));
		uiTextM(ToString8(deshi_temp_allocator, render_get_stats()->totalVoxels,                 " voxels"));
		uiTextM(ToString8(deshi_temp_allocator, render_get_stats()->totalTriangles,              " triangles"));
		uiTextM(ToString8(deshi_temp_allocator, render_get_stats()->totalVertices,               " vertices"));
		uiTextM(ToString8(deshi_temp_allocator, render_get_stats()->totalIndices,                " indices"));
		uiTextM(ToString8(deshi_temp_allocator, camera.position));
		uiTextM(ToString8(deshi_temp_allocator, camera.rotation));
		uiItemE();
	}uiImmediateE();
	}deshi_loop_end();
	
	deshi_cleanup();
}