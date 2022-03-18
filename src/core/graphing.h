
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

//TODO nicely split into 
struct Graph{
    //TODO this should be vec2f64 (vec2d)
    vec2      cameraPosition{0,0}; //in graph space, center of 'camera'
	scalar_t  cameraZoom = 5.0; //represets half of the width the camera can see, so if this is 5 and posiiton is 0,0 then you see from -5 to 5
    vec2    cameraView{5.0,5.0}; //represents how much over the x and y axes we can see
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
    carray<scalar_t> xAxisData;
    carray<scalar_t> yAxisData;



    //misc graph properties that may be useful outside of draw_graph
    //these are calculated in draw_graph_final
    vec2 dimensions_per_unit_length;
    scalar_t aspect_ratio;

};

void draw_graph_final(Graph& g, vec2 position, vec2 dimensions, b32 move_cursor){
    using namespace UI;
    UIItem* item = BeginCustomItem();
    item->position = GetWinCursor();
    item->size = dimensions;
    CustomItem_AdvanceCursor(item, move_cursor);

    color textcol = GetStyle().colors[UIStyleCol_Text];
    color winbgcol = GetStyle().colors[UIStyleCol_WindowBg];


    //graph space
    vec2      cpos = g.cameraPosition;
    scalar_t czoom = g.cameraZoom;
    
    scalar_t view_width = czoom*2; 
    scalar_t aspect_ratio = dimensions.y/dimensions.x;
    g.aspect_ratio = aspect_ratio;
    //dimensions per unit length
    //gives how far along in screen space 1 unit in graph space is
    vec2 dimspul = vec2(dimensions.x/view_width, dimensions.x/view_width);
    g.dimensions_per_unit_length = dimspul;

    //TODO get exponent directly from the exponent of float's bits
    //s32 oom = (1 & 0x1 ? 
    //yep & 0x)

    //TODO all of these vec2s need to somehow represent the same type as scalar_t
    vec2 tl = cpos - czoom*vec2::ONE; //represets the top and left edge of the camera
    vec2 br = cpos + czoom*vec2::ONE;  //represets the bottom and right edge of the camera
    b32 xAxisVisible = g.xShowAxis && tl.x < 0 && br.x > 0;
    b32 yAxisVisible = g.yShowAxis && tl.y < 0 && br.y > 0;
    
    //round left edge to nearest order of magnitude multiplied by increment 
    //TODO set this up to only happen when zoom or position change
    vec2 oom = vec2(Math::order_of_magnitude(czoom), Math::order_of_magnitude(czoom));
    vec2 tentooom = oom;
    vec2 ledgernd = 
    vec2(floor(tl.x / tentooom.x) * tentooom.x, floor(tl.y / tentooom.y) * tentooom.y); 

    vec2 itemspacecenter = dimensions / 2; //positions of the center of our item

    vec2 itemspaceorigin = itemspacecenter - vec2(dimspul.x, dimspul.y*aspect_ratio)*cpos; //position of the center of the origin in item space

    {//draw minor gridlines
        //TODO prevent minor lines from unecessarily drawing where major lines draw when they're enabled
        if(g.xShowMinorLines){
            scalar_t ledge = tl.x, toledge = Clamp(0,tl.x,br.x); //to left edge 
            scalar_t redge = br.x, toredge = Clamp(0,tl.x,br.x); //to right edge
            //Log("", toledge, " ", toredge);
            scalar_t inc = floor((g.xMinorLinesIncrement*czoom)*10)/10;
            while(toledge > ledge || toredge < redge){
                toledge-=inc;
                toredge+=inc;
                if(toledge > ledge){
                    UIDrawCmd drawCmd;
                    CustomItem_DCMakeLine(drawCmd, 
                        vec2(itemspaceorigin.x + toledge*dimspul.x, 0),
                        vec2(itemspaceorigin.x + toledge*dimspul.x, dimensions.y),
                        1, g.xMinorGridlineColor
                    ); CustomItem_AddDrawCmd(item, drawCmd);
                }
                if(toredge < redge){
                    UIDrawCmd drawCmd;
                    CustomItem_DCMakeLine(drawCmd, 
                        vec2(itemspaceorigin.x + toredge*dimspul.x, 0),
                        vec2(itemspaceorigin.x + toredge*dimspul.x, dimensions.y),
                        1, g.xMinorGridlineColor
                    ); CustomItem_AddDrawCmd(item, drawCmd);
                }
            }
        }
        if(g.yShowMinorLines){
            scalar_t tedge = tl.y, totedge = Clamp(0,tl.y,br.y); //to top edge 
            scalar_t bedge = br.y, tobedge = Clamp(0,tl.y,br.y); //to bottom edge
            scalar_t inc = g.yMinorLinesIncrement*pow(10,oom.x);
            while(totedge > tedge || tobedge < bedge){
                totedge-=inc;
                tobedge+=inc;
                if(totedge > tedge){
                    UIDrawCmd drawCmd;
                    CustomItem_DCMakeLine(drawCmd, 
                        vec2(0, itemspaceorigin.y + totedge*dimspul.y),
                        vec2(dimensions.x, itemspaceorigin.y + totedge*dimspul.y),
                        1, g.xMinorGridlineColor
                    ); CustomItem_AddDrawCmd(item, drawCmd);
                }
                if(tobedge < bedge){
                    UIDrawCmd drawCmd;
                    CustomItem_DCMakeLine(drawCmd, 
                        vec2(0, itemspaceorigin.y + tobedge*dimspul.y),
                        vec2(dimensions.x,itemspaceorigin.y + tobedge*dimspul.y),
                        1, g.xMinorGridlineColor
                    ); CustomItem_AddDrawCmd(item, drawCmd);
                }
            }
        }
    }

    {//draw major gridlines and their coord labels
        if(g.xShowMajorLines){
            //this starts at the origin and draws major gridlines until reaching the edge of the graph
            scalar_t ledge = tl.x, toledge = Clamp(0, tl.x, br.x); //to left edge 
            scalar_t redge = br.x, toredge = Clamp(0, tl.x, br.x); //to right edge
            while(toledge > ledge || toredge < redge){
                toledge-= floor((g.xMinorLinesIncrement*czoom)*10)/10;
                toredge+= floor((g.xMinorLinesIncrement*czoom)*10)/10;
                if(toledge > ledge){
                    UIDrawCmd drawCmd;
                    CustomItem_DCMakeLine(drawCmd, 
                        vec2(itemspaceorigin.x + toledge*dimspul.x, 0),
                        vec2(itemspaceorigin.x + toledge*dimspul.x, dimensions.y),
                        1, g.xMajorGridlineColor
                    ); CustomItem_AddDrawCmd(item, drawCmd);
                    //NOTE i dont know if i really like doing this like this, using a layer i mean
                    //     it's probably best to just do this afterwards, but i dont want to recalculate these positions so maybe make an array that holds them
                    if(g.xShowMajorCoords){
                        PushLayer(GetCurrentLayer()+1);
                        //TODO find a way around allocating a string
                        string text = to_string(toledge);
                        vec2 textsize = CalcTextSize(text);
                        vec2 pos = vec2(itemspaceorigin.x+toledge*dimspul.x-textsize.x/2, itemspaceorigin.y-textsize.y-1);
                        CustomItem_DCMakeFilledRect(drawCmd, pos, textsize, winbgcol);
                        CustomItem_DCMakeText(drawCmd, {text.str,text.count}, pos, textcol, vec2::ONE); 
                        CustomItem_AddDrawCmd(item, drawCmd);
                        PopLayer();
                    }
                }
                if(toredge < redge){
                    UIDrawCmd drawCmd;
                    CustomItem_DCMakeLine(drawCmd, 
                        vec2(itemspaceorigin.x + toredge*dimspul.x, 0),
                        vec2(itemspaceorigin.x + toredge*dimspul.x, dimensions.y),
                        1, g.xMajorGridlineColor
                    ); CustomItem_AddDrawCmd(item, drawCmd);
                    if(g.xShowMajorCoords){
                        PushLayer(GetCurrentLayer()+1);
                        //TODO find a way around allocating a string
                        string text = to_string(toredge);
                        vec2 textsize = CalcTextSize(text);
                        vec2 pos = vec2(itemspaceorigin.x+toredge*dimspul.x-textsize.x/2, itemspaceorigin.y-textsize.y-1);
                        CustomItem_DCMakeFilledRect(drawCmd, pos, textsize, winbgcol);
                        CustomItem_DCMakeText(drawCmd,{text.str,text.count},pos,textcol, vec2::ONE); 
                        CustomItem_AddDrawCmd(item, drawCmd);
                        PopLayer();
                    }
                }
                
            }
        }
        if(g.yShowMajorLines){
            scalar_t tedge = tl.y, totedge = Clamp(0,tl.y,br.y); //to top edge 
            scalar_t bedge = br.y, tobedge = Clamp(0,tl.y,br.y); //to bottom edge
            while(totedge > tedge || tobedge < bedge){
                totedge-=g.yMajorLinesIncrement*pow(10,oom.x);
                tobedge+=g.yMajorLinesIncrement*pow(10,oom.x);
                if(totedge > tedge){
                    UIDrawCmd drawCmd;
                    CustomItem_DCMakeLine(drawCmd, 
                        vec2(0, itemspaceorigin.y + totedge*dimspul.y),
                        vec2(dimensions.x, itemspaceorigin.y + totedge*dimspul.y),
                        1, g.xMajorGridlineColor
                    ); CustomItem_AddDrawCmd(item, drawCmd);
                    if(g.yShowMajorCoords){
                        PushLayer(GetCurrentLayer()+1);
                        //TODO find a way around allocating a string
                        string text = to_string(-totedge);
                        vec2 textsize = CalcTextSize(text);
                        vec2 pos = vec2(itemspaceorigin.x-textsize.x-1,itemspaceorigin.y+totedge*dimspul.y-textsize.y/2);
                        CustomItem_DCMakeFilledRect(drawCmd, pos, textsize, winbgcol);
                        CustomItem_DCMakeText(drawCmd, {text.str,text.count}, pos, textcol, vec2::ONE); 
                        CustomItem_AddDrawCmd(item, drawCmd);
                        PopLayer();
                    }
                }
                if(tobedge < bedge){
                    UIDrawCmd drawCmd;
                    CustomItem_DCMakeLine(drawCmd, 
                        vec2(0, itemspaceorigin.y + tobedge*dimspul.y),
                        vec2(dimensions.x,itemspaceorigin.y + tobedge*dimspul.y),
                        1, g.xMajorGridlineColor
                    ); CustomItem_AddDrawCmd(item, drawCmd);
                    if(g.yShowMajorCoords){
                        PushLayer(GetCurrentLayer()+1);
                        //TODO find a way around allocating a string
                        string text = to_string(-tobedge);
                        vec2 textsize = CalcTextSize(text);
                        vec2 pos = vec2(itemspaceorigin.x-textsize.x-1,itemspaceorigin.y+tobedge*dimspul.y-textsize.y/2);
                        CustomItem_DCMakeFilledRect(drawCmd, pos, textsize, winbgcol);
                        CustomItem_DCMakeText(drawCmd, {text.str,text.count}, pos, textcol, vec2::ONE); 
                        CustomItem_AddDrawCmd(item, drawCmd);
                        PopLayer();
                    }
                }
            }
        }
    }//major gridlines

    {//draw axes
        if(xAxisVisible){
            UIDrawCmd drawCmd;
            vec2 start = vec2(0,-tl.y*dimspul.y*aspect_ratio);
            vec2   end = vec2(dimensions.x,-tl.y*dimspul.y*aspect_ratio);
            color  col = g.yAxisColor;
            CustomItem_DCMakeLine(drawCmd, start, end, 1, col);
            CustomItem_AddDrawCmd(item, drawCmd);
        }
        if(yAxisVisible){
            UIDrawCmd drawCmd;
            vec2  start = vec2(-tl.x*dimspul.x,0);
            vec2    end = vec2(-tl.x*dimspul.x,dimensions.y);
            color   col = g.xAxisColor;
            CustomItem_DCMakeLine(drawCmd, start, end, 1, col);
            CustomItem_AddDrawCmd(item, drawCmd);
        }
    }//axes

    {//draw axes labels
        switch(g.axesLabelStyle){
            case GraphAxesLabelStyle_OnAxes:{
                if(g.xAxisLabel.count){
                    vec2 textsize = CalcTextSize(g.xAxisLabel);
                    vec2 pos = vec2(
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
                      GetStyle().colors[UIStyleCol_Text], vec2::ONE); //TODO make a label color parameter maybe
                    CustomItem_AddDrawCmd(item, drawCmd);
                }
                if(g.yAxisLabel.count){
                    vec2 textsize = CalcTextSize(g.yAxisLabel);
                    vec2 pos = vec2( //label follows yaxis but is clamped to edges of the screen
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
                      GetStyle().colors[UIStyleCol_Text], vec2::ONE); //TODO make a label color parameter maybe
                    CustomItem_AddDrawCmd(item, drawCmd);
                }
            }
        }

    }//axes labels

    {//draw data
        Assert(g.xAxisData.count == g.yAxisData.count, "data counts mismatch. make sure the amount of x and y data you provide is the same!");
        carray<scalar_t> xad = g.xAxisData;
        carray<scalar_t> yad = g.yAxisData;

        forI(xad.count){
            vec2 point = vec2(xad[i],yad[i]);
            if(Math::PointInRectangle(point, cpos-vec2::ONE*czoom, vec2(view_width,view_width))){
                UIDrawCmd drawCmd;
                CustomItem_DCMakeFilledCircle(drawCmd, 
                    itemspacecenter+(point-cpos)*vec2(dimspul.x, dimspul.y*aspect_ratio),
                    1,
                    20,
                    Color_Red
                ); CustomItem_AddDrawCmd(item, drawCmd);
            } 
        }
    }

    EndCustomItem();
}

// draws a given Graph in a UI Window with given dimensions
// this doesn't draw any decorations, such as a border.
void draw_graph(Graph& g, vec2 dimensions){DPZoneScoped;
    draw_graph_final(g,UI::GetWinCursor(),dimensions,1);
}

// draws a given Graph in a UI Window with given position and  dimensions
// this doesn't draw any decorations, such as a border.
void draw_graph(Graph& g, vec2 position, vec2 dimensions){
    draw_graph_final(g,position,dimensions,0);
}
