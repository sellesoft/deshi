
#ifndef scalar_t
#define scalar_t f64
#endif

#include "ui.h"
#include "window.h"
#include "math/math.h"
#include "kigu/color.h"
#include "kigu/common.h"
#include "kigu/profiling.h"


//TODOs
// clamp axes to edge of camera (when out of frame or always)

enum GraphAxesLabelStyle : u32{
    GraphAxesLabelStyle_OnAxes,       //draws the axes labels next to their respective axes
    //TODO GraphAxesLabelStyle_OnGraphEdges, //draws the axes labels at the edges of the graph
};

//vec2 used specifically by graph to support different data types
struct vec2g{
    union{
        scalar_t arr[2]={};
        struct {scalar_t x,y;}; //TODO consider adding more common math independent/dependent var names
    };

    vec2g(){};
    vec2g(scalar_t _x, scalar_t _y){x=_x;y=_y;}
    vec2g(const vec2& v){x=v.x;y=v.y;}

    void  operator= (const vec2g& rhs)                {this->x = rhs.x; this->y = rhs.y;}
	vec2g operator* (f32 rhs) const                   {return vec2g(this->x * rhs, this->y * rhs);}
	void  operator*=(f32 rhs)                         {this->x *= rhs; this->y *= rhs;}
	vec2g operator/ (f32 rhs) const                   {return vec2g(this->x / rhs, this->y / rhs);}
	void  operator/=(f32 rhs)                         {this->x /= rhs; this->y /= rhs;}
	vec2g operator+ (const vec2g& rhs) const          {return vec2g(this->x + rhs.x, this->y + rhs.y);}
	void  operator+=(const vec2g& rhs)                {this->x += rhs.x; this->y += rhs.y;}
	vec2g operator- (const vec2g& rhs) const          {return vec2g(this->x - rhs.x, this->y - rhs.y);}
	void  operator-=(const vec2g& rhs)                {this->x -= rhs.x; this->y -= rhs.y;}
	vec2g operator* (const vec2g& rhs) const          {return vec2g(this->x * rhs.x, this->y * rhs.y);}
	void  operator*=(const vec2g& rhs)                {this->x *= rhs.x; this->y *= rhs.y;}
	vec2g operator/ (const vec2g& rhs) const          {return vec2g(this->x / rhs.x, this->y / rhs.y);}
	void  operator/=(const vec2g& rhs)                {this->x /= rhs.x; this->y /= rhs.y;}
	vec2g operator- () const                          {return vec2g(-x, -y);}
	bool operator==(const vec2g& rhs) const           {return abs(this->x - rhs.x) < M_EPSILON && abs(this->y - rhs.y) < M_EPSILON;}
	bool operator!=(const vec2g& rhs) const           {return !(*this == rhs);}
	friend vec2g operator* (f32 lhs, const vec2g& rhs){return rhs * lhs;}
   	
	static const vec2g ZERO;
	static const vec2g ONE;
	static const vec2g UP;
	static const vec2g DOWN;
	static const vec2g LEFT;
	static const vec2g RIGHT;
	static const vec2g UNITX;
	static const vec2g UNITY;
};

inline const vec2g vec2g::ZERO =  vec2g( 0, 0);
inline const vec2g vec2g::ONE =   vec2g( 1, 1);
inline const vec2g vec2g::RIGHT = vec2g( 1, 0);
inline const vec2g vec2g::LEFT =  vec2g(-1, 0);
inline const vec2g vec2g::UP =    vec2g( 0, 1);
inline const vec2g vec2g::DOWN =  vec2g( 0,-1);
inline const vec2g vec2g::UNITX = vec2g( 1, 0);
inline const vec2g vec2g::UNITY = vec2g( 0, 1);

inline vec2::
vec2(const vec2g& v){//yeah
	x=v.x; y=v.y;
}

