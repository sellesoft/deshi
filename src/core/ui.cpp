/*
Index:
@ui_helpers
  item_error(uiItem* item, ...) -> void
  gen_error(str8 file, upt line, ...) -> void
  push_item(uiItem* item) -> void
  pop_item() -> void
@ui_drawcmd
  drawcmd_remove(uiDrawCmd* drawcmd) -> void
  drawcmd_alloc(uiDrawCmd* drawcmd, vec2i counts) -> void
@ui_item	
  ui_setup_item(uiItem* item, uiStyle* style, str8 file, upt line) -> void 
  ui_gen_item(uiItem* item) -> void 
  ui_make_item(uiStyle* style, str8 file, upt line) -> uiItem* 
  ui_begin_item(uiStyle* style, str8 file, upt line) -> uiItem* 
  ui_end_item(str8 file, upt line) -> void 
  ui_remove_item(uiItem* item, str8 file, upt line) -> void 
@ui_context
  ui_init(MemoryContext* memctx, uiContext* uictx) -> void 
  ui_find_static_sized_parent(TNode* node, TNode* child) -> TNode* 
  draw_item_branch(uiItem* item) -> void 
  eval_item_branch(uiItem* item) -> void 
  drag_item(uiItem* item) -> void 
  find_hovered_item(uiItem* item) -> b32 
  ui_recur(TNode* node) -> pair<vec2,vec2> 
  ui_update() -> void 
@ui_debug
  ui_debug() -> void 
@ui_demo
  ui_demo() -> void 
*/

#define UI_PRINT_DRAWCMD_ALLOCS false

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_helpers


#define item_error(item, ...)\
  LogE("ui",CyanFormatComma((item)->file_created), ":", (item)->line_created, ":", RedFormatComma("error"), ": ", __VA_ARGS__)

#define gen_error(file,line,...)\
  LogE("ui",CyanFormatComma(file),":",line,":",RedFormatComma("error"),":",__VA_ARGS__)


void push_item(uiItem* item){DPZoneScoped;
	g_ui->item_stack.add(item);
}

