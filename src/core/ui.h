/* deshi UI Module Index
-----
@ui_keybinds @ui_drawcmd @ui_style @ui_item @ui_generate @ui_immediate @ui_context

Introduction
------------

    The ui system represents all ui elements using a single struct, uiItem. These items can be created and nested much like in HTML, and styled
    much like in CSS. 

Items, Technical
----------------

    The ui system deals exclusively with the uiItem struct. When an item is created using either ui_begin_item() or ui_make_item(), the system will
    perform setup of the item using the ui_setup_item() function. Regardless of whether the user is making a normal item or a custom item, this 
    function should be used to create it internally. It serves 4 main purposes: it ensures that an item is not made during ui_update(), it retrieves 
    immediate mode items from the immediate mode cache, inherits text style properties when appropriate, and allocates an item's draw commands. 

    This is all that is done when making a plain uiItem with ui. If ui_begin_item() is called, then the item is also added to the item stack for 
    nesting and requires that ui_end_item() is called for every ui_begin_item() before ui_update(). After this, the item still needs to be
    evaluated and generated. 

    When ui_update() is called, the style for all items in ui are hashed and checked for changes. If an item's hash is different, ui evaluates and 
    redraws the item, as well as any other item that would be affected by that item changing. An item's hash is 0 when it is first made and so a new
    item will always be 'reevaluated'. When an item is found to be 'dirty', ui first looks for the closest static sized ancestor (an item whose sizing
    style is not set to 'size_auto'), then reevaluates every single decendent of that item.

    This goes through two steps, evaluation and generation. Evaluation is where ui will position and size the item based on its style, as well as all 
    of the item's children, recursively. This stage is handled entirely in the function eval_item_branch(). This stage sets the uiItem's internal 
    size and position information. Immediately after, we regenerate all the items that were changed. Generate is where ui renders each item, and this
    is performed in draw_item_branch().

Custom Items
------------

    The ui system allows you to set function pointers on uiItem to change its behavoir. The two main function pointers are '__evaluate()' and 
    '__generate()'.

    __generate():
        This function is REQUIRED by ALL uiItems, as this is what tells ui how to render an item. This function is expected to use the drawcmds and 
        allocated memory to create vertexes and indexes that ui will pass to the renderer.

    __evaluate():
        This optional function will be called during an item's evaluation, just after its sizing (with scale) has been determined and right before 
        it begins to call evaluate on its children. The purpose of this function is to allow the user to adjust an item's size and bounds, affecting 
		how other items position and size themselves relative to it. Generally, this is where you will tell ui how large the item is due to any custom 
        rendering you may implement. Unless you know what you're doing, you should size the item so that it contains all custom rendering within
        itself.

    There are a couple other optional function pointers as well:

    __update():
        This function pointer behaves just like the 'action' function pointer. When update_trigger is set to a value other than 'action_act_never', 
		ui will call the function when the trigger is satified. This function serves as a way for a custom item to define custom functionality without
		using the action point, so that a user may use it instead.

	__hash():
		If you make a custom item that has variables you'd like to act as style properties, you can tell ui how to hash these properties using 
		the __hash function pointer. 


TODOs
-----
possibly implement a system for suppressing certain errors on some behavoirs or just for suppressing all errors

Implement a system for trimming down how much we have to do to check every item. Currently we have to walk the entire item tree to check every item,
    but this can be linearized into an array that stores uiItem*'s in the order we would have walked it. This also reduces the potentially massive
    amount of recursion that ui can go into.


Item Style
----------

    uiItems may be passed a uiStyle object or a css-style string to determine
    the style an item takes on. Following is docs about each property. Each property
    lists it's valid vaules in both programmatic and string form followed by an example.

    //TODO(sushi) eventually convert this to HTML or markdown for easier viewing

------------------------------------------------------------------------------------------------------------
*   positioning 
    ---
    Determines how a uiItem is positioned.

-   Example:
      in code:   uiStyle style; style.positioning = pos_fixed;
      in string: positioning: static;

-   Values: 
		pos_static  |  static 
			Default value. 
			The item will be placed normally and top, bottom, left, and right values dont affect it. The first
        	item placed in this manner will be positioned at 0,0 plus its margin and borders, then items placed
			after it are placed below it, as it would be in HTML or ImGui.

        pos_relative  |  relative
            The position values will position the item relative to where it would have 
            normally been placed with pos_static. This removes the item from the normal flow, 
            but items added after it will be placed as if the item was where it would have been. 

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
    Determines how an item is sized. This is a flagged value, meaning it can take on multiple of these properties. 

    Percent only applies when the parent's sizing is not size_auto, otherwise the item will fallback to normal sizing.

    Since structs default to 0, this means an item's size is also 0 both ways, so an item must always be either given an explicit size, or have it's
    sizing set to size_auto to be able to see it and its children! 

-   Values: size_normal (default) The item's width and height will be set to its style 'size' property.

        size_auto_x
            The item will automatically set its width to contain all of its children.

        size_auto_y
            The item will automatically set its height to contain all of its children.

        size_auto
            Combination of size_auto_x and size_auto_y

        size_percent_x 
            Sets the items width as a percentage of its parents width.

        size_percent_y 
            Sets the items height as a percentage of its parent's height.

        size_percent   
            Combination of size_percent_x and size_percent_y.

        size_square
            Keeps the item at a 1:1 aspect ratio. This requires that either height or width are set to auto, while
            the other has a specified value. If both values are specified, then this value is ignored

        size_flex
            Indicates that the item is to be considered in a flex container. This overrides the item's width or height
            (depending on if display is set to row or column) to be a ratio to other flex items in the container.

------------------------------------------------------------------------------------------------------------
*   size, width, height
    ---
    Determines the size of the item. Note that text is not affected by this property, and its size is always as it appears on the screen. If you want
    to change text size use font_height.

    This is the size of the area in which items can be placed minus padding. Note that if margins or borders are specified, this increases the total
    area an item takes up visually.

    If sizing is set to size_flex, then one of these values represents a flux ratio. If display is set to display_vertical, then height is the ratio
    of this item's height to any other flex item in the container's height, and similarly for display_horizontal and width. See display for more
    information.

-   Example: uiStyle style; style.width = 30; // width of 30 pixels style.height = 20; // height of 20 pixels

------------------------------------------------------------------------------------------------------------
*   min_width, min_height, max_width, max_height
    ---
    Determins the minimum and maximum size of the item. The minimum and maximum are applied to the the final size of the item including: padding,
    border, and margin.

    Note, max_width overrides width, and min_width overrides max_width. Same with height.

-   Defaults: max_width and max_height both default to 0, meaning they are not applied. min_width and min_height both default to 0.

-   Example: in code: uiStyle style; style.size = Vec2(10, 6); style.margin = Vec4(1, 0, 1, 0); style.border_width = 1; style.max_width = 10; //final
        content width will be 6 style.min_height = 10; //final content height will be 10 in string: size: 10px 6px; margin: 1px 0px 1px 0px;
        border_width: 1px; max_width: 10px; //content width will be 6px, overall width will be 10px min_height: 10px; //content height will be 10px,
        overall height will be 10px


------------------------------------------------------------------------------------------------------------
*   margin, margintl, marginbr, margin_top, margin_bottom, margin_left, margin_right
    ---
    Determines the spacing between a child's edges and its parent's/sibling's edges.

    margin is a vec4 that represents all sides, and so can be used to set all values on a single line 

        margin = {margin_top, margin_left, margin_bottom, margin_right};

    margintl is margin_top and margin_left, and marginbr is similar.

     ┌----------------------------┑
     |  |─────────────────────────── margin_top
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


------------------------------------------------------------------------------------------------------------
*   padding, paddingtl, paddingbr, padding_top, padding_bottom, padding_left, padding_right
    ---
    Determines the spacing between an item's edges and its content's edges. 

    padding is a vec4 that represents all sides, so it can be used to set all values on a single line

        padding = {padding_top, padding_left, padding_bottom, padding_right};

    paddingtl is padding_top and padding_left, and paddingbr is similar.


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


------------------------------------------------------------------------------------------------------------
*   scale, x_scale, y_scale
    ---
    Scales an item's size, padding, border, margin, and children items.

-   Notes:
      Text is only affected by y_scale.
      When scaling, scale on children items is scaled before application by its parent's scale.
      Scale applies from the anchor, not from the center.
      User is responsible for handling scale in custom __generate functions.

-   Defaults: x_scale and y_scale both default to 0 which means no scaling.

-   Dev Notes:
      Scale is applied during the generation step of UI. Sizes after scaling should be floored for pixel consistency.
      TODO(delle) scale item parts individually, currently scales everything at the end (size{48,48},border{1,1,1,1} results in total size of {75,75} when it should be {74,74})
      TODO(delle) support x_scale on text
      TODO(delle) respect min and max size

-   Example:
      in code:
        uiStyle style;
        style.size = Vec2(20, 20);
        style.max_height = 25; //DOESNT WORK CURRENTLY
        style.border_style = border_solid;
        style.border_color = Color_Black;
        style.border_width = 1; //final border width will be 1px because it's floored
        style.scale = Vec2(1.5, 1.5); //final total width will be 32px, content width will be 30px
      in string:
        size: 20px 20px;
        max_height: 25px; //DOESNT WORK CURRENTLY
        border: solid 1px; //final border width will be 1px because it's floored
        scale: 1.5 1.5; //final total width will be 32px


------------------------------------------------------------------------------------------------------------
*   scroll, scrollx, scrolly
    ---
    Scrolls the window when items overflow if overflow is set to allow it.

-   Defaults: Both values default to 0

-   Catches: Scrolling conflicts with setting content_align.

TODO(sushi) example

------------------------------------------------------------------------------------------------------------
*   content_align
    ---
    Determines how to align an item's contents over each axis. This only works on children who's positioning is static or relative. This respects
    margins and padding, content will only be aligned within their valid space. Negative values are not valid. Values larger than 1 are not valid.
    NotImplemented -- This does not apply when alignment is set to inline.

-   Performance: In order to properly do this (as far as I can tell), we must reevaluate all children of an item after having already evaluated them
        because the size of the group of items is not known until all of them have been evaluated. So this more or less doubles the evaluation time of
        every item that uses it.

-   Defaults: Defaults to 0

TODO(sushi) example

------------------------------------------------------------------------------------------------------------
*   content_advance
    ---
    Determines the spacing between child items when they are laid out. This value is added after each child's width/height when advancing the cursor
    position for the next item. For horizontal layouts (display_horizontal), this adds spacing after each item's width. For vertical layouts, this
    adds spacing after each item's height.

-   Defaults: Defaults to 0

-   Example:
      in code:
        uiStyle style;
        style.content_advance = 5; // adds 5 pixels between each child item
      in string:
        content_advance: 5px; // adds 5 pixels between each child item

------------------------------------------------------------------------------------------------------------
*   font
    ---
    Determines the font to use when rendering text. Being a text property, this property is inherited from the item's parent if another style is not
    provided at creation.

TODO(sushi) example

------------------------------------------------------------------------------------------------------------
*   font_height
    ---
    Determines the visible height in pixels of rendered text. Note that this does not have to be the same height the font was loaded at, though it may
    not render as pretty if it isnt.

TODO(sushi) example

------------------------------------------------------------------------------------------------------------
*   background_color
    ---
    Determines the background color of a uiItem. Custom items don't have to implement this and may not.

------------------------------------------------------------------------------------------------------------
*   border_style
    ---
    Determines the style of border a uiItem has

    //TODO(sushi) border_style docs

------------------------------------------------------------------------------------------------------------
*   border_width
    ---
    Determines the width of a uiItem's border. This has no affect if border_style is not set.

------------------------------------------------------------------------------------------------------------
*   text_color
    ---
    Determines the color of text

    //TODO(sushi) text_color docs

------------------------------------------------------------------------------------------------------------
*   overflow
    ---
    Determines how items that go out of their parent's bounds are displayed.

-  Values: overflow_scroll (default) When items extend beyond the parents borders, the items are cut off and scroll bars are shown.

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

-   When using this on an item whose positioning is not fixed, it will affect the positioning of items in the same item as it. This is
        because focusing an item will make it the last child of the parent, so that it is rendered on top of its siblings.
-   When multiple sibling items have the focus flag, the latest-added sibling will be favored.


------------------------------------------------------------------------------------------------------------
*   display
    ---
    Determines how to display an item's children. This is a flagged value, meaning it can take on several different values. For example using
    'display_vertical | display_reverse' will display children in a column but in the reverse order that they were added.

-   Values: display_vertical (default) Displays children from top to bottom. This is mutually exclusive with display_horizontal.

        display_horizontal
            Displays children from left to right. This is mutually exclusive with display_vertical.

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
            Displays the containers items in reverse order, eg with display_horizontal it will display items from 
            right to left. Not from the right side of the container to the left though, this just specifies to
            draw items in the reverse order they were added. If you want to draw items from the right side consider using
            anchors or content align.

        display_hidden
            Doesnt display the parent or its children.

-   Technical: Since we require uiStyle's default to be 0, and I dont want to explicitly set display to column every single time I make an item,
        display_vertical is equal to 0. This sort of makes it so that the first bit is a boolean determining if we are drawing a row or column. This
        is beneficial because it enforces row and column being mutually exclusive.

------------------------------------------------------------------------------------------------------------
*   hover_passthrough
    ---
    Flag that determines if an item should pass its hover status to its parent.

-   Defaults: Defaults to false.
*/
#ifndef DESHI_UI2_H
#define DESHI_UI2_H
#include "kigu/color.h"
#include "kigu/common.h"
#include "kigu/map.h"
#include "kigu/node.h"
#include "kigu/unicode.h"
#include "math/vector.h"
#include "core/graphics.h"
#include "core/text.h"
#include "core/input.h"
#include "kigu/hash.h"
struct Font;
struct Texture;


