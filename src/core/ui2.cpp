#include "ui2.h"
#include "kigu/array.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/window.h"
#include "core/logger.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_memory
#define item_arena g_ui->item_list->arena
#define drawcmd_arena g_ui->drawcmd_list->arena

ArenaList* create_arena_list(ArenaList* old){DPZoneScoped;
	ArenaList* nual = (ArenaList*)memory_alloc(sizeof(ArenaList));
	if(old) NodeInsertNext(&old->node, &nual->node);
	nual->arena = memory_create_arena(Megabytes(1));
	return nual;
}

void* arena_add(Arena* arena, upt size){DPZoneScoped;
	if(arena->size < arena->used+size) 
		g_ui->item_list = create_arena_list(g_ui->item_list);
	u8* cursor = arena->cursor;
	arena->cursor += size;
	arena->used += size;
	return cursor;
}

void push_item(uiItem* item){DPZoneScoped;
	g_ui->item_stack.add(item);
}

uiItem* pop_item(){DPZoneScoped;
	uiItem* ret = *g_ui->item_stack.last;
	g_ui->item_stack.pop();
	return ret;
}

//@uiDrawCmd

//@Item

//@Window


void drawcmd_alloc(uiDrawCmd* drawcmd, RenderDrawCounts counts){DPZoneScoped;
	if(drawcmd_arena->size < drawcmd_arena->used + counts.vertices * sizeof(Vertex2) + counts.indices * sizeof(u32))
		g_ui->drawcmd_list = create_arena_list(g_ui->drawcmd_list);
	drawcmd->vertex_offset = g_ui->vertex_arena->used / sizeof(Vertex2);
	drawcmd->index_offset = g_ui->index_arena->used / sizeof(u32);
	g_ui->vertex_arena->cursor += counts.vertices * sizeof(Vertex2);
	g_ui->vertex_arena->used += counts.vertices * sizeof(Vertex2);
	g_ui->index_arena->cursor += counts.indices * sizeof(u32);
	g_ui->index_arena->used += counts.indices * sizeof(u32);
	drawcmd->counts = counts;
	drawcmd->texture = 0;
}

//@Functions

//@Helpers
#define item_error(item, ...)\
LogE("ui","Error on item created in ", (item)->file_created, " on line ", (item)->line_created, ": ", __VA_ARGS__)

//fills an item struct and make its a child of the current item
void ui_fill_item(uiItem* item, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* curitem = *g_ui->item_stack.last;
	
	insert_last(&curitem->node, &item->node);
	
	if(style) memcpy(&item->style, style, sizeof(uiStyle));
	else      memcpy(&item->style, ui_initial_style, sizeof(uiStyle));
	
	item->file_created = file;
	item->line_created = line;
}

//TODO(sushi) make an option for this to take into account wrapping
vec2 calc_text_size(uiItem* item){DPZoneScoped;
	if(item->tag != uiTextTag){
		item_error(item, "calc_text_size() was called on an item whose tag is not uiTextTag.");
		return vec2{0,0};
	}
	uiStyle* style = &item->style;
	str8 text = uiGetTextData(item)->text;
	vec2 result = vec2{0, (f32)style->font_height};
	f32 line_width = 0;
	switch(style->font->type){
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += style->font_height;
					line_width = 0;
				}
				line_width += style->font->max_width * style->font_height / style->font->aspect_ratio / style->font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += style->font_height;
					line_width = 0;
				}
				line_width += font_packed_char(style->font, codepoint)->xadvance * style->font_height / style->font->aspect_ratio / style->font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}



