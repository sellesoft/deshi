#include "ui2.h"
#include "kigu/array.h"
#include "kigu/array_utils.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/render.h"
#include "core/storage.h"
#include "core/window.h"
#include "core/logger.h"

/*
	Index:
		@memory
		   create_arena_list(ArenaList* old) -> ArenaList*
		   arena_add(Arena* arena, upt size) -> void*
		   push_item(uiItem* item) -> void
		   pop_item() -> void
		@uiDrawCmd
			drawcmd_remove(uiDrawCmd* drawcmd) -> void
			drawcmd_alloc(uiDrawCmd* drawcmd, RenderDrawCounts counts) -> void
		@helpers
			ui_fill_item(uiItem* item, uiStyle* style, str8 file, upt line) -> void 
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
			__ui_debug_callback(uiItem* item) -> void 
			ui_debug() -> void 
		@demo
			ui_demo() -> void 
*/


//---------------------------------------------------------------------------------------------------------------------
// @memory

struct miret{
	uiItem* item;
	void* data;
};	

miret make_item(upt size, upt offset){
	miret ret = {0};
	ret.data = memalloc(size);
	ret.item = (uiItem*)((u8*)ret.data+offset);

	if(g_ui->immediate.active){
		g_ui->immediate_items.add(ret.item);
	}else{
		g_ui->items.add(ret.item);
	}

	return ret;
}

uiDrawCmd* make_drawcmd(upt count){
	return (uiDrawCmd*)memalloc(count*sizeof(uiDrawCmd));
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
	NodeInsertPrev(&g_ui->inactive_drawcmds, &drawcmd->node);
	//g_ui->cleanup = 1;
}

void drawcmd_alloc(uiDrawCmd* drawcmd, RenderDrawCounts counts){DPZoneScoped;
	//if(drawcmd_arena->size < drawcmd_arena->used + counts.vertices * sizeof(Vertex2) + counts.indices * sizeof(u32))
	//	g_ui->drawcmd_list = create_arena_list(g_ui->drawcmd_list);
	
	u32 v_place_next = -1;
	u32 i_place_next = -1;	
	for(Node* n = g_ui->inactive_drawcmds.next; n!=&g_ui->inactive_drawcmds; n = n->next){
		uiDrawCmd* dc = uiDrawCmdFromNode(n);
		s64 vremain = dc->counts.vertices - counts.vertices;
		s64 iremain = dc->counts.indices - counts.indices;
		
		if(vremain >= 3){
			//there are a minimum of 3 vertices needed to make a 2D shape, so if there are more than it
			//we keep the drawcmd for future passes
			v_place_next = dc->vertex_offset;
			dc->vertex_offset += counts.vertices;
			dc->counts.vertices -= counts.vertices;
		}else if(vremain >= 0){
			//otherwise we just take the drawcmd's vertices and make its vertices invalid for future passes
			v_place_next = dc->vertex_offset;
			dc->counts.vertices = 0;
		}
		
		if(iremain >= 3){
			//there are a minimum of 3 indexes to make a 2D shape, so if there is more than this remaining
			//after claiming, keep the drawcmd around 
			i_place_next = dc->index_offset;
			dc->index_offset += counts.indices;
			dc->counts.indices -= counts.indices;
		}else if(iremain >= 0){
			//otherwise we just take the drawcmd's indices and make sure they arent used in future runs
			i_place_next = dc->index_offset;
			dc->counts.indices = 0;
		}

		if(!(dc->counts.vertices || dc->counts.indices)){
			//if both counts are 0 we can go ahead and remove this drawcmd from the list 
			//we do not handle actually removing the drawcmd from the arena it lives in, this is handled later
			//in a cleanup pass
			NodeRemove(n);
			memzfree(dc);
		}

		if(v_place_next != -1 && i_place_next != -1){
			break;
		}
	}
	
	if(v_place_next == -1){
		//we couldnt find a drawcmd with space for our new verts so we must allocate at the end 
		drawcmd->vertex_offset = (g_ui->vertex_arena->cursor - g_ui->vertex_arena->start) / sizeof(Vertex2);
		g_ui->vertex_arena->cursor += counts.vertices * sizeof(Vertex2);
	} else drawcmd->vertex_offset = v_place_next;
	if(i_place_next == -1){
		//we couldnt find a drawcmd with space for our new indices so we must allocate at the end
		drawcmd->index_offset = (g_ui->index_arena->cursor - g_ui->index_arena->start) / sizeof(u32);
		g_ui->index_arena->cursor += counts.indices * sizeof(u32);
	} else drawcmd->index_offset = i_place_next;
	drawcmd->counts = counts;
	drawcmd->texture = 0;
}

