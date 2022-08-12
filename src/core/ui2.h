/* deshi UI Module
Notes
-----

	The basic object of ui is the uiItem struct. uiItems are laid out in memory as they are created and connected by nodes. 
	These connections determine in what order uiItems are rendered. uiItems never move in memory, except when deleted. 

	The way a uiItem appears on screen is determined by uiStyle properties. These properties are the user's 
	interface to changing uiItems. When a property is changed ui detects it and reevaluates the item, and any other item
	that would be affected by that item changing.

	There are extensions to uiItems called widgets. These extensions may define extra data and style properties. 
	A widgets data is allocated after the uiItem struct it extends and their behavoir is defined through an action 
	function pointer on uiItem. When you use a normal uiItem, this function pointer is free to be used. The function
	is called at certain times depending on what the uiItem's action_trigger variable is set to. This function is called 
	before the item is evaluated. 

	In order for ui to understand how to render widgets, they must define the '__generate' function pointer on uiItem.
	This takes in the uiItem* after it has been evaluated and its screen position and size information has been set.
	And in order for an extensions' custom style properties to be recognized by ui, the widget must define the
	'__hash' function on uiItem, telling ui how to hash any properties of the custom data the user may change.

	The basic uiItem, and some widgets, may be told to begin and end. This allows you to nest items within other items
	much like HTML. However, everything supports just being made. This tells ui that the item is not going to have any children.
	Items such as uiText and uiSlider cannot be made using begin, because they cannot reasonably render any other items.
	
	With the exception of scrolling, dragging, and resizing, nothing in ui's internals or in a widget's internals may edit uiStyle properties. 
	uiStyle is the user's interface	for deciding how an item looks and so should never be changed without the user's knowledge. 

	The only thing ui's internals know about is the uiItem and uiStyle structs. Style properties defined by widgets 
	cannot be used by ui's internals to evaluate anything, but the information in uiStyle should be enough already.

	All of ui is interfaced to through uiItem pointers. In order to access data of a widget you must get it
	through the widget's function for doing so (eg. uiGetSlider(uiItem*)). This is to keep the interface from using
	many different types, unless the user explicitly asks for them.


Index
-----
@ui_style
@ui_item
@ui_window
@ui_button
@ui_text
@ui_section
@ui_context

TODOs
-----
possibly implement a system for suppressing certain errors on some behavoirs or just for suppressing all errors

Implement a system for trimming down how much we have to do to check every item.
	Currently we have to walk the entire item tree to check every item, but this can be linearized into an array 
	that stores uiItem*'s in the order we would have walked it. This also reduces the potentially massive amount of
	recursion that ui can go into.

*/

