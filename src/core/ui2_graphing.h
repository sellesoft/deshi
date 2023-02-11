/* deshi UI Graphing Module
Index:
@ui_graphing_vec2g
@ui_graphing_graph_cartesian
@ui_graphing_graph_cartesian_impl

TODO:
- clamp axes to edge of camera (when out of frame or always)
- replace our to_string with a local version to allow for switching to scientific notation at high zoom
- cleanup the massive amount of repetition throughout the code
*/
#ifndef UI2_GRAPHING_H
#define UI2_GRAPHING_H
#include "ui2.h"
#include "kigu/unicode.h"


#ifdef __cplusplus
StartLinkageC();
#endif //#ifdef __cplusplus


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_graphing_vec2g


#ifndef scalar_t
#  define scalar_t f64
#endif //#ifndef scalar_t

    typedef union vec2g{
        scalar_t arr[2] = {0};
        struct{scalar_t x,y;}; //TODO consider adding more common math independent/dependent var names
    }vec2g;


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_graphing_graph_cartesian


typedef u32 GraphAxesLabelStyle; enum{
    GraphAxesLabelStyle_OnAxes,       //draws the axes labels next to their respective axes
    //TODO GraphAxesLabelStyle_OnGraphEdges, //draws the axes labels at the edges of the graph
};

typedef struct uiGraphCartesian{
	//// UI Properties ////
	uiItem item;
	u32    hash;
	
	//// Graph Properties ////
    b32   x_axis_shown; //default: true
    b32   y_axis_shown; //default: true
    b32   x_axis_coords_shown; //default: true
    b32   y_axis_coords_shown; //default: true
    color x_axis_color; //default: Color_White
    color y_axis_color; //default: Color_White
	str8  x_axis_label;
    str8  y_axis_label;
    
    b32      x_major_gridline_shown; //default: true
    b32      y_major_gridline_shown; //default: true
    b32      x_major_gridline_coords_shown; //default: true
    b32      y_major_gridline_coords_shown; //default: true
    color    x_major_gridline_color; //default: color(70,70,70)
    color    y_major_gridline_color; //default: color(70,70,70)
	scalar_t x_major_gridline_increment; //default: 1
	scalar_t y_major_gridline_increment; //default: 1
    
	b32      x_minor_gridline_shown; //default: true
	b32      y_minor_gridline_shown; //default: true
	color    x_minor_gridline_color; //default: color(40,40,40)
    color    y_minor_gridline_color; //default: color(40,40,40)
	scalar_t x_minor_gridline_increment; //default: 0.2
	scalar_t y_minor_gridline_increment; //default: 0.2
    
	//determines where on the graph axes labels will appear
    GraphAxesLabelStyle axes_label_style; //default: GraphAxesLabelStyle_OnAxes
    //TODO axes label alignment options
	
	u32 max_number_string_length; //default: 8
	
    //// Graph Data ////
	vec2g* points_array;
	u64    points_count;
    u64    points_size; //pixel size, default: 1
	b32    points_changed; //manual switch to say that the data changes, default: false
	
	//// Camera Properties ////
    vec2g    camera_position; //center of 'camera' in graph space, default {0,0}
	scalar_t camera_zoom; //represets half of the width the camera can see, so if this is 5 and position is 0,0 then you see from -5 to 5
}uiGraphCartesian;

//Makes a cartesian graph widget
uiGraphCartesian* ui_graph_make_cartesian();


#ifdef __cplusplus
EndLinkageC();
#endif //#ifdef __cplusplus
#endif //UI2_GRAPHING_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(DESHI_IMPLEMENTATION) && !defined(DESHI_UI2_GRAPHING_IMPL)
#define DESHI_UI2_GRAPHING_IMPL


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_graphing_graph_cartesian_impl


#define uiGetGraphCartesian(x) CastFromMember(uiGraphCartesian, item, x)

