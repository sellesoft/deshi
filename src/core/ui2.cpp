/*
Index:
@memory
	push_item(uiItem* item) -> void
	pop_item() -> void
@uiDrawCmd
	drawcmd_remove(uiDrawCmd* drawcmd) -> void
	drawcmd_alloc(uiDrawCmd* drawcmd, vec2i counts) -> void
@helpers
	ui_setup_item(uiItem* item, uiStyle* style, str8 file, upt line) -> void 
	calc_text_size(uiItem* item) -> vec2 
@item	
	ui_gen_item(uiItem* item) -> void 
	ui_make_item(uiStyle* style, str8 file, upt line) -> uiItem* 
	ui_begin_item(uiStyle* style, str8 file, upt line) -> uiItem* 
	ui_end_item(str8 file, upt line) -> void 
	ui_remove_item(uiItem* item, str8 file, upt line) -> void 
@context
	ui_init(MemoryContext* memctx, uiContext* uictx) -> void 
	ui_find_static_sized_parent(TNode* node, TNode* child) -> TNode* 
	draw_item_branch(uiItem* item) -> void 
	eval_item_branch(uiItem* item) -> void 
	drag_item(uiItem* item) -> void 
	find_hovered_item(uiItem* item) -> b32 
	ui_recur(TNode* node) -> pair<vec2,vec2> 
	ui_update() -> void 
@widgets
	@text
		ui_gen_text(uiItem* item) -> void 
		ui_eval_text(uiItem* item) -> void 
		ui_make_text(str8 text, uiStyle* style, str8 file, upt line) -> uiItem* 
	@slider
		ui_gen_slider(uiItem* item) -> void 
		ui_slider_callback(uiItem* item) -> void 
		ui_make_slider(uiStyle* style, str8 file, upt line) -> uiItem* 
		ui_make_slider_f32(f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line) -> uiItem* 
		ui_make_slider_u32(u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line) -> uiItem* 
		ui_make_slider_s32(s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line) -> uiItem* 
@debug
	ui_debug() -> void 
@demo
	ui_demo() -> void 
*/

#include "ui2.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "core/assets.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/window.h"

//---------------------------------------------------------------------------------------------------------------------
// @memory


uiItem* init_item(upt size, str8 file, upt line){
	
	return 0;
}

uiDrawCmd* make_drawcmd(upt count){
	g_ui->stats.drawcmds_reserved += count;
	return (uiDrawCmd*)memalloc(count*sizeof(uiDrawCmd));
}

