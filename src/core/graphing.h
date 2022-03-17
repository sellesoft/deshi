
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

//TODO nicely split into 
struct Graph{
    //TODO this should be vec2f64 (vec2d)
    vec2      cameraPosition{0,0}; //in graph space, center of 'camera'
	scalar_t  cameraZoom = 5.0; //represets half of the width the camera can see, so if this is 5 and posiiton is 0,0 then you see from -5 to 5

    //Graph properties
    //wether or not to show main axes
    //bulk set using set_axes_visible()
    b32 xShowAxis = true;
    b32 yShowAxis = true;
    //labels per axis
    char* xAxisLabel = "";
    char* yAxisLabel = "";
    //colors per axis
    //bulk set using set_axes_colors()
    color xAxisColor=Color_White;
    color yAxisColor=Color_White;
    //colos per major gridline
    color xMajorGridlineColor=color(70,70,70);
    color yMajorGridlineColor=color(70,70,70);
    //TODO scalar_t xAxisScale = 1
	scalar_t gridZoomFit               = 5.0;
	scalar_t gridZoomFitIncrements[3]  = {2.0, 2.5, 2.0};
	u32      gridZoomFitIncrementIndex = 0;
    //bright "major" gridlines
    //bulk set using set_major_gridlines_count()
	u32      xMajorLinesCount       = 12;
	u32      yMajorLinesCount       = 12;
    //the separation between major gridlines
    //bulk set using set_major_gridlines_increment()
	scalar_t xMajorLinesIncrement   = 1.0;
	scalar_t yMajorLinesIncrement   = 1.0;
    //dim minor gridlines
    //bulk set using set_minor_gridlines_count()
	u32      xMinorLinesCount       = 4;
	u32      yMinorLinesCount       = 4;
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
   

    //Graph data


};

//local vec2 
//GraphToScreen(vec2 point){
//	point -= camera_pos;
//	point /= camera_zoom;
//	point.y *= -scalar_t(DeshWindow->width) / scalar_t(DeshWindow->height);
//	point += vec2{1.0, 1.0};
//	point.x *= scalar_t(DeshWindow->dimensions.x); point.y *= scalar_t(DeshWindow->dimensions.y);
//	point /= 2.0;
//	return vec2(point.x, point.y);
//}FORCE_INLINE vec2 ToScreen(scalar_t x, scalar_t y){ return ToScreen({x,y}); }
//

// draws a given Graph at position with the given dimensions
// this doesn't draw any decorations, such as a border.
void draw_graph(const Graph& g, vec2 dimensions){DPZoneScoped;
    using namespace UI;
   
    //graph space
    vec2      cpos = g.cameraPosition;
    scalar_t czoom = g.cameraZoom;
    
    scalar_t view_width = czoom*2; 
    scalar_t aspect_ratio = dimensions.y/dimensions.x;
    //dimensions per unit length
    //gives how far along in screen space 1 unit in graph space is
    vec2 dimspul = vec2(dimensions.x/view_width, dimensions.x/view_width*aspect_ratio);

    //camera position in ui window space
    //vec2 cposwin = 

    //TODO get exponent directly from the exponent of float's bits
    //s32 oom = (1 & 0x1 ? 
    //yep & 0x)

    //TODO all of these vec2s need to somehow represent the same type as scalar_t
    //represets the top and left edge of the camera
    vec2 tl = cpos - czoom*vec2::ONE;
    //represets the bottom and right edge of the camera
    vec2 br = cpos + czoom*vec2::ONE;
    vec2 oom = vec2(Math::order_of_magnitude(czoom), Math::order_of_magnitude(czoom));
    //round left edge to nearest order of magnitude multiplied by increment 
    //TODO set this up to only happen when zoom or position change
    vec2 tentooom = vec2(pow(10, oom.x), pow(10, oom.y));
    vec2 ledgernd = 
    vec2(floor(tl.x / tentooom.x) * tentooom.x, floor(tl.y / tentooom.y) * tentooom.y); 
    UI::Text(toStr("tl   ", tl).str);
    UI::Text(toStr("br   ", br).str);
    UI::Text(toStr("oom  ", oom).str);
    UI::Text(toStr("tlrnd", ledgernd).str);

    {//draw axes
        if(g.xShowMajorLines){
            //TODO just start at 0 and draw major/minor lines starting from there
            //scalar_t ledge = tl.x, toledge = 0;
            //scalar_t redge = br.x, toredge = 0;
            //while(toledge < ledge || toredge < redge){
            //    if(toledge < ledge){
//
            //    }
            //    if(toredge < redge){
//
            //    }
            //}
            while(ledgernd.x < br.x){
                if(ledgernd.x!=0) //don't draw where the main axis is //TODO decide if this shouldnt happen when main axis drawing is disabled 
                    Line(vec2((ledgernd.x-tl.x)*dimspul.x, 0), vec2((ledgernd.x-tl.x)*dimspul.x, dimensions.y), 1, g.xMajorGridlineColor);
                ledgernd.x += g.xMajorLinesIncrement * pow(10, oom.x);
            }
        }
        if(g.xShowAxis && tl.x < 0 && br.x > 0){
            Line(vec2(-tl.x*dimspul.x,0),vec2(-tl.x*dimspul.x,dimensions.y), 1, g.xAxisColor);
            //TODO label
        }
        if(g.yShowMajorLines){
            while(ledgernd.y < br.y){
                if(ledgernd.y!=0) //dont draw where main axis is
                    Line(vec2(0, (ledgernd.y)*dimspul.y), vec2(dimensions.x, (ledgernd.y)*dimspul.y), 1, g.yMajorGridlineColor);
                    Line(vec2(0, (ledgernd.y)*dimspul.y), vec2(dimensions.x, (ledgernd.y)*dimspul.y), 1, g.yMajorGridlineColor);

                ledgernd.y += g.yMajorLinesIncrement * pow(10, oom.y);
            }
        }
        if(g.yShowAxis && tl.y < 0 && br.y > 0){
            Line(vec2(0,-tl.y*dimspul.y),vec2(dimensions.x,-tl.y*dimspul.y), 1, g.yAxisColor);
            //TODO label
        }
        //TODO z axis
    }



}