template<> FORCE_INLINE vec2g Min(vec2g a, vec2g b)                         { return vec2g(Min(a.x, b.x), Min(a.y, b.y));}
template<> FORCE_INLINE vec2g Max(vec2g a, vec2g b)                         { return vec2g(Max(a.x, b.x), Max(a.y, b.y)); }
template<> FORCE_INLINE vec2g Clamp(vec2g value, vec2g min, vec2g max)      { return vec2g(Clamp(value.x, min.x, max.x), Clamp(value.y, min.y, max.y)); };
template<> FORCE_INLINE vec2g ClampMin(vec2g value, vec2g min)              { return vec2g(ClampMin(value.x, min.x), ClampMin(value.y, min.y)); };
template<> FORCE_INLINE vec2g ClampMax(vec2g value,  vec2g max)             { return vec2g(ClampMax(value.x, max.x), ClampMax(value.y, max.y)); };
template<> FORCE_INLINE vec2g Nudge(vec2g value, vec2g target, vec2g delta) { return vec2g(Nudge(value.x, target.x, delta.x), Nudge(value.y, target.y, delta.y)); }


struct Graph{
    //TODO this should be vec2f64 (vec2d)
    vec2g     cameraPosition{0,0}; //in graph space, center of 'camera'
	scalar_t  cameraZoom = 5.0; //represets half of the width the camera can see, so if this is 5 and posiiton is 0,0 then you see from -5 to 5
    vec2g     cameraView{5.0,5.0}; //represents how much over the x and y axes we can see
    //Graph properties
    //wether or not to show main axes
    //bulk set using set_axes_visible()
    b32 xShowAxis = true;
    b32 yShowAxis = true;
    //labels per axis
    cstring xAxisLabel;
    cstring yAxisLabel;
    //colors per axis
    //bulk set using set_axes_colors()
    color xAxisColor=Color_White;
    color yAxisColor=Color_White;
    //colos per major gridline
    //bulk set using set_major_gridlines_color() 
    color xMajorGridlineColor=color(70,70,70);
    color yMajorGridlineColor=color(70,70,70);
    //whether ot not to show major gridline coordinates
    //bulk set using set_major_gridline_coords_visible()
    b32 xShowMajorCoords = true;
    b32 yShowMajorCoords = true;
    //colors per minor gridline
    //bulk set using set_minor_gridlines_color() 
    color xMinorGridlineColor=color(40,40,40);
    color yMinorGridlineColor=color(40,40,40);
    //the separation between major gridlines
    //bulk set using set_major_gridlines_increment()
	scalar_t xMajorLinesIncrement   = 1;
	scalar_t yMajorLinesIncrement   = 1;
    //the separation between minor gridlines
    //bulk set using set_minor_gridlines_increment()
	scalar_t xMinorLinesIncrement   = 0.2;
	scalar_t yMinorLinesIncrement   = 0.2;
    //whether or not to show major gridlines
    //bulk set using set_major_gridlines_visible()
	b32      xShowMajorLines     = true;
	b32      yShowMajorLines     = true;
    //whether or not to show minor gridlines
    //bulk set using set_minor_gridlines_visible()
	b32      xShowMinorLines     = true;
	b32      yShowMinorLines     = true;
    //coordinate labels
    //bulk set using set_axes_coords_visible()
	b32      xShowAxisCoords     = true;
	b32      yShowAxisCoords     = true;
    //determines where on the graph axes labels will appear
    GraphAxesLabelStyle axesLabelStyle = GraphAxesLabelStyle_OnAxes;
    //TODO axes label alignemnt options

    //Graph data
    carray<vec2g> data;

    //misc graph properties that may be useful outside of draw_graph
    //these are calculated in draw_graph_final
    vec2 dimensions_per_unit_length;
    scalar_t aspect_ratio;

};

