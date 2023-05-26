/* deshi UI2 Widgets Module
Index:
@ui2_widgets_text
  uiText
  uiTextM(str8 text) -> uiItem*
  uiTextMS(str8 text, uiStyle* style) -> uiItem*
  uiTextML(string_literal) -> uiItem*
  uiTextMSL(string_literal, uiStyle* style) -> uiItem*
@ui2_widgets_inputtext
  uiInputText
  uiInputTextM() -> uiItem*
  uiInputTextMS(uiStyle* style) -> uiItem*
  uiInputTextMP(str8 preview) -> uiItem*
  uiInputTextMSP(uiStyle* style, str8 preview) -> uiItem*
@ui2_widgets_slider
  uiSlider
  uiSliderf32(f32 min, f32 max, f32* var) -> uiItem*
  uiSliderf32S(f32 min, f32 max, f32* var, uiStyle* style) -> uiItem*
  uiSlideru32(u32 min, u32 max, u32* var) -> uiItem*
  uiSlideru32S(u32 min, u32 max, u32* var, uiStyle* style) -> uiItem*
  uiSliders32(s32 min, s32 max, s32* var) -> uiItem*
  uiSliders32S(s32 min, s32 max, s32* var, uiStyle* style) -> uiItem*
@ui2_widgets_checkbox
  uiCheckbox
  uiCheckboxM(b32* var) -> uiItem*
  uiCheckboxMS(b32* var, uiStyle* style) -> uiItem*
@ui2_widgets_shared_impl
  item_error(uiItem* item, ...) -> void
  gen_error(str8 file, upt line, ...) -> void
  find_text_breaks(arrayT<pair<s64,vec2>>* breaks, uiItem* item, Text text, f32 wrapspace, b32 do_wrapping, b32 reset_size) -> void
  find_hovered_offset(carray<pair<s64,vec2>> breaks, uiItem* item, Text text) -> s64
  render_ui_text(vec2i counts, uiDrawCmd* dc, Vertex2* vp, u32* ip, uiItem* item, Text text, carray<pair<s64,vec2>> breaks) -> vec2i
@ui2_widgets_text_impl
  ui_gen_text(uiItem* item) -> void
  ui_eval_text(uiItem* item) -> void
  ui_make_text(str8 text, uiStyle* style, str8 file, upt line) -> uiItem*
@ui2_widgets_inputtext_impl
  ui_gen_input_text(uiItem* item) -> void
  ui_eval_input_text(uiItem* item) -> void
  ui_update_input_text(uiItem* item) -> void
  ui_make_input_text(str8 preview, uiStyle* style, str8 file, upt line) -> uiItem*
@ui2_widgets_slider_impl
  ui_gen_slider(uiItem* item) -> void
  ui_slider_callback(uiItem* item) -> void
  ui_make_slider(uiStyle* style, str8 file, upt line) -> uiItem*
  ui_make_slider_f32(f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line) -> uiItem*
  ui_make_slider_u32(u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line) -> uiItem*
  ui_make_slider_s32(s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line) -> uiItem*
@ui2_widgets_checkbox_impl
  ui_gen_checkbox(uiItem* item) -> void
  ui_checkbox_callback(uiItem* item) -> void
  ui_make_checkbox(b32* var, uiStyle* style, str8 file, upt line) -> uiItem*
*/
#ifndef DESHI_UI2_WIDGETS_H
#define DESHI_UI2_WIDGETS_H
#include "ui.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_text
//uiText is a terminal node meaning it cannot have any children.


struct uiText{
	uiItem item;
	Text text;
	b32 selecting;
	//TODO(sushi) get rid of array here
	arrayT<pair<s64,vec2>> breaks;
};

#define uiGetText(x) CastFromMember(uiText, item, x)

UI_FUNC_API(uiItem*, ui_make_text, str8 text, uiStyle* style, str8 file, upt line);
//NOTE(sushi) does not automatically make a str8, use uiTextML for that.
#define uiTextM(text)          UI_DEF(make_text((text),           0, STR8(__FILE__),__LINE__))
#define uiTextMS(style, text)  UI_DEF(make_text((text),     (style), STR8(__FILE__),__LINE__))
//NOTE(sushi) this automatically applies STR8() to text, so you can only use literals with this macro
#define uiTextML(text)         UI_DEF(make_text(STR8(text),       0, STR8(__FILE__),__LINE__))
#define uiTextMSL(style, text) UI_DEF(make_text(STR8(text), (style), STR8(__FILE__), __LINE__))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_inputtext


