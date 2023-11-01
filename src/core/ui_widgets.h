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
#include "core/input.h"
#include "ui.h"
#include <cctype>


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
#define ui_get_text(x) ((uiText*)(x))

uiItem* deshi__ui_make_text(str8 text, uiStyle* style, str8 file, upt line);
#define ui_make_text(text, style) deshi__ui_make_text((text), (style), str8l(__FILE__), __LINE__)


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

#define ui_get_input_text(x) ((uiInputText*)(x))

uiItem* deshi__ui_make_input_text(str8 preview, uiStyle* style, str8 file, upt line);
#define ui_make_input_text(preview, style) deshi__ui_make_input_text((preview), (style), str8l(__FILE__), __LINE__)

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_slider


enum{
	slider_dragger_rect = 0,
	slider_dragger_round,
};

// TODO(sushi) currently if there are two sliders on screen that are attached to the same 
//             variable, the sliders will not move together. This can be fixed by calculating
//             the position of the dragger in ui_gen_slider instead of in its callback,
//             but would be less performant as we have to check what kind of slider we're
//             dealing with in the gen function. We could also have a separate gen for each
//             type but that's kinda bloated.
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
#define ui_get_slider(x) ((uiSlider*)(x))

inline u32 slider_style_hash(uiItem* item){
	uiSlider* data = ui_get_slider(item);
	
	u32 seed = UI_HASH_SEED;
	seed ^= data->style.colors.rail.rgba;       seed *= UI_HASH_PRIME;
	seed ^= data->style.colors.dragger.rgba;    seed *= UI_HASH_PRIME;
	seed ^= data->style.dragger_shape;          seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&data->style.rail_thickness; seed *= UI_HASH_PRIME;
	if(data->varf32) {
		seed ^= *data->varu32; seed *= UI_HASH_PRIME;
	}
	
	return seed;
} 

uiItem* deshi__ui_make_slider_f32(f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line);
uiItem* deshi__ui_make_slider_u32(u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line);
uiItem* deshi__ui_make_slider_s32(s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line);

#define ui_make_slider_f32(min, max, var, style) deshi__ui_make_slider_f32(min, max, (var), (style), str8l(__FILE__), __LINE__)
#define ui_make_slider_u32(min, max, var, style) deshi__ui_make_slider_u32(min, max, (var), (style), str8l(__FILE__), __LINE__)
#define ui_make_slider_s32(min, max, var, style) deshi__ui_make_slider_s32(min, max, (var), (style), str8l(__FILE__), __LINE__)


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


#define ui_get_checkbox(x) ((uiCheckbox*)(x))

inline u32 checkbox_style_hash(uiItem* item){
	uiCheckbox* data = ui_get_checkbox(item);
	
	u32 seed = UI_HASH_SEED;
	seed ^= data->style.colors.filling.rgba; seed *= UI_HASH_PRIME;
	seed ^= data->style.fill_type;           seed *= UI_HASH_PRIME;
	return seed;
}

uiItem* deshi__ui_make_checkbox(b32* var, uiStyle* style, str8 file, upt line);
#define ui_make_checkbox(var, style) deshi__ui_make_checkbox((var), (style), str8l(__FILE__), __LINE__)


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @tab
struct uiTabbed {
	uiItem item;

	struct { // style
		struct { // colors
			struct { // active, inactive
				color text;
				color background;
				color border;
			} active, inactive, hovered;
			struct {
				color border;
			} body;
		} colors;
		vec4 body_margins;
		// minimum width of tabs, once this is reached 
		// we rely on scrolling
		f32 min_tab_width;
		// draw a scrollbar when min_tab_width is reached 
		b32 draw_scrollbar;
		// tab size in the direction perpendicular to the direction
		// the tabs run, for example if the tabs are displayed horizontally
		// this will be their height
		// TODO(sushi) vertical tabs and bottom tabs 
		f32 tab_size;
		
		// values between 0-1 determining how to align the text in each tab
		vec2 tab_text_alignment;
	} style;

	u32 selected;
	f32 scroll;
	f32 max_scroll;
};

// NOTE(sushi) I am not implementing hashing for uiTab as I don't have an immediate use case for dynamically changing tab names