//@Functionality
void ui_gen_item(uiItem* item){DPZoneScoped;
	uiDrawCmd* dc = item->drawcmds;
	Vertex2* vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*     ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	RenderDrawCounts counts = {0};
	if(item->style.background_image){
		counts+=render_make_texture(vp,ip,counts,item->style.background_image,
			vec2(item->sx, item->sy),
			vec2(item->sx+item->width, item->sy),
			vec2(item->sx+item->width, item->sy+item->height),
			vec2(item->sx, item->sy+item->height),
			1,0,0
		);
	}
	else if(item->style.background_color.a){
		counts+=render_make_filledrect(vp, ip, counts, item->spos, item->size, item->style.background_color);
	}
	switch(item->style.border_style){
		case border_none:{}break;
		case border_solid:{
			vec2 tl = floor(item->spos);
			vec2 br = ceil(tl+item->size);
			vec2 tr = vec2{br.x, tl.y};
			vec2 bl = vec2{tl.x, br.y}; 
			u32 t = item->style.border_width;
			u32 v = counts.vertices; u32 i = counts.indices;
			ip[i+ 0] = v+0; ip[i+ 1] = v+1; ip[i+ 2] = v+3; 
			ip[i+ 3] = v+0; ip[i+ 4] = v+3; ip[i+ 5] = v+2; 
			ip[i+ 6] = v+2; ip[i+ 7] = v+3; ip[i+ 8] = v+5; 
			ip[i+ 9] = v+2; ip[i+10] = v+5; ip[i+11] = v+4; 
			ip[i+12] = v+4; ip[i+13] = v+5; ip[i+14] = v+7; 
			ip[i+15] = v+4; ip[i+16] = v+7; ip[i+17] = v+6; 
			ip[i+18] = v+6; ip[i+19] = v+7; ip[i+20] = v+1; 
			ip[i+21] = v+6; ip[i+22] = v+1; ip[i+23] = v+0;
			vp[v+0].pos = tl;              vp[v+0].uv = {0,0}; vp[v+0].color = item->style.border_color.rgba;
			vp[v+1].pos = tl+vec2i( t, t); vp[v+1].uv = {0,0}; vp[v+1].color = item->style.border_color.rgba;
			vp[v+2].pos = tr;              vp[v+2].uv = {0,0}; vp[v+2].color = item->style.border_color.rgba;
			vp[v+3].pos = tr+vec2i(-t, t); vp[v+3].uv = {0,0}; vp[v+3].color = item->style.border_color.rgba;
			vp[v+4].pos = br;              vp[v+4].uv = {0,0}; vp[v+4].color = item->style.border_color.rgba;
			vp[v+5].pos = br+vec2i(-t,-t); vp[v+5].uv = {0,0}; vp[v+5].color = item->style.border_color.rgba;
			vp[v+6].pos = bl;              vp[v+6].uv = {0,0}; vp[v+6].color = item->style.border_color.rgba;
			vp[v+7].pos = bl+vec2i( t,-t); vp[v+7].uv = {0,0}; vp[v+7].color = item->style.border_color.rgba;
			counts += {8, 24};
		}break;
	}
}


uiItem* ui_make_item(uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem));
	ui_fill_item(item, style, file, line);
	
	item->memsize = sizeof(uiItem);
	item->drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)); 
	
	RenderDrawCounts counts = //reserve enough room for a background and border 
		render_make_filledrect_counts() +
		render_make_rect_counts();

	item->draw_cmd_count = 1;
	drawcmd_alloc(item->drawcmds, counts);
	item->__generate = &ui_gen_item;
	return item;
}

uiItem* ui_begin_item(uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_item(style, file, line);
	push_item(item);
	return item;
}

void ui_end_item(str8 file, upt line){
	if(*(g_ui->item_stack.last) == &g_ui->base){
		LogE("ui", 
			 "In ", file, " at line ", line, " :\n",
			 "\tAttempted to end base item. Did you call uiItemE too many times? Did you use uiItemM instead of uiItemB?"
		);
		//TODO(sushi) implement a hint showing what instruction could possibly be wrong 
	}
	pop_item();
	
}

void ui_set_item_layer(uiItem* item, u32 idx, str8 file, upt line){
	uiItem* base = &g_ui->base;
	if(base->node.first_child->type != 0xff){
		
	}
}

//-/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @Slider

void ui_gen_slider(uiItem* item){DPZoneScoped;
	if(item->tag != uiSliderTag){
		item_error(item, "Slider generate function was called on an item whose tag is not uiSliderTag.");
		return;
	}

	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	RenderDrawCounts counts = {0};	
	uiSliderData* data = uiGetSliderData(item);

	counts+=render_make_filledrect(vp,ip,counts,
		vec2(item->spos.x, item->spos.y + item->size.y * data->style.rail_thickness / 2),
		vec2(item->size.x, item->size.y * data->style.rail_thickness),
		data->style.colors.rail
	);

	if(data->style.dragger_shape == slider_dragger_rect){
		vec2 dragp = vec2(item->spos.x+data->pos, item->spos.y);
		vec2 drags = vec2(data->width, item->height);
		counts+=render_make_filledrect(vp,ip,counts,dragp,drags,data->style.colors.dragger);
	}else if(data->style.dragger_shape == slider_dragger_round){
		NotImplemented;
	}
}

