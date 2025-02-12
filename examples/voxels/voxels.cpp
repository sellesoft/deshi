#include <random>
#include "deshi.h"
#include "core/baked/fonts.h"
#include "kigu/array.h"

int main(int args_count, char** args){
	deshi_init();
	
	//init ui
	{
		Font* default_font = assets_font_get_by_name(STR8("baked_gohufont_11_bdf"));
		if(!default_font){
			default_font = assets_font_create_from_memory_bdf(baked_font_gohufont_11_bdf.str, baked_font_gohufont_11_bdf.count, STR8("baked_gohufont_11_bdf"));
		}
		
		g_ui->base.style.font        = default_font;
		g_ui->base.style.font_height = 11;
		g_ui->base.style.text_color  = Color_White;
	}
	
	//init camera
	{
		camera->position = Vec3(4,    3, -4);
		camera->rotation = Vec3(28, -45,  0);
		camera->forward  = vec3_normalized(vec3_FORWARD() * mat4::RotationMatrix(camera->rotation));
		camera->right    = vec3_RIGHT();
		camera->up       = vec3_UP();
		render_camera_update_view(camera);
		render_temp_update_camera(camera->position, camera->position + camera->forward);
		render_camera_update_perspective_projection(camera, window->width, window->height, 90.f, .01f, 10000.f);
	}
	
	//init voxels
	enum{
		VoxelType_Empty,
		VoxelType_0,
		VoxelType_1,
		VoxelType_2,
		VoxelType_3,
		VoxelType_4,
		VoxelType_5,
		VoxelType_6,
		VoxelType_7,
		VoxelType_Rock,
		VoxelType_Ice,
		VoxelType_Core,
		VoxelType_COUNT
	};
	RenderVoxelType voxel_types[VoxelType_COUNT] = {
		{ PackColorU32(  0,  0,  0,  0) },
		{ PackColorU32(200,  0,  0,255) },
		{ PackColorU32(  0,200,  0,255) },
		{ PackColorU32(  0,  0,200,255) },
		{ PackColorU32(200,200,  0,255) },
		{ PackColorU32(  0,200,200,255) },
		{ PackColorU32(200,200,200,255) },
		{ PackColorU32(200,  0,200,255) },
		{ PackColorU32( 36, 36, 36,255) },
		{ PackColorU32( 12, 12, 12,255) },
		{ PackColorU32( 92,166,181,255) },
		{ PackColorU32(214,107,  0,255) },
	};
	render_voxel_init(voxel_types, VoxelType_COUNT, 1);
	
	//generate voxel astroids
	{
#define ASTEROIDS_RING_RADIUS_INNER_SQ 50*50//1000*1000
#define ASTEROIDS_RING_RADIUS_OUTER_SQ 75*75//1500*1500
#define ASTEROIDS_RING_HEIGHT_MAX 25//64
#define ASTEROIDS_SIZE_RADIUS_MIN 4
#define ASTEROIDS_SIZE_RADIUS_MAX 10//32
#define ASTEROIDS_COUNT 10//8192
		//setup rng
		std::random_device rng_device;
		std::default_random_engine rng_engine(rng_device());
		std::uniform_real_distribution<f32> rng_dist_alpha(0, 1);
		std::uniform_real_distribution<f32> rng_dist_ring_angle(0, M_TAU);
		std::uniform_real_distribution<f32> rng_dist_ring_height(-ASTEROIDS_RING_HEIGHT_MAX, ASTEROIDS_RING_HEIGHT_MAX);
		std::uniform_int_distribution<s32>  rng_dist_size_radius(ASTEROIDS_SIZE_RADIUS_MIN, ASTEROIDS_SIZE_RADIUS_MAX);
		
		RenderVoxel* asteroid_voxels = 0;
		array_init(asteroid_voxels, ASTEROIDS_SIZE_RADIUS_MIN*ASTEROIDS_SIZE_RADIUS_MAX, deshi_allocator);
		forI(ASTEROIDS_COUNT){
			//asteroid position
			f32 ring_theta  = rng_dist_ring_angle(rng_engine);
			f32 ring_radius_alpha = rng_dist_alpha(rng_engine);
			f32 ring_radius = Sqrt(((1.0f - ring_radius_alpha) * ASTEROIDS_RING_RADIUS_INNER_SQ) + (ring_radius_alpha * ASTEROIDS_RING_RADIUS_OUTER_SQ));
			f32 ring_height = rng_dist_ring_height(rng_engine);
			vec3 position = Vec3(ring_radius * cos(ring_theta), ring_height, ring_radius * sin(ring_theta));
			position = vec3_floor(position);
			//position = vec3_ZERO();
			
			//asteroid rotation
			f32 rotation_x = Degrees(rng_dist_ring_angle(rng_engine));
			f32 rotation_y = Degrees(rng_dist_ring_angle(rng_engine));
			f32 rotation_z = Degrees(rng_dist_ring_angle(rng_engine));
			vec3 rotation = Vec3(rotation_x, rotation_y, rotation_z);
			rotation = vec3_ZERO(); //TODO(delle) remove this
			
			//asteroid size
			s32 radius = rng_dist_size_radius(rng_engine);
			//s32 radius_x = rng_dist_size_radius(rng_engine);
			//s32 radius_y = rng_dist_size_radius(rng_engine);
			//s32 radius_z = rng_dist_size_radius(rng_engine);
			
			//draw voxel sphere
			//TODO(delle) test and draw per-quadrant
			u16 voxel_type = VoxelType_0;
			for(u16 y = 0; y <= radius+radius; ++y){
				for(u16 x = 0; x <= radius+radius; ++x){
					for(u16 z = 0; z <= radius+radius; ++z){
						if(radius == (u16)sqrt((radius-x)*(radius-x) + (radius-y)*(radius-y) + (radius-z)*(radius-z))){
							array_push_value(asteroid_voxels, (RenderVoxel{voxel_type, x, y, z}));
						}
					}
				}
				
				//debug coloring
				voxel_type += 1; //TODO(delle) remove this
				if(voxel_type > VoxelType_7) voxel_type = VoxelType_0;
			}
			
			//upload voxels to chunk
			render_voxel_chunk_create(position, rotation, (radius*2)+1, asteroid_voxels, array_count(asteroid_voxels));
			array_clear(asteroid_voxels);
		}
		array_deinit(asteroid_voxels);
	}
	
	f32 move_speed = 8.0f;
	b32 draw_grid = true;
	deshi_loop_start();{
		//// update camera ////
		if(g_window->focused && !console_is_open()){
			if(key_pressed(Key_C)){
				window_set_cursor_mode(window, (window->cursor_mode == CursorMode_FirstPerson) ? CursorMode_Default : CursorMode_FirstPerson);
			}
			if(key_pressed(Mouse_RIGHT)){
				window_set_cursor_mode(window, CursorMode_FirstPerson);
			}
			if(window->cursor_mode == CursorMode_FirstPerson){
				if(key_released(Mouse_RIGHT)){
					window_set_cursor_mode(window, CursorMode_Default);
				}
				
				if(g_input->scrollY != 0){
					if(g_input->scrollY > 0){
						move_speed *= 2.0f;
					}else{
						move_speed *= 0.5f;
						move_speed = ClampMin(move_speed, 1.0f);
					}
				}
				
				vec3 inputs = vec3_ZERO();
				if(key_down(Key_W))     inputs += camera->forward;
				if(key_down(Key_S))     inputs -= camera->forward;
				if(key_down(Key_D))     inputs += camera->right;
				if(key_down(Key_A))     inputs -= camera->right;
				if(key_down(Key_SPACE)) inputs += camera->up;
				if(key_down(Key_LCTRL)) inputs -= camera->up;
				
				f32 multiplier = 1.0f;
				if(input_lshift_down()){
					multiplier = 4.0f;
				}else if(input_lalt_down()){
					multiplier = 0.5f;
				}
				
				camera->position += inputs * move_speed * multiplier * (g_time->deltaTime / 1000);
				
				camera->rotation.y += (g_input->mouseX - (f32)g_window->center.x) * .075f;
				camera->rotation.x += (g_input->mouseY - (f32)g_window->center.y) * .075f;
				camera->rotation.x = Clamp(camera->rotation.x, -89.0f, 89.0f);
				if(camera->rotation.y >  1440.f) camera->rotation.y -= 1440.f;
				if(camera->rotation.y < -1440.f) camera->rotation.y += 1440.f;
				
				camera->forward = vec3_normalized(vec3_FORWARD() * mat4::RotationMatrix(camera->rotation));
				render_camera_update_view(camera);
				render_temp_update_camera(camera->position, camera->position + camera->forward);
			}
		}
		if(g_window->resized){
			render_camera_update_perspective_projection(camera, window->width, window->height, 90.0f, 0.01f, 10000.0f);
			mat4 projection = Math::PerspectiveProjectionMatrix(window->width, window->height, 90.0f, 0.01f, 10000.0f);
#if DESHI_VULKAN
			camera->proj.arr[5] *= -1;
#endif //#if DESHI_VULKAN
			render_temp_set_camera_projection(projection);
		}
		
		//// update grid ////
		if(key_pressed(Key_G)){
			draw_grid = !draw_grid;
		}
		if(draw_grid){
			int lines = 100;
			f32 xp = floor(camera->position.x) + lines;
			f32 xn = floor(camera->position.x) - lines;
			f32 zp = floor(camera->position.z) + lines;
			f32 zn = floor(camera->position.z) - lines;
			color color(50, 50, 50);
			for(int i = 0; i < lines * 2 + 1; i++){
				vec3 v1 = Vec3(xn + i, 0, zn);
				vec3 v2 = Vec3(xn + i, 0, zp);
				vec3 v3 = Vec3(xn, 0, zn + i);
				vec3 v4 = Vec3(xp, 0, zn + i);
				
				if(xn + i != 0) render_temp_line(v1, v2, color);
				if(zn + i != 0) render_temp_line(v3, v4, color);
			}
			render_temp_line(Vec3(-1000,0,0), Vec3(1000,0,0), Color_Red);
			render_temp_line(Vec3(0,-1000,0), Vec3(0,1000,0), Color_Green);
			render_temp_line(Vec3(0,0,-1000), Vec3(0,0,1000), Color_Blue);
		}
		
		//// update ui ////
		ui_begin_immediate_branch(&g_ui->base);{
			persist Type anchor = anchor_top_left;
			uiItem* window = ui_begin_item(0);
			if(Math::PointInRectangle(input_mouse_position(),window->pos_screen,window->size)) anchor = (anchor+1) % (anchor_bottom_left+1);
			window->style.sizing           = size_auto;
			window->style.background_color = Color_Black;
			window->style.positioning      = pos_absolute;
			window->style.anchor           = anchor;
			window->id                     = STR8("voxels.info_window");
			ui_make_text(to_dstr8v(deshi_temp_allocator, (int)F_AVG(100,1000/DeshTime->deltaTime)," fps").fin, 0);
			ui_make_text(to_dstr8v(deshi_temp_allocator, (int)move_speed,                         " move speed").fin, 0);
			ui_make_text(to_dstr8p(camera->position, 2, deshi_temp_allocator).fin, 0);
			ui_make_text(to_dstr8p(camera->rotation, 2, deshi_temp_allocator).fin, 0);
			ui_end_item();
		}ui_end_immediate_branch();
	}deshi_loop_end();
	deshi_cleanup();
}