#define ui_get_tabbed(x) ((uiTabbed*)(x))

uiItem* deshi__ui_make_tabbed(uiStyle* style, str8 file, upt line);
#define ui_make_tabbed(style) deshi__ui_make_tabbed((style), str8l(__FILE__), __LINE__)

uiItem* deshi__ui_begin_tabbed(uiStyle* style, str8 file, upt line);
#define ui_begin_tabbed(style) deshi__ui_begin_tabbed((style), str8l(__FILE__), __LINE__)

void deshi__ui_end_tabbed(str8 file, upt line);
#define ui_end_tabbed() deshi__ui_end_tabbed(str8l(__FILE__), __LINE__)

inline u32
ui_tabs_style_hash(uiItem* item) {
	uiTabbed* tabs = ui_get_tabbed(item);
	u32 seed = UI_HASH_SEED;
#define hash(x) seed ^= *(u32*)&(tabs->style. x); seed *= UI_HASH_PRIME;
	hash(colors.body.border.rgba);
	hash(colors.active.border.rgba);
	hash(colors.active.text.rgba);
	hash(colors.active.background.rgba);
	hash(colors.inactive.border.rgba);
	hash(colors.inactive.text.rgba);
	hash(colors.inactive.background.rgba);
	hash(colors.hovered.border.rgba);
	hash(colors.hovered.text.rgba);
	hash(colors.hovered.background.rgba);
	hash(body_margins.x);
	hash(body_margins.y);
	hash(body_margins.z);
	hash(body_margins.w);
#undef hash
	return seed;
}

struct uiTab {
	uiItem item;
	str8 name;
};

#define ui_get_tab(x) ((uiTab*)(x))

uiItem* deshi__ui_make_tab(str8 title, uiStyle* style, str8 file, upt line);
#define ui_make_tab(title, style) deshi__ui_make_tab((title), (style), str8l(__FILE__), __LINE__)

uiItem* deshi__ui_begin_tab(str8 title, uiStyle* style, str8 file, upt line);
#define ui_begin_tab(title, style) deshi__ui_begin_tab((title), (style), str8l(__FILE__), __LINE__)

void deshi__ui_end_tab(str8 file, upt line);
#define ui_end_tab() deshi__ui_end_tab(str8l(__FILE__), __LINE__)


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @tree

struct uiTree {
	uiItem item;

	struct {
		f32 indent;
		b32 hlines;
		b32 vlines;
		b32 open_circle;
	} style;
};

uiItem* deshi__ui_make_tree(uiStyle* style, str8 file, upt line);
#define ui_make_tree(style) deshi__ui_make_tree((style) str8l(__FILE__), __LINE__)

uiItem* deshi__ui_begin_tree(uiStyle* style, str8 file, upt line);
#define ui_begin_tree(style) deshi__ui_begin_tree((style), str8l(__FILE__), __LINE__)

void deshi__ui_end_tree(str8 file, upt line);
#define ui_end_tree() deshi__ui_end_tree(str8l(__FILE__), __LINE__)

struct uiTreeNode {
	uiItem item;

	b32 open;
	str8 title;
};

uiItem* deshi__ui_make_tree_node(str8 title, uiStyle* style, str8 file, upt line);
#define ui_make_tree_node(title,style) deshi__ui_make_tree_node((title), (style) str8l(__FILE__), __LINE__)

uiItem* deshi__ui_begin_tree_node(str8 title, uiStyle* style, str8 file, upt line);
#define ui_begin_tree_node(title, style) deshi__ui_begin_tree_node((title), (style), str8l(__FILE__), __LINE__)

void deshi__ui_end_tree_node(str8 file, upt line);
#define ui_end_tree_node() deshi__ui_end_tree_node(str8l(__FILE__), __LINE__)

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