/*
	Item Style Documentation

	uiItems may be passed a uiStyle object or a css-style string to determine
	the style an item takes on. Following is docs about each property. Each property
	lists it's valid vaules in both programmatic and string form followed by an example.

	//TODO(sushi) eventually convert this to HTML or markdown for easier viewing

------------------------------------------------------------------------------------------------------------
*   positioning 
	---
	Determines how a uiItem is positioned.

-   Example:
		in code:
			uiStyle style;
			style.positioning = pos_fixed;
		in string:
			positioning: static;

-   Values:
		pos_static  |  static
			Default value.
			The item will be placed normally and top, bottom, left, and right values dont affect it.
			The first item placed in this manner will just be positioned at 0,0 plus its margin and borders,
			then items placed after it are placed below it, as it would be in HTML or ImGui.

		pos_relative  |  relative
			The position values will position the item relative to where it would have 
			normally been placed. This removes the item from the normal flow, but items
			added after it will be placed as if the item was where it would have been

        pos_absolute  |  absolute
			The position values will position the item relative to where its parent will
            be placed. If its parent's position is dynamically determined then it finds
			the closest ancestor whose position is static and positions relative to it.
			This removes the item from the normal flow.

		pos_fixed  |  fixed
			The item is positioned relative to the viewport and does not move.

		pos_sticky  |  sticy
			The item is positioned just as it would be in relative, but if the item
			were to go out of view by the user scrolling the item will stick to the edge

		pos_draggable_relative | draggable_relative
			The item is positioned the same as relative, but its position may be changed
			by dragging it with the mouse.
		
		pos_draggable_absolute | draggable_relative
			The item is positioned the same as absolute, but its position may be 
			changed by dragging it with the mouse.

		pos_draggable_fixed | draggable_fixed
			The item is positioned the same as fixed, but its position may be changed by 
			dragging it with the mouse

------------------------------------------------------------------------------------------------------------
*   sizing 
	---
    Determines how an item is sized. This is a flagged value, meaning it can take on multiple of these
	properties. For example setting 'size_fill_x | size_percent_x' will tell the renderer to interpret
	the width as a percentage of the remaining space of the item's parent's width.

	Percent and fill only apply when the parent's size has been explicitly set either by sizing it relative to
	it's parent or giving it size values. If its parent is not explicitly sized then the item fallsback 
	to automatic sizing and an error is issued.

-   Values:
		size_normal (default)
			The item's width and height will be set to its style 'size' property.
	
		size_percent_x 
			Sets the items width as a percentage of its parents width.

		size_percent_y 
			Sets the items height as a percentage of its parent's height.

		size_percent   
			Combination of percentx and percenty.

		size_square
			Keeps the item at a 1:1 aspect ratio. This requires that either height or width are set to auto, while
			the other has a specified value. If both values are specified, then this value is ignored

		size_flex
			Indicates that the item is to be considered in a flex container. This overrides the item's width or height
			(depending on if display is set to row or column) to be a ratio to other flex items in the container.

------------------------------------------------------------------------------------------------------------
*   top,left,bottom,right
	---
	Determines where a uiItem is positioned according to it's 'positioning' value. Percents are only supported
	in string styling, by suffixing the literal with a %. Note that if bottom or right are used, they overrule
	top or left respectively.

	Note that if an item wants to use bottom or right, their parent's size must not be automatically determined.
	TODO(sushi) look into a way around this

-   Defaults:
		top and left default to 0, while bottom and right default to MAX_S32, indicating to use top or left instead.

-   Shorthands:
		tl - a vec2i representing the x and y coords of the top left corner of the item
		br - a vec2i representing the x and y coords of the bottom right corner of the item
	
-   Example:
		in code:
			uiStyle style;
			style.positioning = pos_relative; 
			style.top = 10; //sets the item's position to be 10 pixels below 
		in string:
			positioning: static;
			top: 10; 
			////
			positioning: relative;
			lt: 10% 15%; //sets the item's left edge to be 10% of its parents width from its parent left edge and 15% of its parent's height from its parent's top edge

------------------------------------------------------------------------------------------------------------
*   size, width, height
	Determines the size of the item. Note that text is not affected by this property, and its size is always
	as it appears on the screen. If you want to change text size use font_height.

    This is the size of the area in which items can be placed minus padding. Note that if margins or borders are 
    specified, this increases the total area an item takes up visually.

-   Defaults:
		width and height both default to size_auto.

-   Example:
		in code:
			uiStyle style;
			style.width = size_fill; //will fill remaining width of parent
			style.height = 20; //height 20 pixels
		in string:
			width: auto; //size based on content
			height: 20%; //size 20% of parent's width
			width: fill; //fills remaining width of parent.


------------------------------------------------------------------------------------------------------------
*   margin, margin_top, margin_bottom, margin_left, margin_right
	---
	Determines the spacing between a child's edges and its parents edges. Percents are only supported in
	string styling. Note that if bottom or right are used, they overrule top or left respectively.

	 ┌----------------------------┑
	 |     |─────────────────────────── margin_top
	 |    ┌------------------┑    |
	 |━━━━|                  |    |
	 |  | |    child item    |    |
	 |  | |                  |    |
	 |  | |                  |    |
	 |  | |                  |    |
	 |  | └------------------┙    |
	 |  |                         |
	 └--|-------------------------┙
		|
	   margin_left

-   Defaults:
		margin_top and margin_left default to 0, while margin_bottom and margin_right default to MAX_S32
		MAXP_S32 just indicates to the renderer that it should use the same value as the other side

-   Shorthands:
		margin sets margin_left and margin_top

-   Example:
		in code:
			uiStyle style;
			style.margin_top = 10;
		in string:
			margin: 10px 10%;

-   Behavoir:
		Margins do not cause items to push away their siblings, a margin for an item just 

------------------------------------------------------------------------------------------------------------
*   padding, padding_top, padding_bottom, padding_left, padding_right
	---
	Determines the spacing between an item's edges and its content's edges. Percents are only supported in
	string styling. The value is interpretted as a 

	 ┌----------------------------┑
	 |                            |
	 |    ┌------------------┑    |
	 |    |    |─────────────────────────── padding_top
	 |    |----text in item  |    |
	 |    | |                |    |
	 |    | |                |    |
	 |    | |                |    |
	 |    └-|----------------┙    |
	 |      |                     |
	 └------|---------------------┙
			|
			padding_left

-   Defaults:
		padding_top and padding_left default to 0, while padding_bottom and padding_right default to MAX_S32
		MAX_S32 just indicates to the renderer that it should use the same value as the other side

-   Shorthands:
		padding sets padding_left and padding_top

-   Example:
		in code:
			uiStyle style;
			style.padding_left = 10;
			style.padding_bottom = 15;
		in string:
			padding: 20; //sets both padding_left and padding_top to 20 pixels;

------------------------------------------------------------------------------------------------------------
*   scroll, scrollx, scrolly
	---
	Scrolls the window when items overflow if overflow is set to allow it.

-   Defaults:
		Both values default to 0

-   Catches:
		Scrolling conflicts with setting content_align.

TODO(sushi) examples

------------------------------------------------------------------------------------------------------------
*   content_align
	---
	Determines how to align an item's contents over each axis. 
	This only works on children who's positioning is static or relative.
	This respects margins and padding, content will only be aligned within their valid space. 
	Negative values are not valid.
	Values larger than 1 are not valid.
	NotImplemented -- This does not apply when alignment is set to inline.

-   Performance:
		In order to properly do this (as far as I can tell), we must reevaluate all children of an item
		after having already evaluated them because the size of the group of items is not known until
		all of them have been evaluated. So this more or less doubles the evaluation time of every
		item that uses it.

-   Defaults:
		Defaults to 0

TODO(sushi) example

------------------------------------------------------------------------------------------------------------
*   font
	---
	Determines the font to use when rendering text. In code this takes a Font* while in string this takes the
	name of a font file stored in data/fonts.

-   Defaults:
		By default the font used is gohudont-11.bdf which should ship with deshi.

-   Example:
		in code:
			uiStyle style;
			style.font = Storage::CreateFontFromFile("Terminus.ttf").second;
		in string:
			font: Terminus.ttf;

------------------------------------------------------------------------------------------------------------
*   font_height
	---
	Determines the visible height in pixels of rendered text. Note that this does not have to be the same height
	the font was loaded at, though it may not render as pretty if it isnt.

-   Defaults:
		Defaults to 11, the height of the default loaded font.

-   Example:
		in code:
			uiStyle style; 
			style.font_height = 200;
		in string:
			font_height: 200;

------------------------------------------------------------------------------------------------------------
*   background_color
	---
	Determines the background color of a uiItem. Note that this property does not apply to all items.

-   Defaults:
		Defaults to (0,0,0,0)

------------------------------------------------------------------------------------------------------------
*   border_style
	---
	Determines the style of border a uiItem has

	//TODO(sushi) border_style docs

------------------------------------------------------------------------------------------------------------
*   border_width
	---
	Determines the width of a uiItem's border. This has no affect if border style is not set.

------------------------------------------------------------------------------------------------------------
*   text_color
	---
	Determines the color of text

	//TODO(sushi) text_color docs

------------------------------------------------------------------------------------------------------------
*   overflow
	---
	Determines how items that go out of their parent's bounds are displayed.

-  Values:
	
	overflow_scroll (default)
		When items extend beyond the parents borders, the items are cut off and scroll bars are shown.
	
	overflow_scroll_hidden | scroll_hidden
		Same as overflow_scroll, but scrollbars are not shown
	
	overflow_hidden | hidden
		The overflowing items are cut off, but the user cannot scroll.
		In this case you can still manually change the scroll value on the item it just 
		prevents the user from scrolling the item. Good for scrolling things you
		dont want the user to mess with.
	
	overflow_visible | visible
		The overflowing items are shown and there is no scrolling.

------------------------------------------------------------------------------------------------------------
*   focus
	---
	Flag that determines if an item should focus over its siblings when clicked by the mouse.

-   Defaults:
	 	Defaults to false.

-   Catches:
		When using this on an item whose positioning is not fixed, absolute you must remember that
		it will affect the positioing of items around it.

------------------------------------------------------------------------------------------------------------
*   display
	---
	Determines how to display an item's children. This is a flagged value, meaning it can take on several
	different values. For example using 'display_column | display_reverse' will display children in a column
	but in the reverse order that they were added.

-	Values:
		display_column (default)
			Displays children from top to bottom. This is mutually exclusive with display_row.
		
		display_row
			Displays children from left to right. This is mutually exclusive with display_column.

		display_flex 
			Sets the item to act as a flex container. This makes the container able to manually set the size
			of its children whose sizing property is set to size_flex.
			
			When this flag is set the container will treat its children's width or height value (depending on 
			if row or column is set) as a ratio of how large it should be relative to other items in the container.
			For example, if you have 3 children and their ratios are set as 3, 1, 1, then the first child will be 
			3 times as large as the other two and the last two will be the same size. 
			You may also use floating point values as ratios.

			Not all items in a flex container have to use size_flex. If an item is not sized in this way
			flex will size its items around it. (TODO(sushi) flex demo) See the flex demo for examples.
		    
		display_reverse
			Displays the containers items in reverse order, eg with display_column it will display items from 
			right to left. Not from the right side of the container to the left though, this just specifies to
			draw items in the reverse order they were added. If you want to draw items from the right side consider using
			anchors or content align.

		display_hidden
			Doesnt display the parent or its children.

-	Technical:
		Since we require uiStyle's default to be 0, and I dont want to explicitly set display to column
		every single time I make an item, display_column is equal to 0. This sort of makes it so that the first bit
		is a boolean determining if we are drawing a row or column. This is beneficial because it enforces
		row and column being mutually exclusive.
*/