//TODO this works, but at non 1:1 aspect ratios cameraPosition no longer actually represents the center
//     of the 
void draw_graph_final(Graph& g, vec2g position, vec2g dimensions, b32 move_cursor){
    using namespace UI;
    UIItem* item = BeginCustomItem();
    item->position = GetWinCursor();
    item->size = dimensions;
    CustomItem_AdvanceCursor(item, move_cursor);

    color textcol = GetStyle().colors[UIStyleCol_Text];
    color winbgcol = GetStyle().colors[UIStyleCol_WindowBg];

    //DEBUG 
    u32 text_count = 0;
    auto debug_text = [&](string& out){
        UIDrawCmd dc;
        CustomItem_DCMakeText(dc, {out.str, out.count}, vec2(0,text_count*13), Color_White, vec2::ONE);
        CustomItem_AddDrawCmd(item, dc);
        text_count++;
    };
    //DEBUG 

    //graph space
    vec2g     cpos = g.cameraPosition;
    scalar_t czoom = g.cameraZoom;
    
    scalar_t view_width = czoom*2; 
    scalar_t aspect_ratio = dimensions.y/dimensions.x;
    g.aspect_ratio = aspect_ratio;
    //dimensions per unit length
    //gives how far along in screen space 1 unit in graph space is
    vec2g dimspul = vec2g(dimensions.x/view_width, dimensions.x/view_width);
    g.dimensions_per_unit_length = dimspul;

    //TODO get exponent directly from the exponent of float's bits
    //s32 oom = (1 & 0x1 ? 
    //yep & 0x)

    //TODO all of these vec2s need to somehow represent the same type as scalar_t
    vec2g tl = cpos - czoom*vec2g::ONE; //represets the top and left edge of the camera
    vec2g br = cpos + czoom*vec2g::ONE;  //represets the bottom and right edge of the camera
    tl.y *= aspect_ratio;
    br.y *= aspect_ratio;    

    
    b32 xAxisVisible = g.xShowAxis && tl.y < 0 && br.y > 0;
    b32 yAxisVisible = g.yShowAxis && tl.x < 0 && br.x > 0;

    
    //round left edge to nearest order of magnitude multiplied by increment 
    //TODO set this up to only happen when zoom or position change
    scalar_t oom = Math::order_of_magnitude(czoom);
    scalar_t tentooom = pow(10,oom);

    //round each edge to the closest order of magnitude
    vec2g tl_oom_rnd = floor(tl/tentooom)*tentooom;
    vec2g br_oom_rnd = floor(br/tentooom)*tentooom;
    vec2g cpos_oom_rnd = floor(cpos/tentooom)*tentooom;

    vec2g itemspacecenter = dimensions / 2; //positions of the center of our item
    vec2g itemspaceorigin = itemspacecenter - cpos*vec2g(dimspul.x, dimspul.y*aspect_ratio); //position of the center of the origin in item space
 

    persist u32 incmod = 1;
    if(DeshInput->KeyPressed(Key::I)) incmod++;
    if(DeshInput->KeyPressed(Key::K)) incmod = Max(incmod-1, u32(1));

    auto increment = [&](scalar_t x){
        return x*pow(10,oom);
    };


    scalar_t minor_left_edge_rounded = ceil(tl.x / (g.xMinorLinesIncrement * tentooom)) * g.xMinorLinesIncrement * tentooom;
    scalar_t minor_top_edge_rounded  = ceil(tl.y / (g.yMinorLinesIncrement * tentooom)) * g.yMinorLinesIncrement * tentooom;
    scalar_t major_left_edge_rounded = ceil(tl.x / (g.xMajorLinesIncrement * tentooom)) * g.xMajorLinesIncrement * tentooom;
    scalar_t major_top_edge_rounded  = ceil(tl.y / (g.yMajorLinesIncrement * tentooom)) * g.yMajorLinesIncrement * tentooom;
    

    {//draw minor gridlines
        //TODO prevent minor lines from unecessarily drawing where major lines draw when they're enabled
        if(g.xShowMinorLines){
            //this starts at the left edge of the graph, rounded by order of magnitude and increment
            //and keeps drawing lines until it reaches the right edge of the graph
            UIDrawCmd drawCmd;
            scalar_t inc = increment(g.xMinorLinesIncrement);
            scalar_t edgeinc = minor_left_edge_rounded;
            while(edgeinc < br.x){
                scalar_t xloc = (edgeinc - tl.x) * dimspul.x;
                CustomItem_DCMakeLine(drawCmd,
                    vec2g(xloc, 0),
                    vec2g(xloc, dimensions.y),
                    1, g.xMinorGridlineColor
                );
                edgeinc += inc;
            }
            CustomItem_AddDrawCmd(item, drawCmd);
        }
        if(g.yShowMinorLines){
            UIDrawCmd drawCmd;
            scalar_t inc = increment(g.yMinorLinesIncrement);
            scalar_t edgeinc = minor_top_edge_rounded;
            while(edgeinc < br.y){
                scalar_t yloc = (edgeinc - tl.y) * dimspul.y;
                CustomItem_DCMakeLine(drawCmd,
                    vec2g(0,            yloc),
                    vec2g(dimensions.x, yloc),
                    1, g.yMinorGridlineColor
                );
                edgeinc += inc;
            }
            CustomItem_AddDrawCmd(item, drawCmd);
        }
    }

    {//draw major gridlines and their coord labels
        if(g.xShowMajorLines){
            //this starts at the left edge of the graph, rounded by order of magnitude and increment
            //and keeps drawing lines until it reaches the right edge of the graph
            UIDrawCmd drawCmd;
            scalar_t inc = increment(g.xMajorLinesIncrement);
            scalar_t edgeinc = major_left_edge_rounded;
            while(edgeinc < br.x){
                scalar_t xloc = (edgeinc - tl.x) * dimspul.x;
                CustomItem_DCMakeLine(drawCmd,
                    vec2g(xloc, 0),
                    vec2g(xloc, dimensions.y),
                    1, g.xMajorGridlineColor
                );
                //NOTE i dont know if i really like doing this like this, using a layer i mean
                //     it's probably best to just do this afterwards, but i dont want to recalculate these positions so maybe make an array that holds them
                if(g.xShowMajorCoords){
                    PushLayer(GetCurrentLayer()+1);
                    //TODO find a way around allocating a string
                    string text = to_string(edgeinc);
                    vec2g textsize = CalcTextSize(text);
                    vec2g pos = vec2g(xloc-textsize.x/2, itemspaceorigin.y-textsize.y-1);
                    CustomItem_DCMakeFilledRect(drawCmd, pos, textsize, winbgcol);
                    CustomItem_DCMakeText(drawCmd, {text.str,text.count}, pos, textcol, vec2g::ONE); 
                    PopLayer();
                }
                edgeinc += inc;
            }
            CustomItem_AddDrawCmd(item, drawCmd);
        }
        if(g.yShowMajorLines){
            UIDrawCmd drawCmd;
            scalar_t inc = increment(g.yMajorLinesIncrement);
            scalar_t edgeinc = major_top_edge_rounded;
            while(edgeinc < br.y){
                scalar_t yloc = (edgeinc - tl.y) * dimspul.y;
                CustomItem_DCMakeLine(drawCmd,
                    vec2g(0,            yloc),
                    vec2g(dimensions.x, yloc),
                    1, g.yMajorGridlineColor
                );
                //NOTE i dont know if i really like doing this like this, using a layer i mean
                //     it's probably best to just do this afterwards, but i dont want to recalculate these positions so maybe make an array that holds them
                if(g.yShowMajorCoords){
                    PushLayer(GetCurrentLayer()+1);
                    //TODO find a way around allocating a string
                    string    text = to_string(edgeinc);
                    vec2g textsize = CalcTextSize(text);
                    vec2g      pos = vec2g(itemspaceorigin.x-textsize.x-1, yloc-textsize.y/2);
                    CustomItem_DCMakeFilledRect(drawCmd, pos, textsize, winbgcol);
                    CustomItem_DCMakeText(drawCmd, {text.str,text.count}, pos, textcol, vec2g::ONE); 
                    PopLayer();
                }
                edgeinc += inc;
            }
            CustomItem_AddDrawCmd(item, drawCmd);
        }
    }//major gridlines

    {//draw axes
        if(xAxisVisible){
            UIDrawCmd drawCmd;
            vec2g start = vec2g(0,           itemspaceorigin.y);
            vec2g   end = vec2g(dimensions.x,itemspaceorigin.y);
            color   col = g.yAxisColor;
            CustomItem_DCMakeLine(drawCmd, start, end, 1, col);
            CustomItem_AddDrawCmd(item, drawCmd);
        }
        if(yAxisVisible){
            UIDrawCmd drawCmd;
            vec2g start = vec2g(-tl.x*dimspul.x, 0);
            vec2g   end = vec2g(-tl.x*dimspul.x, dimensions.y);
            color   col = g.xAxisColor;
            CustomItem_DCMakeLine(drawCmd, start, end, 1, col);
            CustomItem_AddDrawCmd(item, drawCmd);
        }
    }//axes

    {//draw axes labels
        switch(g.axesLabelStyle){
            case GraphAxesLabelStyle_OnAxes:{
                if(g.xAxisLabel.count){
                    vec2g textsize = CalcTextSize(g.xAxisLabel);
                    vec2g pos = vec2g(
                        itemspacecenter.x+(dimensions.x/2-textsize.x), 
                        Clamp(scalar_t(itemspaceorigin.y+1), scalar_t(0), scalar_t(itemspacecenter.y+(dimensions.y/2-textsize.y)))  
                    );
                    UIDrawCmd drawCmd;
                    //TODO setup the color for this to be more dynamic or something
                    //     probably just make a graph background color parameter and use that
                    drawCmd.scissorOffset=pos;
                    drawCmd.scissorExtent=Min(textsize, dimensions/2);
                    drawCmd.useWindowScissor=false;
                    CustomItem_DCMakeFilledRect(drawCmd, pos, textsize, GetStyle().colors[UIStyleCol_WindowBg]);
                    CustomItem_DCMakeText(drawCmd, g.xAxisLabel, pos,
                      GetStyle().colors[UIStyleCol_Text], vec2g::ONE); //TODO make a label color parameter maybe
                    CustomItem_AddDrawCmd(item, drawCmd);
                }
                if(g.yAxisLabel.count){
                    vec2g textsize = CalcTextSize(g.yAxisLabel);
                    vec2g pos = vec2g( //label follows yaxis but is clamped to edges of the screen
                        Clamp(scalar_t(itemspaceorigin.x+1), scalar_t(0),  scalar_t(itemspacecenter.x+(dimensions.x/2-textsize.x))),
                        itemspacecenter.y-dimensions.y/2
                    );
                    UIDrawCmd drawCmd;
                    //TODO setup the color for this to be more dynamic or something
                    //     probably just make a graph background color parameter and use that
                    drawCmd.scissorOffset=pos;
                    drawCmd.scissorExtent=Max(textsize, dimensions/2);
                    drawCmd.useWindowScissor=false;
                    CustomItem_DCMakeFilledRect(drawCmd, pos, textsize, GetStyle().colors[UIStyleCol_WindowBg]);
                    CustomItem_DCMakeText(drawCmd, g.yAxisLabel, pos,
                      GetStyle().colors[UIStyleCol_Text], vec2g::ONE); //TODO make a label color parameter maybe
                    CustomItem_AddDrawCmd(item, drawCmd);
                }
            }
        }

    }//axes labels

    {//draw data
        carray<vec2g> data = g.data;
        forI(data.count){
            //TODO figure out if negating the y here is safe
            vec2g point = vec2g(data[i].x, -data[i].y);
            if(Math::PointInRectangle(point, cpos-vec2g::ONE*czoom, vec2g(view_width,view_width))){
                UIDrawCmd drawCmd;
                CustomItem_DCMakeFilledRect(drawCmd,
                    floor(itemspacecenter+(point-vec2g(cpos.x, cpos.y*aspect_ratio))*vec2g(dimspul.x,dimspul.y)),
                    vec2::ONE,
                    Color_Red
                ); CustomItem_AddDrawCmd(item, drawCmd);
            } 
        }
    }
    EndCustomItem();
}

// draws a given Graph in a UI Window with given dimensions
// this doesn't draw any decorations, such as a border.
void draw_graph(Graph& g, vec2g dimensions){DPZoneScoped;
    draw_graph_final(g,UI::GetWinCursor(),dimensions,1);
}

// draws a given Graph in a UI Window with given position and  dimensions
// this doesn't draw any decorations, such as a border.
void draw_graph(Graph& g, vec2g position, vec2g dimensions){
    draw_graph_final(g,position,dimensions,0);
}