void
ui_graph_gen_cartesian(uiItem* item){
	uiGraphCartesian* graph = uiGetGraphCartesian(item);
	
	vec2g    dims         = vec2g{(f64)(item->size.x * item->scale.x), (f64)(item->size.y * item->scale.y)};
	vec2g    cam_pos      = vec2g{graph->camera_position.x, -graph->camera_position.y}; //flip the y-axis for camera space
	scalar_t cam_zoom     = graph->camera_zoom;
	scalar_t view_width   = cam_zoom * 2;
	scalar_t aspect_ratio = dims.y / dims.x;
	vec2g    unit_length  = vec2g{dims.x / view_width, dims.y / view_width};
	
	scalar_t order_of_magnitude = floor(log10(cam_zoom));
	scalar_t power_of_ten       = pow(10,order_of_magnitude);
	
	vec2g top_left  = vec2g{cam_pos.x - cam_zoom, (cam_pos.y - cam_zoom) * aspect_ratio};
	vec2g bot_right = vec2g{cam_pos.x + cam_zoom, (cam_pos.y + cam_zoom) * aspect_ratio};
	
	b32 x_axis_visible = graph->x_axis_shown && (top_left.y < 0) && (bot_right.y > 0);
	b32 y_axis_visible = graph->y_axis_shown && (top_left.x < 0) && (bot_right.x > 0);
	
	//round each corner to the closest order of magnitude
	 scalar_t x_top_left_oom_rounded  = floor(top_left.x  / power_of_ten) * power_of_ten;
	scalar_t y_top_left_oom_rounded  = floor(top_left.y  / power_of_ten) * power_of_ten;
	scalar_t x_bot_right_oom_rounded = floor(bot_right.x / power_of_ten) * power_of_ten;
	scalar_t y_bot_right_oom_rounded = floor(bot_right.y / power_of_ten) * power_of_ten;
	scalar_t x_cam_pos_oom_rounded   = floor(cam_pos.x   / power_of_ten) * power_of_ten;
	scalar_t y_cam_pos_oom_rounded   = floor(cam_pos.y   / power_of_ten) * power_of_ten;
	
	vec2g    center_item_space   = vec2g{dims.x / 2.0, dims.y / 2.0};
	scalar_t x_origin_item_space = center_item_space.x - (cam_pos.x * unit_length.x);
	scalar_t y_origin_item_space = center_item_space.y - (cam_pos.y * unit_length.y * aspect_ratio);
	
	scalar_t x_minor_increment_scaled = graph->x_minor_gridline_increment * power_of_ten;
	scalar_t y_minor_increment_scaled = graph->y_minor_gridline_increment * power_of_ten;
	scalar_t minor_left_edge_rounded = ceil(top_left.x / x_minor_increment_scaled) * x_minor_increment_scaled;
	scalar_t minor_top_edge_rounded  = ceil(top_left.y / y_minor_increment_scaled) * y_minor_increment_scaled;
	
	scalar_t x_major_increment_scaled = graph->x_major_gridline_increment * power_of_ten;
	scalar_t y_major_increment_scaled = graph->y_major_gridline_increment * power_of_ten;
	scalar_t major_left_edge_rounded = ceil(top_left.x / x_major_increment_scaled) * x_major_increment_scaled;
	scalar_t major_top_edge_rounded  = ceil(top_left.y / y_major_increment_scaled) * y_major_increment_scaled;
	
	u64 x_minor_line_count = (u64)floor((bot_right.x - minor_left_edge_rounded) / x_minor_increment_scaled);
	u64 y_minor_line_count = (u64)floor((bot_right.y - minor_top_edge_rounded ) / y_minor_increment_scaled);
	u64 x_major_line_count = (u64)floor((bot_right.x - major_left_edge_rounded) / x_major_increment_scaled);
	u64 y_major_line_count = (u64)floor((bot_right.y - major_top_edge_rounded ) / y_major_increment_scaled);
	
	u64 reserved_vertices = 0;
	u64 reserved_indices  = 0;
	{
	//background
	if(item->style.background_color.a != 0){
		reserved_vertices += 4;
		reserved_indices  += 6;
	}
	
	//border
	if(item->style.border_style != border_none){
		reserved_vertices += 8;
		reserved_indices  += 24;
	}
	
	//minor gridlines
	if(graph->x_minor_gridline_shown){
		reserved_vertices += 4 * x_minor_line_count;
		reserved_indices  += 6 * x_minor_line_count;
	}
	if(graph->y_minor_gridline_shown){
		reserved_vertices += 4 * y_minor_line_count;
		reserved_indices  += 6 * y_minor_line_count;
	}
	
	//major gridlines
	if(graph->x_major_gridline_shown){
		reserved_vertices += 4 * x_major_line_count;
		reserved_indices  += 6 * x_major_line_count;
	}
	if(graph->y_major_gridline_shown){
		reserved_vertices += 4 * y_major_line_count;
		reserved_indices  += 6 * y_major_line_count;
		}
		if(graph->x_major_gridline_coords_shown){
			reserved_vertices += 4 * x_major_line_count * graph->max_number_string_length;
		reserved_indices  += 6 * x_major_line_count * graph->max_number_string_length;
		}
		if(graph->y_major_gridline_coords_shown){
			reserved_vertices += 4 * y_major_line_count * graph->max_number_string_length;
		reserved_indices  += 6 * y_major_line_count * graph->max_number_string_length;
		}
		
		//x axis
		if(graph->x_axis_shown){
			reserved_vertices += 4;
			reserved_indices  += 6;
		}
		if(graph->x_axis_coords_shown){
			reserved_vertices += 4 * graph->max_number_string_length;
			reserved_indices  += 6 * graph->max_number_string_length;
		}
		if(graph->x_axis_label.count){
			reserved_vertices += 4 * graph->x_axis_label.count;
			reserved_indices  += 6 * graph->x_axis_label.count;
		}
		
		//y axis
		if(graph->y_axis_shown){
			reserved_vertices += 4;
			reserved_indices  += 6;
		}
		if(graph->y_axis_coords_shown){
			reserved_vertices += 4 * graph->max_number_string_length;
			reserved_indices  += 6 * graph->max_number_string_length;
		}
		if(graph->y_axis_label.count){
			reserved_vertices += 4 * graph->y_axis_label.count;
			reserved_indices  += 6 * graph->y_axis_label.count;
		}
		
		//data
		if(graph->points_array){
		reserved_vertices += 4 * graph->points_count;
			reserved_indices  += 6 * graph->points_count;
		}
	}
	
{//TODO protection against more than max vertices/indices
		item->drawcmd_count = 1;
	item->drawcmds = ui_make_drawcmd(item->drawcmd_count);
		
		ui_drawcmd_alloc(item->drawcmds, vec2i{reserved_vertices,reserved_indices});
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
		vec2i  counts = vec2i{0,0};
		
		vec2 item_margin_top_left  = floor(item->style.margintl * item->scale);
		vec2 item_border_size      = floor((item->style.border_style ? item->style.border_width : 0) * item->scale);
		vec2 item_content_top_left = item->pos_screen + item_margin_top_left + item_border_size;
		
		//background
		counts += ui_gen_background(item, vp, ip, counts);
		
		//minor gridlines
		if(graph->x_minor_gridline_shown){
			scalar_t cursor = minor_left_edge_rounded;
			while(cursor < bot_right.x){
				scalar_t offset = (cursor - top_left.x) * unit_length.x;
				counts += render_make_filledrect(vp, ip, counts, Vec2(), Vec2(), x_minor_gridline_color);
				cursor += x_minor_increment_scaled;
			}
	}
	if(graph->y_minor_gridline_shown){
	}
	
	//major gridlines
	if(graph->x_major_gridline_shown){
	}
	if(graph->y_major_gridline_shown){
		}
		if(graph->x_major_gridline_coords_shown){
		}
		if(graph->y_major_gridline_coords_shown){
		}
		
		//x axis
		if(graph->x_axis_shown){
		}
		if(graph->x_axis_coords_shown){
		}
		
		//y axis
		if(graph->y_axis_shown){
		}
		if(graph->y_axis_coords_shown){
		}
		
		//axes labels
		switch(graph->axes_label_style){
			case GraphAxesLabelStyle_OnAxes:{
				if(graph->x_axis_label.count){
					
		}
				if(graph->y_axis_label.count){
					
			}
			}break;
		}
		
		//data
		if(graph->points_array){
		}
		
		//border
	counts += ui_gen_border(item, vp, ip, counts);
		
	dc->counts_used = counts;
	}
}