#pragma once
#ifndef DESHI_UI2_H
#define DESHI_UI2_H

#include "kigu/color.h"
#include "kigu/common.h"
#include "kigu/node.h"
#include "kigu/unicode.h"
#include "math/vector.h"
#include "core/render.h"
#include "core/text.h"
#include "kigu/hash.h"

#if DESHI_RELOADABLE_UI
#  if DESHI_DLL
#    define UI_FUNC_API(sig__return_type, sig__name, ...) \
external __declspec(dllexport) sig__return_type sig__name(__VA_ARGS__); \
typedef sig__return_type GLUE(sig__name,__sig)(__VA_ARGS__); \
sig__return_type GLUE(sig__name,__stub)(__VA_ARGS__);
#  else
#    define UI_FUNC_API(sig__return_type, sig__name, ...) \
external __declspec(dllexport) sig__return_type sig__name(__VA_ARGS__); \
typedef sig__return_type GLUE(sig__name,__sig)(__VA_ARGS__); \
sig__return_type GLUE(sig__name,__stub)(__VA_ARGS__){return (sig__return_type)0;}
#  endif //DESHI_DLL
#  define UI_DEF(x) GLUE(g_ui->, x)
#else
#  define UI_FUNC_API(sig__return_type, sig__name, ...) external sig__return_type sig__name(__VA_ARGS__)
#  define UI_DEF(x) GLUE(ui_, x)
#endif //DESHI_RELOADABLE_UI