#define UI_UNIQUE_ID(str) str8_static_hash64({str,sizeof(str)})

#define UI_HASH_SEED 2166136261 // 32bit FNV_offset_basis (FNV-1a)
#define UI_HASH_PRIME 16777619 // 32bit FNV_prime (FNV-1a)
#define UI_HASH_VAR(x) seed ^= (u64)(x); seed *= UI_HASH_PRIME

struct uiVertex {
	vec2 pos;
	vec2 uv;
	u32  color;
};
typedef u32 uiIndex;

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_keybinds


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
	
	KeyCode drag_item;
};

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_drawcmd


struct uiDrawCmd{
	Node node;
	Texture* texture;
	u32 vertex_offset; 
	u32 index_offset;
	vec2i counts_reserved; //x: vertex, y: index
	vec2i counts_used; //x: vertex, y: index
	b32 freed;
	str8 removed_by;
	str8 alloced_by;
};

// allocates `count amount of uiDrawCmds generically 
uiDrawCmd* ui_make_drawcmd(upt count);

// deletes the given uiDrawCmd
// it is not recommended to call this on any drawcmd that
// you do not have total control over, eg. if it is a drawcmd from an 
// item that you did not explicitly construct yourself
// you should use ui_drawcmd_remove instead
void ui_drawcmd_delete(uiDrawCmd* dc);