u32
ui_graph_hash_cartesian(uiItem* item){ //TODO(delle) consider using a 64bit hash for ui_graph_hash()
	uiGraphCartesian* graph = uiGetGraphCartesian(item);
	u32 seed = UI_HASH_SEED;
	UI_HASH_VAR(graph->x_axis_shown);
	UI_HASH_VAR(graph->y_axis_shown);
	UI_HASH_VAR(graph->x_axis_coords_shown);
	UI_HASH_VAR(graph->y_axis_coords_shown);
	UI_HASH_VAR(graph->x_axis_color.rgba);
	UI_HASH_VAR(graph->y_axis_color.rgba);
	UI_HASH_VAR(str8_hash32(graph->x_axis_label));
	UI_HASH_VAR(str8_hash32(graph->y_axis_label));
	UI_HASH_VAR(graph->x_major_gridline_shown);
	UI_HASH_VAR(graph->y_major_gridline_shown);
	UI_HASH_VAR(graph->x_major_gridline_coords_shown);
	UI_HASH_VAR(graph->y_major_gridline_coords_shown);
	UI_HASH_VAR(graph->x_major_gridline_color.rgba);
	UI_HASH_VAR(graph->y_major_gridline_color.rgba);
	UI_HASH_VAR((f32)graph->x_major_gridline_increment);
	UI_HASH_VAR((f32)graph->y_major_gridline_increment);
	UI_HASH_VAR(graph->x_minor_gridline_shown);
	UI_HASH_VAR(graph->y_minor_gridline_shown);
	UI_HASH_VAR(graph->x_minor_gridline_color.rgba);
	UI_HASH_VAR(graph->y_minor_gridline_color.rgba);
	UI_HASH_VAR((f32)graph->x_minor_gridline_increment);
	UI_HASH_VAR((f32)graph->y_minor_gridline_increment);
	UI_HASH_VAR(graph->axes_label_style);
	UI_HASH_VAR(graph->point_array);
	UI_HASH_VAR(graph->point_count);
	UI_HASH_VAR(graph->point_size);
	UI_HASH_VAR(graph->points_changed);
	UI_HASH_VAR(graph->camera_position);
	UI_HASH_VAR(graph->camera_zoom);
	return seed;
}