//NOTE(sushi) idea to allow custom prefixing of ui stuff at compile time
#ifndef UI_PREFIX
#define UI_PREFIX 
#endif
#define UI_PRE(x) GLUE(UI_PREFIX, x)

#define UI_UNIQUE_ID(str) str8_static_hash64({str,sizeof(str)})

#define UI_CUSTOM_ITEM(struct_name, item_name) StaticAssert(offsetof(struct_name, item_name)==0);


#define MarginedWidth(item)       ((item)->width - (item)->style.margin_left - (item)->style.margin_right)
#define MarginedHeight(item)      ((item)->height - (item)->style.margin_top - (item)->style.margin_bottom)
#define MarginedArea(item)        (vec2{MarginedWidth(item), MarginedHeight(item)})
#define MarginedStyleWidth(item)  ((item)->style.width + (item)->style.margin_left + (item)->style.margin_right)
#define MarginedStyleHeight(item) ((item)->style.height + (item)->style.margin_top + (item)->style.margin_bottom)
#define MarginedStyleArea(item)   (vec2{MarginedStyleWidth(item), MarginedStyleHeight(item)})
#define BorderedWidth(item)       (MarginedWidth(item) - ((item)->style.border_style ? 2*(item)->style.border_width : 0))
#define BorderedHeight(item)      (MarginedHeight(item) - ((item)->style.border_style ? 2*(item)->style.border_width : 0))
#define BorderedArea(item)        (vec2{BorderedWidth(item), BorderedHeight(item)})
#define BorderedStyleWidth(item)  ((item)->style.width + ((item)->style.border_style ? 2*(item)->style.border_width : 0))
#define BorderedStyleHeight(item) ((item)->style.height + ((item)->style.border_style ? 2*(item)->style.border_width : 0))
#define BorderedStyleArea(item)   (vec2{BorderedStyleWidth(item), BorderedStyleHeight(item)})
#define PaddedWidth(item)         (BorderedWidth(item) - (item)->style.padding_left - (item)->style.padding_right)
#define PaddedHeight(item)        (BorderedHeight(item) - (item)->style.padding_top - (item)->style.padding_bottom)
#define PaddedArea(item)          (vec2{PaddedWidth(item), PaddedHeight(item)})
#define PaddedStyleWidth(item)    ((item)->style.width - (item)->style.padding_left - (item)->style.padding_right)
#define PaddedStyleHeight(item)   ((item)->style.height - (item)->style.padding_top - (item)->style.padding_bottom)
#define PaddedStyleArea(item)     (vec2{PaddedStyleWidth(item), PaddedStyleHeight(item)})