// removes the given uiDrawCmd, reserving the memory it points to 
// for use with later drawcmds. See the implementation function
// for further info.
void ui_drawcmd_remove(uiDrawCmd* drawcmd);

void ui_drawcmd_alloc(uiDrawCmd* drawcmd, vec2i counts);

struct uiDrawCmdPtrs {
	uiVertex* vertexes;
	uiIndex* indexes;
};

// retrieves the start of the given drawcmds drawinfo in 
// ui's drawinfo arenas
uiDrawCmdPtrs ui_drawcmd_get_ptrs(uiDrawCmd* dc);

// if needed, reallocates the drawinfo of the given drawcmd
// this does not copy the previous information and is meant to be used 
// in cases where you are regenerating an item and need more memory for
// its drawinfo
uiDrawCmdPtrs ui_drawcmd_realloc(uiDrawCmd* dc, vec2i counts);

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_style


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
	
	display_vertical    = 0,
	display_horizontal  = 1 << 0,
	display_flex        = 1 << 1,
	display_reverse     = 1 << 2,
	display_hidden      = 1 << 3,
	
	// NOTE(sushi) I've changed these to be flagged due to needing to support hash_change
	//             and mouse actions on the same item. Be careful working with these since now 
	//             setting one flag could totally override another. This is also nice 
	//             because you can do something on just press and release.
	// TODO(sushi) update documentation to reflect this
	action_act_never                = 0,
	action_act_mouse_hover          = 1 << 0, // call action when the mouse is positioned visually over the item
	action_act_mouse_hover_children = 1 << 1, // call action when the mouse is positioned over any of the children of the item
	action_act_mouse_pressed        = 1 << 2, // call action when the mouse is pressed over the item
	action_act_mouse_released       = 1 << 3, // call action when the mouse is released over the item
	action_act_mouse_down           = 1 << 4, // call action when the mouse is down over the item
	action_act_mouse_scroll         = 1 << 5, // call action when the mouse wheel is moved in either direction
	action_act_always               = 1 << 6, // call action every frame
	action_act_hash_change          = 1 << 7, // call action when the item's hash changes
	
};

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
		struct{f32 x_scale, y_scale;};
		vec2 scale;
	};
	union{
		struct{f32 scrx, scry;};
		vec2 scroll;
	};
	color background_color;
	Texture* background_image;
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
	f32   content_advance;
	b32   hover_passthrough;
	
	void operator=(const uiStyle& rhs){ memcpy(this, &rhs, sizeof(uiStyle)); }
};


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_item