void __ui_slider_callback(uiItem* item){DPZoneScoped;
	if(item->tag != uiSliderTag){
		item_error(item, "Slider callback was called on an item whose tag is not uiSliderTag.");
		return;
	}
	uiSliderData* data = uiGetSliderData(item);
	vec2 mp = input_mouse_position();
	vec2 lmp = mp - item->spos;
	switch(data->type){
		case 0:{
			f32 dragpos;
			f32 dragwidth;
			f32  min = data->minf32;
			f32  max = data->maxf32;
			f32* var = data->varf32;
			*var = Clamp(*var, min, max);
			dragpos = Remap(*var, 0.f, item->width, min, max);
			dragwidth = item->width/8;
			data->width = dragwidth;
			data->pos = dragpos;
			if(input_lmouse_pressed() && Math::PointInRectangle(lmp, vec2(dragpos,0), vec2(dragwidth, item->height))){
				data->active = 1;
				data->mouse_offset = -lmp.x + dragpos;
			}
			if(data->active){
				*var = Remap(Clamp(lmp.x + data->mouse_offset, 0.f, item->width-dragwidth), min, max, 0.f, item->width);
				item->dirty = 1;
				item->action_trigger = action_act_always;
			}
			if(input_lmouse_released()){
				data->active = 0;
				item->action_trigger = action_act_mouse_hover;
			}

		}break;
	}
}

uiItem* __ui_make_slider(uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem) + sizeof(uiSliderData));
	ui_fill_item(item, style, file, line);

	item->tag = uiSliderTag;
	item->memsize = sizeof(uiItem) + sizeof(uiSliderData);
	item->drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd));
	item->__generate = &ui_gen_slider;
	item->trailing_data = item+sizeof(uiItem);

	uiSliderData* data = uiGetSliderData(item);

	RenderDrawCounts counts = //reserve enough room for slider rail, dragger, and outline
		render_make_filledrect_counts()*2+
		render_make_rect_counts();

	item->draw_cmd_count = 1;
	drawcmd_alloc(item->drawcmds, counts);

	item->action_trigger = action_act_mouse_hover;

	//setup trailing data 

	data->style.dragger_shape = slider_dragger_rect;
	data->style.rail_thickness = 1;
	data->style.colors.rail = color(80,80,80);
	data->style.colors.dragger = color(14,50,100);

	item->__hash = &slider_style_hash;
	return item;
}

uiItem* ui_make_slider_f32(f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = __ui_make_slider(style, file, line);
	
	item->tag = uiSliderTag;
	item->action = &__ui_slider_callback;
	uiGetSliderData(item)->minf32 = min;
	uiGetSliderData(item)->maxf32 = max;
	uiGetSliderData(item)->varf32 = var;
	uiGetSliderData(item)->type = 0;

	return item;
}

uiItem* ui_make_slider_u32(u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = __ui_make_slider(style, file, line);

	item->tag = uiSliderTag;
	item->action = &__ui_slider_callback;
	uiGetSliderData(item)->minu32 = min;
	uiGetSliderData(item)->maxu32 = max;
	uiGetSliderData(item)->varu32 = var;
	uiGetSliderData(item)->type = 1;
	
	return item;
}

uiItem* ui_make_slider_s32(s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = __ui_make_slider(style, file, line);

	item->tag = uiSliderTag;
	item->action = &__ui_slider_callback;
	uiGetSliderData(item)->mins32 = max;
	uiGetSliderData(item)->maxs32 = max;
	uiGetSliderData(item)->vars32 = var;
	uiGetSliderData(item)->type = 2;

	return item;
}

uiItem* ui_make_window(str8 name, Flags flags, uiStyle* style, str8 file, upt line){DPZoneScoped;
	//uiBeginItem(uiWindow, win, &g_ui->base, uiItemType_Window, flags, 1, file, line);
	uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem));
	NotImplemented;	
	return 0;
}

uiItem* ui_begin_window(str8 name, Flags flags, uiStyle* style, str8 file, upt line){DPZoneScoped;
	ui_make_window(name, flags, style, file, line);
	NotImplemented;
	return 0;
}

uiItem* ui_end_window(){
	NotImplemented;
	return 0;
}

//-/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @Button

uiButton* ui_make_button(Action action, void* action_data, Flags flags, str8 file, upt line){DPZoneScoped;
	NotImplemented;
	return 0;
}