//@helpers
#define item_error(item, ...)\
LogE("ui","Error on item created in ", (item)->file_created, " on line ", (item)->line_created, ": ", __VA_ARGS__);


//fills an item struct and make its a child of the current item
void ui_fill_item(uiItem* item, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* curitem = *g_ui->item_stack.last;
	
	insert_last(&curitem->node, &item->node);
	
	if(style) memcpy(&item->style, style, sizeof(uiStyle));
	else item->style = {0};
	
	item->file_created = file;
	item->line_created = line;
}

//TODO(sushi) make an option for this to take into account wrapping
vec2 calc_text_size(uiItem* item){DPZoneScoped;
	uiStyle* style = &item->style;
	str8 text = uiGetText(item)->text;
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

//NOTE(sushi) these are abstracted because widgets may use them as well
inline
RenderDrawCounts gen_background(uiItem* item, Vertex2* vp, u32* ip, RenderDrawCounts counts){
	vec2 bor = (item->style.border_style ? item->style.border_width : 0) * vec2::ONE; 
	vec2 pos = item->spos + item->style.margintl + bor;
	vec2 siz = item->size - (item->style.margintl + item->style.marginbr + 2*bor);
	return render_make_filledrect(vp, ip, counts, pos, siz, item->style.background_color);
}

inline
RenderDrawCounts gen_border(uiItem* item, Vertex2* vp, u32* ip, RenderDrawCounts counts){
	switch(item->style.border_style){
		case border_none:{}break;
		case border_solid:{
			vec2 tl = item->spos + item->style.margintl;
			vec2 br = tl+(item->size - (item->style.marginbr+item->style.margintl));
			vec2 tr = vec2{br.x, tl.y};
			vec2 bl = vec2{tl.x, br.y}; 
			f32 t = item->style.border_width;
			u32 v = counts.vertices; u32 i = counts.indices;
			ip[i+ 0] = v+0; ip[i+ 1] = v+1; ip[i+ 2] = v+3; 
			ip[i+ 3] = v+0; ip[i+ 4] = v+3; ip[i+ 5] = v+2; 
			ip[i+ 6] = v+2; ip[i+ 7] = v+3; ip[i+ 8] = v+5; 
			ip[i+ 9] = v+2; ip[i+10] = v+5; ip[i+11] = v+4; 
			ip[i+12] = v+4; ip[i+13] = v+5; ip[i+14] = v+7; 
			ip[i+15] = v+4; ip[i+16] = v+7; ip[i+17] = v+6; 
			ip[i+18] = v+6; ip[i+19] = v+7; ip[i+20] = v+1; 
			ip[i+21] = v+6; ip[i+22] = v+1; ip[i+23] = v+0;
			vp[v+0].pos = tl;             vp[v+0].uv = {0,0}; vp[v+0].color = item->style.border_color.rgba;
			vp[v+1].pos = tl+vec2( t, t); vp[v+1].uv = {0,0}; vp[v+1].color = item->style.border_color.rgba;
			vp[v+2].pos = tr;             vp[v+2].uv = {0,0}; vp[v+2].color = item->style.border_color.rgba;
			vp[v+3].pos = tr+vec2(-t, t); vp[v+3].uv = {0,0}; vp[v+3].color = item->style.border_color.rgba;
			vp[v+4].pos = br;             vp[v+4].uv = {0,0}; vp[v+4].color = item->style.border_color.rgba;
			vp[v+5].pos = br+vec2(-t,-t); vp[v+5].uv = {0,0}; vp[v+5].color = item->style.border_color.rgba;
			vp[v+6].pos = bl;             vp[v+6].uv = {0,0}; vp[v+6].color = item->style.border_color.rgba;
			vp[v+7].pos = bl+vec2( t,-t); vp[v+7].uv = {0,0}; vp[v+7].color = item->style.border_color.rgba;
			return {8,24};
		}break;
	}
	return {0,0};
}

//@item
void ui_gen_item(uiItem* item){DPZoneScoped;
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	RenderDrawCounts counts = {0};
	counts+=gen_background(item, vp, ip, counts);
	counts+=gen_border(item, vp, ip, counts);
}

uiItem* ui_make_item(uiStyle* style, str8 file, upt line){DPZoneScoped;
	auto [item, data] = make_item(sizeof(uiItem), 0);
	ui_fill_item(item, style, file, line);
	
	item->memsize = sizeof(uiItem);
	item->drawcmds = make_drawcmd(1);
	
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

void ui_end_item(str8 file, upt line){DPZoneScoped;
	if(*(g_ui->item_stack.last) == &g_ui->base){
		LogE("ui", 
			 "In ", file, " at line ", line, " :\n",
			 "\tAttempted to end base item. Did you call uiItemE too many times? Did you use uiItemM instead of uiItemB?"
		);
		//TODO(sushi) implement a hint showing what instruction could possibly be wrong 
	}else pop_item();
	
}

void ui_remove_item(uiItem* item, str8 file, upt line){DPZoneScoped;
	//TODO(sushi) check for contiguous regions of memory in the drawcmds' vertex and index regions so we can combine drawcmds into one 
	forI(item->draw_cmd_count){
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
	}
	
	g_ui->immediate.active = 1;
	g_ui->immediate.file = file;
	g_ui->immediate.line = line;

	if(parent){
		push_item(parent);
		g_ui->immediate.pushed = 1;
	} 
}

void ui_end_immediate_branch(str8 file, upt line){
	if(!g_ui->immediate.active){
		LogE("ui", "In file ", file, " on line ", line, ": Attempted to end an immediate branch before one was ever started");
		return;
	}
	
	g_ui->immediate.active = 0;

	if(g_ui->immediate.pushed){
		pop_item();
		g_ui->immediate.pushed = 0;
	}
}



//---------------------------------------------------------------------------------------------------------------------
// @context
void ui_init(MemoryContext* memctx, uiContext* uictx){DPZoneScoped;
#if DESHI_RELOADABLE_UI
	g_memory = memctx;
	g_ui     = uictx;
#endif

	//g_ui->cleanup = 0;

	g_ui->immediate.active = 0;
	g_ui->immediate.pushed = 0;
	
	g_ui->inactive_drawcmds.next = &g_ui->inactive_drawcmds;
	g_ui->inactive_drawcmds.prev = &g_ui->inactive_drawcmds;

	g_ui->vertex_arena = memory_create_arena(Megabytes(1));
	g_ui->index_arena  = memory_create_arena(Megabytes(1));
	
	g_ui->base = uiItem{0};
	g_ui->base.file_created = STR8(__FILE__);
	g_ui->base.line_created = __LINE__;
	g_ui->base.style.width = DeshWindow->width;
	g_ui->base.style.height = DeshWindow->height;
	g_ui->base.id = STR8("base");
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
			item->spos = item->style.pos;
		}else{
			item->spos = uiItemFromNode(item->node.parent)->spos + item->lpos;
		}
	}
	
	if(item->draw_cmd_count){
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
	}flex;
};