struct uiItem{
	TNode node;
	Node  link; // circular list connecting all existing items to base for iterating
	str8  id; //NOTE(sushi) mostly for debugging, not sure if this will have any other use in the interface
	uiStyle style;
	u64 userVar; // variable never touched internally, for user use;
	
	//an items action call back function 
	//this function is called in situations defined by the flags in the in the uiStyle enum
	//and is always called before anything happens to the item in ui_update
	void (*action)(uiItem*);
	void* action_data; //a pointer to arbitrary data to be accessed in the action callback
	Type action_trigger; //how the action is triggered
	
	//// INTERNAL ////
	u32 style_hash;
	
	vec2 pos_local;  // position relative to parent
	vec2 pos_screen; // position relative to screen
	
	union{ // size that the item visually takes up on the screen (before scaling)
		struct{ f32 width, height; };
		vec2 size;
	};
	union{ // scale after applying parent scale
		struct{ f32 x_scale, y_scale; };
		vec2 scale;
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

	GraphicsDescriptorSet* descriptor_set;
	
	// TODO(sushi) we may want to pull the following functions out into a 
	//             table stored else where, like vtables. Performance 
	//             shouldn't take much of a hit since the system is focused
	//             on retained ui and this stuff shouldn't be invoked 
	//             very frequenty (probably)