//-/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @Text
void ui_gen_text(uiItem* item){DPZoneScoped;
	if(item->tag != uiTextTag){
		item_error(item, "ui_gen_text() was called on an item whose tag is not uiTextTag");
		return;
	}

	RenderDrawCounts counts = {0};
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	uiTextData* data = uiGetTextData(item);

	//temp attempt at dynamic text
	//TODO(sushi) this needs to be heaped
	RenderDrawCounts nucounts = render_make_text_counts(data->text.count);
	if(nucounts.vertices > dc->counts.vertices || nucounts.indices > dc->counts.indices){
		drawcmd_alloc(dc, nucounts);
	    vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
		ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	}

	str8 scan = data->text;
	str8 start = data->text;
	f32 y = 0;
	while(scan){
		u32 cp = str8_advance(&scan).codepoint;
		if(cp == U'\n' || !scan){
			counts+=render_make_text(vp, ip, counts, {start.str, start.count-scan.count}, item->style.font, item->spos + vec2{0, y}, item->style.text_color, vec2::ONE * item->style.font_height / item->style.font->max_height);
			y+=item->style.font_height;
			start = scan;
		}
	}

	//NOTE(sushi) text size is always determined here and NEVER set by the user
	//TODO(sushi) text sizing should probably be moved to eval_item_branch and made to take into account wrapping
	item->style.size = calc_text_size(item);
}