enum{
	input_cursor_line = 0,
	input_cursor_underline,
	input_cursor_rect,
	input_cursor_filled_rect,
};

struct uiInputText{
	uiItem item;
	str8   preview;
	Text   text;
	b32    selecting;
	//TODO(sushi) get rid of array here
	arrayT<pair<s64,vec2>> breaks;
	
	Stopwatch repeat_hold;
	Stopwatch repeat_throttle;
	
	struct{
		//b32 allow_multiline; TODO
		struct{
			color preview;
			color cursor;
		}colors;
		Type cursor;
		f32 hold_time;     //time until repeating starts in ms
		f32 throttle_time; //time between repeats in ms
	}style;
};

#define uiGetInputText(x) CastFromMember(uiInputText, item, x)

UI_FUNC_API(uiItem*, ui_make_input_text, str8 preview, uiStyle* style, str8 file, upt line);
#define uiInputTextM()                UI_DEF(make_input_text(    {0},       0, STR8(__FILE__),__LINE__))
#define uiInputTextMS(style)          UI_DEF(make_input_text(    {0}, (style), STR8(__FILE__),__LINE__))
#define uiInputTextMP(preview)        UI_DEF(make_input_text(preview,       0, STR8(__FILE__),__LINE__))
#define uiInputTextMSP(style,preview) UI_DEF(make_input_text(preview, (style), STR8(__FILE__),__LINE__))


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_slider


enum{
	slider_dragger_rect = 0,
	slider_dragger_round,
};

struct uiSlider{
	uiItem item;
	
	union{
		u32 maxu32;
		s32 maxs32;
		f32 maxf32;
	};
	union{
		u32 minu32;
		s32 mins32;
		f32 minf32;
	};
	union{
		u32* varu32;
		s32* vars32;
		f32* varf32;
	};
	Type type; //0:f32 1:u32 2:s32
	f32 mouse_offset;
	b32 active;
	f32 pos;
	f32 width;
	
	struct{
		struct{
			color rail;
			color rail_outline;
			color dragger;
		}colors;
		Type dragger_shape;
		f32 rail_thickness; //percentage of full thickness, 0-1
	}style;
};

#define uiGetSlider(x) CastFromMember(uiSlider, item, x)

inline u32 slider_style_hash(uiItem* item){
	uiSlider* data = uiGetSlider(item);
	
	u32 seed = UI_HASH_SEED;
	seed ^= data->style.colors.rail.rgba;       seed *= UI_HASH_PRIME;
	seed ^= data->style.colors.dragger.rgba;    seed *= UI_HASH_PRIME;
	seed ^= data->style.dragger_shape;          seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&data->style.rail_thickness; seed *= UI_HASH_PRIME;
	
	return seed;
} 

UI_FUNC_API(uiItem*, ui_make_slider_f32, f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line);
#define uiSliderf32(min,max,var)        UI_DEF(make_slider_f32(min,max,var,0,STR8(__FILE__),__LINE__));
#define uiSliderf32S(min,max,var,style) UI_DEF(make_slider_f32(min,max,var,(style),STR8(__FILE__),__LINE__));

UI_FUNC_API(uiItem*, ui_make_slider_u32, u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line);
#define uiSlideru32(min,max,var)        UI_DEF(make_slider_u32(min,max,var,0,STR8(__FILE__),__LINE__));
#define uiSlideru32S(min,max,var,style) UI_DEF(make_slider_u32(min,max,var,(style),STR8(__FILE__),__LINE__));

UI_FUNC_API(uiItem*, ui_make_slider_s32, s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line);
#define uiSliders32(min,max,var)        UI_DEF(make_slider_s32(min,max,var,0,STR8(__FILE__),__LINE__));
#define uiSliders32S(min,max,var,style) UI_DEF(make_slider_s32(min,max,var,(style),STR8(__FILE__),__LINE__));


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_checkbox
//uiCheckbox is a terminal node, so it cannot hold any children