	// called according to 'update_trigger' during ui_update
	// (optional)
	void    (*__update)(uiItem*);
	// determines how the item is sized and positioned 
	// relative to other items
	// (optional)
	void    (*__evaluate)(uiItem*);
	// generates draw information for rendering the item
	// (required)
	void    (*__generate)(uiItem*);
	// determines how to hash a custom item
	// (optional) but without this the item won't dynamically respond to style changes 
	u32     (*__hash)(uiItem*);
	// cleans up anything the item may leave behind upon deletion
	// (optional) but please implement this if you allocate data
	void    (*__cleanup)(uiItem*);
	// called when an item is copied. it's up to the widget whether
	// they share certain memory or not (such as two text objects sharing the same text buffer)
	// (required if you want to use deep_copy)
	uiItem* (*__copy)(uiItem*);
	
	str8 file_created;
	upt  line_created;
	
	//size of the item in memory and the offset of the item member on widgets 
	u64 memsize;
	
	//indicates if the item has been cached or not.
	//this only applies to immediate mode items 	
	b32 cached;
	
	// breaks the program when this item is encountered in ui_update
	b32 break_on_update;
	b32 break_on_evaluate;

	Stopwatch since_last_update;
		
	struct{
		u32 evals;
		u32 draws;
	}debug_frame_stats;
	
	void operator=(const uiItem& rhs){memcpy(this, &rhs, sizeof(uiItem));}
};

struct uiItemSetup{
	upt size; 
	uiStyle* style; 
	str8 file; 
	upt line; 
	Type update_trigger; 
	void    (*update)(uiItem*); 
	void    (*generate)(uiItem*); 
	void    (*evaluate)(uiItem*);
	u32     (*hash)(uiItem*);
	uiItem* (*copy)(uiItem*);
	