//reevaluates an entire brach of items
void eval_item_branch(uiItem* item, EvalContext context){DPZoneScoped;
	EvalContext contextout = {0};
	
	uiItem* parent = uiItemFromNode(item->node.parent);
	
	b32 wauto = HasFlag(item->style.sizing, size_auto_x); 
	b32 hauto = HasFlag(item->style.sizing, size_auto_y); 
	f32 wborder = (item->style.border_style ? item->style.border_width : 0);
	b32 disprow = HasFlag(item->style.display, display_row);
	
	/*-------------------------------------------------------------------------------------------------------
		at this point we know if the item is to be automatically sized based on its content and what we should consider
		it's border width

		next we evaluate what the item's size is going to be if it is not automatically sized
	*/

	vec2 parent_size_padded;
	//TODO(sushi) this can probably be cleaned up 
	if(!hauto){
		if(context.flex.flex_container && !context.flex.disprow && HasFlag(item->style.sizing, size_flex)){
			item->height = item->style.height / context.flex.ratio_sum * context.flex.effective_size;
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
		if(context.flex.flex_container && context.flex.disprow && HasFlag(item->style.sizing, size_flex)){
			item->width = item->style.width / context.flex.ratio_sum * context.flex.effective_size;
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

	
	/*-------------------------------------------------------------------------------------------------------
		at this point we know what the items size is if it is explicitly sized, or if it is to be automatically sized
		we have set the size to 0.

		next if the item has a custom evaluation function assigned to it we call it.
	*/

	
	if(HasFlag(item->style.display, display_flex)){
		contextout.flex.flex_container = 1;
		contextout.flex.effective_size = (disprow ? ((((item)->width - (item)->style.margin_left - (item)->style.margin_right) - ((item)->style.border_style ? 2*(item)->style.border_width : 0)) - (item)->style.padding_left - (item)->style.padding_right) : PaddedHeight(item));
		contextout.flex.ratio_sum = 0;
		contextout.flex.disprow = disprow;

		if(disprow && wauto){
			item_error(item, "\x1b[31m\x1b[7mFATAL\x1b[0m: Display flags 'display_flex' and 'display_row' were set, but the containers sizing property was set with flag 'size_auto_x'.");
			return;	
		}else if(hauto){
			item_error(item, "\x1b[31m\x1b[7mFATAL\x1b[0m: Display flags 'display_flex' and 'display_column' were set, but the containers sizing property was set with flag 'size_auto_y'.");
			return;
		}

		//first pass to figure out ratios
		for_node(item->node.first_child){
			uiItem* child = uiItemFromNode(it);
			if(HasFlag(child->style.sizing, size_flex)){
				contextout.flex.ratio_sum += (disprow ? child->style.width : child->style.height);
			}else{
				contextout.flex.effective_size -= (disprow ? child->style.width : child->style.height);
			}
		}
	}


	if(item->__evaluate) item->__evaluate(item);

	/*-------------------------------------------------------------------------------------------------------
		at this point the item has finished its custom evaluation.

		next we evaluate all of the items children, positioning them based on the current item and child's properties
	*/

	vec2 cursor = item->style.margintl + item->style.paddingtl + vec2{wborder,wborder} - item->style.scroll;
	TNode* it = (HasFlag(item->style.display, display_reverse) ? item->node.last_child : item->node.first_child);
	while(it){
		uiItem* child = uiItemFromNode(it);
		if(child->style.hidden) continue;
		eval_item_branch(child, contextout);    
		switch(child->style.positioning){
			case pos_static:{
				//child->lpos =  child->style.margin;
				//if(item->style.border_style)
					//child->lpos += item->style.border_width * vec2::ONE;
				child->lpos = cursor;
				if(HasFlag(item->style.display, display_row))
					cursor.x = child->lpos.x + child->width;
				else{
					cursor.y = child->lpos.y + child->height;
					cursor.x = item->style.padding_left;
				}
			}break;
			case pos_relative:
			case pos_draggable_relative:{
				child->lpos = child->style.margintl;
				if(item->style.border_style)
					child->lpos += item->style.border_width * vec2::ONE;
				child->lpos += cursor;
				
				if(HasFlag(item->style.display, display_row))
					cursor.x = child->lpos.x + child->width;
				else
					cursor.y = child->lpos.y + child->height;

				switch(child->style.anchor){
					case anchor_top_left:{
						child->lx += child->style.x;
						child->ly += child->style.y;
					}break;
					case anchor_top_right:{
						if(!wauto) child->lx += (item->width - 2*wborder - item->style.padding_left - item->style.padding_right) - child->style.x;
						else item_error(item, "Item's anchor was specified as top_right, but the item's width is set to auto.");
						
						child->ly += child->style.y;
					}break;
					case anchor_bottom_right:{
						if(!wauto) child->lx += (item->width - 2*wborder - item->style.padding_left - item->style.padding_right) - child->style.x;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's width is set to auto.");

						if(!hauto) child->lx += (item->height - 2*wborder - item->style.padding_bottom - item->style.padding_top) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
					case anchor_bottom_left:{
						child->lx += child->style.x;

						if(!hauto) child->lx += (item->height - 2*wborder - item->style.padding_bottom - item->style.padding_top) - child->style.y;
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
						child->lx = child->style.x;
						child->ly = child->style.y;
					}break;
					case anchor_top_right:{
						if(!wauto) child->lx = (item->width - 2*wborder - item->style.padding_left - item->style.padding_right) - child->style.x;
						else item_error(item, "Item's anchor was specified as top_right, but the item's width is set to auto.");
						
						child->ly = child->style.y;
					}break;
					case anchor_bottom_right:{
						if(!wauto) child->lx = (item->width - 2*wborder - item->style.padding_left - item->style.padding_right) - child->style.x;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's width is set to auto.");

						if(!hauto) child->lx = (item->height - 2*wborder - item->style.padding_bottom - item->style.padding_top) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
					case anchor_bottom_left:{
						child->lx = child->style.x;

						if(!hauto) child->lx = (item->height - 2*wborder - item->style.padding_bottom - item->style.padding_top) - child->style.y;
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
		child->lpos = floor(child->lpos*1)/1;

		item->max_scroll = Max(item->max_scroll, child->lpos - item->style.scroll);

		if(wauto) item->width  = Max(item->width,  child->lpos.x + child->width);
        if(hauto) item->height = Max(item->height, child->lpos.y + child->height);
        

		it = (HasFlag(item->style.display, display_reverse) ? it->prev : it->next);
	}

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

	/*-------------------------------------------------------------------------------------------------------
		at this point the item is finished. 
		
		we quantize its position since we are working in floating point to avoid some silly fp things
		this isn't fully tested though

		we also set the item's csize, which indicates the area that its content actually occupies inside it, as well
		as the item's cpos which indicates where in the item its content starts
	*/

	item->lpos = floor(item->lpos);
	item->cx = wborder + item->style.padding_left + item->style.margin_left;
	item->cy = wborder + item->style.padding_top + item->style.margin_top;
	item->cheight = item->height - 2*wborder - (item->style.padding_top + item->style.padding_bottom) - (item->style.margin_top + item->style.margin_bottom);
	item->cwidth = item->width - 2*wborder - (item->style.padding_left + item->style.padding_right) - (item->style.margin_left + item->style.margin_right);
}

void drag_item(uiItem* item){DPZoneScoped;
	if(!item) return;
	if(item->style.positioning == pos_draggable_fixed || 
	   item->style.positioning == pos_draggable_relative){
		vec2 mp_cur = input_mouse_position();
		persist b32 dragging = false;
		persist vec2 mp_offset;
		if(key_pressed(Mouse_LEFT) && Math::PointInRectangle(mp_cur, item->spos, item->size)){
			mp_offset = item->style.pos - mp_cur;
			dragging = true;
			g_ui->istate = uiISDragging;            
		}
		if(key_released(Mouse_LEFT)){ dragging = false;  g_ui->istate = uiISNone; }
		
		if(dragging){
			item->style.pos = input_mouse_position() + mp_offset;
		}
	}
}

//depth first walk to ensure we find topmost hovered item
b32 find_hovered_item(uiItem* item){DPZoneScoped;
    //early out if the mouse is not within the item's known children bbx 
	if(!Math::PointInRectangle(input_mouse_position(),item->children_bbx_pos,item->children_bbx_size)) return false;
    for_node_reverse(item->node.last_child){
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
		move_to_parent_last(&item->node);
		item->dirty = true;
	}

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
			render_start_cmd2(5, item->drawcmds[i].texture, scoff, scext);
			render_add_vertices2(5, 		
				(Vertex2*)g_ui->vertex_arena->start + item->drawcmds[i].vertex_offset, 
				item->drawcmds[i].counts.vertices, 
				(u32*)g_ui->index_arena->start + item->drawcmds[i].index_offset,
				item->drawcmds[i].counts.indices
			);
		}
	}

	vec2 pos = item->spos, siz = item->size;
    for_node(node->first_child){
        auto [cpos, csiz] = ui_recur(it);
		if(csiz.x == -MAX_F32) continue;
        pos = Min(cpos, item->spos);
        siz = Max((item->spos - pos)+siz, (cpos-pos)+csiz); 
    }

    item->children_bbx_pos=pos;
    item->children_bbx_size=siz;

    return {pos,siz};
}

void ui_update(){DPZoneScoped;
	if(g_ui->item_stack.count > 1){
		forI(g_ui->item_stack.count-1){
			if(i==g_ui->item_stack.count-2){
				item_error(g_ui->item_stack[i+1], "Items are still left on the item stack. Did you forget to call uiItemE? Did you mean to use uiItemM?");
			}
			else {
				item_error(g_ui->item_stack[i+1]);
			}
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

	forI(g_ui->immediate_items.count){
		uiItemR(g_ui->immediate_items[i]);
	}
	g_ui->immediate_items.clear();
}


/*---------------------------------------------------------------------------------------------------------------------

	@widgets

*/

//---------------------------------------------------------------------------------------------------------------------
// @text
void ui_gen_text(uiItem* item){DPZoneScoped;
	RenderDrawCounts counts = {0};
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	uiText* data = uiGetText(item);

	dc->texture = item->style.font->tex;

	RenderDrawCounts nucounts = render_make_text_counts(str8_length(data->text));
	if(nucounts.vertices > dc->counts.vertices || nucounts.indices > dc->counts.indices){
	    item->drawcmds = make_drawcmd(1);
		drawcmd_remove(dc);
		dc = item->drawcmds;
		drawcmd_alloc(dc, nucounts);
	    vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
		ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	}

	f32 space_width = font_visual_size(item->style.font, STR8(" ")).x * item->style.font_height / item->style.font->max_height;

	vec2 cursor = item->spos;
	forI(data->breaks.count-1){
		auto [idx, pos] = data->breaks[i];
		counts+=render_make_text(vp, ip, counts, 
					{data->text.str+idx, s64(data->breaks[i+1].first - data->breaks[i].first)}, 
					item->style.font,
					item->spos + pos, item->style.text_color,  
					vec2::ONE * item->style.font_height / item->style.font->max_height
				);
	
	}
}

void ui_eval_text(uiItem* item){
	uiItem* parent = uiItemFromNode(item->node.parent);

	b32 do_wrapping = (parent->style.width != size_auto) && (item->style.text_wrap != text_wrap_none);

	f32 wrapspace = parent->width - parent->style.padding_left - parent->style.padding_right;

	uiText* data = uiGetText(item);
	data->breaks.clear();
	data->breaks.add({0,{0,0}});

	str8 last_space_or_tab = data->text;
	str8 scan = data->text;
	f32 space_width = font_visual_size(item->style.font, STR8(" ")).x * item->style.font_height / item->style.font->max_height;
	f32 width_since_last_word = 0;
	f32 xoffset = 0;
	f32 yoffset = 0;
	while(scan){
		DecodedCodepoint dc = str8_advance(&scan);
		if(dc.codepoint == U'\n'){
			width_since_last_word = 0;
			yoffset+=item->style.font_height;
			xoffset=0;
			data->breaks.add({scan.str-data->text.str, {xoffset,yoffset}});
		}else if(dc.codepoint == U'\t'){
			width_since_last_word = 0;
			xoffset+=item->style.tab_spaces*space_width;
			last_space_or_tab = scan;
			data->breaks.add({scan.str-data->text.str, {xoffset,yoffset}});
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
				if(last_space_or_tab.str - data->text.str == data->breaks.last->first){
					data->breaks.last->second = {xoffset,yoffset};
				}else if(last_space_or_tab.str - data->text.str > data->breaks.last->first){
					data->breaks.add({last_space_or_tab.str - data->text.str, {xoffset,yoffset}});
				}
				xoffset = width_since_last_word;
			}else if(item->style.text_wrap == text_wrap_char){
				xoffset = 0;
				yoffset += item->style.font_height;
				
				if(dc.codepoint == '\t'){
					data->breaks.last->second.y = yoffset;
					data->breaks.last->second.x = 0;
				}else{
					if(scan.str - dc.advance - data->text.str > data->breaks.last->first){
						data->breaks.add({scan.str-dc.advance-data->text.str, {xoffset,yoffset}});
					}else if(scan.str - dc.advance - data->text.str == data->breaks.last->first){
						data->breaks.last->second.y = yoffset;
						data->breaks.last->second.x = 0;
					}
				}
			
				xoffset = font_visual_size(item->style.font, str8{scan.str-dc.advance,dc.advance}).x * item->style.font_height / item->style.font->max_height;
			}
		}
		item->width = Max(item->width, xoffset);
		item->height = yoffset + item->style.font_height;
	}
	data->breaks.add({data->text.count, {xoffset,yoffset}});
}

uiItem* ui_make_text(str8 text, uiStyle* style, str8 file, upt line){DPZoneScoped;
	auto [item, datav] = make_item(sizeof(uiText), offsetof(uiText, item));
	uiText* data = (uiText*)datav;
	uiItem* curitem = *g_ui->item_stack.last;
	
	insert_first(&curitem->node, &item->node);
	
	if(style) memcpy(&item->style, style, sizeof(uiStyle));
	else{
		uiStyle* pstyle = &curitem->style;
		item->style.text_color  = pstyle->text_color;
		item->style.font        = pstyle->font;
		item->style.font_height = pstyle->font_height;
		item->style.tab_spaces  = pstyle->tab_spaces;
		item->style.text_wrap   = pstyle->text_wrap; 
	}
	
	item->file_created = file;
	item->line_created = line;
	
	item->memsize = sizeof(uiText);
	item->drawcmds = make_drawcmd(1);
	item->__generate = &ui_gen_text;
	item->__evaluate = &ui_eval_text;
	
	uiGetText(item)->text = text;
	uiGetText(item)->breaks.allocator = deshi_allocator;

	RenderDrawCounts counts = render_make_text_counts(str8_length(text));
	
	item->draw_cmd_count = 1;
	drawcmd_alloc(item->drawcmds, counts);
	return item;
}

//---------------------------------------------------------------------------------------------------------------------
// @slider

void ui_gen_slider(uiItem* item){DPZoneScoped;
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	RenderDrawCounts counts = {0};	
	uiSlider* data = uiGetSlider(item);
	
	vec2 pos = item->spos + item->cpos;
	vec2 size = item->csize;

	counts+=gen_background(item, vp, ip, counts);
	counts+=gen_border(item, vp, ip, counts);

	counts+=render_make_filledrect(vp,ip,counts,
		vec2(pos.x, pos.y + size.y*(1 - data->style.rail_thickness)/2),
		vec2(size.x, size.y * data->style.rail_thickness),
		data->style.colors.rail
	);

	if(data->style.dragger_shape == slider_dragger_rect){
		vec2 dragp = vec2(pos.x+data->pos, pos.y);
		vec2 drags = vec2(data->width, size.y);
		counts+=render_make_filledrect(vp,ip,counts,dragp,drags,data->style.colors.dragger);
	}else if(data->style.dragger_shape == slider_dragger_round){
		NotImplemented;
	}
	dc->counts = counts;
}

void ui_slider_callback(uiItem* item){DPZoneScoped;
	uiSlider* data = uiGetSlider(item);
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
			dragwidth = item->width/8;
			dragpos = Remap(*var, 0.f, item->cwidth-dragwidth, min, max);
			if(input_lmouse_pressed() && Math::PointInRectangle(lmp, vec2(dragpos,0), vec2(dragwidth, item->cheight))){
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
}

uiItem* ui_make_slider(uiStyle* style, str8 file, upt line){DPZoneScoped;
	auto [item, datav] = make_item(sizeof(uiSlider), offsetof(uiSlider, item));
	uiSlider* data = (uiSlider*)datav;
	ui_fill_item(item, style, file, line);

	item->memsize = sizeof(uiSlider);
	item->__generate = &ui_gen_slider;

	RenderDrawCounts counts = //reserve enough room for slider rail, dragger, and outline
		render_make_filledrect_counts()*2+
		render_make_rect_counts();

	item->drawcmds = make_drawcmd(1);
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
	uiCheckbox* data = uiGetCheckbox(item);
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	RenderDrawCounts counts = {0};	

	vec2 fillingpos = item->spos + data->style.fill_padding + item->cpos;
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

	dc->counts = counts;
}

void ui_checkbox_callback(uiItem* item){
	uiCheckbox* data = uiGetCheckbox(item);
	*data->var = !*data->var;
	item->dirty = 1;
}

uiItem* ui_make_checkbox(b32* var, uiStyle* style, str8 file, upt line){
	auto [item, datav] = make_item(sizeof(uiCheckbox), offsetof(uiCheckbox, item));
	uiCheckbox* data = (uiCheckbox*)datav;
	ui_fill_item(item, style, file, line);

	item->action = &ui_checkbox_callback;
	item->__hash = &checkbox_style_hash;
	item->__generate = *ui_gen_checkbox;
	item->action_trigger = action_act_mouse_pressed;

	data->style.colors.filling = color(100,150,200);
	data->style.fill_type = checkbox_fill_box;
	data->style.fill_padding = vec2{2,2};
	data->var = var;

	RenderDrawCounts counts = //reserve enough room for background, box filling, and outline
		render_make_filledrect_counts()*2+
		render_make_rect_counts();

	item->drawcmds = make_drawcmd(1);
	item->draw_cmd_count = 1;
	drawcmd_alloc(item->drawcmds, counts);
	
	return item;
}

/*---------------------------------------------------------------------------------------------------------------------

	@ui_debug

*/

struct ui_debug_win_info{
	uiItem* selected_item;
	
	b32 selecting_item;


	uiItem* internal_info;

}ui_dwi;

void ui_debug_callback(uiItem* item){


	if(g_ui->hovered){
		// render_start_cmd2(7, 0, vec2::ZERO, DeshWindow->dimensions);
		// vec2 ipos = g_ui->hovered->spos;
		// vec2 mpos = ipos + g_ui->hovered->style.margintl;
		// vec2 bpos = mpos + (g_ui->hovered->style.border_style ? g_ui->hovered->style.border_width : 0) * vec2::ONE;
		// vec2 ppos = bpos + g_ui->hovered->style.paddingtl;

		// render_quad2(ipos, g_ui->hovered->size, Color_Red);
		// render_quad2(mpos, MarginedArea(g_ui->hovered), Color_Magenta);
		// render_quad2(bpos, BorderedArea(g_ui->hovered), Color_Blue);
		// render_quad2(ppos, PaddedArea(g_ui->hovered), Color_Green);

	}




}

void ui_debug_panel_callback(uiItem* item){
	if(Math::PointInRectangle(
		input_mouse_position(),
		item->spos + item->size.xComp() - vec2(10,0), item->size.yComp() + vec2(10, 0))){
		
	}

}

void ui_debug(){
	
	ui_dwi = {0};

	uiStyle def_style{0};
		def_style.sizing = size_auto;
		def_style.text_color = Color_White;
		def_style.text_wrap = text_wrap_none;
		def_style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
		def_style.font_height = 11;
		def_style.background_color = color(14,14,14);
		def_style.tab_spaces = 4;
		def_style.border_color = color(188,188,188);
		def_style.border_width = 1;

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
		window->id = STR8("_ui_ debug win");
		window->action = &ui_debug_callback;
		window->action_trigger = action_act_always;
		style = &window->style;
		style->positioning = pos_draggable_relative;
		style->background_color = color(14,14,14);
		style->border_style = border_solid;
		style->border_color = color(188,188,188);
		style->border_width = 1;
		style->focus = 1;
		style->size = {500,300};
		style->display = display_flex | display_row;
		style->padding = {5,5,5,5};

		{uiItem* panel = uiItemBS(&panel_style); //selected information
			panel->id = STR8("_ui_ debug win panel0");
			panel->action = &ui_debug_panel_callback;
			panel->action_trigger = action_act_always;
			panel->style.width = 1;
			//panel->style.margin_right = 1;

			{uiItem* internal_info = uiItemB(); 
				internal_info->style = def_style;
				internal_info->id = STR8("_ui_debug internal info");
				internal_info->style.sizing = size_percent_x;
				internal_info->style.width = 100;
				internal_info->style.height = 100;
				internal_info->style.background_color = color(14,14,14);
				internal_info->style.content_align = {0.5, 0.5};


				uiItemE();
			}


		}uiItemE();

		if(0){uiItem* panel = uiItemBS(&panel_style);
			panel->id = STR8("_ui_ debug win panel1");
			panel->action = &ui_debug_panel_callback;
			panel->action_trigger = action_act_always;
			panel->style.width = 0.5;

			
			// {uiItem* item_list = uiItemBS(&itemlist_style);
			// 	for_node(g_ui->base.node.first_child){

			// 	}

			// }uiItemE();
			uiItemE();
		}
	}uiItemE();
}

/*-----------------------------------------------------------------------------------------------------------------

	@demo

*/

void ui_demo(){
	{//test sizer
		uiItem* container = uiItemB();{
			container->style.size = {200, 100};
			container->style.display = display_flex | display_row;
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
				c0->style.font = Storage::CreateFontFromFileBDF(STR8("gohufont-11.bdf")).second;
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
}


#undef ui_alloc
#undef ui_realloc
#undef ui_free
#undef ui_talloc
#undef ui_create_arena
#undef ui_delete_arena
#undef item_arena
#undef drawcmd_arena