// a singleton collection of keybinds for use throughout ui
struct uiKeybinds{
	struct{
		struct{
			KeyCode left;
			KeyCode left_word;
			KeyCode left_wordpart;

			KeyCode right;
			KeyCode right_word;
			KeyCode right_wordpart;

			KeyCode up;
			KeyCode down;
    		
			//KeyCode anchor;
		}cursor, select;

		struct{
			KeyCode left;
			KeyCode left_word;
			KeyCode left_wordpart;

			KeyCode right;
			KeyCode right_word;
			KeyCode right_wordpart;
		}del;
	}inputtext;
}global uikeys;


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @item
struct Texture;
struct uiDrawCmd{
	Node node;
	Texture* texture;
	u32 vertex_offset; 
	u32 index_offset;
	RenderDrawCounts counts;
	vec2i scissorOffset;
	vec2i scissorExtent;
};
#define uiDrawCmdFromNode(x) CastFromMember(uiDrawCmd, node, x)

enum{
	pos_static=0,
	pos_relative,
	pos_absolute,
	pos_fixed,
	pos_sticky,
	pos_draggable_relative,
	pos_draggable_absolute,
	pos_draggable_fixed,

	anchor_top_left = 0,
	anchor_top_right,
	anchor_bottom_right,
	anchor_bottom_left,

	size_normal    = 0,
	size_auto_y    = 1 << 0,
	size_auto_x    = 1 << 1,
	size_auto      = size_auto_x | size_auto_y,
    size_percent_x = 1 << 2,
    size_percent_y = 1 << 3,
    size_percent   = size_percent_x | size_percent_y,
	size_square    = 1 << 4,
	size_flex      = 1 << 5,

	border_none = 0,
	border_solid,
	
	overflow_scroll = 0,
	overflow_scroll_hidden,
	overflow_hidden,
	overflow_visible,

	text_wrap_none = 0,
	text_wrap_char, 
	text_wrap_word,

	display_column      = 0,
	display_row         = 1 << 0,
	display_flex        = 1 << 1,
	display_reverse     = 1 << 2,
	display_hidden      = 1 << 3,

	action_act_never = 0,
	action_act_mouse_hover,    // call action when the mouse is positioned over the item
	action_act_mouse_pressed,  // call action when the mouse is pressed over the item
	action_act_mouse_released, // call action when the mouse is released over the item
	action_act_mouse_down,     // call action when the mouse is down over the item
	action_act_always,         // call action every frame
};

struct Font;
external struct uiStyle{
	Type positioning; 
	Type anchor;
    Type sizing;
	union{
		struct{f32 x, y;};
		vec2 pos;
	};
	union{
		struct{f32 width, height;};
		vec2 size;
	};
	union{
		struct{f32 min_width, min_height;};
		vec2 min_size;
	};
	union{
		struct{f32 max_width, max_height;};
		vec2 max_size;
	};
	union{
		vec4 margin;
		struct{
			union{
				struct{f32 margin_left, margin_top;};	
				vec2 margintl;
			};
			union{
				struct{f32 margin_right, margin_bottom;};
				vec2 marginbr; 
			};
		};
	};
	union{
		vec4 padding;
		struct{
			union{
				struct{f32 padding_left, padding_top;};
				vec2 paddingtl;
			};
			union{
				struct{f32 padding_right, padding_bottom;};
				vec2 paddingbr;        
			};
		};
	};
	union{
		struct{f32 scrx, scry;};
		vec2 scroll;
	};
	color background_color;
	Type  border_style;
	color border_color;
	f32   border_width;
	Font* font;
	u32   font_height;
	Type  text_wrap;
	color text_color;
	u64   tab_spaces;
	b32   focus;
	Type  display;
	Type  overflow;
	vec2  content_align; 
	
	void operator=(const uiStyle& rhs){ memcpy(this, &rhs, sizeof(uiStyle)); }
};