	vec2i* drawinfo_reserve;
	u32 drawcmd_count;
};

#define uiItemFromNode(x) CastFromMember(uiItem, node, x)

global uiItem* 
ui_item_from_link(Node* n) {
	return CastFromMember(uiItem, link, n);
}

//Hashes the uiItem's style and merges it with the result of a custom hash function if one is set
inline u32 ui_hash_style(uiItem* item){DPZoneScoped;
	uiStyle* s = &item->style;
	u32 seed = UI_HASH_SEED;
	seed ^= *(u32*)&s->positioning;     seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->sizing;          seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->anchor;          seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->x;               seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->y;               seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->width;           seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->height;          seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->margin_left;     seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->margin_top;      seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->margin_bottom;   seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->margin_right;    seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->padding_left;    seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->padding_top;     seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->padding_bottom;  seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->padding_right;   seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->x_scale;         seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->y_scale;         seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->content_align.x; seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->content_align.y; seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->content_advance; seed *= UI_HASH_PRIME;
	seed ^= (u64)s->font;               seed *= UI_HASH_PRIME;
	seed ^= s->font_height;             seed *= UI_HASH_PRIME;
	seed ^= s->background_color.rgba;   seed *= UI_HASH_PRIME;
	seed ^= s->border_style;            seed *= UI_HASH_PRIME;
	seed ^= s->border_color.rgba;       seed *= UI_HASH_PRIME;
	seed ^= *(u32*)&s->border_width;    seed *= UI_HASH_PRIME;
	seed ^= s->text_color.rgba;         seed *= UI_HASH_PRIME;
	seed ^= s->overflow;                seed *= UI_HASH_PRIME;
	seed ^= s->focus;                   seed *= UI_HASH_PRIME;
	
	if(item->__hash){
		seed ^= item->__hash(item);
		seed *= UI_HASH_PRIME;
	}
	
	return seed;
}

enum {
	// returns true only if the mouse is in the area
	// of the item and isn't blocked by anything else
	hovered_strict,
	// returns true if the mouse is over any of the
	// children of the item
	hovered_child,
	// returns true if the mouse is in the item's
	// area at all
	hovered_area,
};
b32 ui_item_hovered(uiItem* item, u32 mode = hovered_strict);

// helper to make an item hidden
inline void 
ui_item_hide(uiItem* item) {
	AddFlag(item->style.display, display_hidden);
}

// helper to reveal an item
inline void 
ui_item_show(uiItem* item) {
	RemoveFlag(item->style.display, display_hidden);
}

// base implementation of uiItem copying
// this should be used by widgets implementing __copy
// instead of trying to copy the data manually
// expects the new uiItem to have already been allocated
// this is a *shallow copy*, it does not copy the items 
// children, the node of the new item will be zeroed
// file_created and line_created will be zeroed and it's up
// to the caller to set them if desired
void ui_item_copy_base(uiItem* to, uiItem* from);


#if BUILD_SLOW
#define RELEASE_INLINE
#else
#define RELEASE_INLINE FORCE_INLINE
#endif


RELEASE_INLINE global f32 
ui_margined_width(uiItem* item) {
	return item->width - item->style.margin_left - item->style.margin_right;
}

RELEASE_INLINE global f32
ui_margined_height(uiItem* item) {
	return item->height - item->style.margin_top - item->style.margin_bottom;
}

RELEASE_INLINE global vec2
ui_margined_area(uiItem* item) {
	return {ui_margined_width(item), ui_margined_height(item)};
}

RELEASE_INLINE global f32 
ui_margined_style_width(uiItem* item) {
	return item->style.width - item->style.margin_left - item->style.margin_right;
}

RELEASE_INLINE global f32
ui_margined_style_height(uiItem* item) {
	return item->style.height - item->style.margin_top - item->style.margin_bottom;
}

RELEASE_INLINE global vec2
ui_margined_style_area(uiItem* item) {
	return vec2{ui_margined_style_width(item), ui_margined_style_height(item)};
}

RELEASE_INLINE global f32
ui_bordered_width(uiItem* item) {
	return ui_margined_width(item) - (item->style.border_style ?  2 * item->style.border_width : 0);
}

RELEASE_INLINE global f32
ui_bordered_height(uiItem* item) {
	return ui_margined_height(item) - (item->style.border_style ? 2 * item->style.border_width : 0);
}

RELEASE_INLINE global vec2
ui_bordered_area(uiItem* item) {
	return vec2{ui_bordered_width(item), ui_bordered_height(item)};
}

RELEASE_INLINE global f32
ui_bordered_style_width(uiItem* item) {
	return item->style.width + (item->style.border_style ? 2 * item->style.border_width : 0);
}


RELEASE_INLINE global f32
ui_bordered_style_height(uiItem* item) {
	return item->style.height + (item->style.border_style ? 2 * item->style.border_width : 0);
}

RELEASE_INLINE global vec2
ui_bordered_style_area(uiItem* item) {
	return vec2{ui_bordered_style_width(item), ui_bordered_style_height(item)};
}

RELEASE_INLINE global f32
ui_padded_width(uiItem* item) {
	return ui_bordered_width(item) - item->style.padding_left - item->style.padding_right;
}

RELEASE_INLINE global f32
ui_padded_height(uiItem* item) {
	return ui_bordered_height(item) - item->style.padding_top - item->style.padding_bottom;
}

RELEASE_INLINE global vec2
ui_padded_area(uiItem* item) {
	return vec2{ui_padded_width(item), ui_padded_height(item)};
}

RELEASE_INLINE global f32
ui_padded_style_width(uiItem* item) {
	return item->style.width - item->style.padding_left - item->style.padding_right;
}

RELEASE_INLINE global f32
ui_padded_style_height(uiItem* item) {
	return item->style.height - item->style.padding_top - item->style.padding_top;
}

RELEASE_INLINE global vec2
ui_padded_style_area(uiItem* item) {
	return vec2{ui_padded_style_width(item), ui_padded_style_height(item)};
}


// invokes __copy recursively on the item's children and finally 
// itself to produce a full copy of the item's branch.
// when finished the new item appends itself to the original
// item's parent.
// returns the newly created item
uiItem* ui_deep_copy(uiItem* item);

uiItem* deshi__ui_make_item(uiStyle* style, str8 file, upt line);
#define ui_make_item(style) deshi__ui_make_item((style), str8l(__FILE__), __LINE__)

uiItem* deshi__ui_begin_item(uiStyle* style, str8 file, upt line);
#define ui_begin_item(style) deshi__ui_begin_item((style), str8l(__FILE__), __LINE__)

void deshi__ui_end_item(str8 file, upt line);
#define ui_end_item() deshi__ui_end_item(STR8(__FILE__),__LINE__)

void deshi__ui_remove_item(uiItem* item, str8 file, upt line);
#define ui_remove_item(item) deshi__ui_remove_item((item), str8l(__FILE__), __LINE__)

uiItem* ui_setup_item(uiItemSetup setup, b32* retrieved = 0);

// pushes an item onto the global item stack for appending to windows that have already been ended.
void deshi__ui_push_item(uiItem* item, str8 file, upt line);
#define ui_push_item(item) deshi__ui_push_item((item), str8l(__FILE__), __LINE__)

// pops 'count' items from the global item stack and returns the last item popped.
uiItem* deshi__ui_pop_item(u32 count, str8 file, upt line);
#define ui_pop_item(count) deshi__ui_pop_item((count), str8l(__FILE__), __LINE__)

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_generate


vec2i ui_gen_background(uiItem* item, uiVertex* vp, u32* ip, vec2i counts);

vec2i ui_gen_border(uiItem* item, uiVertex* vp, u32* ip, vec2i counts);


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_immediate


void deshi__ui_begin_immediate_branch(uiItem* parent, str8 file, upt line);
#define ui_begin_immediate_branch(parent) deshi__ui_begin_immediate_branch((parent), str8l(__FILE__), __LINE__)

void deshi__ui_end_immediate_branch(str8 file, upt line);
#define ui_end_immediate_branch() deshi__ui_end_immediate_branch(str8l(__FILE__), __LINE__)

void deshi__ui_push_id(s64 x, str8 file, upt line);
#define ui_push_id(x) deshi__ui_push_id((x), str8l(__FILE__), __LINE__)

void deshi__ui_pop_id(str8 file, upt line);
#define ui_pop_id() deshi__ui_pop_id(str8l(__FILE__), __LINE__)


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @ui_context


void deshi__ui_init(Window* window);
#define ui_init(window) deshi__ui_init(window)

void deshi__ui_update(Window* window);
#define ui_update(window) deshi__ui_update(window)

typedef u32 uiInputState; enum{
	uiISNone,
	uiISScrolling,
	uiISDragging,
	uiISResizing,
	uiISPreventInputs,
	uiISExternalPreventInputs,
};

struct uiContext{
	