enum{
	checkbox_fill_box = 0,
	checkbox_fill_checkmark,
};


struct uiCheckbox{
	uiItem item;
	
	b32* var;
	
	struct{
		struct{
			color filling;
		}colors;
		Type fill_type;
		vec2 fill_padding;
	}style;
};


#define uiGetCheckbox(x) CastFromMember(uiCheckbox, item, x)

inline u32 checkbox_style_hash(uiItem* item){
	uiCheckbox* data = uiGetCheckbox(item);
	
	u32 seed = UI_HASH_SEED;
	seed ^= data->style.colors.filling.rgba; seed *= UI_HASH_PRIME;
	seed ^= data->style.fill_type;           seed *= UI_HASH_PRIME;
	return seed;
}

UI_FUNC_API(uiItem*, ui_make_checkbox, b32* var, uiStyle* style, str8 file, upt line);
#define uiCheckboxM(var)         UI_DEF(make_checkbox((var), 0, STR8(__FILE__),__LINE__))
#define uiCheckboxMS(var, style) UI_DEF(make_checkbox((var), 0, STR8(__FILE__),__LINE__))


#endif //DESHI_UI2_WIDGETS_H
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if defined(DESHI_IMPLEMENTATION) && !defined(DESHI_UI2_WIDGETS_IMPL)
#define DESHI_UI2_WIDGETS_IMPL
#include "render.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_shared_impl


#define item_error(item, ...)\
LogE("ui",CyanFormatComma((item)->file_created), ":", (item)->line_created, ":", RedFormatComma("error"), ": ", __VA_ARGS__)

#define gen_error(file,line,...)\
LogE("ui",CyanFormatComma(file),":",line,":",RedFormatComma("error"),":",__VA_ARGS__)


//common functionality between uiText and uiInputText
//NOTE(sushi) I would like to avoid this and instead inherit uiText into uiInputText, so uiInputText can just act as an extension 
//            of uiText. The way the API is setup right now doesn't really allow for this, at least not in any nice way.
//            It may be better to keep them separate however.
void
find_text_breaks(arrayT<pair<s64,vec2>>* breaks, uiItem* item, Text text, f32 wrapspace, b32 do_wrapping, b32 reset_size = 0){DPZoneScoped;
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

s64
find_hovered_offset(carray<pair<s64,vec2>> breaks, uiItem* item, Text text){DPZoneScoped;
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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_text_impl


void
ui_gen_text(uiItem* item){DPZoneScoped;
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
		//s32 break_start = -1;
		//forI(data->breaks.count-1){
		//	if(break_start != -1 && data->breaks[i].first > data->text.cursor.pos){
		//		break_start = i-1;
		//	}else if(data->breaks[i].first > data->selection){
		//	}
		//}
	}
	
	vec2i nucounts = render_make_text_counts(str8_length(data->text.buffer.fin));
	if(nucounts.x != dc->counts_reserved.x || nucounts.y != dc->counts_reserved.y){
	    item->drawcmds = ui_make_drawcmd(1);
		ui_drawcmd_remove(dc);
		dc = item->drawcmds;
		ui_drawcmd_alloc(dc, nucounts);
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
			if(   (idx + j > Min(data->text.cursor.pos, data->text.cursor.pos+data->text.cursor.count))
			   && (idx + j < Max(data->text.cursor.pos, data->text.cursor.pos + data->text.cursor.count))){
				counts += render_make_text(vp,ip,counts, {data->text.buffer.str + idx + j, 1}, item->style.font, cursor, Color_Red,
										   vec2::ONE * item->style.font_height / item->style.font->max_height);
			}else{
				counts += render_make_text(vp,ip,counts, {data->text.buffer.str + idx + j, 1}, item->style.font, cursor, item->style.text_color,  
										   vec2::ONE * item->style.font_height / item->style.font->max_height);
			}
			cursor.x += csize.x;
		}
	}
	
	dc->counts_used = counts;
}