struct uiItem{
	TNode node;
	str8 id; //NOTE(sushi) mostly for debugging, not sure if this will have any other use in the interface
	uiStyle style;
	
	//an items action call back function 
	//this function is called in situations defined by the flags in the in the uiStyle enum
	//and is always called before anything happens to the item in ui_update
	void (*action)(uiItem*);
	void* action_data; //a pointer to arbitrary data to be accessed in the action callback
	Type action_trigger; //how the action is triggered

	//// INTERNAL ////
	u32 style_hash;
	
	union{ // position relative to parent
		struct{ f32 lx, ly; };
		vec2 lpos;
	};
	union{ // position relative to screen
		struct{ f32 sx, sy; };
		vec2 spos;
	};
	union{ // position that the item's content starts at relative to the item's position
		struct{ f32 cx, cy; };
		vec2 cpos;
	};
	union{ // size that the item visually takes up on the screen
		struct{ f32 width, height; };
		vec2 size;
	};
	union{ // size that content occupies
		struct{ f32 cwidth, cheight; };
		vec2 csize;
	};
	vec2 max_scroll;

    //screen position and size of the bounding box containing all of an items
    //children, this is used to optimize things later, such as finding the hovered item
    vec2 children_bbx_pos;
    vec2 children_bbx_size;

	//the visible size of the item after being cut by overflow
	vec2 visible_start;
	vec2 visible_size; 
	
	u64 drawcmd_count;
	//TODO(sushi) make this an index into the drawcmds arena
	uiDrawCmd* drawcmds;
	
	//set to manually force the item to regenerate
	b32 dirty;

	Type update_trigger;

	void (*__update)(uiItem*);
	void (*__evaluate)(uiItem*);
	void (*__generate)(uiItem*);
	u32  (*__hash)(uiItem*);
	
	str8 file_created;
	upt  line_created;

	//size of the item in memory and the offset of the item member on widgets 
	u64 memsize;
	
	//indicates if the item has been initialized or not yet
	b32 initialized; 
	
	void operator=(const uiItem& rhs){memcpy(this, &rhs, sizeof(uiItem));}
};
#define uiItemFromNode(x) CastFromMember(uiItem, node, x)

UI_FUNC_API(uiItem*, ui_make_item, uiStyle* style, str8 file, upt line);
#define uiItemM()       UI_DEF(make_item( 0,STR8(__FILE__),__LINE__))
#define uiItemMS(style) UI_DEF(make_item((style),STR8(__FILE__),__LINE__))

UI_FUNC_API(uiItem*, ui_begin_item, uiStyle* style, str8 file, upt line);
#define uiItemB()       UI_DEF(begin_item(0,STR8(__FILE__),__LINE__))
#define uiItemBS(style) UI_DEF(begin_item((style),STR8(__FILE__),__LINE__))

UI_FUNC_API(void, ui_end_item, str8 file, upt line);
#define uiItemE() UI_DEF(end_item(STR8(__FILE__),__LINE__))

UI_FUNC_API(void, ui_remove_item, uiItem* item, str8 file, upt line);
#define uiItemR(item) UI_DEF(remove_item((item), STR8(__FILE__),__LINE__))