void drawcmd_delete(uiDrawCmd* dc){
	memzfree(dc);
	g_ui->stats.drawcmds_reserved--;
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

void drawcmd_remove(uiDrawCmd* drawcmd){DPZoneScoped;
	carray<uiDrawCmd*> varr = {g_ui->inactive_drawcmds_vertex_sorted.data, g_ui->inactive_drawcmds_vertex_sorted.count};
	carray<uiDrawCmd*> iarr = {g_ui->inactive_drawcmds_index_sorted.data, g_ui->inactive_drawcmds_index_sorted.count};

	if(g_ui->inactive_drawcmds_vertex_sorted.count){
		s32 idx = -1;
		s32 mid =  0;
		s32 left = 0;
		s32 right = varr.count-1;
		while(left <= right){
			mid = left + (right-left)/2;
			//I dont think this should ever happen but tell me if it does
			Assert(varr[mid]->vertex_offset != drawcmd->vertex_offset);
			if(varr[mid]->vertex_offset < drawcmd->vertex_offset){
				left = mid + 1;
				mid = left+((right-left)/2);
			}else{
				right = mid - 1;
			}
		}

		g_ui->inactive_drawcmds_vertex_sorted.insert(drawcmd, mid);

		if(mid != g_ui->inactive_drawcmds_vertex_sorted.count-1){
			uiDrawCmd* right = g_ui->inactive_drawcmds_vertex_sorted[mid+1];
			if(right->vertex_offset - drawcmd->counts_reserved.x == drawcmd->vertex_offset){
				// in this case we have found a drawcmd on the right side that is aligned with the one we are currently 
				// deleting, so we can take its vertices and set its count to 0
				drawcmd->counts_reserved.x += right->counts_reserved.x;
				right->counts_reserved.x = 0;
				g_ui->inactive_drawcmds_vertex_sorted.remove(mid+1);
				if(!right->counts_reserved.y){
					drawcmd_delete(right);
				}
			}
		}
		if(mid){
			uiDrawCmd* left = g_ui->inactive_drawcmds_vertex_sorted[mid-1];
			if(left->vertex_offset + left->counts_reserved.x == drawcmd->vertex_offset){
				// in this case we have found a drawcmd on the left side that is aligned with the one we are currently 
				// deleting, so we can give it it's vertices and set them to 0
				left->counts_reserved.x += drawcmd->counts_reserved.x;
				drawcmd->counts_reserved.x = 0; 
				g_ui->inactive_drawcmds_vertex_sorted.remove(mid);
				//since this is the drawcmd we are currently working with, its index count shouldn't be zero so we dont check that
			}
		}
	}else{
		g_ui->inactive_drawcmds_vertex_sorted.add(drawcmd);
	}

	if(g_ui->inactive_drawcmds_index_sorted.count){
		s32 idx = -1;
		s32 mid =  0;
		s32 left = 0;
		s32 right = iarr.count-1;
		while(left <= right){
			mid = left + (right-left)/2;
			//I dont think this should ever happen but tell me if it does
			Assert(iarr[mid]->index_offset != drawcmd->index_offset);
			if(iarr[mid]->index_offset < drawcmd->index_offset){
				left = mid + 1;
				mid = left+((right-left)/2);
			}else{
				right = mid - 1;
			}
		}

		g_ui->inactive_drawcmds_index_sorted.insert(drawcmd, mid);

		if(mid != g_ui->inactive_drawcmds_index_sorted.count-1){
			uiDrawCmd* right = g_ui->inactive_drawcmds_index_sorted[mid+1];
			if(right->index_offset - drawcmd->counts_reserved.y == drawcmd->index_offset){
				// in this case we have found a drawcmd on the right side that is aligned with the one we are currently 
				// deleting, so we can take its vertices and set its count to 0
				drawcmd->counts_reserved.y += right->counts_reserved.y;
				right->counts_reserved.y = 0;
				g_ui->inactive_drawcmds_index_sorted.remove(mid+1);
				if(!right->counts_reserved.x){
					drawcmd_delete(right);
				}
			}
		}
		if(mid){
			uiDrawCmd* left = g_ui->inactive_drawcmds_index_sorted[mid-1];
			if(left->index_offset + left->counts_reserved.y == drawcmd->index_offset){
				// in this case we have found a drawcmd on the left side that is aligned with the one we are currently 
				// deleting, so we can give it it's vertices and set them to 0
				left->counts_reserved.y += drawcmd->counts_reserved.y;
				drawcmd->counts_reserved.y = 0; 
				g_ui->inactive_drawcmds_index_sorted.remove(mid);
				if(!drawcmd->counts_reserved.x){
					//this drawcmd has been absorbed by others, so its safe to delete it
					drawcmd_delete(drawcmd);
				}
			}
		}
	}else{
		g_ui->inactive_drawcmds_index_sorted.add(drawcmd);
	}
}

void drawcmd_alloc(uiDrawCmd* drawcmd, vec2i counts){DPZoneScoped;
	u32 v_place_next = -1;
	u32 i_place_next = -1;	

	forI(g_ui->inactive_drawcmds_vertex_sorted.count){
		uiDrawCmd* dc = g_ui->inactive_drawcmds_vertex_sorted[i];
		s64 vremain = dc->counts_reserved.x - counts.x;
		if(vremain >= 0){
			v_place_next = dc->vertex_offset;
			if(!vremain){
				g_ui->inactive_drawcmds_vertex_sorted.remove(i);
				dc->counts_reserved.x = 0;
				if(!dc->counts_reserved.y){
					drawcmd_delete(dc);
				}
			}else{
				dc->vertex_offset += counts.x;
				dc->counts_reserved.x -= counts.x;
			}
			break;
		}
	}

	forI(g_ui->inactive_drawcmds_index_sorted.count){
		uiDrawCmd* dc = g_ui->inactive_drawcmds_index_sorted[i];
		s64 iremain = dc->counts_reserved.y - counts.y;
		if(iremain >= 0){
			i_place_next = dc->index_offset;
			if(!iremain){
				g_ui->inactive_drawcmds_index_sorted.remove(i);
				dc->counts_reserved.y = 0;
				if(!dc->counts_reserved.x){
					drawcmd_delete(dc);
				}
			}else{
				dc->index_offset += counts.y;
				dc->counts_reserved.y -= counts.y;
			}
			break;
		}
	}
	
	if(v_place_next == -1){
		//we couldnt find a drawcmd with space for our new verts so we must allocate at the end 
		g_ui->stats.vertices_reserved += counts.x;
		drawcmd->vertex_offset = (g_ui->vertex_arena->cursor - g_ui->vertex_arena->start) / sizeof(Vertex2);
		g_ui->vertex_arena->cursor += counts.x * sizeof(Vertex2);
		g_ui->vertex_arena->used += counts.x * sizeof(Vertex2);
	} else drawcmd->vertex_offset = v_place_next;
	if(i_place_next == -1){
		//we couldnt find a drawcmd with space for our new indices so we must allocate at the end
		g_ui->stats.indices_reserved += counts.y;
		drawcmd->index_offset = (g_ui->index_arena->cursor - g_ui->index_arena->start) / sizeof(u32);
		g_ui->index_arena->cursor += counts.y * sizeof(u32);
		g_ui->index_arena->used += counts.y * sizeof(u32);
	} else drawcmd->index_offset = i_place_next;
	drawcmd->counts_reserved = counts;
}

//@helpers
#define item_error(item, ...)\
LogE("ui",CyanFormatComma((item)->file_created), ":", (item)->line_created, ":", RedFormatComma("error"), ": ", __VA_ARGS__)

#define gen_error(file,line,...)\
LogE("ui",CyanFormatComma(file),":",line,":",RedFormatComma("error"),":",__VA_ARGS__)

struct uiItemSetup{
	upt size; 
	uiStyle* style; 
	str8 file; 
	upt line; 
	void (*update)(uiItem*); 
	Type update_trigger; 
	void (*generate)(uiItem*); 
	void (*evaluate)(uiItem*);
	u32  (*hash)(uiItem*);
	
	vec2i* drawinfo_reserve;
	u32 drawcmd_count;
};

//optionally takes a pointer to a boolean to indicate if the item was retrieved from the cache
uiItem* ui_setup_item(uiItemSetup setup, b32* retrieved = 0){DPZoneScoped;
	if(g_ui->updating){
		LogE("ui", 
			 "In file, ", setup.file, " on line ", setup.line, ": ui_setup_item() was called during ui_update().\n",
			 "\tui_update() requires that all items are made outside of it.\n",
			 "\tA possible cause of this is trying to make an item in another item's action, update, generate, or evaluate function."
			 );
		Assert(0);	
	}
	
	uiItem* parent = *(g_ui->item_stack.last);

	//initialize the item in memory or retrieve it from cache if it already exists
	uiItem* item;
	b32 retrieved_ = 0;
	if(g_ui->immediate.active){
		//attempt to unique identify items without requiruing the user to provide unique ids
		//im pretty sure this will fail in some cases 
		//if its REALLY bad then we should just go with allowing the user to give unique ids
		u64 hash = str8_hash64(setup.file);
		hash ^=        setup.line;     hash *= 16777619;
		hash ^=        setup.size;     hash *= 16777619;
		hash ^= *(u64*)setup.generate; hash *= 16777619;
		hash ^= g_ui->immediate.id;
		//load item from immediate cache if it has been made before
		if(g_ui->immediate.cache.has(hash)){
			item = *g_ui->immediate.cache.at(hash);
			retrieved_ = 1;
		}else{
			item = (uiItem*)memalloc(setup.size);
			g_ui->immediate.cache.add(hash, item);
		}
		g_ui->immediate_items.add(item);
		//increment immediate's internal id
		//this keeps track of unique function calls in immediate blocks
		//because otherwise in loops a function will be called from the same file line and number and be mistaken as the same item
		g_ui->immediate.id++;
	}else{
		g_ui->stats.items_reserved++;
		item = (uiItem*)memalloc(setup.size);
		g_ui->items.add(item);
	}
	item->memsize = setup.size;
	
	if(retrieved_){
		//at this time, a retrieved item must always be reevaluated and regenerated.
		item->dirty = 1;
		if(retrieved) *retrieved = 1;
	}
	
	uiItem* curitem = *g_ui->item_stack.last;
	
	insert_last(&curitem->node, &item->node);
	
	if(!retrieved_){
		if(setup.style) memcpy(&item->style, setup.style, sizeof(uiStyle));
		else item->style = {0};
	}else if(setup.style) memcpy(&item->style, setup.style, sizeof(uiStyle));
	
	//inherit parent's text properties if they arent set
	//TODO(sushi) this may be wrong in some cases, such as where a user wants the font_height to start at 0, but we interpret this
	//            as them not setting it. I'm not sure yet how we can get around this other than just not doing this.
	//            We dont have anyway to explicitly see what values the user has set right now and anyway I can think of doing 
	//            this in C/C++ is tedious and/or would hurt performance.
	if(!item->style.font)         item->style.font        = parent->style.font;
	if(!item->style.font_height)  item->style.font_height = parent->style.font_height;
	if(!item->style.text_color.a) item->style.text_color  = parent->style.text_color;
	if(!item->style.text_wrap)    item->style.text_wrap   = parent->style.text_wrap;
	if(!item->style.tab_spaces)   item->style.tab_spaces  = parent->style.text_wrap;

	item->file_created = setup.file;
	item->line_created = setup.line;
	
	item->__update       = setup.update;
	item->update_trigger = setup.update_trigger;
	item->__generate     = setup.generate;
	item->__evaluate     = setup.evaluate;
	item->__hash         = setup.hash;
	
	//for now, a cached items drawcmds are always regenerated
	//TODO(sushi) eventually we should only do this if we need to, that or we can put a rule on items that their drawcmd count should never
	//            after initial creation, though this is limiting
	//TODO(sushi) this wastes a lot of time for items whose drawcmds have not changed
	if(retrieved_){
		forI(item->drawcmd_count){
			drawcmd_remove(item->drawcmds + i);
		}
	}
	
	item->drawcmd_count = setup.drawcmd_count;
	item->drawcmds = make_drawcmd(setup.drawcmd_count);
	forI(setup.drawcmd_count){
		drawcmd_alloc(item->drawcmds+i, setup.drawinfo_reserve[i]);
	}
	return item;
}

vec2 calc_text_size(uiItem* item){DPZoneScoped;
	uiStyle* style = &item->style;
	str8 text = uiGetText(item)->text.buffer.fin;
	f32 scaled_font_height = (f32)style->font_height * item->y_scale;
	vec2 result = vec2{0, scaled_font_height};
	f32 line_width = 0;
	switch(style->font->type){
		case FontType_BDF: case FontType_NONE:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += scaled_font_height;
					line_width = 0;
				}
				line_width += style->font->max_width * scaled_font_height / style->font->aspect_ratio / style->font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		case FontType_TTF:{
			u32 codepoint;
			while(text && (codepoint = str8_advance(&text).codepoint)){
				if(codepoint == '\n'){
					result.y += scaled_font_height;
					line_width = 0;
				}
				line_width += font_packed_char(style->font, codepoint)->xadvance * scaled_font_height / style->font->aspect_ratio / style->font->max_width;
				if(line_width > result.x) result.x = line_width;
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return result;
}

//NOTE(sushi) these are abstracted because widgets may use them as well
inline
vec2i gen_background(uiItem* item, Vertex2* vp, u32* ip, vec2i counts){
	vec2 mtl = floor(item->style.margintl * item->scale);
	vec2 mbr = floor(item->style.marginbr * item->scale);
	vec2 bor = floor((item->style.border_style ? item->style.border_width : 0) * item->scale);
	vec2 pos = item->pos_screen + mtl + bor;
	vec2 siz = floor(item->size * item->scale); //NOTE(delle) item->size already has margins and borders applied in eval_item_branch
	return render_make_filledrect(vp, ip, counts, pos, siz, item->style.background_color);
}

inline
vec2i gen_border(uiItem* item, Vertex2* vp, u32* ip, vec2i counts){
	switch(item->style.border_style){
		case border_none:{}break;
		case border_solid:{
			vec2 tl = item->pos_screen + (item->style.margintl * item->scale);
			vec2 br = tl + ((item->size - item->style.marginbr - item->style.margintl) * item->scale);
			vec2 tr = vec2{br.x, tl.y};
			vec2 bl = vec2{tl.x, br.y}; 
			f32 w = floor(item->style.border_width * item->x_scale);
			f32 h = floor(item->style.border_width * item->y_scale);
			u32 v = counts.x; u32 i = counts.y;
			ip[i+ 0] = v+0; ip[i+ 1] = v+1; ip[i+ 2] = v+3; //top border, bottom triangle
			ip[i+ 3] = v+0; ip[i+ 4] = v+3; ip[i+ 5] = v+2; //top border, top triangle
			ip[i+ 6] = v+2; ip[i+ 7] = v+3; ip[i+ 8] = v+5; //right border, left triangle
			ip[i+ 9] = v+2; ip[i+10] = v+5; ip[i+11] = v+4; //right border, right triangle
			ip[i+12] = v+4; ip[i+13] = v+5; ip[i+14] = v+7; //bottom border, top triangle
			ip[i+15] = v+4; ip[i+16] = v+7; ip[i+17] = v+6; //bottom border, bottom triangle
			ip[i+18] = v+6; ip[i+19] = v+7; ip[i+20] = v+1; //left border, right triangle
			ip[i+21] = v+6; ip[i+22] = v+1; ip[i+23] = v+0; //left border, left triangle
			vp[v+0].pos = tl;             vp[v+0].uv = {0,0}; vp[v+0].color = item->style.border_color.rgba;
			vp[v+1].pos = tl+Vec2( w, h); vp[v+1].uv = {0,0}; vp[v+1].color = item->style.border_color.rgba;
			vp[v+2].pos = tr;             vp[v+2].uv = {0,0}; vp[v+2].color = item->style.border_color.rgba;
			vp[v+3].pos = tr+Vec2(-w, h); vp[v+3].uv = {0,0}; vp[v+3].color = item->style.border_color.rgba;
			vp[v+4].pos = br;             vp[v+4].uv = {0,0}; vp[v+4].color = item->style.border_color.rgba;
			vp[v+5].pos = br+Vec2(-w,-h); vp[v+5].uv = {0,0}; vp[v+5].color = item->style.border_color.rgba;
			vp[v+6].pos = bl;             vp[v+6].uv = {0,0}; vp[v+6].color = item->style.border_color.rgba;
			vp[v+7].pos = bl+Vec2( w,-h); vp[v+7].uv = {0,0}; vp[v+7].color = item->style.border_color.rgba;
			return {8,24};
		}break;
	}
	return {0,0};
}

b32 mouse_in_item(uiItem* item){
	return Math::PointInRectangle(input_mouse_position(), item->pos_screen, item->size * item->scale);
}

b32 ui_item_hovered(uiItem* item, b32 strict){
	if(strict) return g_ui->hovered == item;
	return mouse_in_item(item);
}

//@item
void ui_gen_item(uiItem* item){DPZoneScoped;
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	vec2i counts = {0};
	counts+=gen_background(item, vp, ip, counts);
	counts+=gen_border(item, vp, ip, counts);
	dc->counts_used = counts;
}

uiItem* ui_make_item(uiStyle* style, str8 file, upt line){DPZoneScoped;
	
	uiItemSetup setup = {0};
	setup.size = sizeof(uiItem);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.generate = &ui_gen_item;
	setup.drawcmd_count = 1;
	vec2i counts[1] = {
		render_make_filledrect_counts() + render_make_rect_counts()
	};
	setup.drawinfo_reserve = counts;
	
	uiItem* item = ui_setup_item(setup);
	
	return item;
}

uiItem* ui_begin_item(uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_item(style, file, line);
	push_item(item);
	return item;
}

void ui_end_item(str8 file, upt line){DPZoneScoped;
	if(g_ui->item_stack.count == 1){
		LogE("ui", 
			 "In ", file, " at line ", line, " :\n",
			 "\tAttempted to end base item. Did you call uiItemE too many times? Did you use uiItemM instead of uiItemB?"
			 );
		//TODO(sushi) implement a hint showing what instruction could possibly be wrong 
	}else pop_item();
	
}

void ui_remove_item(uiItem* item, str8 file, upt line){DPZoneScoped;
	//TODO(sushi) check for contiguous regions of memory in the drawcmds' vertex and index regions so we can combine drawcmds into one 
	forI(item->drawcmd_count){
		drawcmd_remove(item->drawcmds + i);
	}
	
	remove(&item->node);
	memzfree(item);
}


//---------------------------------------------------------------------------------------------------------------------
// @immediate
void ui_begin_immediate_branch(uiItem* parent, str8 file, upt line){
	if(g_ui->immediate.active){
		LogE("ui", "In file ", file, " on line ", line, ": Attempted to start an immediate branch before ending one that was started in file ", g_ui->immediate.file, " on line ", g_ui->immediate.line);
		return;
	}else if(g_ui->updating){
		LogE("ui", "In file ", file, " on line ", line, ": \n",
			 "\tAttempted to start an immediate branch during ui_update().\n",
			 "\tui_update() requires all items to have been made outside of it.\n"
			 "\tDid you try to make items in an items action?");
		Assert(0);
	}
	
	g_ui->immediate.active = 1;
	g_ui->immediate.file = file;
	g_ui->immediate.line = line;
	g_ui->immediate.id = 0;
	
	if(parent){
		push_item(parent);
		g_ui->immediate.pushed = 1;
	} 
}

void ui_end_immediate_branch(str8 file, upt line){
	if(!g_ui->immediate.active){
		LogE("ui", "In file ", file, " on line ", line, ": Attempted to end an immediate branch before one was ever started");
		return;
	}else if(g_ui->updating){
		LogE("ui", "In file ", file, " on line ", line, ": \n",
			 "\tAttempted to end an immediate branch during ui_update().\n",
			 "\tui_update() requires all items to have been made outside of it.\n"
			 "\tDid you try to make items in an items action?");
		Assert(0);
	}
	
	g_ui->immediate.active = 0;
	
	if(g_ui->immediate.pushed){
		pop_item();
		g_ui->immediate.pushed = 0;
	}
}

void ui_push_id(s64 x, str8 file, upt line){
	//g_ui->immediate.id_stack.add(x);
}

void ui_pop_id(str8 file, upt line){
	//if(!g_ui->immediate.id_stack.count) gen_error(file,line,"ui_pop_id was called before any calls to ui_push_id were made");
	//else g_ui->immediate.id_stack.pop();
}

//---------------------------------------------------------------------------------------------------------------------
// @context
void ui_init(MemoryContext* memctx, uiContext* uictx){DPZoneScoped;
	DeshiStageInitStart(DS_UI2, DS_MEMORY, "Attempted to initialize UI2 module before initializing the Memory module");
	
#if DESHI_RELOADABLE_UI
	g_memory = memctx;
	g_ui     = uictx;
#endif
	
	//g_ui->cleanup = 0;
	
	g_ui->immediate.active = 0;
	g_ui->immediate.pushed = 0;
	
	g_ui->inactive_drawcmds.next = &g_ui->inactive_drawcmds;
	g_ui->inactive_drawcmds.prev = &g_ui->inactive_drawcmds;
	
	g_ui->vertex_arena = memory_create_arena(g_memory->arena_heap.size / 16);
	g_ui->index_arena  = memory_create_arena(g_memory->arena_heap.size / 16);
	
	g_ui->base = uiItem{0};
	g_ui->base.id = STR8("base");
	g_ui->base.file_created = STR8(__FILE__);
	g_ui->base.line_created = __LINE__;
	g_ui->base.style.width = DeshWindow->width;
	g_ui->base.style.height = DeshWindow->height;
	g_ui->base.style_hash = hash_style(&g_ui->base);
	push_item(&g_ui->base);

	//setup default keybinds
	//TODO(sushi) export these to a config file and load them instead
	uikeys.inputtext.cursor.          left = Key_LEFT  | InputMod_None;
	uikeys.inputtext.cursor.     left_word = Key_LEFT  | InputMod_AnyCtrl;
	uikeys.inputtext.cursor. left_wordpart = Key_LEFT  | InputMod_AnyAlt;
	uikeys.inputtext.cursor.         right = Key_RIGHT | InputMod_None;
	uikeys.inputtext.cursor.    right_word = Key_RIGHT | InputMod_AnyCtrl;
	uikeys.inputtext.cursor.right_wordpart = Key_RIGHT | InputMod_AnyAlt;
	uikeys.inputtext.cursor.            up = Key_UP    | InputMod_None;
	uikeys.inputtext.cursor.          down = Key_DOWN  | InputMod_None;

	uikeys.inputtext.select.          left = Key_LEFT  | InputMod_AnyShift;
	uikeys.inputtext.select.     left_word = Key_LEFT  | InputMod_AnyShift | InputMod_AnyCtrl;
	uikeys.inputtext.select. left_wordpart = Key_LEFT  | InputMod_AnyShift | InputMod_AnyAlt;
	uikeys.inputtext.select.         right = Key_RIGHT | InputMod_AnyShift;
	uikeys.inputtext.select.    right_word = Key_RIGHT | InputMod_AnyShift | InputMod_AnyCtrl;
	uikeys.inputtext.select.right_wordpart = Key_RIGHT | InputMod_AnyShift | InputMod_AnyAlt;
	uikeys.inputtext.select.            up = Key_UP    | InputMod_AnyShift;
	uikeys.inputtext.select.          down = Key_DOWN  | InputMod_AnyShift;

	uikeys.inputtext.del.             left = Key_BACKSPACE | InputMod_None;
	uikeys.inputtext.del.        left_word = Key_BACKSPACE | InputMod_AnyCtrl;
	uikeys.inputtext.del.    left_wordpart = Key_BACKSPACE | InputMod_AnyAlt;
	uikeys.inputtext.del.            right = Key_DELETE    | InputMod_None;
	uikeys.inputtext.del.       right_word = Key_DELETE    | InputMod_AnyCtrl;
	uikeys.inputtext.del.   right_wordpart = Key_DELETE    | InputMod_AnyAlt;

	//g_ui->render_buffer = render_create_external_2d_buffer(Megabytes(1), Megabytes(1));
}

//pass 0 for child on first call
//TODO(sushi) look into caching this while evaluating items
TNode* ui_find_static_sized_parent(TNode* node, TNode* child){DPZoneScoped;
	if(node == &g_ui->base.node) return node;
	uiItem* item = uiItemFromNode(node);
	if(!child) return ui_find_static_sized_parent(item->node.parent, &item->node);
	if(!HasFlag(item->style.sizing, size_auto|size_flex)){
		return &item->node;
	}else{
		return ui_find_static_sized_parent(item->node.parent, &item->node);
	}
}

void draw_item_branch(uiItem* item){DPZoneScoped;
	if(HasFlag(item->style.display, display_hidden)) return;
	
	item->debug_frame_stats.draws++;
	
	if(item != &g_ui->base){
		if(match_any(item->style.positioning, pos_fixed, pos_draggable_fixed)){
			item->pos_screen = item->style.pos;
		}else{
			item->pos_screen = uiItemFromNode(item->node.parent)->pos_screen + item->pos_local;
		}
	}

	if(item->drawcmd_count){
		Assert(item->__generate, "item with no generate function");
		item->__generate(item);
	}
	
	if(item->node.first_child)
		for_node(item->node.first_child){
		draw_item_branch(uiItemFromNode(it));
	}
}

struct EvalContext{
	struct{
		b32 flex_container; //set true if the parent is a flex container
		b32 disprow; //set true if the parent is displaying items in a row
		f32 ratio_sum; //sum of flex sized child ratios
		f32 effective_size; //size that the parent can fit flexed items in
		u32 n_ceils; //the amount of times a child's size should be ceil'd rather than floored 
	}flex;
};

b32 last_flex_floored = 1;

//reevaluates an entire brach of items
void eval_item_branch(uiItem* item, EvalContext* context){DPZoneScoped;
	//an array of item indexes into the child item list that indicate to the main eval loop that the item has already beem
	//evaluated before it. currently this only happens when the item is a flex container and contains an automatically sized child.
	array<u32> already_evaluated;
	
	EvalContext contextout = {0};
	
	uiItem* parent = uiItemFromNode(item->node.parent);
	
	item->debug_frame_stats.evals++;

	b32 wauto = HasFlag(item->style.sizing, size_auto_x); 
	b32 hauto = HasFlag(item->style.sizing, size_auto_y); 
	f32 wborder = (item->style.border_style ? item->style.border_width : 0);
	b32 disprow = HasFlag(item->style.display, display_horizontal);
	
	//TODO(sushi) this can probably be cleaned up 
	if(!hauto){
		if(context && context->flex.flex_container && !context->flex.disprow && HasFlag(item->style.sizing, size_flex)){
			item->height = item->style.height / context->flex.ratio_sum * context->flex.effective_size;
			if(context->flex.n_ceils){
				item->height = ceil(item->height);
				context->flex.n_ceils--;
			}else{
				item->height = floor(item->height);
			}
			last_flex_floored = !last_flex_floored;
		}else if(HasFlag(item->style.sizing, size_percent_y)){
			if(item->style.height < 0){
				item_error(item, "Sizing flag 'size_percent_y' was specified, but the given value for height '", item->style.height, "' is negative.");
			}else if(HasFlag(parent->style.sizing, size_percent_y)){ //if the parent's sizing is also set to percentage, we know it is already sized
				item->height = item->style.height/100.f * PaddedHeight(parent);
			}else if (parent->style.height >= 0){ 
				item->height = item->style.height/100.f * PaddedStyleHeight(parent);
			}else{
				//TODO(sushi) consider removing this error as the user may want this behavoir to happen on purpose
				item_error(item, "Sizing flag 'size_percent_y' was specified, but the item's parent's height is not explicitly sized.");
				hauto = 1;
			}
		}else item->height = item->style.height + item->style.margin_top + item->style.margin_bottom + 2*wborder;
	}else item->height = 0;
	
	if(!wauto){
		if(context && context->flex.flex_container && context->flex.disprow && HasFlag(item->style.sizing, size_flex)){
			item->width = item->style.width / context->flex.ratio_sum * context->flex.effective_size;
			if(context->flex.n_ceils){
				item->width = ceil(item->width);
				context->flex.n_ceils--;
			}else{
				item->width = floor(item->width);
			}
			last_flex_floored = !last_flex_floored;
		}else if(HasFlag(item->style.sizing, size_percent_x)){
			if(item->style.width < 0) 
				item_error(item, "Sizing value was specified with size_percent_x, but the given value for width '", item->style.width, "' is negative.");
			if(HasFlag(parent->style.sizing, size_percent_x) || HasFlag(parent->style.sizing, size_flex)){
				item->width = item->style.width/100.f * PaddedWidth(parent);
			}else if (parent->style.width >= 0){
				item->width = item->style.width/100.f * PaddedStyleWidth(parent);
			}else{
				//TODO(sushi) consider removing this error as the user may want this behavoir to happen on purpose
				item_error(item, "Sizing flag 'size_percent_x' was specified but the item's parent's width is not explicitly sized.");
				hauto = 1;
			}
		}else item->width = item->style.width + item->style.margin_left + item->style.margin_right + 2*wborder;
	}else item->width = 0;
	
	if(HasFlag(item->style.sizing, size_square)){
		if     (!wauto &&  hauto) item->height = item->width;
		else if( wauto && !hauto) item->width = item->height;
		else item_error(item, "Sizing flag 'size_square' was specifed but width and height are both ", (wauto && hauto ? "unspecified." : "specified."));
	}
	
	if(HasFlag(item->style.display, display_flex)){
		contextout.flex.flex_container = 1;
		contextout.flex.effective_size = (disprow ? ((((item)->width - (item)->style.margin_left - (item)->style.margin_right) - ((item)->style.border_style ? 2*(item)->style.border_width : 0)) - (item)->style.padding_left - (item)->style.padding_right) : PaddedHeight(item));
		contextout.flex.ratio_sum = 0;
		contextout.flex.disprow = disprow;
		
		if(disprow && wauto){
			item_error(item, "\x1b[31m\x1b[7mFATAL\x1b[0m: Display flags 'display_flex' and 'display_horizontal' were set, but the containers sizing property was set with flag 'size_auto_x'.");
			return;	
		}else if(hauto){
			item_error(item, "\x1b[31m\x1b[7mFATAL\x1b[0m: Display flags 'display_flex' and 'display_vertical' were set, but the containers sizing property was set with flag 'size_auto_y'.");
			return;
		}
		
		//first pass to figure out ratios
		u32 idx = 0;
		//TODO(sushi) need to eventually move the disprow checks out so we dont do it so much
		for_node(item->node.first_child){
			uiItem* child = uiItemFromNode(it);
			if(HasFlag(child->style.sizing, size_flex)){
				contextout.flex.ratio_sum += (disprow ? child->style.width : child->style.height);
			}else{
				
				if((disprow && HasFlag(child->style.sizing, size_auto_x)) || HasFlag(child->style.sizing, size_auto_y)){
					//if a child has automatic sizing we can still support using it in flex containers by just evaluating it 
					//early. if we do this we need to tell the main eval loop later that we dont need to evaluate it again
					eval_item_branch(child, &contextout);
					contextout.flex.effective_size -= (disprow ? child->width : child->height);
					already_evaluated.add(idx);
				}else{
					contextout.flex.effective_size -= (disprow ? child->style.width : child->style.height);
				}
				
			}
			idx++;
		}
		
		//calc how many item's sizes will need to be ceil'd rather than floored to properly fit inside the container
		f32 floored_sum = 0;
		for_node(item->node.first_child){
			uiItem* child = uiItemFromNode(it);
			floored_sum += floor((disprow ? child->style.width : child->style.height) / contextout.flex.ratio_sum * contextout.flex.effective_size);
		}
		contextout.flex.n_ceils = contextout.flex.effective_size - floored_sum;
	}

	//// calculate item scale based on parent scale ////
	item->scale = (parent) ? parent->scale : vec2::ONE;
	if(item->style.x_scale) item->scale.x *= item->style.x_scale;
	if(item->style.y_scale) item->scale.y *= item->style.y_scale;
	
	//// custom item evaluate ////
	if(item->__evaluate) item->__evaluate(item);
	
	//// evaluate item children ////
	vec2 cursor = item->style.margintl + item->style.paddingtl + vec2{wborder,wborder} - item->style.scroll;
	TNode* it = (HasFlag(item->style.display, display_reverse) ? item->node.last_child : item->node.first_child);
	u32 aeidx = 0; //index into the already evaluated array, incremented when we find one thats already been eval'd
	u32 idx = 0;
	while(it){
		uiItem* child = uiItemFromNode(it);
		if(HasFlag(child->style.display, display_hidden)){
			idx++;
			Assert(it!=it->next, "infinite loop.");
			it = (HasFlag(item->style.display, display_reverse) ? it->prev : it->next);
			continue;
		} 
		
		if(already_evaluated.count < aeidx && already_evaluated[aeidx] == idx){
			aeidx++;
		}else{
			eval_item_branch(child, &contextout);    
		}
		
		switch(child->style.positioning){
			case pos_static:{
				//child->pos_local =  child->style.margin;
				//if(item->style.border_style)
				//child->pos_local += item->style.border_width * vec2::ONE;
				child->pos_local = cursor;
				if(HasFlag(item->style.display, display_horizontal))
					cursor.x = child->pos_local.x + child->width;
				else{
					cursor.y = child->pos_local.y + child->height;
				}
			}break;
			case pos_relative:
			case pos_draggable_relative:{
				child->pos_local = child->style.margintl;
				if(item->style.border_style)
					child->pos_local += item->style.border_width * vec2::ONE;
				child->pos_local += cursor;
				
				if(HasFlag(item->style.display, display_horizontal)){
					cursor.x = child->pos_local.x + child->width;
				}else{
					cursor.y = child->pos_local.y + child->height;
				}
				
				switch(child->style.anchor){
					case anchor_top_left:{
						child->pos_local.x += child->style.x;
						child->pos_local.y += child->style.y;
					}break;
					case anchor_top_right:{
						if(!wauto) child->pos_local.x += (PaddedWidth(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as top_right, but the item's width is set to auto.");
						
						child->pos_local.y += child->style.y;
					}break;
					case anchor_bottom_right:{
						if(!wauto) child->pos_local.x += (PaddedWidth(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's width is set to auto.");
						
						if(!hauto) child->pos_local.y += (PaddedHeight(item) - child->height) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
					case anchor_bottom_left:{
						child->pos_local.x += child->style.x;
						
						if(!hauto) child->pos_local.y += (PaddedHeight(item) - child->height) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
				}
				
				
			}break;
			case pos_fixed:
			case pos_draggable_fixed:{
				//do nothing, because this is just handled in draw_branch
			}break;
			case pos_absolute:
			case pos_draggable_absolute:{
				switch(child->style.anchor){
					case anchor_top_left:{
						child->pos_local.x = child->style.x;
						child->pos_local.y = child->style.y;
					}break;
					case anchor_top_right:{
						if(!wauto) child->pos_local.x = (PaddedWidth(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as top_right, but the item's width is set to auto.");
						
						child->pos_local.y = child->style.y;
					}break;
					case anchor_bottom_right:{
						if(!wauto) child->pos_local.x = (PaddedWidth(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's width is set to auto.");
						
						if(!hauto) child->pos_local.y = (PaddedHeight(item) - child->height) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
					case anchor_bottom_left:{
						child->pos_local.x = child->style.x;
						
						if(!hauto) child->pos_local.y = (PaddedHeight(item) - child->height) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
				}
			}break;
		}
		
		//NOTE(sushi) in order to try and avoid excessive jitteriness when animating objects
		//            we quantize the floating point precision, so the item doesnt actually move
		//            as much as it would if we used full precision
		//            this also fixes some issues like items not touching when they should
		//TODO(sushi) maybe make a global define for this
		child->pos_local = floor(child->pos_local*1)/1;
		
		if(wauto) item->width  = Max(item->width,  child->pos_local.x + child->width);
		if(hauto) item->height = Max(item->height, child->pos_local.y + child->height);
		
		idx++;
		Assert(it!=it->next, "infinite loop.");
		it = (HasFlag(item->style.display, display_reverse) ? it->prev : it->next);
	}
	
	//// calculate size ////
	if(wauto){
		item->width += item->style.padding_right;
		item->width += item->style.margin_right;
		item->width += wborder;
	} 
	if(hauto){
		item->height += item->style.padding_bottom;
		item->height += item->style.margin_bottom;
		item->height += wborder;
	} 
	
	if(item->style.max_width)  item->width  = Min(item->style.max_width,  item->width);
	if(item->style.max_height) item->height = Min(item->style.max_height, item->height);
	item->width  = Max(item->style.min_width,  item->width);
	item->height = Max(item->style.min_height, item->height);

	//// calculate max scroll ////
	item->max_scroll = Max(vec2{0,0}, cursor - PaddedArea(item));
	
	//// calculate content alignment ////
	//TODO(sushi) I'm pretty sure the x part of this can be moved into the child loop above, so we dont have to do a second
	//            pass if y isnt set
	if(item->style.content_align.x > 0 || item->style.content_align.y > 0){
		f32 last_static_offset = 0;
		f32 padr = item->style.padding_right;
		f32 padl = item->style.padding_left;
		f32 padt = item->style.padding_top;
		f32 padb = item->style.padding_bottom;
		//space that children may actually occupy
		vec2 child_space = Vec2(
			(item->width  - ((padr == MAX_F32) ? padl : padr)) - padl, 
			(item->height - ((padb == MAX_F32) ? padt : padb)) - padt
		);
		f32 y_offset = ceil(item->style.content_align.y*(child_space.y - cursor.y));
		for_node(item->node.first_child){
			uiItem* child = uiItemFromNode(it);
			if(child->style.positioning == pos_static){
				last_static_offset = child->pos_local.x;
				f32 marr = child->style.margin_right;
				f32 marl = child->style.margin_left;
				f32 mart = child->style.margin_top;
				f32 marb = child->style.margin_bottom;
				//the actual size the child occupies
				//TODO(sushi) probably cache this
				vec2 true_size = Vec2(
					child->width + (marr==MAX_F32?marl:marr) + marl,
					child->height + (marb==MAX_F32?mart:marb) +mart
				);
				child->pos_local.x = item->style.padding_left + child->style.margin_left + item->style.content_align.x * (child_space.x - true_size.x);
				last_static_offset = child->pos_local.x - last_static_offset;
				child->pos_local.y += y_offset;
			}else if(child->style.positioning==pos_relative){
				child->pos_local.x += last_static_offset;
				child->pos_local.y += y_offset;
			}
		}
	}
	
	/*-------------------------------------------------------------------------------------------------------
		at this point the item is finished. 
		
		we quantize its position since we are working in floating point to avoid some silly fp things
		this isn't fully tested though and may not be necessary
		
	*/
	
	item->pos_local = floor(item->pos_local);
}

void drag_item(uiItem* item){DPZoneScoped;
	if(!item) return;
	if(   item->style.positioning == pos_draggable_fixed
	   || item->style.positioning == pos_draggable_relative
	   || item->style.positioning == pos_draggable_absolute){
		persist vec2 mouse_begin;
		persist vec2 item_begin;
		persist b32 dragging = false;
		vec2 mouse_current = input_mouse_position();
		if(key_pressed(Mouse_LEFT) && mouse_in_item(item)){
			dragging = true;
			g_ui->istate = uiISDragging;            
			mouse_begin = mouse_current;
			item_begin = item->style.pos;
		}
		if(key_released(Mouse_LEFT)){
			dragging = false;
			g_ui->istate = uiISNone;
		}
		if(dragging){
			vec2 mouse_difference = mouse_current - mouse_begin;
			if(item->style.anchor == anchor_top_left || item->style.anchor == anchor_bottom_left){
				item->style.pos.x = item_begin.x + mouse_difference.x;
			}else{
				item->style.pos.x = item_begin.x - mouse_difference.x;
			}
			if(item->style.anchor == anchor_top_left || item->style.anchor == anchor_top_right){
				item->style.pos.y = item_begin.y + mouse_difference.y;
			}else{
				item->style.pos.y = item_begin.y - mouse_difference.y;
			}
		}
	}
}

//depth first walk to ensure we find topmost hovered item
//TODO(sushi) remove this in favor of doing it where its needed instead of every frame
b32 find_hovered_item(uiItem* item){DPZoneScoped;
	//early out if the mouse is not within the item's known children bbx 
	//NOTE(sushi) this does not work properly anymore now that we support immediate mode blocks
	//TODO(sushi) come up with a way around this
	//if(!Math::PointInRectangle(input_mouse_position(),item->children_bbx_pos,item->children_bbx_size)) return false;
	for_node_reverse(item->node.last_child){
		if(HasFlag(uiItemFromNode(it)->style.display, display_hidden)) continue;
		if(find_hovered_item(uiItemFromNode(it))) return 1;
	}
	if(mouse_in_item(item)){
		uiItem* cur = item;
		while(cur->style.hover_passthrough) cur = uiItemFromNode(cur->node.parent);
		g_ui->hovered = cur;
		return 1;
	}
	return 0;
}	


pair<vec2,vec2> ui_recur(TNode* node){DPZoneScoped;
	uiItem* item = uiItemFromNode(node);
	uiItem* parent = uiItemFromNode(node->parent);
	
	if(g_ui->hovered == item && item->style.focus && input_lmouse_pressed()){
		move_to_parent_last(&item->node);
		item->dirty = true;
	}

	//call the items update function if it exists
	if(item->__update) item->__update(item);
	
	//check if an item's style was modified, if so reevaluate the item,
	//its children, and every child of its parents until a manually sized parent is found
	u32 nuhash = hash_style(item);
	if(item->dirty || nuhash!=item->style_hash){
		item->dirty = 0;
		item->style_hash = nuhash; 
		uiItem* sspar = uiItemFromNode(ui_find_static_sized_parent(&item->node, 0));
		eval_item_branch(sspar, {0});
		draw_item_branch(sspar);
	}

	if(item->action && item->action_trigger){
		if(item->action_trigger == action_act_always)
			item->action(item);
		else if(g_ui->hovered == item){
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
	
	// -MAX_F32 signals the function that the node is hidden and not to consider it 
	if(HasFlag(item->style.display, display_hidden)) return {vec2::ZERO,vec2{-MAX_F32,-MAX_F32}};
	
	//render item
	
	//determine scissoring for overflow_hidden items
	vec2 scoff;
	vec2 scext;
	if(parent && parent->style.overflow != overflow_visible){
		vec2 cpos = item->pos_screen + item->style.margintl + (item->style.border_style ? item->style.border_width : 0) * vec2::ONE;
		vec2 csiz = BorderedArea(item);
		
		scoff = Max(vec2{0,0}, Max(parent->visible_start, Min(item->pos_screen, parent->visible_start+parent->visible_size)));
		vec2 br = Max(parent->visible_start, Min(item->pos_screen + (item->size * item->scale), parent->visible_start+parent->visible_size));
		scext = Max(vec2{0,0}, br-scoff);
		item->visible_start =  Max(vec2{0,0}, Max(parent->visible_start, Min(cpos, parent->visible_start+parent->visible_size)));
		br =  Max(parent->visible_start, Min(cpos+csiz, parent->visible_start+parent->visible_size));
		
		item->visible_size = br - item->visible_start; 
	}else{
		scoff = Max(vec2::ZERO, item->pos_screen);
		scext = Max(vec2::ZERO, Min(item->pos_screen + (item->size * item->scale), Vec2(DeshWindow->width,DeshWindow->height))-scoff);
		item->visible_size = item->size * item->scale;
		item->visible_start = item->pos_screen;
	}
	
	//if the scissor is offscreen or outside of the item it resides in, dont render it.
	if(scoff.x < DeshWindow->dimensions.x && scoff.y < DeshWindow->dimensions.y &&
	   scext.x != 0                       && scext.y != 0                       &&
	   scoff.x + scext.x > 0              && scoff.y + scext.y > 0){
		g_ui->stats.items_visible++;
		forI(item->drawcmd_count){
			g_ui->stats.drawcmds_visible++;
			g_ui->stats.vertices_visible += item->drawcmds[i].counts_reserved.x;
			g_ui->stats.indices_visible += item->drawcmds[i].counts_reserved.y;
			render_set_active_surface_idx(0);
			render_start_cmd2(5, item->drawcmds[i].texture, scoff, scext);
			render_add_vertices2(5, 		
				(Vertex2*)g_ui->vertex_arena->start + item->drawcmds[i].vertex_offset, 
				item->drawcmds[i].counts_used.x, 
				(u32*)g_ui->index_arena->start + item->drawcmds[i].index_offset,
				item->drawcmds[i].counts_used.y
			);
		}
	}
	
	vec2 pos = item->pos_screen;
	vec2 siz = item->size * item->scale;
	for_node(node->first_child){
		auto [cpos, csiz] = ui_recur(it);
		if(csiz.x == -MAX_F32) continue;
		pos = Min(cpos, item->pos_screen);
		siz = Max((item->pos_screen - pos)+siz, (cpos-pos)+csiz); 
	}
	
	item->children_bbx_pos=pos;
	item->children_bbx_size=siz;
	
	return {pos,siz};
}

void ui_update(){DPZoneScoped;
	g_ui->updating = 1;
	
	g_ui->stats.drawcmds_visible = 0;
	g_ui->stats.indices_visible = 0;
	g_ui->stats.items_visible = 0;
	g_ui->stats.vertices_visible = 0;
	
	for(auto item : g_ui->items){
#if 0
		Log("", 
			item->id, ".draws: ", item->debug_frame_stats.draws, "\n",
			item->id, ".evals: ", item->debug_frame_stats.evals
		);
#endif
		item->debug_frame_stats = {0};
	}
	
	if(g_ui->item_stack.count > 1){
		forI(g_ui->item_stack.count-1){
			if(i==g_ui->item_stack.count-2){
				item_error(g_ui->item_stack[i+1], "Items are still left on the item stack. Did you forget to call uiItemE? Did you mean to use uiItemM?");
			}else{
				item_error(g_ui->item_stack[i+1], "");
			}
		}
		Assert(false);
	}
	
	if(g_ui->istate == uiISNone) 
		find_hovered_item(&g_ui->base);

	if(input_lmouse_pressed())
		g_ui->active = g_ui->hovered;
	
	if(g_ui->istate == uiISNone || g_ui->istate == uiISDragging) 
		drag_item(g_ui->hovered);
	
	g_ui->base.style.width = DeshWindow->width;
	g_ui->base.style.height = DeshWindow->height;
	g_ui->base.visible_start = vec2::ZERO;
	g_ui->base.visible_size = Vec2(DeshWindow->width,DeshWindow->height);
	
	ui_recur(&g_ui->base.node);
	
	forI(g_ui->immediate_items.count){
		uiItem* item = g_ui->immediate_items[i];
		remove(&item->node);
	}
	g_ui->immediate_items.clear();

	
	g_ui->updating = 0;
}


/*---------------------------------------------------------------------------------------------------------------------

	@widgets

*/

//---------------------------------------------------------------------------------------------------------------------
// @slider

void ui_gen_slider(uiItem* item){DPZoneScoped;
	FixMe;
	/*
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	vec2i counts = {0};	
	uiSlider* data = uiGetSlider(item);
	
	vec2 pos = item->pos_screen + item->pos_client;
	vec2 size = item->csize;
	
	counts+=gen_background(item, vp, ip, counts);
	counts+=gen_border(item, vp, ip, counts);
	
	counts+=render_make_filledrect(vp,ip,counts,
								   Vec2(pos.x, pos.y + size.y*(1 - data->style.rail_thickness)/2),
								   Vec2(size.x, size.y * data->style.rail_thickness),
								   data->style.colors.rail
								   );
	
	if(data->style.dragger_shape == slider_dragger_rect){
		vec2 dragp = Vec2(pos.x+data->pos, pos.y);
		vec2 drags = Vec2(data->width, size.y);
		counts+=render_make_filledrect(vp,ip,counts,dragp,drags,data->style.colors.dragger);
	}else if(data->style.dragger_shape == slider_dragger_round){
		NotImplemented;
	}
	dc->counts_used = counts;
	*/
}

void ui_slider_callback(uiItem* item){DPZoneScoped;
	FixMe;
	/*
	uiSlider* data = uiGetSlider(item);
	vec2 mp = input_mouse_position();
	vec2 lmp = mp - item->pos_screen;
	switch(data->type){
		case 0:{
			f32 dragpos;
			f32 dragwidth;
			f32  min = data->minf32;
			f32  max = data->maxf32;
			f32* var = data->varf32;
			*var = Clamp(*var, min, max);
			dragwidth = item->width/8;
			dragpos = Remap(*var, 0.f, item->cwidth-dragwidth, min, max);
			if(input_lmouse_pressed() && Math::PointInRectangle(lmp, Vec2(dragpos,0), Vec2(dragwidth, item->cheight))){
				data->active = 1;
				data->mouse_offset = -lmp.x + dragpos;
			}
			if(data->active){
				*var = Remap(Clamp(lmp.x + data->mouse_offset, 0.f, item->cwidth-dragwidth), min, max, 0.f, item->cwidth-dragwidth);
				item->dirty = 1;
				item->action_trigger = action_act_always;
			}
			if(input_lmouse_released()){
				data->active = 0;
				item->action_trigger = action_act_mouse_hover;
			}
			data->width = dragwidth;
			data->pos = dragpos;
		}break;
		case 1:{NotImplemented;}break;
		case 2:{NotImplemented;}break;
	}
	*/
}

uiItem* ui_make_slider(uiStyle* style, str8 file, upt line){DPZoneScoped;
	FixMe;
	// auto [item, datav] = init_item(sizeof(uiSlider), offsetof(uiSlider, item), file, line);
	// uiSlider* data = (uiSlider*)datav;
	// ui_setup_item(item, style, file, line);
	
	// if(g_ui->updating){
	// 	item_error(item, 
	// 	"\n\tAttempted to make an item during ui_update().\n",
	// 	  "\tui_update() requires that all items are made outside of it.\n",
	// 	  "\tDid you try to make an item in another item's action?");
	// 	Assert(0);	
	// }
	
	// item->memsize = sizeof(uiSlider);
	// item->__generate = &ui_gen_slider;
	
	// vec2i counts = //reserve enough room for slider rail, dragger, and outline
	// 	render_make_filledrect_counts()*2+
	// 	render_make_rect_counts();
	
	// item->drawcmds = make_drawcmd(1);
	// item->drawcmd_count = 1;
	// drawcmd_alloc(item->drawcmds, counts);
	
	// item->action_trigger = action_act_mouse_hover;
	
	// //setup trailing data 
	
	// data->style.dragger_shape = slider_dragger_rect;
	// data->style.rail_thickness = 1;
	// data->style.colors.rail = color(80,80,80);
	// data->style.colors.dragger = color(14,50,100);
	
	// item->__hash = &slider_style_hash;
	// return item;
	return 0;
}

uiItem* ui_make_slider_f32(f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	item->action = &ui_slider_callback;
	uiGetSlider(item)->minf32 = min;
	uiGetSlider(item)->maxf32 = max;
	uiGetSlider(item)->varf32 = var;
	uiGetSlider(item)->type = 0;
	
	return item;
}

uiItem* ui_make_slider_u32(u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	item->action = &ui_slider_callback;
	uiGetSlider(item)->minu32 = min;
	uiGetSlider(item)->maxu32 = max;
	uiGetSlider(item)->varu32 = var;
	uiGetSlider(item)->type = 1;
	
	return item;
}

uiItem* ui_make_slider_s32(s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	item->action = &ui_slider_callback;
	uiGetSlider(item)->mins32 = max;
	uiGetSlider(item)->maxs32 = max;
	uiGetSlider(item)->vars32 = var;
	uiGetSlider(item)->type = 2;
	
	return item;
}

//---------------------------------------------------------------------------------------------------------------------
// @checkbox

void ui_gen_checkbox(uiItem* item){
	FixMe;
	/*
	uiCheckbox* data = uiGetCheckbox(item);
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	vec2i counts = {0};	
	
	vec2 fillingpos = item->pos_screen + data->style.fill_padding + item->pos_client;
	vec2 fillingsize = item->csize - data->style.fill_padding * 2;
	
	counts+=gen_background(item, vp, ip, counts);
	counts+=gen_border(item, vp, ip, counts);
	
	if(!data->var){
		item_error(item, "A checkbox was created but was given no boolean to act on.");
		return;
	}
	if(*data->var){
		counts+=render_make_filledrect(vp,ip,counts,fillingpos,fillingsize,data->style.colors.filling);
	}
	
	dc->counts_used = counts;
	*/
}

void ui_checkbox_callback(uiItem* item){
	FixMe;
	/*
	uiCheckbox* data = uiGetCheckbox(item);
	*data->var = !*data->var;
	item->dirty = 1;
	*/
}

uiItem* ui_make_checkbox(b32* var, uiStyle* style, str8 file, upt line){
	FixMe;
	// auto [item, datav] = init_item(sizeof(uiCheckbox), offsetof(uiCheckbox, item), file, line);
	// uiCheckbox* data = (uiCheckbox*)datav;
	// ui_setup_item(item, style, file, line);
	
	// if(g_ui->updating){
	// 	item_error(item, 
	// 	"\n\tAttempted to make an item during ui_update().\n",
	// 	  "\tui_update() requires that all items are made outside of it.\n",
	// 	  "\tDid you try to make an item in another item's action?");
	// 	Assert(0);	
	// }
	
	// ui_setup_item(action, action_trigger, hash, generate, evaluate);
	
	// item->action = &ui_checkbox_callback;
	// item->__hash = &checkbox_style_hash;
	// item->__generate = *ui_gen_checkbox;
	// item->action_trigger = action_act_mouse_pressed;
	
	// data->style.colors.filling = color(100,150,200);
	// data->style.fill_type = checkbox_fill_box;
	// data->style.fill_padding = vec2{2,2};
	// data->var = var;
	
	// vec2i counts = //reserve enough room for background, box filling, and outline
	// 	render_make_filledrect_counts()*2+
	// 	render_make_rect_counts();
	
	// item->drawcmds = make_drawcmd(1);
	// item->drawcmd_count = 1;
	// drawcmd_alloc(item->drawcmds, counts);
	
	return 0;
}

//---------------------------------------------------------------------------------------------------------------------
// @text

//common functionality between uiText and uiInputText
//NOTE(sushi) I would like to avoid this and instead inherit uiText into uiInputText, so uiInputText can just act as an extension 
//            of uiText. The way the API is setup right now doesn't really allow for this, at least not in any nice way.
//            It may be better to keep them separate however.

void find_text_breaks(array<pair<s64,vec2>>* breaks, uiItem* item, Text text, f32 wrapspace, b32 do_wrapping, b32 reset_size = 0){DPZoneScoped;
	breaks->clear();
	breaks->add({0,{0,0}});
	str8 last_space_or_tab = text.buffer.fin;
	str8 scan = text.buffer.fin;
	f32 space_width = font_visual_size(item->style.font, STR8(" ")).x * item->style.font_height / item->style.font->max_height;
	f32 width_since_last_word = 0;
	f32 xoffset = 0; //horizonal offset from top left corner of item
	f32 yoffset = 0; //vertical offset from top left corner of item
	if(reset_size) item->width = 0, item->height = 0;
	while(scan){
		DecodedCodepoint dc = str8_advance(&scan);
		if(dc.codepoint == U'\n'){
			width_since_last_word = 0;
			yoffset+=item->style.font_height;
			xoffset=0;
			breaks->add({scan.str-text.buffer.str, {xoffset,yoffset}});
		}else if(dc.codepoint == U'\t'){
			width_since_last_word = 0;
			xoffset+=item->style.tab_spaces*space_width;
			last_space_or_tab = scan;
			breaks->add({scan.str-text.buffer.str, {xoffset,yoffset}});
		}else if(dc.codepoint == U' '){
			width_since_last_word = 0;
			last_space_or_tab = scan;
			xoffset += space_width;
		}else{
			f32 cwidth = font_visual_size(item->style.font, str8{scan.str-dc.advance,dc.advance}).x * item->style.font_height / item->style.font->max_height;
			width_since_last_word += cwidth;
			xoffset += cwidth;
		}
		if(do_wrapping && xoffset > wrapspace){
			if(item->style.text_wrap == text_wrap_word){
				xoffset = 0;
				yoffset += item->style.font_height;
				//if we are wrapping where a break already is, we dont need to make another break, just adjust it
				if(last_space_or_tab.str - text.buffer.str == breaks->last->first){
					breaks->last->second = {xoffset,yoffset};
				}else if(last_space_or_tab.str - text.buffer.str > breaks->last->first){
					breaks->add({last_space_or_tab.str - text.buffer.str, {xoffset,yoffset}});
				}
				xoffset = width_since_last_word;
			}else if(item->style.text_wrap == text_wrap_char){
				xoffset = 0;
				yoffset += item->style.font_height;
				
				if(dc.codepoint == '\t'){
					breaks->last->second.y = yoffset;
					breaks->last->second.x = 0;
				}else{
					if(scan.str - dc.advance - text.buffer.str > breaks->last->first){
						breaks->add({scan.str-dc.advance-text.buffer.str, {xoffset,yoffset}});
					}else if(scan.str - dc.advance - text.buffer.str == breaks->last->first){
						breaks->last->second.y = yoffset;
						breaks->last->second.x = 0;
					}
				}
				xoffset = font_visual_size(item->style.font, str8{scan.str-dc.advance,dc.advance}).x * item->style.font_height / item->style.font->max_height;
			}
		}
		item->width = Max(item->width, xoffset);
		item->height = yoffset + item->style.font_height;
	}
	breaks->add({text.buffer.count, {xoffset,yoffset}});
}

s64 find_hovered_offset(carray<pair<s64,vec2>> breaks, uiItem* item, Text text){DPZoneScoped;
	if(item->style.font->type == FontType_TTF) {
		LogW("ui", "find_hovered_offset() is not tested on non monospace fonts! Selection may not work properly, if it does come delete this please.");
	}
	vec2 mpl = input_mouse_position() - item->pos_screen;
	forX(i,breaks.count){
		if(i!=breaks.count-1 && mpl.y > breaks[i].second.y + item->style.font_height) continue;
		//breaks dont only happen on newlines, they happen for tabs too
		//so we must make sure that if there is more than 1 break on this line we are inspecting the right one
		for(int j=i; j<breaks.count; j++){
			if(breaks[i].second.y!=breaks[j].second.y){
				//dont want the cursor to go into newline stuff (probably)
				if(*(text.buffer.str + breaks[j].first - 1) == '\n'){
					if(*(text.buffer.str + breaks[j].first - 2) == '\r'){
						return breaks[j].first - 3;
					}else{
						return breaks[j].first - 2;
					}
				}
				return breaks[j].first - 1;
			}
			if(j==breaks.count-1){
				//the last break doesnt actually have anything in it and just serves as a boundry
				//if we reach this then we must have clicked beyond the very end
				return breaks[j].first;
			}
			//iterate over individual characters in the break until we're under the mouse 
			f32 xoffset = 0;
			forX(k,breaks[j+1].first - breaks[j].first){
				str8 c = {text.buffer.str + breaks[j].first + k, 1};
				f32 cw = font_visual_size(item->style.font, c).x * item->style.font_height / item->style.font->max_height;
				if(breaks[j].second.x+xoffset < mpl.x &&  
					breaks[j].second.x+xoffset+cw > mpl.x ){
					return breaks[j].first + k;
				}
				xoffset+=cw;
			}
		}
		return -1;
	}
	return -1;
}

//TODO(sushi) remove this abstraction because they arent the same between inputtext and text
vec2i render_ui_text(vec2i counts, uiDrawCmd* dc, Vertex2* vp, u32* ip, uiItem* item, Text text, carray<pair<s64,vec2>> breaks){DPZoneScoped;
	f32 space_width = font_visual_size(item->style.font, STR8(" ")).x * item->style.font_height / item->style.font->max_height;
	vec2 cursor = item->pos_screen;
	forI(breaks.count-1){
		auto [idx, pos] = breaks[i];
		cursor = pos+item->pos_screen;
		forX(j,breaks[i+1].first-idx){
			vec2 csize = font_visual_size(item->style.font, {text.buffer.str+idx+j,1}) * item->style.font_height / item->style.font->max_height;
			if(idx+j > Min(text.cursor.pos, text.cursor.pos+text.cursor.count) && idx+j < Max(text.cursor.pos, text.cursor.pos+text.cursor.count)){
				//render_make_rect()
				counts+=render_make_text(vp,ip,counts,
					{text.buffer.str+idx+j,1}, item->style.font,
					cursor, Color_Red,
					vec2::ONE * item->style.font_height / item->style.font->max_height
				);
			}else{
				counts+=render_make_text(vp, ip, counts,
					{text.buffer.str+idx+j,1}, 
					item->style.font,
					cursor, item->style.text_color,  
					vec2::ONE * item->style.font_height / item->style.font->max_height
				);
			}
			cursor.x += csize.x;
		}
	}
	return counts;
}


void ui_gen_text(uiItem* item){DPZoneScoped;
	vec2i counts = {0};
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	uiText* data = uiGetText(item);
	
	dc->texture = item->style.font->tex;
	
	//if there is an active selection we need to figure out how to render the selection boxes
	//TODO(sushi) this kind of sucks because it means we do this pass twice when a selection is active
	//            try and find a way to do this by gathering information as update happens 
	u32 selection_lines = 0;
	if(data->text.cursor.count){
		//TODO(sushi) finish implementing this
		// s32 break_start = -1;
		// forI(data->breaks.count-1){
		// 	if(break_start != -1 && data->breaks[i].first > data->text.cursor.pos){
		// 		break_start = i-1;
		// 	}else if(data->breaks[i].first > data->selection){

		// 	}		

		// }
	}

	vec2i nucounts = render_make_text_counts(str8_length(data->text.buffer.fin));
	if(nucounts.x != dc->counts_reserved.x || nucounts.y != dc->counts_reserved.y){
	    item->drawcmds = make_drawcmd(1);
		drawcmd_remove(dc);
		dc = item->drawcmds;
		drawcmd_alloc(dc, nucounts);
		dc->texture = item->style.font->tex;
	    vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
		ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	}

	counts+=render_ui_text(counts,dc,vp,ip,item,data->text,{data->breaks.data,data->breaks.count});

	dc->counts_used = counts;
}


void ui_eval_text(uiItem* item){
	if(!item->style.font){
		item_error(item, "uiText's evaluation function was called, but no font was specified for the item. You must either specify a font on the uiText's item handle, or on one of its ancestors.");
		return;
	}
	uiItem* parent = uiItemFromNode(item->node.parent);
	uiText* data = uiGetText(item);
	find_text_breaks(&data->breaks, item, data->text, PaddedWidth(parent), (item->style.text_wrap != text_wrap_none) && (!HasFlag(parent->style.sizing, size_auto)), 1);
}

//performs checks on the text element for things like mouse selection
//and copy when a selection is active
void ui_update_text(uiItem* item){DPZoneScoped;
	uiText* data = uiGetText(item);
	if(g_ui->hovered == item && input_lmouse_pressed()){
		data->text.cursor.pos = find_hovered_offset({data->breaks.data,data->breaks.count},item,data->text);
		data->selecting = 1;
		item->dirty = 1;
	}
	if(data->selecting){
		if(input_lmouse_released()){
			if(!data->text.cursor.count){
				data->selecting = 0;
			}
		}else if(input_lmouse_down()){
			u64 second_offset = find_hovered_offset({data->breaks.data,data->breaks.count},item,data->text);
			data->text.cursor.count = second_offset - data->text.cursor.pos;
			item->dirty = 1;
		}
	}
}

uiItem* ui_make_text(str8 text, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItemSetup setup = {0};
	setup.size = sizeof(uiText);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.update = &ui_update_text;
	setup.generate = &ui_gen_text;
	setup.evaluate = &ui_eval_text;
	vec2i counts[1] = {render_make_text_counts(str8_length(text))};
	setup.drawinfo_reserve = counts;
	if(text.count){
		setup.drawcmd_count = 1;
	}
	
	b32 retrieved = 0;
	uiItem* item = ui_setup_item(setup, &retrieved);
	uiText* data = (uiText*)item;
	
	//if this item was retrieved we need to update its text
	//otherwise its possible for it to take on the text of a different item 
	if(!retrieved)
		data->text = text_init(text);
	else text_clear_and_replace(&data->text, text);
	data->breaks.allocator = deshi_allocator;
	
	return item;
}

//---------------------------------------------------------------------------------------------------------------------
// @inputtext

void ui_gen_input_text(uiItem* item){DPZoneScoped;
	vec2i counts = {0};
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	uiInputText* data = uiGetInputText(item);
	
	dc->texture = item->style.font->tex;

	vec2i nucounts = 
		render_make_text_counts(str8_length(data->text.buffer.fin)) +
		render_make_filledrect_counts(); //cursor
	if(nucounts.x != dc->counts_reserved.x || nucounts.y != dc->counts_reserved.y){
	    item->drawcmds = make_drawcmd(1);
		drawcmd_remove(dc);
		dc = item->drawcmds;
		drawcmd_alloc(dc, nucounts);
		dc->texture = item->style.font->tex;
	    vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
		ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	}

	f32 space_width = font_visual_size(item->style.font, STR8(" ")).x * item->style.font_height / item->style.font->max_height;
	vec2 cursor = item->pos_screen;
	forI(data->breaks.count-1){
		auto [idx, pos] = data->breaks[i];
		cursor = pos+item->pos_screen;
		forX(j,data->breaks[i+1].first-idx){
			vec2 csize = font_visual_size(item->style.font, {data->text.buffer.str+idx+j,1}) * item->style.font_height / item->style.font->max_height;
			if(idx+j > Min(data->text.cursor.pos, data->text.cursor.pos+data->text.cursor.count) && idx+j < Max(data->text.cursor.pos, data->text.cursor.pos+data->text.cursor.count)){
				//render_make_rect()
				counts+=render_make_text(vp,ip,counts,
					{data->text.buffer.str+idx+j,1}, item->style.font,
					cursor, Color_Red,
					vec2::ONE * item->style.font_height / item->style.font->max_height
				);
			}else{
				counts+=render_make_text(vp, ip, counts,
					{data->text.buffer.str+idx+j,1}, 
					item->style.font,
					cursor, item->style.text_color,  
					vec2::ONE * item->style.font_height / item->style.font->max_height
				);
			}
			//draw cursor
			//TODO(sushi) implement different styles of cursors
			if(idx+j == data->text.cursor.pos){
				counts+=render_make_line(vp,ip,counts,cursor,Vec2(cursor.x,cursor.y+item->style.font_height),1,data->style.colors.cursor);
			}	
			cursor.x += csize.x;
		}
	}
	if(data->text.cursor.pos == data->text.buffer.count){
		//HACK(sushi) to fix the cursor drawing at the end
		//NOTE(sushi) this happens because of how the very last break represents the last position, but its not iterated above since
		//            it is used as a boundry instead
		counts+=render_make_line(vp,ip,counts,cursor,Vec2(cursor.x,cursor.y+item->style.font_height),1,data->style.colors.cursor);
	}

	dc->counts_used = counts;
}

void ui_eval_input_text(uiItem* item){DPZoneScoped;
	if(!item->style.font){
		item_error(item, "uiInputText's evaluation function was called, but no font was specified for the item. You must either specify a font on the uiText's item handle, or on one of its ancestors.");
		return;
	}
	uiItem* parent = uiItemFromNode(item->node.parent);
	uiInputText* data = uiGetInputText(item);
	find_text_breaks(&data->breaks, item, data->text, PaddedWidth(parent), (item->style.text_wrap != text_wrap_none) && (!HasFlag(parent->style.sizing, size_auto)), 0);
}

void ui_update_input_text(uiItem* item){DPZoneScoped;
	if(g_ui->active != item) return;
	uiInputText* data = uiGetInputText(item);
	Text* t = &data->text;
	
	b32 repeat = 0;
	if(any_key_pressed() || any_key_released()){
		reset_stopwatch(&data->repeat_hold);
	}
	else if((peek_stopwatch(data->repeat_hold) > data->style.hold_time) && (peek_stopwatch(data->repeat_throttle) > data->style.throttle_time)){
		reset_stopwatch(&data->repeat_throttle);
		repeat = 1;
	}

	//allows input on first press, then allows repeats when the repeat bool is set on input
	//do action depending on bind pressed
#define CanDoInput(x) (key_pressed(x) || key_down(x) && repeat)
	if(CanDoInput(uikeys.inputtext.cursor.left))           text_move_cursor_left(t),             item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.cursor.left_word))      text_move_cursor_left_word(t),        item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.cursor.left_wordpart))  text_move_cursor_left_wordpart(t),    item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.cursor.right))          text_move_cursor_right(t),            item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.cursor.right_word))     text_move_cursor_right_word(t),       item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.cursor.right_wordpart)) text_move_cursor_right_wordpart(t),   item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.select.left))           text_move_cursor_left(t,1),           item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.select.left_word))      text_move_cursor_left_word(t,1),      item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.select.left_wordpart))  text_move_cursor_left_wordpart(t,1),  item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.select.right))          text_move_cursor_right(t,1),          item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.select.right_word))     text_move_cursor_right_word(t,1),     item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.select.right_wordpart)) text_move_cursor_right_wordpart(t,1), item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.del   .right))          text_delete_right(t),                 item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.del   .right_word))     text_delete_right_word(t),            item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.del   .right_wordpart)) text_delete_right_wordpart(t),        item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.del   .left))           text_delete_left(t),                  item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.del   .left_word))      text_delete_left_word(t),             item->dirty = 1;
	if(CanDoInput(uikeys.inputtext.del   .left_wordpart))  text_delete_left_wordpart(t),         item->dirty = 1;