void
ui_draw_text_into_region(str8 text, vec2 pos, vec2 size, uiDrawCmd* dc) {
	
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_text_impl


void
ui_gen_text(uiItem* item){DPZoneScoped;
	vec2i counts = {0};
	uiDrawCmd* dc = item->drawcmds;
	Vertex2*   vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	u32*       ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	uiText* data = (uiText*)item;
	
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
	
	// check how much draw data we need for the text and reallocate the drawcmd if needed
	vec2i nucounts = render_make_text_counts(str8_length(data->text.buffer.fin));
	auto p = ui_drawcmd_realloc(dc, nucounts);
	vp = p.vertexes;
	ip = p.indexes;
	
	f32 space_width = font_visual_size(item->style.font, STR8(" ")).x * item->style.font_height / item->style.font->max_height;
	vec2 cursor = item->pos_screen;
	forI(data->breaks.count-1){
		auto [idx, pos] = data->breaks[i];
		cursor = pos+item->pos_screen;
		forX(j,data->breaks[i+1].first-idx){
			vec2 csize = font_visual_size(item->style.font, {data->text.buffer.str+idx+j,1}) * item->style.font_height / item->style.font->max_height;
			if(   (idx + j > Min(data->text.cursor.pos, data->text.cursor.pos+data->text.cursor.count)) // make selected text red 
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
		item_error(item, "uiText's evaluation function was called, but no font was specified for the item. You must either specify a font on the uiText's item handle or on one of its ancestors.");
		return;
	}
	uiItem* parent = uiItemFromNode(item->node.parent);
	uiText* data = (uiText*)item;
	find_text_breaks(&data->breaks, item, data->text, ui_padded_width(parent), (item->style.text_wrap != text_wrap_none) && (!HasFlag(parent->style.sizing, size_auto)), 1);
}

//performs checks on the text element for things like mouse selection
//and copy when a selection is active
void
ui_update_text(uiItem* item){DPZoneScoped;
	uiText* data = (uiText*)item;
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
ui_copy_text(uiItem* old_item) {DPZoneScoped;
	auto old_text = ui_get_text(old_item);
	auto new_item = (uiItem*)memalloc(sizeof(uiText));
	auto new_text = ui_get_text(new_item);
	ui_item_copy_base(new_item, old_item);
	new_text->text = text_init(old_text->text.buffer.fin, old_text->text.buffer.allocator);
	new_text->text.cursor = old_text->text.cursor;
	return new_item;
}

uiItem*
deshi__ui_make_text(str8 text, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItemSetup setup = {0};
	setup.size = sizeof(uiText);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.update = ui_update_text;
	setup.generate = ui_gen_text;
	setup.evaluate = ui_eval_text;
	setup.copy = ui_copy_text;
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
	uiInputText* data = (uiInputText*)item;
	
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
	uiInputText* data = (uiInputText*)item;
	find_text_breaks(&data->breaks, item, data->text, ui_padded_width(parent), (item->style.text_wrap != text_wrap_none) && (!HasFlag(parent->style.sizing, size_auto)), 0);
}

void
ui_update_input_text(uiItem* item){DPZoneScoped;
	if(g_ui->active != item) return;
	uiInputText* data = ui_get_input_text(item);
	Text* t = &data->text;
	
	b32 repeat = 0;
	if(any_key_pressed() || any_key_released()){
		reset_stopwatch(&data->repeat_hold);
	}
	else if((peek_stopwatch(data->repeat_hold) > data->style.hold_time) && (peek_stopwatch(data->repeat_throttle) > data->style.throttle_time)){
		reset_stopwatch(&data->repeat_throttle);
		repeat = 1;
	}
	
	// allows input on first press, then allows repeats when the repeat bool is set on input
	// do action depending on bind pressed
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
	// NOTE(sushi) manually filter out control characters until I setup linux's input to do so there
	if(DeshInput->charCount && !iscntrl(DeshInput->charIn[0])){
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
ui_copy_input_text(uiItem* old_item) {DPZoneScoped;
	auto old_text = ui_get_input_text(old_item);
	auto new_item = (uiItem*)memalloc(sizeof(uiText));
	auto new_text = ui_get_input_text(new_item);
	ui_item_copy_base(new_item, old_item);

	new_text->style = old_text->style;
	new_text->text = text_init(new_text->text.buffer.fin, new_text->text.buffer.allocator);
	new_text->text.cursor = old_text->text.cursor;
	new_text->preview = old_text->preview;
	return new_item;
}

uiItem*
deshi__ui_make_input_text(str8 preview, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItemSetup setup = {0};
	setup.size = sizeof(uiInputText);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.update = ui_update_input_text;
	setup.generate = ui_gen_input_text;
	setup.evaluate = ui_eval_input_text;
	setup.copy = ui_copy_input_text;
	vec2i counts[1] = {render_make_text_counts(str8_length(preview))+render_make_rect_counts()};
	setup.drawinfo_reserve = counts;
	setup.drawcmd_count = 1;
	
	b32 retrieved = 0;
	uiItem*      item = ui_setup_item(setup, &retrieved);
	uiInputText* data = ui_get_input_text(item);
	
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
	auto dc = item->drawcmds;
	auto vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	auto ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	vec2i counts = {0};
	auto s = ui_get_slider(item);

	vec2 pos = item->pos_screen;
	vec2 size = item->size;

	counts += ui_gen_background(item, vp, ip, counts);
	counts += ui_gen_border(item, vp, ip, counts);

	counts += render_make_filledrect(vp, ip, counts, 
			  Vec2(pos.x, pos.y + size.y * (1 - s->style.rail_thickness)/2),
			  Vec2(size.x, size.y * s->style.rail_thickness),
			  s->style.colors.rail);
	
	if(s->style.dragger_shape == slider_dragger_rect) {
		counts += render_make_filledrect(vp, ip, counts,
				Vec2(pos.x + s->pos, pos.y),
				Vec2(s->width, size.y),
				s->style.colors.dragger);
	} else {
		NotImplemented;
	}

	dc->counts_used = counts;
}

void
ui_slider_callback(uiItem* item){DPZoneScoped;
	auto s   = ui_get_slider(item);
	auto mp  = input_mouse_position();
	auto lmp = mp - item->pos_screen;
	switch(s->type) {
		case 0: {
			f32 dragpos, dragwidth,
				min = s->minf32,
				max = s->maxf32,
			   *var = s->varf32;
			
			*var = Clamp(*var, min, max);
			dragwidth = item->width / 8;
			dragpos = Remap(*var, 0.f, item->width-dragwidth, min, max);
			if(input_lmouse_pressed() && 
			   Math::PointInRectangle(lmp, Vec2(dragpos, 0), Vec2(dragwidth, item->height))) {
				s->active = true;
				s->mouse_offset = -lmp.x + dragpos;
			}
			if(s->active) {
				*var = Remap(Clamp(lmp.x + s->mouse_offset, 0.f, item->width - dragwidth), min, max, 0.f, item->width - dragwidth);
				item->dirty = true;
				item->action_trigger = action_act_always;
			}
			if(input_lmouse_released()) {
				s->active = false;
				item->action_trigger = action_act_mouse_hover;
			}
			s->width = dragwidth;
			s->pos = dragpos;
		} break;
		default: NotImplemented;
	}
}

uiItem*
ui_copy_slider(uiItem* old_item) {
	auto old_slider = ui_get_slider(old_item);
	auto new_item = (uiItem*)memalloc(sizeof(uiSlider));
	auto new_slider = ui_get_slider(new_item);
	ui_item_copy_base(new_item, old_item);

	new_slider->style = old_slider->style;
	new_slider->maxf32 = old_slider->maxf32;
	new_slider->minf32 = old_slider->minf32;
	new_slider->varf32 = old_slider->varf32;
	new_slider->type = old_slider->type;
	return new_item;
}

uiItem*
ui_make_slider(uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItemSetup setup = {0};
	setup.size = sizeof(uiSlider);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.generate = ui_gen_slider;
	setup.hash = slider_style_hash;
	setup.copy = ui_copy_slider;
	setup.update = ui_slider_callback;
	setup.update_trigger = action_act_mouse_hover | action_act_hash_change;
	vec2i counts[1] = {2*render_make_filledrect_counts()+render_make_rect_counts()};
	setup.drawinfo_reserve = counts;
	setup.drawcmd_count = 1;

	auto item = ui_setup_item(setup);

	auto s = ui_get_slider(item);
	s->style.colors.rail = color(80,80,80);
	s->style.colors.dragger = color(14,50,100);

	return item;
}

uiItem*
deshi__ui_make_slider_f32(f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	auto s = ui_get_slider(item);
	s->minf32 = min;
	s->maxf32 = max;
	s->varf32 = var;
	s->type = 0;
	
	return item;
}

uiItem*
deshi__ui_make_slider_u32(u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	auto s = ui_get_slider(item);
	s->minu32 = min;
	s->maxu32 = max;
	s->varu32 = var;
	s->type = 1;
	
	return item;
}

uiItem*
deshi__ui_make_slider_s32(s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line){DPZoneScoped;
	uiItem* item = ui_make_slider(style, file, line);
	
	auto s = ui_get_slider(item);
	s->mins32 = max;
	s->maxs32 = max;
	s->vars32 = var;
	s->type = 2;
	
	return item;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui2_widgets_checkbox_impl


void
ui_gen_checkbox(uiItem* item){DPZoneScoped;
	auto cb = ui_get_checkbox(item);
	auto dc = item->drawcmds;
	auto vp = (Vertex2*)g_ui->vertex_arena->start + dc->vertex_offset;
	auto ip = (u32*)g_ui->index_arena->start + dc->index_offset;
	vec2i counts = {0};

	vec2 fillpos = item->pos_screen + cb->style.fill_padding;
	vec2 fillsiz = item->size - 2 * cb->style.fill_padding;

	counts += ui_gen_background(item, vp, ip, counts);
	counts += ui_gen_border(item, vp, ip, counts);

	Assert(cb->var, "Null data pointer for checkbox"); 
	if(*cb->var) {
		counts += render_make_filledrect(vp, ip, counts, fillpos, fillsiz, cb->style.colors.filling);
	}

	dc->counts_used = counts;
}

void
ui_checkbox_callback(uiItem* item){DPZoneScoped;
	auto cb = ui_get_checkbox(item);
	*cb->var = !*cb->var;
	item->dirty = 1;
}

uiItem*
ui_copy_checkbox(uiItem* old_item) {DPZoneScoped;
	auto old_checkbox = ui_get_checkbox(old_item);
	auto new_item = (uiItem*)memalloc(sizeof(uiCheckbox));
	auto new_checkbox = ui_get_checkbox(new_item);
	ui_item_copy_base(new_item, old_item);

	new_checkbox->style = old_checkbox->style;
	new_checkbox->var = old_checkbox->var;
	return new_item;
}

uiItem*
deshi__ui_make_checkbox(b32* var, uiStyle* style, str8 file, upt line){
	uiItemSetup setup = {0};
	setup.size = sizeof(uiCheckbox);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.generate = ui_gen_checkbox;
	setup.hash = checkbox_style_hash;
	setup.copy = ui_copy_checkbox;
	vec2i counts[1] = {2*render_make_filledrect_counts()+render_make_rect_counts()};
	setup.drawinfo_reserve = counts;
	setup.drawcmd_count = 1;

	auto item = ui_setup_item(setup);
	item->action = ui_checkbox_callback;
	item->action_trigger = action_act_mouse_pressed;

	auto cb = ui_get_checkbox(item);
	cb->style.colors.filling = color(100, 150, 200);
	cb->style.fill_type = checkbox_fill_box;
	cb->style.fill_padding = {2,2};
	cb->var = var;
	
	return item;
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @tabs_implementation

struct TabbedInfo {
	u32 n_tabs;
	f32 width;
	f32 tab_width;
	b32 need_scroll;
	f32 tab_height;
	u32 n_can_fit;
	u32 start;
	f32 loffset;
};

TabbedInfo
ui_tabbed_get_info(uiTabbed* tabbed) {
	TabbedInfo out;
	out.n_tabs = tabbed->item.node.child_count;
	out.width = tabbed->item.width;
	out.tab_width = out.width / out.n_tabs;
	out.need_scroll = out.tab_width < tabbed->style.min_tab_width;
	out.tab_width = Max(out.tab_width, tabbed->style.min_tab_width);
	out.tab_height = tabbed->style.tab_size;
	out.n_can_fit = ceil(out.width / out.tab_width) + 1;
	out.start = floor(tabbed->scroll / out.tab_width);
	out.loffset = out.start * out.tab_width - tabbed->scroll;
	return out;
}

u32
ui_get_hovered_tab(uiTabbed* tabbed) {
	uiItem* item = &tabbed->item;
	if(!ui_item_hovered(item, hovered_area)) return -1;
	TabbedInfo ti = ui_tabbed_get_info(tabbed);
	vec2 local_mouse = input_mouse_position() - item->pos_screen;
	if(local_mouse.y > ti.tab_height) return -1;
	f32 first_split = ti.tab_width + ti.loffset;
	forI(ti.n_can_fit) {
		if(local_mouse.x < first_split + i * ti.tab_width) return ti.start + i;
	}
	return -1;
}
void
ui_gen_tabbed(uiItem* item) {
	auto t  = ui_get_tabbed(item);
	auto tabs_dc = item->drawcmds;
	auto tabs_ptrs = ui_drawcmd_get_ptrs(tabs_dc);
	auto text_dc = item->drawcmds + 1;
	auto text_ptrs = ui_drawcmd_get_ptrs(text_dc);
	vec2i text_counts = {0};
	vec2i tabs_counts = {0};
	
	TabbedInfo ti = ui_tabbed_get_info(t);
	
	u32 max_chars = ti.tab_width / item->style.font->max_width;
	
	// figure out how much room we need for text
	// TODO(sushi) do this better sometime
	uiTab* scan = (uiTab*)item->node.first_child;
	uiTab* start = 0;
	vec2i text_counts_needed = {0};
	forI(ti.n_tabs) { 
		text_counts_needed += render_make_text_counts(Min(max_chars, str8_length(scan->name)));
		if(i == ti.start) start = scan;
		scan = (uiTab*)scan->item.node.next;
	}
	text_ptrs = ui_drawcmd_realloc(text_dc, text_counts_needed);
	text_dc->texture = item->style.font->tex;

	vec2i tab_counts_needed = ti.n_can_fit * render_make_filledrect_counts();
	tabs_ptrs = ui_drawcmd_realloc(tabs_dc, tab_counts_needed);
	tabs_dc->texture = 0;

	uiTab* iter = start;
	forI(ti.n_can_fit) {
		color bg;
		if(i + ti.start == t->selected) {
			bg = t->style.colors.active.background;
		} else if(i + ti.start == ui_get_hovered_tab(t)) {
			bg = t->style.colors.hovered.background;
		}else {
			bg = t->style.colors.inactive.background;
		}
		vec2 pos = item->pos_screen;
		pos.x += i * ti.tab_width + ti.loffset;
		vec2 size = {ti.tab_width, ti.tab_height};
		tabs_counts += render_make_filledrect(tabs_ptrs.vertexes, tabs_ptrs.indexes, tabs_counts, pos, size, bg);
		str8 displayed_text = str8{iter->name.str, Min(max_chars, iter->name.count)};
		vec2 text_size = font_visual_size(item->style.font, displayed_text) * item->style.font_height / item->style.font->max_height;
		vec2 diff = size - text_size;
		vec2 text_pos = ceil(t->style.tab_text_alignment * diff + pos);
		text_counts += render_make_text(text_ptrs.vertexes, text_ptrs.indexes, text_counts, str8{iter->name.str, Min(max_chars, iter->name.count)}, item->style.font, text_pos, Color_White, vec2_ONE());
		iter = (uiTab*)iter->item.node.next;
		if(!iter) break;
	}

	tabs_dc->counts_used = tabs_counts;
	text_dc->counts_used = text_counts;
}

void
ui_update_tabbed(uiItem* item) {
	if(input_lmouse_pressed()) {
		uiTabbed* t = ui_get_tabbed(item);
		u32 s = ui_get_hovered_tab(t);
		if(s!=-1) {
			TabbedInfo ti = ui_tabbed_get_info(t);
			ui_get_tabbed(item)->selected = s;
			u32 i = 0;
			for_node(item->node.first_child) {
				uiItem* child = (uiItem*)it;
				if(i == t->selected) {
					RemoveFlag(child->style.display, display_hidden);
					child->style.sizing = size_normal;
					child->style.width = item->width;
					child->style.height = item->height - ti.tab_height;
					child->style.positioning = pos_relative;
					child->style.pos = Vec2(0, ti.tab_height);
					child->dirty = true;
				} else AddFlag(child->style.display, display_hidden);
				i += 1;
			}
		}
	}
	item->dirty = true; // regen so the hovered tab is colored
}

uiItem*
deshi__ui_make_tabbed(uiStyle* style, str8 file, upt line) {
	uiItemSetup setup = {0};
	setup.size = sizeof(uiTabbed);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.generate = ui_gen_tabbed;
	setup.update = ui_update_tabbed;
	setup.update_trigger = action_act_mouse_hover;
	setup.drawcmd_count = 2;
	vec2i counts[2] = {render_make_filledrect_counts(), render_make_filledrect_counts()};
	setup.drawinfo_reserve = counts;
	
	uiItem* item = ui_setup_item(setup);
	return item;
}

uiItem*
deshi__ui_begin_tabbed(uiStyle* style, str8 file, upt line) {
	auto i = deshi__ui_make_tabbed(style, file, line);
	ui_push_item(i);
	return i;
}

void
deshi__ui_end_tabbed(str8 file, upt line) {
	deshi__ui_end_item(file, line);
}

void
ui_gen_tab(uiItem* item) {
	auto ptrs = ui_drawcmd_get_ptrs(item->drawcmds);
	item->drawcmds->counts_used = {0};
	item->drawcmds->counts_used += ui_gen_background(item, ptrs.vertexes, ptrs.indexes, item->drawcmds->counts_used);
	item->drawcmds->counts_used += ui_gen_border(item, ptrs.vertexes, ptrs.indexes, item->drawcmds->counts_used);
}

uiItem* 
deshi__ui_make_tab(str8 title, uiStyle* style, str8 file, upt line) {
	uiItemSetup setup = {0};
	setup.size = sizeof(uiTab);
	setup.style = style;
	setup.file = file;
	setup.line = line;
	setup.generate = ui_gen_tab;
	setup.drawcmd_count = 1;
	vec2i counts[1] = {render_make_filledrect_counts() + render_make_rect_counts()};
	setup.drawinfo_reserve = counts;

	uiItem* item = ui_setup_item(setup);
	uiTab* tab = ui_get_tab(item);
	tab->name = title;
	item->dirty = true;
	return item;
}

uiItem* 
deshi__ui_begin_tab(str8 title, uiStyle* style, str8 file, upt line) {
	auto i = deshi__ui_make_tab(title, style, file, line);
	ui_push_item(i);
	return i;
}

void 
deshi__ui_end_tab(str8 file, upt line) {
	deshi__ui_end_item(file, line);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @tree_implementation

void
ui_gen_tree(uiItem* item) {
	
}

void
ui_eval_tree(uiItem* item) {

}

uiItem* 
deshi__ui_make_tree(uiStyle* style, str8 file, upt line) {
	uiItemSetup setup = {0};
	setup.size = sizeof(uiTree);
	setup.file = file;
	setup.line = line;
	setup.generate = ui_gen_tree;
	setup.drawcmd_count = 1;
	vec2i counts[1] = {render_make_filledrect_counts()};
	setup.drawinfo_reserve = counts;

	uiItem* item = ui_setup_item(setup);
	return item;
}

uiItem* 
deshi__ui_begin_tree(uiStyle* style, str8 file, upt line) {
	auto i = deshi__ui_begin_tree(style, file, line);
	ui_push_item(i);
	return i;
}

void 
deshi__ui_end_tree(str8 file, upt line) {
	deshi__ui_end_item(file, line);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @tree_node_implementation

uiItem* 
deshi__ui_make_tree_node(str8 title, uiStyle* style, str8 file, upt line) {

}

uiItem* 
deshi__ui_begin_tree_node(str8 title, uiStyle* style, str8 file, upt line) {

}

void 
deshi__ui_end_tree_node(str8 file, upt line) {

}

#endif //defined(DESHI_IMPLEMENTATION) && !defined(DESHI_UI2_WIDGETS_IMPL)