inline u32 hash_style(uiItem* item){DPZoneScoped;
	uiStyle* s = &item->style;
	u32 seed = 2166136261;
	seed ^= *(u32*)&s->positioning;     seed *= 16777619;
	seed ^= *(u32*)&s->sizing;			seed *= 16777619;
	seed ^= *(u32*)&s->anchor;			seed *= 16777619;
	seed ^= *(u32*)&s->x;               seed *= 16777619;
	seed ^= *(u32*)&s->y;               seed *= 16777619;
	seed ^= *(u32*)&s->width;           seed *= 16777619;
	seed ^= *(u32*)&s->height;          seed *= 16777619;
	seed ^= *(u32*)&s->margin_left;     seed *= 16777619;
	seed ^= *(u32*)&s->margin_top;      seed *= 16777619;
	seed ^= *(u32*)&s->margin_bottom;   seed *= 16777619;
	seed ^= *(u32*)&s->margin_right;    seed *= 16777619;
	seed ^= *(u32*)&s->padding_left;    seed *= 16777619;
	seed ^= *(u32*)&s->padding_top;     seed *= 16777619;
	seed ^= *(u32*)&s->padding_bottom;  seed *= 16777619;
	seed ^= *(u32*)&s->padding_right;   seed *= 16777619;
	seed ^= *(u32*)&s->content_align.x; seed *= 16777619;
	seed ^= *(u32*)&s->content_align.y; seed *= 16777619;
	seed ^= (u64)s->font;               seed *= 16777619;
	seed ^= s->font_height;             seed *= 16777619;
	seed ^= s->background_color.rgba;   seed *= 16777619;
	seed ^= s->border_style;            seed *= 16777619;
	seed ^= s->border_color.rgba;       seed *= 16777619;
	seed ^= *(u32*)&s->border_width;    seed *= 16777619;
	seed ^= s->text_color.rgba;         seed *= 16777619;
	seed ^= s->overflow;                seed *= 16777619;
	seed ^= s->focus;                   seed *= 16777619;
	
	if(item->__hash) { seed ^= item->__hash(item); seed *= 16777619; }
	
	return seed;
}

//-------------------------------------------------------------------------------------------------
// @immediate
UI_FUNC_API(void, ui_begin_immediate_branch, uiItem* parent, str8 file, upt line);
UI_FUNC_API(void, ui_end_immediate_branch, str8 file, upt line);
#define uiImmediateB()        UI_DEF(begin_immediate_branch(       0, STR8(__FILE__),__LINE__))
#define uiImmediateBP(parent) UI_DEF(begin_immediate_branch((parent), STR8(__FILE__),__LINE__))
#define uiImmediateE()        UI_DEF(end_immediate_branch(STR8(__FILE__),__LINE__))

//-------------------------------------------------------------------------------------------------
// @slider
UI_FUNC_API(uiItem*, ui_make_slider_f32, f32 min, f32 max, f32* var, uiStyle* style, str8 file, upt line);
#define uiSliderf32(min,max,var)        UI_DEF(make_slider_f32(min,max,var,0,STR8(__FILE__),__LINE__));
#define uiSliderf32S(min,max,var,style) UI_DEF(make_slider_f32(min,max,var,(style),STR8(__FILE__),__LINE__));
UI_FUNC_API(uiItem*, ui_make_slider_u32, u32 min, u32 max, u32* var, uiStyle* style, str8 file, upt line);
#define uiSlideru32(min,max,var)        UI_DEF(make_slider_u32(min,max,var,0,STR8(__FILE__),__LINE__));
#define uiSlideru32S(min,max,var,style) UI_DEF(make_slider_u32(min,max,var,(style),STR8(__FILE__),__LINE__));
UI_FUNC_API(uiItem*, ui_make_slider_s32, s32 min, s32 max, s32* var, uiStyle* style, str8 file, upt line);
#define uiSliders32(min,max,var)        UI_DEF(make_slider_s32(min,max,var,0,STR8(__FILE__),__LINE__));
#define uiSliders32S(min,max,var,style) UI_DEF(make_slider_s32(min,max,var,(style),STR8(__FILE__),__LINE__));

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

enum{
	slider_dragger_rect = 0,
	slider_dragger_round,
};

inline u32 slider_style_hash(uiItem* item){
	uiSlider* data = uiGetSlider(item);
	
	u32 seed = 2166136261;
	seed ^= data->style.colors.rail.rgba;       seed *= 16777619;
	seed ^= data->style.colors.dragger.rgba;    seed *= 16777619;
	seed ^= data->style.dragger_shape;          seed *= 16777619;
	seed ^= *(u32*)&data->style.rail_thickness; seed *= 16777619;

	return seed;
} 

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @checkbox

//uiCheckbox is a terminal node, so it cannot hold any children
UI_FUNC_API(uiItem*, ui_make_checkbox, b32* var, uiStyle* style, str8 file, upt line);
#define uiCheckboxM(var)         UI_DEF(make_checkbox((var), 0, STR8(__FILE__),__LINE__))
#define uiCheckboxMS(var, style) UI_DEF(make_checkbox((var), 0, STR8(__FILE__),__LINE__))

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

	u32 seed = 2166136261;
	seed ^= data->style.colors.filling.rgba;    seed *= 16777619;
	seed ^= data->style.fill_type;              seed *= 16777619;
	return seed;
}

//---------------------------------------------------------------------------------------------
// @text