#undef CanDoInput
	
	//text input
	if(DeshInput->charCount){
		text_insert_string(t, {DeshInput->charIn,(s64)DeshInput->charCount});
		item->dirty = 1;
	}
	
	if(g_ui->hovered == item && input_lmouse_pressed()){
		g_ui->active = item;
		data->text.cursor.pos = find_hovered_offset({data->breaks.data,data->breaks.count}, item, data->text);
		data->selecting = 1;
		item->dirty = 1;
	}

	if(data->selecting){
		if(input_lmouse_released()){
			if(!data->text.cursor.count){
				data->selecting = 0;
			}
		}else if(input_lmouse_down()){
			u64 second_offset = find_hovered_offset({data->breaks.data,data->breaks.count}, item, data->text);
			data->text.cursor.count = second_offset - data->text.cursor.pos;
			item->dirty = 1;
		}
	}
}

uiItem* ui_make_input_text(str8 preview, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItemSetup setup = {0};
	setup.size = sizeof(uiInputText);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.update = &ui_update_input_text;
	setup.generate = &ui_gen_input_text;
	setup.evaluate = &ui_eval_input_text;
	vec2i counts[1] = {render_make_text_counts(str8_length(preview))+render_make_rect_counts()};
	setup.drawinfo_reserve = counts;
	setup.drawcmd_count = 1;

	b32 retrieved = 0;
	uiItem*      item = ui_setup_item(setup, &retrieved);
	uiInputText* data = uiGetInputText(item);

	if(!retrieved)
		data->text = text_init({0}, deshi_allocator);
	data->preview = preview;
	data->breaks.allocator = deshi_allocator;
	data->style.colors.cursor = Color_White;
	data->style.hold_time = 500;
	data->style.throttle_time = 50;
	
	return item;
}