	//// state ////
	// ultimate parent of any uiItem in the tree.
	// 'link' on base serves as a circular list connecting 
	// all uiItems in a linear fashion for easy iteration.
	// However, this list is in no specific order.
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
		u32 id; 
	}immediate;
	
	b32 updating; //set true while ui_update is running
	
	//// memory ////
	arrayT<uiItem*> immediate_items;
	Node inactive_drawcmds; //list of drawcmds that have been removed and contain info about where we can allocate data next
	arrayT<uiDrawCmd*> inactive_drawcmds_vertex_sorted;
	arrayT<uiDrawCmd*> inactive_drawcmds_index_sorted;
	
	// TODO(sushi) because we have item_push/pop now, we should store file/line that pushed the item
	//             so that we can report it where things go wrong
	arrayT<uiItem*> item_stack; //TODO(sushi) eventually put this in it's own arena since we can do a stack more efficiently in it
	
	struct {
		GraphicsBuffer* handle;
		// offset to the end of the total amount of space we have allocated so far
		// not accounting for freed vertices
		u64 cursor;
	} vertex_buffer, index_buffer;

	GraphicsRenderPass* render_pass;
	GraphicsFramebuffer* framebuffer;
	GraphicsPipeline* pipeline;
	GraphicsPushConstant push_constant;
	GraphicsDescriptorSet* blank_descriptor_set;

	Window* updating_window;

	Texture* last_texture;

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
	
	// root of nodes used to track memory that ui has allocated
	Node allocator_root;
	
	//// other ////
	uiKeybinds keys;
};
extern uiContext* g_ui; //global UI pointer

// header for things allocated with deshi_ui_allocator
// allows things in ui to detect if certain things were allocated using 
// this allocator and to remove it if so 
struct uiMemoryHeader {
	Node node;
	u32 magic;
};
#define ui_memory_header(ptr) ((uiMemoryHeader*)(ptr) - 1)
#define is_ui_memory(ptr) (ui_memory_header(ptr)->magic == g_ui->memory_\magic)

const u32 ui_memory_magic = 0xf00f00f0;