//uiText is a terminal node meaning it cannot have any children.
UI_FUNC_API(uiItem*, ui_make_text, str8 text, uiStyle* style, str8 file, upt line);
//NOTE(sushi) does not automatically make a str8, use uiTextML for that.
#define uiTextM(text)          UI_DEF(make_text((text),           0, STR8(__FILE__),__LINE__))
#define uiTextMS(style, text)  UI_DEF(make_text((text),     (style), STR8(__FILE__),__LINE__))
//NOTE(sushi) this automatically applies STR8() to text, so you can only use literals with this macro
#define uiTextML(text)         UI_DEF(make_text(STR8(text),       0, STR8(__FILE__),__LINE__))
#define uiTextMSL(style, text) UI_DEF(make_text(STR8(text), (style), STR8(__FILE__), __LINE__))

struct uiText{
	uiItem item;
	Text text;
	b32 selecting;
	//TODO(sushi) get rid of array here
	array<pair<s64,vec2>> breaks;
};
#define uiGetText(x) CastFromMember(uiText, item, x)


//---------------------------------------------------------------------------------------------
// @inputtext

UI_FUNC_API(uiItem*, ui_make_input_text, str8 preview, uiStyle* style, str8 file, upt line);
#define uiInputTextM()                UI_DEF(make_input_text(    {0},       0, STR8(__FILE__),__LINE__))
#define uiInputTextMS(style)          UI_DEF(make_input_text(    {0}, (style), STR8(__FILE__),__LINE__))
#define uiInputTextMP(preview)        UI_DEF(make_input_text(preview,       0, STR8(__FILE__),__LINE__))
#define uiInputTextMSP(style,preview) UI_DEF(make_input_text(preview, (style), STR8(__FILE__),__LINE__))

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
	array<pair<s64,vec2>> breaks;

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

//-------------------------------------------------------------------------------------------------
// @context
struct MemoryContext;
struct uiContext;
UI_FUNC_API(void, ui_init, MemoryContext* memctx, uiContext* uictx);
#define uiInit(memctx,uictx) UI_DEF(init((memctx),(uictx)))

UI_FUNC_API(void, ui_update);
#define uiUpdate() UI_DEF(update())

//we cant grow the arena because it will move the memory, so we must chunk 
struct Arena;
struct ArenaList{
	Node node;   
	Arena* arena;
};
#define ArenaListFromNode(x) CastFromMember(ArenaList, node, x);

typedef u32 uiInputState; enum{
	uiISNone,
	uiISScrolling,
	uiISDragging,
	uiISResizing,
	uiISPreventInputs,
	uiISExternalPreventInputs,
};

struct uiContext{
#if DESHI_RELOADABLE_UI
	//// functions ////
	void* module;
	b32   module_valid;
	ui_make_item__sig*         make_item;
	ui_begin_item__sig*        begin_item;
	ui_end_item__sig*          end_item;
	ui_make_window__sig*       make_window;
	ui_begin_window__sig*      begin_window;
	ui_end_window__sig*        end_window;
	ui_make_button__sig*       make_button;
	ui_make_text__sig*         make_text;
	ui_init__sig*              init;
	ui_update__sig*            update;
#endif //#if DESHI_RELOADABLE_UI
	
	//// state ////
	uiItem base;
	uiItem* hovered; //item currently hovered by the mouse
	uiItem* active;  //item last interacted with
	uiInputState istate;

	struct{
		b32  active;
		b32  pushed;
		str8 file;
		upt  line;
		map<u64, uiItem*> cache;
	}immediate;

	b32 updating; //set true while ui_update is running

	//// memory ////
	//b32 cleanup; //set to true when ui needs to consider cleaning up/organizing its memory 	
	array<uiItem*> items;
	array<uiItem*> immediate_items;
	Node inactive_drawcmds; //list of drawcmds that have been removed and contain info about where we can allocate data next
	Arena* vertex_arena;
	Arena* index_arena;
	RenderTwodBuffer render_buffer;
	array<uiItem*> item_stack; //TODO(sushi) eventually put this in it's own arena since we can do a stack more efficiently in it

	struct{
		//visible on screen
		u64 items_visible;
		u64 drawcmds_visible;
		u64 vertices_visible; 
		u64 indices_visible;  
		//reserved in memory
		u64 items_reserved;
		u64 drawcmds_reserved;
		u64 vertices_reserved; 
		u64 indices_reserved;  
	}stats;
};

//global UI pointer
extern uiContext* g_ui;

void ui_debug();
void ui_demo();

#endif //DESHI_UI2_H