/*---------------------------------------------------------------------------------------------------------------------

	@ui_debug

*/

struct ui_debug_win_info{
	b32 init = 0;
	
	uiItem* selected_item;
	
	b32 selecting_item;
	
	uiItem* internal_info;
	uiItem* panel0;
	uiItem* panel1;
	
	uiItem* panel1text;
	
	uiStyle def_style;
	
	b32 internal_info_header = true;
	
}ui_dwi={0};

void ui_debug_callback(uiItem* item){
	
	
	
}

void ui_debug_panel_callback(uiItem* item){
	
}

void ui_debug(){
	if(!ui_dwi.init){
		ui_dwi = {0};
		
		uiStyle def_style{0};
		def_style.sizing = size_auto;
		def_style.text_color = Color_White;
		def_style.text_wrap = text_wrap_none;
		def_style.font = assets_font_create_from_file_bdf(STR8("gohufont-11.bdf"));
		def_style.font_height = 11;
		def_style.background_color = color(14,14,14);
		def_style.tab_spaces = 4;
		def_style.border_color = color(188,188,188);
		def_style.border_width = 1;
		ui_dwi.def_style = def_style;
		
		uiStyle panel_style{0}; panel_style = def_style;
		panel_style.paddingtl = {3,3};
		panel_style.paddingbr = {3,3};
		panel_style.sizing = size_flex | size_percent_y;
		panel_style.height = 100;
		panel_style.border_style = border_none;
		panel_style.border_color = color(188,188,188);
		panel_style.border_width = 1;
		panel_style.background_color = color(50,50,50);
		panel_style.margintl = {2,2};
		panel_style.marginbr = {2,2};
		
		uiStyle itemlist_style{0}; itemlist_style = def_style;
		itemlist_style.paddingtl = {2,2};
		itemlist_style.paddingbr = {2,2};
		itemlist_style.min_height = 100;
		itemlist_style.sizing = size_percent_x;
		itemlist_style.width = 100;
		
		uiStyle* style;
		{uiItem* window = uiItemB();
			window->id = STR8("ui_debug win");
			window->action = &ui_debug_callback;
			window->action_trigger = action_act_always;
			style = &window->style;
			style->positioning = pos_draggable_fixed;
			style->background_color = color(14,14,14);
			style->border_style = border_solid;
			style->border_color = color(188,188,188);
			style->border_width = 1;
			style->focus = 1;
			style->size = {500,300};
			style->display = display_flex | display_horizontal;
			style->padding = {5,5,5,5};
			
			{uiItem* panel = uiItemBS(&panel_style); //selected information
				panel->id = STR8("ui_debug win panel0");
				panel->action = &ui_debug_panel_callback;
				panel->action_trigger = action_act_always;
				panel->style.width = 1;
				
				
				panel->style.margin_right = 1;
				
				{ui_dwi.internal_info = uiItemB(); 
					ui_dwi.internal_info->style = def_style;
					ui_dwi.internal_info->id = STR8("ui_debug internal info");
					ui_dwi.internal_info->style.sizing = size_percent_x;
					ui_dwi.internal_info->style.width = 100;
					ui_dwi.internal_info->style.height = 100;
					ui_dwi.internal_info->style.background_color = color(14,14,14);
					ui_dwi.internal_info->style.content_align = {0.5, 0.5};
				
				
					uiItemE();
				}
				
				ui_dwi.panel0 = panel;
			}uiItemE();
			
			{uiItem* panel = uiItemBS(&panel_style);
				panel->id = STR8("ui_debug win panel1");
				panel->action = &ui_debug_panel_callback;
				panel->action_trigger = action_act_always;
				panel->style.width = 0.5;
				
				ui_dwi.panel1text = uiTextML("stats");
				
				uiItemE();
				ui_dwi.panel1 = panel;
			}
		}uiItemE();
		ui_dwi.init = 1;
	}
	
	uiImmediateBP(ui_dwi.panel0);{//make internal info header
		//header stores an action that toggles its boolean in the data struct
		{uiItem* header = uiItemBS(&ui_dwi.def_style);
			header->id = STR8("header");
			header->style.sizing = size_auto_y | size_percent_x;
			header->style.width = 100;
			header->style.padding = {2,2,2,2};
			header->style.background_color = color(14,14,14);
			header->action = [](uiItem* item){
				if(input_lmouse_pressed()){
					ui_dwi.internal_info_header = !ui_dwi.internal_info_header;
				}
			};	
			header->action_trigger = action_act_mouse_hover;
			
			//uiTextML("internal info")->id = STR8("header text");
		}uiItemE();
		
		if(ui_dwi.internal_info_header){
			{uiItem* cont = uiItemBS(&ui_dwi.def_style);
				cont->id = STR8("header cont");
				
				cont->style.sizing = size_percent_x;
				cont->style.width = 100;
				cont->style.height = 100;
				
				if(ui_dwi.selected_item){
					
				}else if(ui_dwi.selecting_item){
					
					ui_dwi.internal_info->style.content_align = {0.5,0.5};
					uiTextML("selecting item...");
					
					if(g_ui->hovered && input_lmouse_pressed()){
						ui_dwi.selected_item = g_ui->hovered;
					}
					
				}else{
					// {uiItem* item = uiItemB();
					// 	item->id = STR8("button");
					// 	item->style.background_color = Color_VeryDarkCyan;
					// 	item->style.sizing = size_auto;
					// 	item->style.padding = {1,1,1,1};
					// 	item->style.margin = {1,1,1,1};
					// 	item->style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
					// 	item->style.font_height = 11;
					// 	item->style.text_color = Color_White;
					// 	item->action = [](uiItem* item) { 
					// 		ui_dwi.selecting_item = 1; 
					// 	};
					// 	item->action_trigger = action_act_mouse_pressed;
					// 	uiTextML("O");
					// }uiItemE();
					// ui_dwi.internal_info->style.content_align = {0.5,0.5};
					// uiTextML("no item selected.");
					if(g_ui->active){
						uiItem* sel = g_ui->active;
						uiText* text = 0;
						if(sel->memsize == sizeof(uiText)) text = uiGetText(sel);
						uiTextM(ToString8(deshi_temp_allocator,
							sel->id,"\n",
							sel->node.child_count
						));
						if(text){
							uiTextM(text->text.buffer.fin);
						}
					}
				}
			}uiItemE();
		}
	}uiImmediateE();
	
	u64 memsum = 0;
	forI(g_ui->items.count){
		memsum += g_ui->items[i]->memsize;
	}
	forI(g_ui->immediate_items.count){
		memsum += g_ui->immediate_items[i]->memsize;
	}
	memsum += g_ui->stats.vertices_reserved * sizeof(Vertex2);
	memsum += g_ui->stats.indices_reserved * sizeof(u32);
	memsum += g_ui->stats.drawcmds_reserved * sizeof(uiDrawCmd);

	ui_dwi.panel1text->style.text_wrap = text_wrap_none;
	text_clear_and_replace(&uiGetText(ui_dwi.panel1text)->text, ToString8(deshi_temp_allocator,
		"visible: \n",
		"	   items: ", g_ui->stats.items_visible, "\n",
		"	drawcmds: ", g_ui->stats.drawcmds_visible, "\n",
		"	vertices: ", g_ui->stats.vertices_visible, "\n",
		"	 indices: ", g_ui->stats.indices_visible, "\n",
		"reserved: \n",
		"	   items: ", g_ui->stats.items_reserved, "\n",
		"	drawcmds: ", g_ui->stats.drawcmds_reserved, "\n",
		"	vertices: ", g_ui->stats.vertices_reserved, "\n",
		"	 indices: ", g_ui->stats.indices_reserved, "\n",
		"  total mem: ", memsum / bytesDivisor(memsum), bytesUnit(memsum)
	));
	ui_dwi.panel1text->dirty = 1;

	if(g_ui->hovered){
		render_start_cmd2(7, 0, vec2::ZERO, Vec2(DeshWindow->width,DeshWindow->height));
		vec2 ipos = g_ui->hovered->pos_screen;
		vec2 mpos = ipos + g_ui->hovered->style.margintl;
		vec2 bpos = mpos + (g_ui->hovered->style.border_style ? g_ui->hovered->style.border_width : 0) * vec2::ONE;
		vec2 ppos = bpos + g_ui->hovered->style.paddingtl;
		
		render_quad2(ipos, g_ui->hovered->size, Color_Red);
		render_quad2(mpos, MarginedArea(g_ui->hovered), Color_Magenta);
		render_quad2(bpos, BorderedArea(g_ui->hovered), Color_Blue);
		render_quad2(ppos, PaddedArea(g_ui->hovered), Color_Green);
		
	}
}