uiItem* pop_item(){DPZoneScoped;
	uiItem* ret = *g_ui->item_stack.last;
	g_ui->item_stack.pop();
	return ret;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_drawcmd


uiDrawCmd* 
ui_make_drawcmd(upt count){
	g_ui->stats.drawcmds_reserved += count;
	return (uiDrawCmd*)memalloc(count*sizeof(uiDrawCmd));
}

void 
ui_drawcmd_delete(uiDrawCmd* dc) {
	memzfree(dc);
	g_ui->stats.drawcmds_reserved--;
}


pair<s32, b32>
find_drawcmd_vertex(uiDrawCmd* dc) {
	if(!g_ui->inactive_drawcmds_vertex_sorted.count) return {0, false};
	s32 l = 0, m = 0, r = g_ui->inactive_drawcmds_vertex_sorted.count - 1;
	while(l <= r) {
		m = l+(r-l)/2;
		if(g_ui->inactive_drawcmds_vertex_sorted[m]->vertex_offset == dc->vertex_offset) {
			return {m,true};
		} else if(g_ui->inactive_drawcmds_vertex_sorted[m]->vertex_offset < dc->vertex_offset) {
			l = m+1;
		} else {
			r = m-1;
		}
	}
	return {m, false};
}

pair<s32, b32>
find_drawcmd_index(uiDrawCmd* dc) {
	if(!g_ui->inactive_drawcmds_index_sorted.count) return {0, false};
	s32 l = 0, m = 0, r = g_ui->inactive_drawcmds_index_sorted.count - 1;
	while(l <= r) {
		m = l+(r-l)/2;
		if(g_ui->inactive_drawcmds_index_sorted[m]->index_offset == dc->index_offset) {
			return {m,true};
		} else if(g_ui->inactive_drawcmds_index_sorted[m]->index_offset < dc->index_offset) {
			l = m+1;
		} else {
			r = m-1;
		}
	}
	return {m, false};
}


void 
ui_drawcmd_remove(uiDrawCmd* drawcmd){DPZoneScoped;
	if(!(drawcmd->counts_reserved.x || drawcmd->counts_reserved.y)) {
		ui_drawcmd_delete(drawcmd);
		return;
	}

	carray<uiDrawCmd*> varr = {g_ui->inactive_drawcmds_vertex_sorted.data, g_ui->inactive_drawcmds_vertex_sorted.count};
	carray<uiDrawCmd*> iarr = {g_ui->inactive_drawcmds_index_sorted.data, g_ui->inactive_drawcmds_index_sorted.count};
	
	if(g_ui->inactive_drawcmds_vertex_sorted.count){
		auto [index, found] = find_drawcmd_vertex(drawcmd);
		Assert(!found, "we shouldn't come across a drawcmd that's already in this list.");
		g_ui->inactive_drawcmds_vertex_sorted.insert(drawcmd, index);
		if(index != g_ui->inactive_drawcmds_vertex_sorted.count-1){
			uiDrawCmd* right = g_ui->inactive_drawcmds_vertex_sorted[index+1];
			if(right->vertex_offset - drawcmd->counts_reserved.x == drawcmd->vertex_offset){
				// in this case we have found a drawcmd on the right side that is aligned with the one we are currently 
				// deleting, so we can take its vertices and set its count to 0
				drawcmd->counts_reserved.x += right->counts_reserved.x;
				right->counts_reserved.x = 0;
				g_ui->inactive_drawcmds_vertex_sorted.remove(index+1);
				if(!right->counts_reserved.y){
					ui_drawcmd_delete(right);
				}
			}
		}
		if(index){
			uiDrawCmd* left = g_ui->inactive_drawcmds_vertex_sorted[index-1];
			if(left->vertex_offset + left->counts_reserved.x == drawcmd->vertex_offset){
				// in this case we have found a drawcmd on the left side that is aligned with the one we are currently 
				// deleting, so we can give it it's vertices and set them to 0
				left->counts_reserved.x += drawcmd->counts_reserved.x;
				drawcmd->counts_reserved.x = 0; 
				g_ui->inactive_drawcmds_vertex_sorted.remove(index);
				//since this is the drawcmd we are currently working with, its index count shouldn't be zero so we dont check that
			}
		}
	}else{
		g_ui->inactive_drawcmds_vertex_sorted.add(drawcmd);
	}
	
	if(g_ui->inactive_drawcmds_index_sorted.count){
		auto [index, found] = find_drawcmd_index(drawcmd);
		Assert(!found, "we shouldn't come across a drawcmd that's already in this list.");
		g_ui->inactive_drawcmds_index_sorted.insert(drawcmd, index);
		if(index != g_ui->inactive_drawcmds_index_sorted.count-1){
			uiDrawCmd* right = g_ui->inactive_drawcmds_index_sorted[index+1];
			if(right->index_offset - drawcmd->counts_reserved.y == drawcmd->index_offset){
				// in this case we have found a drawcmd on the right side that is aligned with the one we are currently 
				// deleting, so we can take its vertices and set its count to 0
				drawcmd->counts_reserved.y += right->counts_reserved.y;
				right->counts_reserved.y = 0;
				g_ui->inactive_drawcmds_index_sorted.remove(index+1);
				if(!right->counts_reserved.x){
					ui_drawcmd_delete(right);
				}
			}
		}
		if(index){
			uiDrawCmd* left = g_ui->inactive_drawcmds_index_sorted[index-1];
			if(left->index_offset + left->counts_reserved.y == drawcmd->index_offset){
				// in this case we have found a drawcmd on the left side that is aligned with the one we are currently 
				// deleting, so we can give it it's vertices and set them to 0
				left->counts_reserved.y += drawcmd->counts_reserved.y;
				drawcmd->counts_reserved.y = 0; 
				g_ui->inactive_drawcmds_index_sorted.remove(index);
				if(!drawcmd->counts_reserved.x){
					//this drawcmd has been absorbed by others, so its safe to delete it
					ui_drawcmd_delete(drawcmd);
				}
			}
		}
	}else{
		g_ui->inactive_drawcmds_index_sorted.add(drawcmd);
	}
}

void 
ui_drawcmd_alloc(uiDrawCmd* drawcmd, vec2i counts){DPZoneScoped;
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
					auto [index, found] = find_drawcmd_index(dc);
					if(found) g_ui->inactive_drawcmds_index_sorted.remove(index);
					ui_drawcmd_delete(dc);
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
		if(dc->freed) { // it's possible this dc was already deleted above, so we have to remove it from the list here
			g_ui->inactive_drawcmds_index_sorted.remove(i);
			i--;
		} 

		s64 iremain = dc->counts_reserved.y - counts.y;
		if(iremain >= 0){
			i_place_next = dc->index_offset;
			if(!iremain){
				g_ui->inactive_drawcmds_index_sorted.remove(i);
				dc->counts_reserved.y = 0;
				if(!dc->counts_reserved.x){
					ui_drawcmd_delete(dc);
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
		drawcmd->vertex_offset = g_ui->vertex_buffer.cursor;
		g_ui->vertex_buffer.cursor += counts.x;
	} else drawcmd->vertex_offset = v_place_next;
	if(i_place_next == -1){
		//we couldnt find a drawcmd with space for our new indices so we must allocate at the end
		g_ui->stats.indices_reserved += counts.y;
		drawcmd->index_offset = g_ui->index_buffer.cursor;
		g_ui->index_buffer.cursor += counts.y;
	} else drawcmd->index_offset = i_place_next;
	drawcmd->counts_reserved = counts;

#if UI_PRINT_DRAWCMD_ALLOCS
	Log("ui", "allocated drawcmd: vo(", drawcmd->vertex_offset, ") vc(", drawcmd->counts_reserved.x, ") io(", drawcmd->index_offset, ") ic(", drawcmd->counts_reserved.y, ")");
#endif
}


uiDrawCmdPtrs
ui_drawcmd_realloc(uiDrawCmd* dc, vec2i counts) {
	if(dc->counts_reserved.x >= counts.x && dc->counts_reserved.y >= counts.y){
		return ui_drawcmd_get_ptrs(dc);
	}
	
	auto restore_texture = dc->texture;
	
	// the given drawcmd needs to release its drawinfo but still be usable by whatever is calling 
	// this function, so we make a copy of it, call remove on that drawcmd, then reallocate this one
	auto dummy = (uiDrawCmd*)memalloc(sizeof(uiDrawCmd));
	*dummy = *dc;
	ui_drawcmd_remove(dummy);
	
	*dc = {};
	dc->texture = restore_texture;
	
	ui_drawcmd_alloc(dc, counts);
	return ui_drawcmd_get_ptrs(dc);
}

uiDrawCmdPtrs 
ui_drawcmd_get_ptrs(uiDrawCmd* dc) {
	return {(uiVertex*)graphics_buffer_mapped_data(g_ui->vertex_buffer.handle) + dc->vertex_offset, (u32*)graphics_buffer_mapped_data(g_ui->index_buffer.handle) + dc->index_offset};
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_item


//optionally takes a pointer to a boolean to indicate if the item was retrieved from the cache
uiItem* ui_setup_item(uiItemSetup setup, b32* retrieved){DPZoneScoped;
	uiItem* parent = *g_ui->item_stack.last;
	
	//initialize the item in memory or retrieve it from cache if it already exists
	uiItem* item;
	b32 retrieved_ = 0;
	if(g_ui->immediate.active){
		//attempt to unique identify items without requiruing the user to provide unique ids
		//im pretty sure this will fail in some cases 
		//if its REALLY bad then we should just go with allowing the user to give unique ids
		u64 hash = str8_hash64(setup.file);
		hash ^=        setup.line;     hash *= UI_HASH_PRIME;
		hash ^=        setup.size;     hash *= UI_HASH_PRIME;
		hash ^= *(u64*)setup.generate; hash *= UI_HASH_PRIME;
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
		item->link.prev = item->link.next = &item->link;
		NodeInsertPrev(&g_ui->base.link, &item->link);
	}
	item->memsize = setup.size;
	
	if(retrieved_){
		//at this time, a retrieved item must always be reevaluated and regenerated.
		item->style_hash = 0;
		if(retrieved) *retrieved = 1;
	}
	
	insert_last(&parent->node, &item->node);
	
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
	item->__copy         = setup.copy;
	
	// if an item was retrieved we leave its drawcmds alone and it's expected
	// to manage those itself
	// It used to be that we would clear an immediate item's drawcmds every frame, but
	// with the render rewrite we no longer associate descriptor sets with textures 
	// in the render backend, we have to create them manually here. The only place 
	// it makes sense to me (atm) to put descriptor sets is on the drawcmds when they
	// have textures. This needs to be changed eventually, especially because someone
	// wanting to make a ui widget should not be required to mess with the render 
	// api at all.
	if(!retrieved_) {
		item->drawcmd_count = setup.drawcmd_count;
		item->drawcmds = ui_make_drawcmd(setup.drawcmd_count);
		if(setup.drawinfo_reserve){
			forI(setup.drawcmd_count){
				ui_drawcmd_alloc(item->drawcmds+i, setup.drawinfo_reserve[i]);
			}
		}
	}

	item->since_last_update = start_stopwatch();
	return item;
}

void 
deshi__ui_push_item(uiItem* item, str8 file, upt line) {
	g_ui->item_stack.add(item);
}

uiItem* 
deshi__ui_pop_item(u32 count, str8 file, upt line) {
	if(count >= g_ui->item_stack.count) {
		LogE("ui", "too many items requested for popping. There are ", g_ui->item_stack.count, " items on the stack and ", count, " items were requested to be popped.");
		LogE("ui", "Note: called from ", file, ":", line);
	}
	return g_ui->item_stack.pop(count);
}

vec2i 
ui_gen_background(uiItem* item, uiVertex* vp, u32* ip, vec2i counts){
	vec2 mtl = floor(item->style.margintl * item->scale);
	vec2 mbr = floor(item->style.marginbr * item->scale);
	vec2 bor = floor((item->style.border_style ? item->style.border_width : 0) * item->scale);
	vec2 pos = item->pos_screen + mtl + bor;
	vec2 siz = floor(item->size * item->scale); //NOTE(delle) item->size already has margins and borders applied in eval_item_branch
	return ui_put_filledrect(vp, ip, counts, pos, siz, item->style.background_color);
}

vec2i 
ui_gen_border(uiItem* item, uiVertex* vp, u32* ip, vec2i counts){
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

b32 
mouse_in_item(uiItem* item){
	return Math::PointInRectangle(input_mouse_position(), item->pos_screen, item->size * item->scale);
}

b32 
ui_item_hovered(uiItem* item, u32 mode){
	switch(mode) {
		case hovered_strict: return g_ui->hovered == item;
		case hovered_area: return mouse_in_item(item);
		case hovered_child: {
			auto i = g_ui->hovered;
			while(i) {
				if(i == item) return true;
				if(i == &g_ui->base) return false;
				i = (uiItem*)i->node.parent;
			}
			Assert(0, "an item given to ui_item_hovered must be a part of the active item tree (ie. it must be an ancestor of g_ui->base)");
		} break;
	}
	return false;
}

uiItem*
ui_deep_copy_lower(uiItem* item) {
	Assert(item->__copy, "deep copied items are required to implemented __copy");

	// allocate a new item with the same size
	auto nu = (uiItem*)memalloc(item->memsize);

	// copy all memory literally
	CopyMemory(nu, item, item->memsize);

	// clear the node since we're about to replace all 
	// children with copies
	nu->node = {0};

	// invoke deep_copy on all children first
	for(auto i = (uiItem*)item->node.first_child; i; i = (uiItem*)i->node.next) {
		insert_last(&nu->node, &ui_deep_copy_lower(i)->node);
	}
	
	nu->drawcmds = ui_make_drawcmd(item->drawcmd_count);

	// allocate drawcmds for this item
	forI(item->drawcmd_count) {
		auto dc = item->drawcmds + i;
		ui_drawcmd_alloc(nu->drawcmds + i, dc->counts_reserved);
	}

	nu->dirty = true;
	
	return nu;
}

uiItem*
ui_deep_copy(uiItem* item) {
	Assert(item, "ui_deep_copy passed a null item");
	Assert(item != &g_ui->base, "cannot deep copy base item");

	auto i = ui_deep_copy_lower(item);
	insert_last(item->node.parent, &i->node);

	return i;
}

void
ui_item_copy_base(uiItem* to, uiItem* from) {DPZoneScoped;
	// TODO(sushi) consider automatically appending '-copy' to the id here
	to->id  = from->id;
	to->userVar = from->userVar;
	to->style = from->style;
	to->action = from->action;
	// should this be copied?
	to->action_data = from->action_data;
	to->action_trigger = from->action_trigger;
	to->update_trigger = from->update_trigger;
	to->__update = from->__update;
	to->__evaluate = from->__evaluate;
	to->__generate = from->__generate;
	to->__hash = from->__hash;
	to->__cleanup = from->__cleanup;
	to->__copy = from->__copy;
	to->file_created = {};
	to->line_created = 0;
	to->memsize = from->memsize;
}

// internal copy function for plain uiItems
// this should NOT be used for widgets!!!
// incorrect behavoir will occur if so!
uiItem*
ui_copy_item(uiItem* item) { DPZoneScoped;
	auto nu = (uiItem*)memalloc(sizeof(uiItem));
	ui_item_copy_base(nu, item);
	return nu;
}

void 
ui_gen_item(uiItem* item){DPZoneScoped;
	uiDrawCmd* dc = item->drawcmds;
	auto [vp, ip] = ui_drawcmd_get_ptrs(dc);
	vec2i counts = {0};
	counts += ui_gen_background(item, vp, ip, counts);
	counts += ui_gen_border(item, vp, ip, counts);
	dc->texture = item->style.background_image;
	dc->counts_used = counts;
}

uiItem* 
deshi__ui_make_item(uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItemSetup setup = {0};
	setup.size = sizeof(uiItem);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.generate = &ui_gen_item;
	setup.copy = ui_copy_item;
	setup.drawcmd_count = 1;
	vec2i counts[1] = {
		ui_put_filledrect_counts() + ui_put_rect_counts()
	};
	setup.drawinfo_reserve = counts;
	
	uiItem* item = ui_setup_item(setup);
	
	return item;
}

uiItem* 
deshi__ui_begin_item(uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = deshi__ui_make_item(style, file, line);
	push_item(item);
	return item;
}

void 
deshi__ui_end_item(str8 file, upt line){DPZoneScoped;
	if(g_ui->item_stack.count == 1){
		LogE("ui", 
			 "In ", file, " at line ", line, " :\n",
			 "\tAttempted to end base item. Did you call ui_end_item too many times? Did you use uiItemM instead of uiItemB?"
			 );
		//TODO(sushi) implement a hint showing what instruction could possibly be wrong 
	}else pop_item();
	
}

void 
deshi__ui_remove_item(uiItem* item, str8 file, upt line){DPZoneScoped;
	NodeRemove(&item->link);

	forI(item->drawcmd_count){
		ui_drawcmd_remove(item->drawcmds + i);
	}
	
	remove(&item->node);
	memzfree(item);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_immediate


void 
deshi__ui_begin_immediate_branch(uiItem* parent, str8 file, upt line){
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

void 
deshi__ui_end_immediate_branch(str8 file, upt line){
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

void 
deshi__ui_push_id(s64 x, str8 file, upt line){
	//g_ui->immediate.id_stack.add(x);
	NotImplemented; // TODO(sushi) decide if these are necessary
}

void 
deshi__ui_pop_id(str8 file, upt line){
	//if(!g_ui->immediate.id_stack.count) gen_error(file,line,"ui_pop_id was called before any calls to ui_push_id were made");
	//else g_ui->immediate.id_stack.pop();
	NotImplemented; // TODO(sushi) decide if these are necessary
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_context


void
deshi__ui_init(Window* window) {
	DeshiStageInitStart(DS_UI, DS_MEMORY, "Attempted to initialize UI2 module before initializing the Memory module");
	
	g_ui->allocator_root.next = g_ui->allocator_root.prev = &g_ui->allocator_root;
	
	g_ui->immediate.active = 0;
	g_ui->immediate.pushed = 0;
	
	g_ui->inactive_drawcmds.next = &g_ui->inactive_drawcmds;
	g_ui->inactive_drawcmds.prev = &g_ui->inactive_drawcmds;

	g_ui->render_pass = graphics_render_pass_allocate();
	g_ui->render_pass->debug_name = str8l("<ui> render pass");
	g_ui->render_pass->use_color_attachment = true;
	g_ui->render_pass->color_attachment.          format = graphics_format_of_presentation_frames(window);
	g_ui->render_pass->color_attachment.         load_op = GraphicsLoadOp_Load;
	g_ui->render_pass->color_attachment.        store_op = GraphicsStoreOp_Store;
	g_ui->render_pass->color_attachment. stencil_load_op = GraphicsLoadOp_Dont_Care;
	g_ui->render_pass->color_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	g_ui->render_pass->color_attachment.  initial_layout = GraphicsImageLayout_Present;
	g_ui->render_pass->color_attachment.    final_layout = GraphicsImageLayout_Present;
	g_ui->render_pass->use_depth_attachment = true;
	g_ui->render_pass->depth_attachment.          format = graphics_current_present_frame_of_window(window)->render_pass->depth_attachment.format;
	g_ui->render_pass->depth_attachment.         load_op = GraphicsLoadOp_Clear;
	g_ui->render_pass->depth_attachment.        store_op = GraphicsStoreOp_Store;
	g_ui->render_pass->depth_attachment. stencil_load_op = GraphicsLoadOp_Clear;
	g_ui->render_pass->depth_attachment.stencil_store_op = GraphicsStoreOp_Dont_Care;
	g_ui->render_pass->depth_attachment.  initial_layout = GraphicsImageLayout_Undefined;
	g_ui->render_pass->depth_attachment.    final_layout = GraphicsImageLayout_Depth_Stencil_Attachment_Optimal;
	graphics_render_pass_update(g_ui->render_pass);

	g_ui->pipeline = graphics_pipeline_allocate();
	g_ui->pipeline->debug_name = str8l("<ui> pipeline");

	g_ui->pipeline->vertex_shader = graphics_shader_allocate();
	g_ui->pipeline->vertex_shader->debug_name = str8l("<ui> vertex shader");
	g_ui->pipeline->vertex_shader->shader_stage = GraphicsShaderStage_Vertex;
	g_ui->pipeline->vertex_shader->source = baked_shader_twod_vert;
	graphics_shader_update(g_ui->pipeline->vertex_shader);

	g_ui->pipeline->fragment_shader = graphics_shader_allocate();
	g_ui->pipeline->fragment_shader->debug_name = str8l("<ui> fragment shader");
	g_ui->pipeline->fragment_shader->shader_stage = GraphicsShaderStage_Fragment;
	g_ui->pipeline->fragment_shader->source = baked_shader_twod_frag;
	graphics_shader_update(g_ui->pipeline->fragment_shader);

	g_ui->pipeline->            front_face = GraphicsFrontFace_CCW;
	g_ui->pipeline->               culling = GraphicsPipelineCulling_None;
	g_ui->pipeline->          polygon_mode = GraphicsPolygonMode_Fill;
	g_ui->pipeline->            depth_test = true;
	g_ui->pipeline->      depth_compare_op = GraphicsCompareOp_Always;
	g_ui->pipeline->            line_width = 1.f;
	g_ui->pipeline->           color_blend = true;
	g_ui->pipeline->        color_blend_op = GraphicsBlendOp_Add;
	g_ui->pipeline->color_src_blend_factor = GraphicsBlendFactor_Source_Alpha;
	g_ui->pipeline->color_dst_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
	g_ui->pipeline->        alpha_blend_op = GraphicsBlendOp_Add;
	g_ui->pipeline->alpha_src_blend_factor = GraphicsBlendFactor_One_Minus_Source_Alpha;
	g_ui->pipeline->alpha_dst_blend_factor = GraphicsBlendFactor_Zero;
	g_ui->pipeline->        blend_constant = color(10,10,10,255);
	g_ui->pipeline->           render_pass = graphics_render_pass_of_window_presentation_frames(window);

	g_ui->pipeline->dynamic_viewport = 
	g_ui->pipeline->dynamic_scissor  = true;

	array_init_with_elements(g_ui->pipeline->vertex_input_bindings, 
			{{0, sizeof(uiVertex)}}, deshi_allocator);
	array_init_with_elements(g_ui->pipeline->vertex_input_attributes, {
				{0, 0, GraphicsFormat_R32G32_Float,   offsetof(uiVertex, pos)},
				{1, 0, GraphicsFormat_R32G32_Float,   offsetof(uiVertex, uv)},
				{2, 0, GraphicsFormat_R8G8B8A8_UNorm, offsetof(uiVertex, color)},
			}, deshi_allocator);
	auto layout = graphics_descriptor_set_layout_allocate();
	layout->debug_name = str8l("ui descriptor layout");
	array_init_with_elements(layout->bindings,{
			{
				GraphicsDescriptorType_Combined_Image_Sampler,
				GraphicsShaderStage_Fragment,
				0
			}}, deshi_allocator);
	graphics_descriptor_set_layout_update(layout);

	g_ui->push_constant.size = 2 * sizeof(vec2);
	g_ui->push_constant.offset = 0;
	g_ui->push_constant.shader_stages = GraphicsShaderStage_Vertex;
	g_ui->push_constant.name_in_shader = "PushConstant";

	auto pipeline_layout = graphics_pipeline_layout_allocate();
	pipeline_layout->debug_name = str8l("ui pipeline layout");
	array_init_with_elements(pipeline_layout->descriptor_layouts, {layout});
	array_init_with_elements(pipeline_layout->push_constants, {g_ui->push_constant});
	graphics_pipeline_layout_update(pipeline_layout);

	g_ui->pipeline->layout = pipeline_layout;
	graphics_pipeline_update(g_ui->pipeline);

	g_ui->vertex_buffer.handle = graphics_buffer_create(
			0, 
			g_memory->arena_heap.size / 16,
			GraphicsBufferUsage_VertexBuffer,
			GraphicsMemoryPropertyFlag_HostVisible | GraphicsMemoryPropertyFlag_HostCoherent,
			GraphicsMemoryMapping_Persistent);
	g_ui->vertex_buffer.handle->debug_name = str8l("<ui> vertex buffer");
	g_ui->index_buffer.handle = graphics_buffer_create(
			0, 
			g_memory->arena_heap.size / 16,
			GraphicsBufferUsage_IndexBuffer,
			GraphicsMemoryPropertyFlag_HostVisible | GraphicsMemoryPropertyFlag_HostCoherent,
			GraphicsMemoryMapping_Persistent);
	g_ui->index_buffer.handle->debug_name = str8l("<ui> index buffer");
	
	GraphicsDescriptor* descriptors = array_create(GraphicsDescriptor, 1, deshi_allocator);
	GraphicsDescriptor* descriptor = array_push(descriptors);
	descriptor->          type = GraphicsDescriptorType_Combined_Image_Sampler;
	descriptor->name_in_shader = "font_texture";
	descriptor->    image.view = assets_font_null()->tex->image_view;
	descriptor-> image.sampler = assets_font_null()->tex->sampler;
	descriptor->  image.layout = GraphicsImageLayout_Shader_Read_Only_Optimal;

	g_ui->blank_descriptor_set = graphics_descriptor_set_allocate();
	g_ui->blank_descriptor_set->debug_name = str8l("ui blank descriptor set");
	g_ui->blank_descriptor_set->descriptors = descriptors;
	array_init_with_elements(g_ui->blank_descriptor_set->layouts, 
			{g_ui->pipeline->layout->descriptor_layouts[0]});
	graphics_descriptor_set_update(g_ui->blank_descriptor_set);
	graphics_descriptor_set_write(g_ui->blank_descriptor_set);

	g_ui->base = uiItem{0};
	g_ui->base.id = STR8("base");
	g_ui->base.file_created = STR8(__FILE__);
	g_ui->base.line_created = __LINE__;
	g_ui->base.style.width = DeshWindow->width;
	g_ui->base.style.height = DeshWindow->height;
	g_ui->base.style_hash = ui_hash_style(&g_ui->base);
	g_ui->base.link.prev = g_ui->base.link.next = &g_ui->base.link;
	push_item(&g_ui->base);
	
	//setup default keybinds
	//TODO(sushi) export these to a config file and load them instead
	g_ui->keys.inputtext.cursor.          left = Key_LEFT  | InputMod_None;
	g_ui->keys.inputtext.cursor.     left_word = Key_LEFT  | InputMod_AnyCtrl;
	g_ui->keys.inputtext.cursor. left_wordpart = Key_LEFT  | InputMod_AnyAlt;
	g_ui->keys.inputtext.cursor.         right = Key_RIGHT | InputMod_None;
	g_ui->keys.inputtext.cursor.    right_word = Key_RIGHT | InputMod_AnyCtrl;
	g_ui->keys.inputtext.cursor.right_wordpart = Key_RIGHT | InputMod_AnyAlt;
	g_ui->keys.inputtext.cursor.            up = Key_UP    | InputMod_None;
	g_ui->keys.inputtext.cursor.          down = Key_DOWN  | InputMod_None;
	
	g_ui->keys.inputtext.select.          left = Key_LEFT  | InputMod_AnyShift;
	g_ui->keys.inputtext.select.     left_word = Key_LEFT  | InputMod_AnyShift | InputMod_AnyCtrl;
	g_ui->keys.inputtext.select. left_wordpart = Key_LEFT  | InputMod_AnyShift | InputMod_AnyAlt;
	g_ui->keys.inputtext.select.         right = Key_RIGHT | InputMod_AnyShift;
	g_ui->keys.inputtext.select.    right_word = Key_RIGHT | InputMod_AnyShift | InputMod_AnyCtrl;
	g_ui->keys.inputtext.select.right_wordpart = Key_RIGHT | InputMod_AnyShift | InputMod_AnyAlt;
	g_ui->keys.inputtext.select.            up = Key_UP    | InputMod_AnyShift;
	g_ui->keys.inputtext.select.          down = Key_DOWN  | InputMod_AnyShift;
	
	g_ui->keys.inputtext.del.             left = Key_BACKSPACE | InputMod_None;
	g_ui->keys.inputtext.del.        left_word = Key_BACKSPACE | InputMod_AnyCtrl;
	g_ui->keys.inputtext.del.    left_wordpart = Key_BACKSPACE | InputMod_AnyAlt;
	g_ui->keys.inputtext.del.            right = Key_DELETE    | InputMod_None;
	g_ui->keys.inputtext.del.       right_word = Key_DELETE    | InputMod_AnyCtrl;
	g_ui->keys.inputtext.del.   right_wordpart = Key_DELETE    | InputMod_AnyAlt;
	
	g_ui->keys.drag_item = Mouse_LEFT;
	
	DeshiStageInitEnd(DS_UI);
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
			switch(item->style.anchor) {
				case anchor_top_left:{
					item->pos_screen.x = item->style.x;
					item->pos_screen.y = item->style.y;
				}break;
				case anchor_top_right:{
					item->pos_screen.x = g_ui->base.width - item->style.x - item->width;
					item->pos_screen.y = item->style.y;
				}break;
				case anchor_bottom_right:{
					item->pos_screen.x = g_ui->base.width - item->style.x - item->width;
					item->pos_screen.y = g_ui->base.height - item->style.y - item->height;
				}break;
				case anchor_bottom_left:{
					item->pos_screen.x = item->style.x;
					item->pos_screen.y = g_ui->base.height - item->style.y - item->height;
				}break;
			}
		}else{
			item->pos_screen = uiItemFromNode(item->node.parent)->pos_screen + item->pos_local;
		}
	}
	
	if(item->drawcmd_count){
		Assert(item->__generate, "item with no generate function");
		item->__generate(item);
	}
	
	if(item->node.first_child){
		for_node(item->node.first_child){
			draw_item_branch(uiItemFromNode(it));
		}
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
#if BUILD_SLOW
	if(item->break_on_evaluate) DebugBreakpoint;
#endif

	//an array of item indexes into the child item list that indicate to the main eval loop that the item has already beem
	//evaluated before it. currently this only happens when the item is a flex container and contains an automatically sized child.
	arrayT<u32> already_evaluated;
	
	EvalContext contextout = {0};
	
	uiItem* parent = uiItemFromNode(item->node.parent);
	
	item->debug_frame_stats.evals++;
	
	b32 wauto = HasFlag(item->style.sizing, size_auto_x); 
	b32 hauto = HasFlag(item->style.sizing, size_auto_y); 
	f32 wborder = (item->style.border_style ? item->style.border_width : 0);
	b32 disprow = HasFlag(item->style.display, display_horizontal);
	//TODO(sushi) this can probably be cleaned up 
	/*0
	

		Sizing logic

	
	*/
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
				item->height = item->style.height/100.f * ui_padded_height(parent);
			}else if (parent->style.height >= 0){ 
				item->height = item->style.height/100.f * ui_padded_style_height(parent);
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
				item->width = item->style.width/100.f * ui_padded_width(parent);
			}else if (parent->style.width >= 0){
				item->width = item->style.width/100.f * ui_padded_style_width(parent);
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
	
	/*1
	

		flex logic

	
	*/
	
	if(HasFlag(item->style.display, display_flex)){
		contextout.flex.flex_container = 1;
		contextout.flex.effective_size = (disprow ? ((((item)->width - (item)->style.margin_left - (item)->style.margin_right) - ((item)->style.border_style ? 2*(item)->style.border_width : 0)) - (item)->style.padding_left - (item)->style.padding_right) : ui_padded_height(item));
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
					// this is done using the already_evaluated array
					eval_item_branch(child, &contextout);
					contextout.flex.effective_size -= (disprow ? child->width : child->height);
					already_evaluated.add(idx);
				}else if(HasFlag(child->style.sizing, size_percent)){
					contextout.flex.effective_size -= (disprow ? child->style.width/100.f * item->width : child->style.height/100.f * item->height);
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
				
				switch(child->style.anchor){
					case anchor_top_left:{
						child->pos_local.x += child->style.x;
						child->pos_local.y += child->style.y;
					}break;
					case anchor_top_right:{
						if(!wauto) child->pos_local.x = (ui_padded_width(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as top_right, but the item's width is set to auto.");
						
						child->pos_local.y += child->style.y;
					}break;
					case anchor_bottom_right:{
						if(!wauto) child->pos_local.x = (ui_padded_width(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's width is set to auto.");
						
						if(!hauto) child->pos_local.y = (ui_padded_height(item) - child->height) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
					case anchor_bottom_left:{
						child->pos_local.x += child->style.x;
						
						if(!hauto) child->pos_local.y = (ui_padded_height(item) - child->height) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
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
						if(!wauto) child->pos_local.x = (ui_padded_width(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as top_right, but the item's width is set to auto.");
						
						child->pos_local.y += child->style.y;
					}break;
					case anchor_bottom_right:{
						if(!wauto) child->pos_local.x = (ui_padded_width(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's width is set to auto.");
						
						if(!hauto) child->pos_local.y = (ui_padded_height(item) - child->height) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
					case anchor_bottom_left:{
						child->pos_local.x += child->style.x;
						
						if(!hauto) child->pos_local.y = (ui_padded_height(item) - child->height) - child->style.y;
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
						if(!wauto) child->pos_local.x = (ui_padded_width(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as top_right, but the item's width is set to auto.");
						
						child->pos_local.y = child->style.y;
					}break;
					case anchor_bottom_right:{
						if(!wauto) child->pos_local.x = (ui_padded_width(item) - child->width) - child->style.x;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's width is set to auto.");
						
						if(!hauto) child->pos_local.y = (ui_padded_height(item) - child->height) - child->style.y;
						else item_error(item, "Item's anchor was specified as bottom_right, but the item's height is set to auto.");
					}break;
					case anchor_bottom_left:{
						child->pos_local.x = child->style.x;
						
						if(!hauto) child->pos_local.y = (ui_padded_height(item) - child->height) - child->style.y;
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
	item->max_scroll = Max(vec2{0,0}, cursor - ui_padded_area(item));
	
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
	item->style_hash = ui_hash_style(item);
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
		if(key_pressed(g_ui->keys.drag_item) && mouse_in_item(item)){
			dragging = true;
			g_ui->istate = uiISDragging;
			mouse_begin = mouse_current;
			item_begin = item->style.pos;
		}
		if(key_released(g_ui->keys.drag_item)){
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
		if(HasFlag(uiItemFromNode(it)->style.display, display_hidden) || item->style.positioning == pos_fixed) continue;
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

#if BUILD_SLOW
	if(item->break_on_update) DebugBreakpoint;
#endif
	
	if(g_ui->hovered == item && item->style.focus && input_lmouse_pressed()){
		move_to_parent_last(&item->node);
		item->dirty = true;
	}
	

	//call the items update function if it exists
	if(item->__update) item->__update(item);
	
	//check if an item's style was modified, if so reevaluate the item,
	//its children, and every child of its parents until a manually sized parent is found
	u32 nuhash = ui_hash_style(item);
	b32 changed = false;
	if(item->dirty || nuhash!=item->style_hash){
		changed = true;
		item->dirty = 0;
		item->style_hash = nuhash; 
		reset_stopwatch(&item->since_last_update);
		uiItem* sspar = uiItemFromNode(ui_find_static_sized_parent(&item->node, 0));
		eval_item_branch(sspar, {});
		draw_item_branch(sspar);
	}

	if(item->__update && (
			HasFlag(item->update_trigger, action_act_always) ||
			HasFlag(item->update_trigger, action_act_hash_change) && changed ||
			ui_item_hovered(item, hovered_area) && (
			HasFlag(item->update_trigger, action_act_mouse_hover) && ui_item_hovered(item, hovered_strict) ||
			HasFlag(item->update_trigger, action_act_mouse_hover_children) && ui_item_hovered(item, hovered_child) ||
			HasFlag(item->update_trigger, action_act_mouse_scroll) && g_input->scrollY ||
			HasFlag(item->update_trigger, action_act_mouse_pressed) && input_lmouse_pressed() || 
			HasFlag(item->update_trigger, action_act_mouse_released) && input_lmouse_released() ||
			HasFlag(item->update_trigger, action_act_mouse_down) && input_lmouse_down()))) {
		item->__update(item);
	}

	if(item->action && (
			HasFlag(item->action_trigger, action_act_always) ||
			HasFlag(item->action_trigger, action_act_hash_change) && changed ||
			ui_item_hovered(item, hovered_area) && (
			HasFlag(item->action_trigger, action_act_mouse_hover) && ui_item_hovered(item, hovered_strict) ||
			HasFlag(item->action_trigger, action_act_mouse_hover_children) && ui_item_hovered(item, hovered_child) ||
			HasFlag(item->action_trigger, action_act_mouse_scroll) && g_input->scrollY ||
			HasFlag(item->action_trigger, action_act_mouse_pressed) && input_lmouse_pressed() || 
			HasFlag(item->action_trigger, action_act_mouse_released) && input_lmouse_released() ||
			HasFlag(item->action_trigger, action_act_mouse_down) && input_lmouse_down()))) {
		item->action(item);
	}
	
	// -MAX_F32 signals the function that the node is hidden and not to consider it 
	if(HasFlag(item->style.display, display_hidden)) return {vec2::ZERO,vec2{-MAX_F32,-MAX_F32}};
	
	//render item
	
	//determine scissoring for overflow_hidden items
	vec2 scoff;
	vec2 scext;
	if (parent && parent->style.overflow != overflow_visible) {
		vec2 cpos = item->pos_screen + item->style.margintl + (item->style.border_style ? item->style.border_width : 0) * vec2::ONE;
		vec2 csiz = ui_bordered_area(item);
		
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

			auto texture = item->drawcmds[i].texture;

			if(texture && texture != g_ui->last_texture) {
				if(!texture->ui_descriptor_set) {
					auto descriptors = array<GraphicsDescriptor>::create_with_count(1, deshi_allocator);
					descriptors[0].type = GraphicsDescriptorType_Combined_Image_Sampler;
					descriptors[0].name_in_shader = "font_texture";
					descriptors[0].image = {
						texture->image_view,
						texture->sampler,
						GraphicsImageLayout_Shader_Read_Only_Optimal,
					};
					
					// NOTE(sushi) atm we store a pointer to a descriptor set on asset textures so that ui can make
					//             one per texture used within it. The reason we do this is because otherwise we would
					//             need a way to match textures to descriptor sets internally in ui, which would probably
					//             have to be some kind of map which I really don't want to have to deal with every single
					//             time we draw something in ui. This needs to be fixed later on though because it's weird
					//             to me that assets stores a pointer on Textures specifically for ui.
					texture->ui_descriptor_set = graphics_descriptor_set_allocate();
					texture->ui_descriptor_set->descriptors = descriptors.ptr;
					texture->ui_descriptor_set->layouts = array_copy(g_ui->blank_descriptor_set->layouts).ptr;
					texture->ui_descriptor_set->debug_name = to_dstr8v(deshi_temp_allocator, "<ui> texture descriptor set").fin;
					graphics_descriptor_set_update(texture->ui_descriptor_set);
					graphics_descriptor_set_write(texture->ui_descriptor_set);
				}	
				graphics_cmd_bind_descriptor_set(g_ui->updating_window, 0, texture->ui_descriptor_set);
			} else if(!texture) {
				graphics_cmd_bind_descriptor_set(g_ui->updating_window, 0, g_ui->blank_descriptor_set);
			}
			graphics_cmd_draw_indexed(g_ui->updating_window, item->drawcmds[i].counts_used.y, item->drawcmds[i].index_offset, item->drawcmds[i].vertex_offset);
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

struct {
	vec2 scale;
	vec2 translation;
} pc;

void 
deshi__ui_update(Window* window) {
	g_ui->updating_window = window;

	pc = {
#if DESHI_VULKAN
		{2.0f/window->width, 2.0f/window->height},
		{-1.0f, -1.0f}
#elif DESHI_OPENGL //#if DESHI_VULKAN
		{2.0f/window->width, -2.0f/window->height},
		{-1.0f, 1.0f}
#else //#elif DESHI_OPENGL //#if DESHI_VULKAN
#  error "unhandled graphics backend"
#endif //#else //#elif DESHI_OPENGL //#if DESHI_VULKAN
	};

	graphics_cmd_begin_render_pass(window, g_ui->render_pass, graphics_current_present_frame_of_window(window));
	graphics_cmd_bind_pipeline(window, g_ui->pipeline);
	graphics_cmd_set_viewport(window, vec2_ZERO(), Vec2(window->width, window->height));
	graphics_cmd_set_scissor(window, vec2_ZERO(), Vec2(window->width, window->height));
	graphics_cmd_push_constant(window, GraphicsShaderStage_Vertex, &pc, 0, sizeof(pc));
	graphics_cmd_bind_vertex_buffer(window, g_ui->vertex_buffer.handle);
	graphics_cmd_bind_index_buffer(window, g_ui->index_buffer.handle);
	graphics_cmd_bind_descriptor_set(window, 0, g_ui->blank_descriptor_set);
	
	g_ui->updating = 1;
	
	g_ui->stats.drawcmds_visible = 0;
	g_ui->stats.indices_visible = 0;
	g_ui->stats.items_visible = 0;
	g_ui->stats.vertices_visible = 0;
	
	for(auto n = g_ui->base.link.next; n != &g_ui->base.link; n = n->next){
		auto item = ui_item_from_link(n);
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
				item_error(g_ui->item_stack[i+1], "Items are still left on the item stack. Did you forget to call ui_end_item? Did you mean to use uiItemM?");
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

	graphics_cmd_end_render_pass(window);
	g_ui->updating_window = 0;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_debug


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
		
		Font* default_font = assets_font_get_by_name(STR8("baked_gohufont_11_bdf"));
		if(!default_font){
			default_font = assets_font_create_from_memory_bdf(baked_font_gohufont_11_bdf.str, baked_font_gohufont_11_bdf.count, STR8("baked_gohufont_11_bdf"));
		}
		
		uiStyle def_style{0};
		def_style.sizing = size_auto;
		def_style.text_color = Color_White;
		def_style.text_wrap = text_wrap_none;
		def_style.font = default_font;
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
		{uiItem* window = ui_begin_item(0);
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
			
			{uiItem* panel = ui_begin_item(&panel_style); //selected information
				panel->id = STR8("ui_debug win panel0");
				panel->action = &ui_debug_panel_callback;
				panel->action_trigger = action_act_always;
				panel->style.width = 1;
				panel->style.margin_right = 1;
				
				{ui_dwi.internal_info = ui_begin_item(0); 
					ui_dwi.internal_info->style = def_style;
					ui_dwi.internal_info->id = STR8("ui_debug internal info");
					ui_dwi.internal_info->style.sizing = size_percent_x;
					ui_dwi.internal_info->style.width = 100;
					ui_dwi.internal_info->style.height = 100;
					ui_dwi.internal_info->style.background_color = color(14,14,14);
					ui_dwi.internal_info->style.content_align = {0.5, 0.5};
					
					
					ui_end_item();
				}
				
				ui_dwi.panel0 = panel;
			}ui_end_item();
			
			{uiItem* panel = ui_begin_item(&panel_style);
				panel->id = STR8("ui_debug win panel1");
				panel->action = &ui_debug_panel_callback;
				panel->action_trigger = action_act_always;
				panel->style.width = 0.5;
				
				ui_dwi.panel1text = ui_make_text(str8l("stats"), 0);
				
				ui_end_item();
				ui_dwi.panel1 = panel;
			}
		}ui_end_item();
		ui_dwi.init = 1;
	}
	
	ui_begin_immediate_branch(ui_dwi.panel0);{//make internal info header
		//header stores an action that toggles its boolean in the data struct
		{uiItem* header = ui_begin_item(&ui_dwi.def_style);
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
		}ui_end_item();
		
		if(ui_dwi.internal_info_header){
			{uiItem* cont = ui_begin_item(&ui_dwi.def_style);
				cont->id = STR8("header cont");
				
				cont->style.sizing = size_percent_x;
				cont->style.width = 100;
				cont->style.height = 100;
				
				if(ui_dwi.selected_item){
					
				}else if(ui_dwi.selecting_item){
					
					ui_dwi.internal_info->style.content_align = {0.5,0.5};
					ui_make_text(str8l("selecting item..."), 0);
					
					if(g_ui->hovered && input_lmouse_pressed()){
						ui_dwi.selected_item = g_ui->hovered;
					}
					
				}else{
					{uiItem* item = ui_begin_item(0);
						item->id = STR8("button");
						item->style.background_color = Color_VeryDarkCyan;
						item->style.sizing = size_auto;
						item->style.padding = {1,1,1,1};
						item->style.margin = {1,1,1,1};
						item->style.text_color = Color_White;
						item->action = [](uiItem* item) {
							ui_dwi.selecting_item = 1;
						};
						item->action_trigger = action_act_mouse_pressed;
						ui_make_text(str8l("O"), 0);
					}ui_end_item();
					ui_dwi.internal_info->style.content_align = {0.5,0.5};
					ui_make_text(str8l("no item selected."), 0);
					if(g_ui->active){
						uiItem* sel = g_ui->active;
						uiText* text = 0;
						if(sel->memsize == sizeof(uiText)) text = ui_get_text(sel);
						ui_make_text(to_dstr8v(deshi_temp_allocator,
							sel->id,"\n",
							sel->node.child_count
						).fin, 0);
						if(text){
							ui_make_text(text->text.buffer.fin, 0);
						}
					}
				}
			}ui_end_item();
		}
	}ui_end_immediate_branch();
	
	u64 memsum = 0;
	for(auto n = g_ui->base.link.next; n != &g_ui->base.link; n = n->next){
		memsum += ui_item_from_link(n)->memsize;
	}
	forI(g_ui->immediate_items.count){
		memsum += g_ui->immediate_items[i]->memsize;
	}
	memsum += g_ui->stats.vertices_reserved * sizeof(uiVertex);
	memsum += g_ui->stats.indices_reserved * sizeof(u32);
	memsum += g_ui->stats.drawcmds_reserved * sizeof(uiDrawCmd);
	
	ui_dwi.panel1text->style.text_wrap = text_wrap_none;
	dstr8 t = to_dstr8v(deshi_temp_allocator,
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
	);
	defer{dstr8_deinit(&t);};
	text_clear_and_replace(&ui_get_text(ui_dwi.panel1text)->text, t.fin);
	ui_dwi.panel1text->dirty = 1;
	
	if(g_ui->hovered){
	//	render_start_cmd2(7, 0, vec2::ZERO, Vec2(DeshWindow->width,DeshWindow->height));
	//	vec2 ipos = g_ui->hovered->pos_screen;
	//	vec2 mpos = ipos + g_ui->hovered->style.margintl;
	//	vec2 bpos = mpos + (g_ui->hovered->style.border_style ? g_ui->hovered->style.border_width : 0) * vec2::ONE;
	//	vec2 ppos = bpos + g_ui->hovered->style.paddingtl;
	//	
	//	render_quad2(ipos, g_ui->hovered->size, Color_Red);
	//	render_quad2(mpos, ui_margined_area(g_ui->hovered), Color_Magenta);
	//	render_quad2(bpos, ui_bordered_area(g_ui->hovered), Color_Blue);
	//	render_quad2(ppos, ui_padded_area(g_ui->hovered), Color_Green);
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_demo


local uiItem* deshi__ui_demo_window = 0;
void ui_demo(){
	if(deshi__ui_demo_window){
		ui_remove_item(deshi__ui_demo_window);
		deshi__ui_demo_window = 0;
		return;
	}
	
	Font* default_font = assets_font_get_by_name(STR8("baked_gohufont_11_bdf"));
	if(!default_font){
		default_font = assets_font_create_from_memory_bdf(baked_font_gohufont_11_bdf.str, baked_font_gohufont_11_bdf.count, STR8("baked_gohufont_11_bdf"));
	}
	
	deshi__ui_demo_window = ui_begin_item(0);{
		uiItem* window = deshi__ui_demo_window;
		window->id = STR8("demo.window");
		window->style.positioning = pos_draggable_absolute;
		//window->style.sizing = size_resizeable;  //TODO requires item resizing
		window->style.size = {598/*pixels*/,298/*pixels*/}; //NOTE because border_width = 1
		window->style.min_size = {25/*pixels*/,25/*pixels*/};
		window->style.border_style = border_solid;
		window->style.border_color = Color_DarkCyan;
		window->style.border_width = 1/*pixels*/;
		window->style.font = default_font;
		window->style.font_height = 14/*pixels*/;
		window->style.text_color = Color_White;
		window->style.focus = true;
		window->style.display = display_vertical | display_flex;
		
		uiItem* window_decoration = ui_begin_item(0);{
			window_decoration->id = STR8("demo.window_decoration");
			window_decoration->style.sizing = size_percent_x;
			window_decoration->style.size = {100/*percent*/,18/*pixels*/};
			window_decoration->style.min_size = {50/*pixels*/,18/*pixels*/};
			window_decoration->style.max_size = {0,18/*pixels*/};
			window_decoration->style.padding_left = 5/*pixels*/;
			window_decoration->style.background_color = Color_DarkCyan;
			window_decoration->style.content_align = {0,.5f};
			
			uiItem* title = ui_make_text(str8l("Demo Window"), 0);{
				title->id = STR8("demo.window_decoration.title");
				title->style.text_color = Color_LightGrey;
			}
			
			//TODO maximize button
			//TODO close button
		}ui_end_item();//window_decoration
		
		uiItem* window_content = ui_begin_item(0);{
			window_content->id = STR8("demo.window_content");
			window_content->style.sizing = size_flex;
			window_content->style.size = {1/*ratio of 1*/,1/*ratio of 1*/};
			window_content->style.background_color = Color_VeryDarkCyan;
			window_content->style.display = display_horizontal | display_flex;
			
			persist uiStyle preview_style{};
			
			uiItem* item_panel = ui_begin_item(0);{
				item_panel->id = STR8("demo.window_content.item_panel");
				item_panel->style.sizing = size_flex/*| size_resizeable_x*/; //TODO requires item resizing
				item_panel->style.size = {1/*ratio of 3*/,1/*ratio of 1*/};
				item_panel->style.border_style = border_solid; //TODO remove this once separators are added
				item_panel->style.border_color = Color_DarkCyan;
				item_panel->style.border_width = 1/*pixels*/;
				item_panel->style.display = display_vertical | display_flex;
				
				uiItem* item_tree = ui_begin_item(0);{
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
				}ui_end_item();//item_tree
				
				//uiItem* item_panel_separator = uiSeparator(1/*pixels*/);  //TODO requires separator widget
				//item_panel_separator->id = STR8("demo.window_content.item_panel.separator");
				//item_panel_separator->style.background_color = Color_DarkCyan;
				//item_panel_separator->style.positioning = pos_draggable_relative;
				
				uiItem* item_settings = ui_begin_item(0);{
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
				}ui_end_item();//item_settings
			}ui_end_item();//item_panel
			
			//uiItem* window_content_separator = uiSeparator(1/*pixels*/);  //TODO requires separator widget
			//window_content_separator->id = STR8("demo.window_content.item_panel.separator");
			//window_content_separator->style.background_color = Color_DarkCyan;
			//window_content_separator->style.positioning = pos_draggable_relative;
			
			uiItem* item_preview = ui_begin_item(0);{
				item_preview->id = STR8("demo.window_content.item_preview");
				item_preview->style.sizing = size_flex/*| size_resizeable_x*/; //TODO requires item resizing //NOTE(delle) resizeable here in addition to item_panel for extended resize hover region
				item_preview->style.size = {2/*ratio of 3*/,1/*ratio of 1*/};
				item_preview->style.border_style = border_solid;
				item_preview->style.border_color = Color_DarkCyan;
				item_preview->style.border_width = 1/*pixels*/; //TODO remove this once separators are added
				
				//TODO requires dynamically-added items
			}ui_end_item();//item_preview
		}ui_end_item();//window_content
	}ui_end_item();//window
	/*
	{//test sizer
		uiItem* container = ui_begin_item(0);{
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
			}ui_end_item();
			uiItem* c1 = uiItemBS(&flex_style);{
				c1->style.background_color = Color_Green;
				c1->style.width = 2;
				c1->id = STR8("c1");
			}ui_end_item();
			uiItem* c2 = uiItemBS(&flex_style);{
				c2->style.background_color = Color_Blue;
				c2->style.width = 1;
				c2->id = STR8("c2");
			}ui_end_item();
		}ui_end_item();
	}
	*/
	/*
	{//test scaling
		uiItem* test = ui_begin_item(0);{
			test->style.size = {200, 200};
			test->style.positioning = pos_draggable_absolute;
			test->style.background_color = Color_White;
			test->style.paddingtl = {10,10};
			test->style.paddingbr = {10,10};
			
			uiItem* test2 = ui_begin_item(0);{
				test2->style.positioning = pos_absolute;
				test2->style.pos = {50, 50};
				test2->style.size = {48, 48};
				test2->style.border_style = border_solid;
				test2->style.border_color = Color_Black;
				test2->style.border_width = 1;
				test2->style.scale = {1.5,1.5};
				test2->style.background_color = Color_Green;
			}ui_end_item();
			
			uiItem* test3 = ui_begin_item(0);{
				test3->style.positioning = pos_absolute;
				test3->style.pos = {50, 50};
				test3->style.size = {50, 50};
				test3->style.background_color = Color_Blue;
			}ui_end_item();
		}ui_end_item();
	}
	*/
}


dstr8
indent(str8 s) {
	dstr8 out; dstr8_init(&out, {}, deshi_temp_allocator);
	auto do_indent = [&]() { dstr8_append(&out, "  "); };
	do_indent();
	while(s) {
		dstr8_append(&out, *s.str);
		if(*s.str == '\n') {
			do_indent();
		} 
		str8_advance(&s);
	}
	return out;
}

dstr8
ui_print_tree_recur(uiItem* item, void (*info)(dstr8*, uiItem*)) {
	dstr8 out; dstr8_init(&out, {}, deshi_temp_allocator);
	if(item->node.child_count) {
		dstr8_append(&out, "(");
	}

	if(item->id.count) {
		dstr8_append(&out, item->id);
	} else {
		dstr8_append(&out, (void*)item);
	}

	dstr8_append(&out, " ");
	if(info) {
		info(&out, item);
	}

	dstr8_append(&out, "\n  ");

	for(uiItem* child = (uiItem*)item->node.first_child; child; child = (uiItem*)child->node.next) {
		dstr8_append(&out, indent(ui_print_tree_recur(child, info).fin));
	}

	return out;
}

void 
ui_print_tree(void (*info)(dstr8*, uiItem*)) {
	Log("", ui_print_tree_recur(&g_ui->base, info));	
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @make


vec2i
ui_put_line(uiVertex* putverts, u32* putindices, vec2i offsets, vec2 start, vec2 end, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	uiVertex* vp = putverts + offsets.x;
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
	
	return ui_put_line_counts();
}

//3 verts, 3 indices
vec2i 
ui_put_filledtriangle(uiVertex* putverts, u32* putindices, vec2i offsets, vec2 p1, vec2 p2, vec2 p3, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	uiVertex* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	ip[0] = offsets.x; ip[1] = offsets.x + 1; ip[2] = offsets.x + 2;
	vp[0].pos = p1; vp[0].uv = { 0,0 }; vp[0].color = col;
	vp[1].pos = p2; vp[1].uv = { 0,0 }; vp[1].color = col;
	vp[2].pos = p3; vp[2].uv = { 0,0 }; vp[2].color = col;
	
	return ui_put_filledtriangle_counts();
}

vec2i
ui_put_triangle(uiVertex* putverts, u32* putindices, vec2i offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	uiVertex* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	vec2i sum;
	sum += ui_put_line(vp, ip, {    0,    0}, p0, p1, 1, color);
	sum += ui_put_line(vp, ip, {sum.x,sum.y}, p1, p2, 1, color);
	sum += ui_put_line(vp, ip, {sum.x,sum.y}, p2, p0, 1, color);
	
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
ui_put_filledrect(uiVertex* putverts, u32* putindices, vec2i offsets, vec2 pos, vec2 size, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	uiVertex* vp = putverts + offsets.x;
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
	
	return ui_put_filledrect_counts();
}

vec2i
ui_put_rect(uiVertex* putverts, u32* putindices, vec2i offsets, vec2 pos, vec2 size, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	uiVertex* vp = putverts + offsets.x;
	u32*     ip = putindices + offsets.y;
	
	// vec2 tl = pos;
	// vec2 br = pos + size;
	// vec2 bl = pos + Vec2(0, size.y);
	// vec2 tr = pos + Vec2(size.x, 0);
	
	// vec2i sum = {0};
	// sum += ui_put_line(vp, ip, sum, tl,tr,thickness,color);
	// sum += ui_put_line(vp, ip, sum, tr,br,thickness,color);
	// sum += ui_put_line(vp, ip, sum, br,bl,thickness,color);
	// sum += ui_put_line(vp, ip, sum, bl,tl,thickness,color);
	
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
	
	return ui_put_rect_counts();
}

vec2i
ui_put_circle(uiVertex* putverts, u32* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	uiVertex* vp = putverts + offsets.x;
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
	
	return ui_put_circle_counts(subdivisions_int);
}

vec2i 
ui_put_filledcircle(uiVertex* putverts, u32* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color){DPZoneScoped;
	Assert(putverts && putindices);
	if(color.a == 0) return{0,0};
	
	u32     col = color.rgba;
	uiVertex* vp = putverts + offsets.x;
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
	
	return ui_put_filledcircle_counts(subdivisions_int);
}

vec2i
ui_put_text(uiVertex* putverts, u32* putindices, vec2i offsets, str8 text, Font* font, vec2 pos, color color, vec2 scale){DPZoneScoped;
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
				u32      col = color.rgba;
				uiVertex* vp = putverts + offsets.x + 4 * i;
				u32*      ip = putindices + offsets.y + 6 * i;
				
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
				sum+=ui_put_text_counts(1);
			}
		}break;
		//// TTF font rendering ////
		case FontType_TTF:{
			u32 codepoint;
			str8 remaining = text;
			u32 i = 0;
			while(remaining && (codepoint = str8_advance(&remaining).codepoint)){
				u32      col = color.rgba;
				uiVertex* vp = putverts + offsets.x + 4 * i;
				u32*      ip = putindices + offsets.y + 6 * i;
				FontAlignedQuad q = font_aligned_quad(font, codepoint, &pos, scale);
				
				ip[0] = offsets.x+4*i; ip[1] = offsets.x+4*i + 1; ip[2] = offsets.x+4*i + 2;
				ip[3] = offsets.x+4*i; ip[4] = offsets.x+4*i + 2; ip[5] = offsets.x+4*i + 3;
				vp[0].pos = { q.x0,q.y0 }; vp[0].uv = { q.u0,q.v0 }; vp[0].color = col; //top left
				vp[1].pos = { q.x1,q.y0 }; vp[1].uv = { q.u1,q.v0 }; vp[1].color = col; //top right
				vp[2].pos = { q.x1,q.y1 }; vp[2].uv = { q.u1,q.v1 }; vp[2].color = col; //bot right
				vp[3].pos = { q.x0,q.y1 }; vp[3].uv = { q.u0,q.v1 }; vp[3].color = col; //bot left
				i += 1;
				sum+=ui_put_text_counts(1);
			}
		}break;
		default: Assert(!"unhandled font type"); break;
	}
	return sum;
}

vec2i 
ui_put_texture(uiVertex* putverts, u32* putindices, vec2i offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx = 0, b32 flipy = 0){DPZoneScoped;
	Assert(putverts && putindices);
	if(!alpha) return{0,0};
	
	u32     col = PackColorU32(255,255,255,255.f * alpha);
	uiVertex* vp = putverts + offsets.x;
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
	return ui_put_texture_counts();
}