uiGraphCartesian*
ui_graph_make_cartesian(){
	uiItemSetup setup = {0};
	setup.size = sizeof(uiGraphCartesian);
	setup.generate = &ui_graph_gen_cartesian;
	setup.hash = &ui_graph_hash_cartesian;
	
	b32 retrieved = false;
	uiItem* item = ui_setup_item(setup, &retrieved);
	uiGraphCartesian* graph = uiGetGraphCartesian(item);
	
	if(!retrieved){ //new item
		graph->x_axis_shown = true;
       graph->y_axis_shown = true;
    graph->x_axis_coords_shown = true;
    graph->y_axis_coords_shown = true;
    graph->x_axis_color = Color_White;
    graph->y_axis_color = Color_White;
	graph->x_axis_label = str8{};
		graph->y_axis_label = str8{};
    graph->x_major_gridline_shown = true;
    graph->y_major_gridline_shown = true;
    graph->x_major_gridline_coords_shown = true;
    graph->y_major_gridline_coords_shown = true;
    graph->x_major_gridline_color = color(70,70,70);
    graph->y_major_gridline_color = color(70,70,70);
	graph->x_major_gridline_increment = 1;
	graph->y_major_gridline_increment = 1;
	graph->x_minor_gridline_shown = true;
	graph->y_minor_gridline_shown = true;
	graph->x_minor_gridline_color = color(40,40,40);
		graph->y_minor_gridline_color = color(40,40,40);
	graph->x_minor_gridline_increment = 0.2;
		graph->y_minor_gridline_increment = 0.2;
		graph->axes_label_style = GraphAxesLabelStyle_OnAxes;
		graph->max_number_string_length = 8;
		graph->point_array = 0;
		graph->point_count = 0;
		graph->point_size = 1;
		graph->camera_position = Vec2(0,0);
		graph->camera_zoom = 5;
	}
	
	return graph;
}

#endif //defined(DESHI_IMPLEMENTATION) && !defined(DESHI_UI2_GRAPHING_IMPL)