/*-----------------------------------------------------------------------------------------------------------------

	@demo

*/

local uiItem* deshi__ui_demo_window = 0;
void ui_demo(){
	if(deshi__ui_demo_window){
		uiItemR(deshi__ui_demo_window);
		deshi__ui_demo_window = 0;
		return;
	}
	
	deshi__ui_demo_window = uiItemB();{
		uiItem* window = deshi__ui_demo_window;
		window->id = STR8("demo.window");
		window->style.positioning = pos_draggable_absolute;
		//window->style.sizing = size_resizeable;  //TODO requires item resizing
		window->style.size = {598/*pixels*/,298/*pixels*/}; //NOTE because border_width = 1
		window->style.min_size = {25/*pixels*/,25/*pixels*/};
		window->style.border_style = border_solid;
		window->style.border_color = Color_DarkCyan;
		window->style.border_width = 1/*pixels*/;
		window->style.font = assets_font_create_from_file(STR8("gohufont-uni-14.ttf"), 14);
		window->style.font_height = 14/*pixels*/;
		window->style.text_color = Color_White;
		window->style.focus = true;
		window->style.display = display_vertical | display_flex;
		
		uiItem* window_decoration = uiItemB();{
			window_decoration->id = STR8("demo.window_decoration");
			window_decoration->style.sizing = size_percent_x;
			window_decoration->style.size = {100/*percent*/,18/*pixels*/};
			window_decoration->style.min_size = {50/*pixels*/,18/*pixels*/};
			window_decoration->style.max_size = {0,18/*pixels*/};
			window_decoration->style.padding_left = 5/*pixels*/;
			window_decoration->style.background_color = Color_DarkCyan;
			window_decoration->style.content_align = {0,.5f};
			
			uiItem* title = uiTextML("Demo Window");{
				title->id = STR8("demo.window_decoration.title");
				title->style.text_color = Color_LightGrey;
			}
			
			//TODO maximize button
			
			//TODO close button
		}uiItemE();//window_decoration
		
		uiItem* window_content = uiItemB();{
			window_content->id = STR8("demo.window_content");
			window_content->style.sizing = size_flex;
			window_content->style.size = {1/*ratio of 1*/,1/*ratio of 1*/};
			window_content->style.background_color = Color_VeryDarkCyan;
			window_content->style.display = display_horizontal | display_flex;
			
			persist uiStyle preview_style{};
			
			uiItem* item_panel = uiItemB();{
				item_panel->id = STR8("demo.window_content.item_panel");
				item_panel->style.sizing = size_flex/*| size_resizeable_x*/; //TODO requires item resizing
				item_panel->style.size = {1/*ratio of 3*/,1/*ratio of 1*/};
				item_panel->style.border_style = border_solid; //TODO remove this once separators are added
				item_panel->style.border_color = Color_DarkCyan;
				item_panel->style.border_width = 1/*pixels*/;
				item_panel->style.display = display_vertical | display_flex;
				
				uiItem* item_tree = uiItemB();{
					item_tree->id = STR8("demo.window_content.item_panel.item_tree");
					item_tree->style.sizing = size_flex/*| size_resizeable_y*/; //TODO requires item resizing
					item_tree->style.size = {1/*ratio of 1*/,1/*ratio of 3*/};
					item_tree->style.border_style = border_solid; //TODO remove this once separators are added
					item_tree->style.border_color = Color_DarkCyan;
					item_tree->style.border_width = 1/*pixels*/;
					
					//TODO add item button
					//TODO add text button
					//TODO remove item button
					//TODO item tree
				}uiItemE();//item_tree
				
				//uiItem* item_panel_separator = uiSeparator(1/*pixels*/);  //TODO requires separator widget
				//item_panel_separator->id = STR8("demo.window_content.item_panel.separator");
				//item_panel_separator->style.background_color = Color_DarkCyan;
				//item_panel_separator->style.positioning = pos_draggable_relative;
				
				uiItem* item_settings = uiItemB();{
					item_settings->id = STR8("demo.window_content.item_panel.item_settings");
					item_settings->style.sizing = size_flex/*| size_resizeable_y*/; //TODO requires item resizing //NOTE(delle) resizeable here in addition to item_tree for extended resize hover region
					item_settings->style.size = {1/*ratio of 1*/,2/*ratio of 3*/};
					item_settings->style.border_style = border_solid; //TODO remove this once separators are added
					item_settings->style.border_color = Color_DarkCyan;
					item_settings->style.border_width = 1/*pixels*/;
					
					//TODO positioning  combobox
					//TODO anchor  combobox
					//TODO sizing  radio
					//TODO position  input_vec2
					//TODO size  input_vec2
					//TODO min_size  input_vec2
					//TODO max_size  input_vec2
					//TODO margin  input_vec4
					//TODO padding  input_vec4
					//TODO scale  input_vec2
					//TODO scroll  input_vec2
					//TODO border_style  combobox
					//TODO border_color  input_color/color dialog
					//TODO border_width  slider
					//TODO font  input_text
					//TODO font_height  input_number
					//TODO text_wrap  combobox
					//TODO text_color  input_color/color dialog
					//TODO tab_spaces  input_number
					//TODO focus  checkbox
					//TODO display  radio
					//TODO overflow  combobox
					//TODO content_align  input_vec2
					//TODO hover_passthrough  checkbox
				}uiItemE();//item_settings
			}uiItemE();//item_panel
			
			//uiItem* window_content_separator = uiSeparator(1/*pixels*/);  //TODO requires separator widget
			//window_content_separator->id = STR8("demo.window_content.item_panel.separator");
			//window_content_separator->style.background_color = Color_DarkCyan;
			//window_content_separator->style.positioning = pos_draggable_relative;
			
			uiItem* item_preview = uiItemB();{
				item_preview->id = STR8("demo.window_content.item_preview");
				item_preview->style.sizing = size_flex/*| size_resizeable_x*/; //TODO requires item resizing //NOTE(delle) resizeable here in addition to item_panel for extended resize hover region
				item_preview->style.size = {2/*ratio of 3*/,1/*ratio of 1*/};
				item_preview->style.border_style = border_solid;
				item_preview->style.border_color = Color_DarkCyan;
				item_preview->style.border_width = 1/*pixels*/; //TODO remove this once separators are added
				
				//TODO requires dynamically-added items
			}uiItemE();//item_preview
		}uiItemE();//window_content
	}uiItemE();//window
	/*
	{//test sizer
		uiItem* container = uiItemB();{
			container->style.size = {200, 100};
			container->style.display = display_flex | display_horizontal;
			container->style.background_color = color(50,60,100);
			container->style.paddingtl = {10,10};
			container->style.paddingbr = {10,10};
			container->id = STR8("container");
			
			uiStyle flex_style={0};
			flex_style.sizing = size_flex;
			flex_style.height = 20;
			
			uiItem* c0 = uiItemBS(&flex_style);{
				c0->style.background_color = Color_Red;
				c0->style.width = 3;
				c0->id = STR8("c0");
				c0->style.text_wrap = text_wrap_word;
				c0->style.font = assets_font_create_from_file_bdf(STR8("gohufont-11.bdf"));
				c0->style.font_height = 11;
				c0->style.text_color = color(255,255,255);
				uiTextML("some text to put in the flexed item ok")->id=STR8("text");
				c0->action = [](uiItem* item){	
					item->style.width = BoundTimeOsc(1,3);
				};
				c0->action_trigger = action_act_always;
			}uiItemE();
			uiItem* c1 = uiItemBS(&flex_style);{
				c1->style.background_color = Color_Green;
				c1->style.width = 2;
				c1->id = STR8("c1");
			}uiItemE();
			uiItem* c2 = uiItemBS(&flex_style);{
				c2->style.background_color = Color_Blue;
				c2->style.width = 1;
				c2->id = STR8("c2");
			}uiItemE();
		}uiItemE();
	}
	*/
	/*
	{//test scaling
		uiItem* test = uiItemB();{
			test->style.size = {200, 200};
			test->style.positioning = pos_draggable_absolute;
			test->style.background_color = Color_White;
			test->style.paddingtl = {10,10};
			test->style.paddingbr = {10,10};
			
			uiItem* test2 = uiItemB();{
				test2->style.positioning = pos_absolute;
				test2->style.pos = {50, 50};
				test2->style.size = {48, 48};
				test2->style.border_style = border_solid;
				test2->style.border_color = Color_Black;
				test2->style.border_width = 1;
				test2->style.scale = {1.5,1.5};
				test2->style.background_color = Color_Green;
			}uiItemE();
			
			uiItem* test3 = uiItemB();{
				test3->style.positioning = pos_absolute;
				test3->style.pos = {50, 50};
				test3->style.size = {50, 50};
				test3->style.background_color = Color_Blue;
			}uiItemE();
		}uiItemE();
	}
	*/
}