global void* 
deshi__ui_memory_reserve(upt size) {
	uiMemoryHeader* header = (uiMemoryHeader*)memalloc(sizeof(uiMemoryHeader) + size);
	header->magic = ui_memory_magic;
	NodeInsertPrev(&g_ui->allocator_root, &header->node);
	return (void*)(header+1);
}

global void 
deshi__ui_memory_release(void* ptr) {
	uiMemoryHeader* header = ui_memory_header(ptr);
	Assert(header->magic == ui_memory_magic, "attempted to release memory that does not belong to ui.");
	NodeRemove(&header->node);
	memzfree(header);
}

global void* 
deshi__ui_memory_resize(void* ptr, upt size) {
	// TODO(sushi) for some reason this causes a heap error in memory
	FixMe;
	uiMemoryHeader* header = ui_memory_header(ptr);
	Assert(header->magic == ui_memory_magic, "attempted to resize memory that does not belong to ui.");
	NodeRemove(&header->node);
	header = (uiMemoryHeader*)memrealloc(header, sizeof(uiMemoryHeader)+size);
	return (void*)(header+1);
}

global void
ui_set_active_item(uiItem* item){
	g_ui->active = item;
}

// A generic allocator for memory that should be considered owned and managed by
// the ui system. Currently the main use of this is for dynamically allocated uiItem ids. 
// This prefixes the memory with a header used to identify memory belonging to ui. If a uiItem's
// id is found to be a string allocated by this allocator, it will be released when the item is 
// deleted.
global Allocator _deshi_ui_allocator {
	deshi__ui_memory_reserve,
	deshi__ui_memory_release,
	deshi__ui_memory_resize
};
global Allocator* deshi_ui_allocator = &_deshi_ui_allocator;

// Creates a window containing information and tools for debugging ui
// Very far from finished. 
void ui_toggle_debug_window();

//Creates the demo window (or destroys if already created)
void ui_toggle_demo_window();

void ui_print_tree(void (*info)(dstr8*,uiItem*));


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// @make
// TODO(sushi) I feel like these are more appropriate somewhere else, but I think it's only used by ui atm anyways
//             so it will just be here for now since I need to get things out of the old render module.


FORCE_INLINE vec2i ui_put_line_counts()                  {return { 4, 6};};
FORCE_INLINE vec2i ui_put_filledtriangle_counts()        {return { 3, 3};};
FORCE_INLINE vec2i ui_put_triangle_counts()              {return {12,18};};
FORCE_INLINE vec2i ui_put_filledrect_counts()            {return { 4, 6};};
FORCE_INLINE vec2i ui_put_rect_counts()                  {return {16,24};};
FORCE_INLINE vec2i ui_put_circle_counts(u32 subdiv)      {return {2*(s32)subdiv,6*(s32)subdiv};};
FORCE_INLINE vec2i ui_put_filledcircle_counts(u32 subdiv){return {1+(s32)subdiv,3*(s32)subdiv};};
FORCE_INLINE vec2i ui_put_text_counts(u32 charcount)     {return {4*(s32)charcount,6*(s32)charcount};};
FORCE_INLINE vec2i ui_put_texture_counts()               {return { 8,24};};

vec2i ui_put_line(uiVertex* putverts, uiIndex* putindices, vec2i offsets, vec2 start, vec2 end, f32 thickness, color color);
vec2i ui_put_filledtriangle(uiVertex* putverts, uiIndex* putindices, vec2i offsets, vec2 p1, vec2 p2, vec2 p3, color color);
vec2i ui_put_triangle(uiVertex* putverts, uiIndex* putindices, vec2i offsets, vec2 p0, vec2 p1, vec2 p2, f32 thickness, color color);
vec2i ui_put_filledrect(uiVertex* putverts, uiIndex* putindices, vec2i offsets, vec2 pos, vec2 size, color color);
vec2i ui_put_rect(uiVertex* putverts, uiIndex* putindices, vec2i offsets, vec2 pos, vec2 size, f32 thickness, color color);
vec2i ui_put_circle(uiVertex* putverts, uiIndex* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, f32 thickness, color color);
vec2i ui_put_filledcircle(uiVertex* putverts, uiIndex* putindices, vec2i offsets, vec2 pos, f32 radius, u32 subdivisions_int, color color);
vec2i ui_put_text(uiVertex* putverts, uiIndex* putindices, vec2i offsets, str8 text, Font* font, vec2 pos, color color, vec2 scale);
vec2i ui_put_texture(uiVertex* putverts, uiIndex* putindices, vec2i offsets, Texture* texture, vec2 p0, vec2 p1, vec2 p2, vec2 p3, f32 alpha, b32 flipx, b32 flipy);



#endif //DESHI_UI2_H