void
ui_eval_text(uiItem* item){DPZoneScoped;
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
void
ui_update_text(uiItem* item){DPZoneScoped;
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

uiItem*
ui_make_text(str8 text, uiStyle* style, str8 file, upt line){DPZoneScoped;
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
	setup.drawcmd_count = 1;
	
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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_inputtext_impl


void
ui_gen_input_text(uiItem* item){DPZoneScoped;
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
	    item->drawcmds = ui_make_drawcmd(1);
		ui_drawcmd_remove(dc);
		dc = item->drawcmds;
		ui_drawcmd_alloc(dc, nucounts);
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

void
ui_eval_input_text(uiItem* item){DPZoneScoped;
	if(!item->style.font){
		item_error(item, "uiInputText's evaluation function was called, but no font was specified for the item. You must either specify a font on the uiText's item handle, or on one of its ancestors.");
		return;
	}
	uiItem* parent = uiItemFromNode(item->node.parent);
	uiInputText* data = uiGetInputText(item);
	find_text_breaks(&data->breaks, item, data->text, PaddedWidth(parent), (item->style.text_wrap != text_wrap_none) && (!HasFlag(parent->style.sizing, size_auto)), 0);
}

void
ui_update_input_text(uiItem* item){DPZoneScoped;
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
	if(CanDoInput(g_ui->keys.inputtext.cursor.left))           text_move_cursor_left(t),             item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.cursor.left_word))      text_move_cursor_left_word(t),        item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.cursor.left_wordpart))  text_move_cursor_left_wordpart(t),    item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.cursor.right))          text_move_cursor_right(t),            item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.cursor.right_word))     text_move_cursor_right_word(t),       item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.cursor.right_wordpart)) text_move_cursor_right_wordpart(t),   item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.select.left))           text_move_cursor_left(t,1),           item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.select.left_word))      text_move_cursor_left_word(t,1),      item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.select.left_wordpart))  text_move_cursor_left_wordpart(t,1),  item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.select.right))          text_move_cursor_right(t,1),          item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.select.right_word))     text_move_cursor_right_word(t,1),     item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.select.right_wordpart)) text_move_cursor_right_wordpart(t,1), item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.del   .right))          text_delete_right(t),                 item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.del   .right_word))     text_delete_right_word(t),            item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.del   .right_wordpart)) text_delete_right_wordpart(t),        item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.del   .left))           text_delete_left(t),                  item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.del   .left_word))      text_delete_left_word(t),             item->dirty = 1;
	if(CanDoInput(g_ui->keys.inputtext.del   .left_wordpart))  text_delete_left_wordpart(t),         item->dirty = 1;
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

uiItem*
ui_make_input_text(str8 preview, uiStyle* style, str8 file, upt line){DPZoneScoped;
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


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_slider_impl


void
ui_gen_slider(uiItem* item){DPZoneScoped;
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

void
ui_slider_callback(uiItem* item){DPZoneScoped;
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

uiItem*
ui_make_slider(uiStyle* style, str8 file, upt line){DPZoneScoped;
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

uiItem*
ui_make_slider_f32(f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	item->action = &ui_slider_callback;
	uiGetSlider(item)->minf32 = min;
	uiGetSlider(item)->maxf32 = max;
	uiGetSlider(item)->varf32 = var;
	uiGetSlider(item)->type = 0;
	
	return item;
}

uiItem*
ui_make_slider_u32(u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	item->action = &ui_slider_callback;
	uiGetSlider(item)->minu32 = min;
	uiGetSlider(item)->maxu32 = max;
	uiGetSlider(item)->varu32 = var;
	uiGetSlider(item)->type = 1;
	
	return item;
}

uiItem*
ui_make_slider_s32(s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	item->action = &ui_slider_callback;
	uiGetSlider(item)->mins32 = max;
	uiGetSlider(item)->maxs32 = max;
	uiGetSlider(item)->vars32 = var;
	uiGetSlider(item)->type = 2;
	
	return item;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_checkbox_impl


void
ui_gen_checkbox(uiItem* item){
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

void
ui_checkbox_callback(uiItem* item){
	FixMe;
	/*
	uiCheckbox* data = uiGetCheckbox(item);
	*data->var = !*data->var;
	item->dirty = 1;
	*/
}

uiItem*
ui_make_checkbox(b32* var, uiStyle* style, str8 file, upt line){
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


#endif //defined(DESHI_IMPLEMENTATION) && !defined(DESHI_UI2_WIDGETS_IMPL)