uiItem* ui_make_text(str8 text, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = (uiItem*)arena_add(item_arena, sizeof(uiItem) + sizeof(uiTextData));
	ui_fill_item(item, style, file, line);
	
	item->tag = uiTextTag;
	item->memsize = sizeof(uiItem) + sizeof(uiTextData);
	item->drawcmds = (uiDrawCmd*)arena_add(drawcmd_arena, sizeof(uiDrawCmd)); 
	item->__generate = &ui_gen_text;
	item->trailing_data = item + sizeof(uiItem);
	
	uiGetTextData(item)->text = text;

	RenderDrawCounts counts = render_make_text_counts(str8_length(text));
	
	item->draw_cmd_count = 1;
	drawcmd_alloc(item->drawcmds, counts);
	return item;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_context
void ui_init(MemoryContext* memctx, uiContext* uictx){DPZoneScoped;
#if DESHI_RELOADABLE_UI
	g_memory = memctx;
	g_ui     = uictx;
#endif
	
	g_ui->item_list    = create_arena_list(0);
	g_ui->drawcmd_list = create_arena_list(0);
	g_ui->vertex_arena = memory_create_arena(Megabytes(1));
	g_ui->index_arena = memory_create_arena(Megabytes(1));
	
	ui_initial_style->     positioning = pos_static;
	ui_initial_style->          sizing = 0;
	ui_initial_style->            left = 0;
	ui_initial_style->             top = 0;
	ui_initial_style->           right = MAX_F32;
	ui_initial_style->          bottom = MAX_F32;
	ui_initial_style->           width = size_auto;
	ui_initial_style->          height = size_auto;
	ui_initial_style->     margin_left = 0;
	ui_initial_style->      margin_top = 0;
	ui_initial_style->    margin_right = MAX_F32;
	ui_initial_style->   margin_bottom = MAX_F32;
	ui_initial_style->    padding_left = 0;
	ui_initial_style->     padding_top = 0;
	ui_initial_style->   padding_right = MAX_F32;
	ui_initial_style->  padding_bottom = MAX_F32;
	ui_initial_style->   content_align = vec2{0,0};
	ui_initial_style->            font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
	ui_initial_style->     font_height = 11;
	ui_initial_style->background_color = color{0,0,0,0};
	ui_initial_style->background_image = 0;
	ui_initial_style->    border_style = border_none;
	ui_initial_style->    border_color = color{180,180,180,255};
	ui_initial_style->    border_width = 1;
	ui_initial_style->      text_color = color{255,255,255,255};
	ui_initial_style->        overflow = overflow_visible;
	ui_initial_style->           focus = 0;
	ui_initial_style->          hidden = 0;
	
	g_ui->base = uiItem{0};
	g_ui->base.style = *ui_initial_style;
	g_ui->base.file_created = STR8(__FILE__);
	g_ui->base.line_created = __LINE__;
	g_ui->base.style.width = DeshWindow->width;
	g_ui->base.style.height = DeshWindow->height;
	g_ui->base.id = STR8("base");
	g_ui->base.tag = uiItemTag;
	g_ui->base.style_hash = hash_style(&g_ui->base);
	push_item(&g_ui->base);
	
	//g_ui->render_buffer = render_create_external_2d_buffer(Megabytes(1), Megabytes(1));
}

//pass 0 for child on first call
//TODO(sushi) look into caching this while evaluating items
TNode* ui_find_static_sized_parent(TNode* node, TNode* child){DPZoneScoped;
	if(node == &g_ui->base.node) return node;
	uiItem* item = uiItemFromNode(node);
	if(!child) return ui_find_static_sized_parent(item->node.parent, &item->node);
	if(item->style.width != size_auto && item->style.height != size_auto){
		return &item->node;
	}else{
		return ui_find_static_sized_parent(item->node.parent, &item->node);
	}
}

void draw_item_branch(uiItem* item){DPZoneScoped;
	if(item != &g_ui->base){
		if(match_any(item->style.positioning, pos_fixed, pos_draggable_fixed)){
			item->spos = item->style.tl;
		}else{
			item->spos = uiItemFromNode(item->node.parent)->spos + item->lpos;
		}
	}
	
	if(item->draw_cmd_count){
		Assert(item->__generate, "item wtih no generate function");
		item->__generate(item);
	}
	
	if(item->node.first_child)
		for_node(item->node.first_child){
			draw_item_branch(uiItemFromNode(it));
		}
}

//reevaluates an entire brach of items
void eval_item_branch(uiItem* item){DPZoneScoped;
	uiItem* parent = uiItemFromNode(item->node.parent);
	
	b32 wauto = item->style.width == size_auto;
	b32 hauto = item->style.height == size_auto;
	u32 wborder = (item->style.border_style ? item->style.border_width : 0);

	vec2 parent_size_padded;

    if(!hauto){
        if(HasFlag(item->style.sizing, size_percent_y)){
            if(item->style.height < 0){
                item_error(item, "Sizing flag 'size_percent_y' was specified, but the given value for height '", item->style.height, "' is negative.");
            }else if(HasFlag(parent->style.sizing, size_percent_y)){ //if the parent's sizing is also set to percentage, we know it is already sized
				parent_size_padded.y = parent->height - (parent->style.padding_bottom==MAX_F32?parent->style.padding_top:parent->style.padding_bottom) - parent->style.padding_top;
                item->height = item->style.height/100.f * parent_size_padded.y;
            }else if (parent->style.height >= 0){ 
				parent_size_padded.y = parent->style.height - (parent->style.padding_bottom==MAX_F32?parent->style.padding_top:parent->style.padding_bottom) - parent->style.padding_top;
                item->height = item->style.height/100.f * parent_size_padded.y;
            }else{
                item_error(item, "Sizing flag 'size_percent_y' was specified, but the item's parent's height is not explicitly sized.");
                hauto = 1;
            }
        }else item->height = item->style.height;
    }else item->height = 0;

    if(!wauto){
        if(HasFlag(item->style.sizing, size_percent_x)){
            if(item->style.width < 0) 
                item_error(item, "Sizing value was specified with size_percent_x, but the given value for width '", item->style.width, "' is negative.");
            if(HasFlag(parent->style.sizing, size_percent_x)){
				parent_size_padded.x = parent->width - (parent->style.padding_right==MAX_F32?parent->style.padding_left:parent->style.padding_right) - parent->style.padding_left;
                item->width = item->style.width/100.f * parent_size_padded.x;
            }else if (parent->style.width >= 0){
				parent_size_padded.x = parent->style.width - (parent->style.padding_right==MAX_F32?parent->style.padding_left:parent->style.padding_right) - parent->style.padding_left;
                item->width = item->style.width/100.f * parent_size_padded.x;
            }else{
                item_error(item, "Sizing flag 'size_percent_x' was specified but the item's parent's width is not explicitly sized.");
                hauto = 1;
            }
        }else item->width = item->style.width;
    }else item->width = 0;

	if(HasFlag(item->style.sizing, size_square)){
		if     (!wauto &&  hauto) item->height = item->width;
		else if( wauto && !hauto) item->width = item->height;
		else item_error(item, "Sizing flag 'size_square' was specifed but width and height are both ", (wauto && hauto ? "unspecified" : "specified."));
	}

	vec2 cursor = item->style.padding - item->style.scroll;
	for_node(item->node.first_child){
		uiItem* child = uiItemFromNode(it);
		if(child->style.hidden) continue;
		eval_item_branch(child);    
		switch(child->style.positioning){
			case pos_static:{
				child->lpos =  child->style.margin;
				if(item->style.border_style)
					child->lpos += floor(item->style.border_width) * vec2::ONE;
				child->lpos += cursor;
				cursor.y = child->lpos.y + child->height;
			}break;
			case pos_relative:
			case pos_draggable_relative:{
				child->lpos =  child->style.margin;
				if(item->style.border_style)
					child->lpos += floor(item->style.border_width) * vec2::ONE;
				child->lpos += cursor;
				cursor.y = child->lpos.y + child->height;
				child->lpos += child->style.tl;
			}break;
			case pos_fixed:
			case pos_draggable_fixed:{
				//do nothing, because this is just handled in draw_branch
			}break;
			case pos_absolute:
			case pos_draggable_absolute:{
				child->lpos = child->style.tl;
			}break;
		}

		//NOTE(sushi) in order to try and avoid excessive jitteriness when animating objects
		//            we quantize the floating point precision, so the item doesnt actually move
		//            as much as it would if we used full precision
		//            this also fixes some issues like items not touching when they should
		//TODO(sushi) maybe make a global define for this
		child->lpos = floor(child->lpos*1)/1;

		item->max_scroll = Max(item->max_scroll, child->lpos - item->style.scroll);

        cursor.x = item->style.padding_left;
        
    }

    if(hauto){
        if(item->style.padding_bottom == MAX_F32) item->height += item->style.padding_top;
        else if(item->style.padding_bottom > 0) item->height += item->style.padding_bottom;
    }
    if(wauto){
        if(item->style.padding_right == MAX_F32) item->width += item->style.padding_left;
        else if(item->style.padding_right > 0) item->width += item->style.padding_right;
    }

    item->width += (wauto ? 1 : 2) * wborder;
    item->height += (hauto ? 1 : 2) * wborder;
	

	//TODO(sushi) I'm pretty sure the x part of this can be moved into the child loop above, so we dont have to do a second
	//            pass if y isnt set
    if(item->style.content_align.x > 0 || item->style.content_align.y > 0){
        f32 last_static_offset = 0;
        f32 padr = item->style.padding_right;
        f32 padl = item->style.padding_left;
		f32 padt = item->style.padding_top;
		f32 padb = item->style.padding_bottom;
		//space that children may actually occupy
        vec2 child_space = vec2(
			(item->width-(padr==MAX_F32?padl:padr))-padl, 
			(item->height-(padb==MAX_F32?padt:padb))-padt
		);
		f32 y_offset = ceil(item->style.content_align.y*(child_space.y - cursor.y));
        for_node(item->node.first_child){
            uiItem* child = uiItemFromNode(it);
            if(child->style.positioning == pos_static){
                last_static_offset = child->lpos.x;
                f32 marr = child->style.margin_right;
                f32 marl = child->style.margin_left;
                f32 mart = child->style.margin_top;
                f32 marb = child->style.margin_bottom;
				//the actual size the child occupies
				//TODO(sushi) probably cache this
                vec2 true_size = vec2(
					child->width + (marr==MAX_F32?marl:marr) + marl,
					child->height + (marb==MAX_F32?mart:marb) +mart
				);
                child->lx = item->style.padding_left + child->style.margin_left + item->style.content_align.x * (child_space.x - true_size.x);
                last_static_offset = child->lpos.x - last_static_offset;
				child->ly += y_offset;
            }else if(child->style.positioning==pos_relative){
                child->lpos.x += last_static_offset;
				child->ly += y_offset;
            }
        }
    }
	item->lpos = round(item->lpos);
	
}

void drag_item(uiItem* item){DPZoneScoped;
	if(!item) return;
	if(item->style.positioning == pos_draggable_fixed || 
	   item->style.positioning == pos_draggable_relative){
		vec2 mp_cur = input_mouse_position();
		persist b32 dragging = false;
		persist vec2 mp_offset;
		if(key_pressed(Mouse_LEFT) && Math::PointInRectangle(mp_cur, item->spos, item->size)){
			mp_offset = item->style.tl - mp_cur;
			dragging = true;
			g_ui->istate = uiISDragging;            
		}
		if(key_released(Mouse_LEFT)){ dragging = false;  g_ui->istate = uiISNone; }
		
		if(dragging){
			item->style.tl = input_mouse_position() + mp_offset;
		}
	}
}

//depth first walk to ensure we find topmost hovered item
b32 find_hovered_item(uiItem* item){DPZoneScoped;
    //early out if the mouse is not within the item's known children bbx 
	if(!Math::PointInRectangle(input_mouse_position(),item->children_bbx_pos,item->children_bbx_size)) return false;
    for_node(item->node.first_child){
		if(find_hovered_item(uiItemFromNode(it))) return 1;
	}
	if(Math::PointInRectangle(input_mouse_position(), item->spos, item->size)){
		g_ui->hovered = item;
		return 1;
	}
	return 0;
}

pair<vec2,vec2> ui_recur(TNode* node){DPZoneScoped;
	uiItem* item = uiItemFromNode(node);
	uiItem* parent = uiItemFromNode(node->parent);
	

	if(item->action && item->action_trigger){
		if(item->action_trigger == action_act_always)
			item->action(item);
		else if(g_ui->hovered==item){
			if     (item->action_trigger == action_act_mouse_hover)
				item->action(item);
			else if(item->action_trigger == action_act_mouse_pressed && input_lmouse_pressed())
			    item->action(item);
			else if(item->action_trigger == action_act_mouse_released && input_lmouse_released())
				item->action(item);
			else if(item->action_trigger == action_act_mouse_down && input_lmouse_down())
			    item->action(item);
		}
	}

	if(g_ui->hovered == item && item->style.focus && input_lmouse_pressed()){
		move_to_parent_first(&item->node);
		item->dirty = true;
	}

	//check if an item's style was modified, if so reevaluate the item,
	//its children, and every child of its parents until a manually sized parent is found
	u32 nuhash = hash_style(item);
	if(item->dirty || nuhash!=item->style_hash){
		item->dirty = 0;
		item->style_hash = nuhash; 
		uiItem* sspar = uiItemFromNode(ui_find_static_sized_parent(&item->node, 0));
		eval_item_branch(sspar);
		draw_item_branch(sspar);
	}
	
	// -MAX_F32 signals the function that the node is hidden and not to consider it 
	if(item->style.hidden) return {vec2::ZERO,vec2{-MAX_F32,-MAX_F32}};
	
	//render item

	//determine scissoring for overflow_hidden items
	vec2 scoff;
	vec2 scext;
	if(parent && parent->style.overflow != overflow_visible){
		scoff = Max(vec2{0,0}, Max(parent->visible_start, Min(item->spos, parent->visible_start+parent->visible_size)));
		vec2 br = Max(parent->visible_start, Min(item->spos+item->size, parent->visible_start+parent->visible_size));
		scext = Max(vec2{0,0}, br-scoff);
		item->visible_start = scoff;
		item->visible_size = scext; 
	}else{
		scoff = Max(vec2::ZERO, item->spos); scext = Max(vec2::ZERO, Min(item->spos+item->size, vec2(DeshWindow->dimensions))-scoff);
		item->visible_size = item->size;
		item->visible_start = item->spos;
	}

	//if the scissor is offscreen or outside of the item it resides in, dont render it.
	if(scoff.x < DeshWindow->dimensions.x && scoff.y < DeshWindow->dimensions.y &&
	   scext.x != 0                       && scext.y != 0                       &&
	   scoff.x + scext.x > 0              && scoff.y + scext.y > 0){
		forI(item->draw_cmd_count){
			render_set_active_surface_idx(0);
			Texture* tex = 0;
			if(item->tag == uiTextTag){
				tex = item->style.font->tex;
			}else if(item->style.background_image){
				tex = item->style.background_image;
			}
			render_start_cmd2(5, tex, scoff, scext);
			render_add_vertices2(5, 
				(Vertex2*)g_ui->vertex_arena->start + item->drawcmds[i].vertex_offset, 
				item->drawcmds[i].counts.vertices, 
				(u32*)g_ui->index_arena->start + item->drawcmds[i].index_offset,
				item->drawcmds[i].counts.indices
			);
		}
	}

	vec2 pos = item->spos, siz = item->size;
    for_node_reverse(node->last_child){
        auto [cpos, csiz] = ui_recur(it);
		if(csiz.x == -MAX_F32) continue;
        pos = Min(cpos, item->spos);
        siz = Max((item->spos - pos)+item->size, (cpos-pos)+csiz); 
    }

    item->children_bbx_pos=pos;
    item->children_bbx_size=siz;

    return {pos,siz};
}

void ui_update(){DPZoneScoped;
	if(g_ui->item_stack.count > 1){
		forI(g_ui->item_stack.count-1){
			if(i==g_ui->item_stack.count-2)
				item_error(g_ui->item_stack[i+1], "Items are still left on the item stack. Did you forget to call uiItemE? Did you mean to use uiItemM?");
			else item_error(g_ui->item_stack[i+1]);
		}
		Assert(false);
	}

	//Log("test","ayyyye"); //NOTE(delle) uncomment after reloading .dll for testing
	g_ui->base.style.width = DeshWindow->width;
	g_ui->base.style.height = DeshWindow->height;
	g_ui->base.visible_start = vec2::ZERO;
	g_ui->base.visible_size = DeshWindow->dimensions;

	//in order to prevent the mouse from entering another window during input
	//this can probably be moved to only be done on mouse input, but maybe left so 
	//user can get this info at any time OR make a function for that
	if(g_ui->istate == uiISNone) 
		find_hovered_item(&g_ui->base);
	
	if(g_ui->istate == uiISNone || g_ui->istate == uiISDragging) 
		drag_item(g_ui->hovered);
	
	//if(g_ui->base.node.child_count){
		ui_recur(&g_ui->base.node);
	//}
}

struct __ui_debug_win_info{
	uiItem* text;
}__ui_dwi;

void __ui_debug_callback(uiItem* item){
	uiTextData* text = uiGetTextData(__ui_dwi.text);
	uiItem* hov = g_ui->hovered;
	if(!hov) return;
	str8b s = toStr8(
		"id:   ", hov->id, "\n"
		"lpos: ", hov->lpos, "\n"
		"spos: ", hov->spos, "\n"
		"size: ", hov->size, "\n"
		"dcc:  ", hov->draw_cmd_count, "\n"
		"style:", "\n"
		"     positioning: ", hov->style.positioning, "\n"
		"          sizing: ", hov->style.sizing, "\n"
		"              tl: ", hov->style.tl, "\n"
		"              br: ", hov->style.br, "\n"
		"            size: ", hov->style.size, "\n"
		"        margintl: ", hov->style.margin, "\n"
		"        marginbr: ", hov->style.marginbr, "\n"
		"       padding: ", hov->style.padding, "\n"
		"       paddingbr: ", hov->style.paddingbr, "\n"
		"   content_align: ", hov->style.content_align,  "\n"
		"            font: ", (hov->style.font?hov->style.font->name:STR8("No font")), "\n"
		"     font_height: ", hov->style.font_height, "\n"
		"background_color: ", hov->style.background_color, "\n"
		"background_image: ", (hov->style.background_image?hov->style.background_image->name:"No background"), "\n"
		"foreground_color: ", hov->style.foreground_color, "\n"
		"    border_style: ", hov->style.border_style, "\n"
		"    border_color: ", hov->style.border_color, "\n"
		"    border_width: ", hov->style.border_width, "\n"
		"      text_color: ", hov->style.text_color, "\n"
		"        overflow: ", hov->style.overflow, "\n"
	);
	text->text = s.fin;
	item->dirty = 1;
}

void ui_debug(){
	{uiItem* window = uiItemB();
		uiStyle* style = &window->style;
		style->positioning = pos_draggable_relative;
		style->background_color = color(14,14,14);
		style->border_style = border_solid;
		window->id = STR8("_ui_ debug win");
		__ui_dwi.text = uiTextM((str8{0,0}));
		__ui_dwi.text->id = STR8("_ui_ debug win text");
		window->action_trigger = action_act_always;
		window->action = &__ui_debug_callback;
	}
}

void ui_demo(){

	
	{//window with a title bar
		uiItem* titlebar = uiItemB();{
			titlebar->style.background_color = color(50,50,50);
			titlebar->style.positioning = pos_draggable_fixed;
			titlebar->style.size = {300,15};
			titlebar->style.content_align = {0, 0.5};
			titlebar->id = STR8("titlebar");
			//make title
			uiTextML("demo window");
			
			uiItem* body = uiItemB();{
				//set body to be absolutly positioned, allowing us to place it below the titlebar
				body->style.positioning = pos_absolute;
				body->style.tl = {0, 15};
				body->style.background_color = color(14,14,14);
				body->style.padding = {10,10};
				body->style.size = {300, 270};
				uiTextML("some text in the window body");
				uiTextML("some more text in the window body\nthis one is newlined.");
			}uiItemE();
		}uiItemE();
	}

}


#undef ui_alloc
#undef ui_realloc
#undef ui_free
#undef ui_talloc
#undef ui_create_arena
#undef ui_delete_arena
#undef item_arena
#undef drawcmd_arena