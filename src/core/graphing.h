
#ifndef scalar_t
#define scalar_t f64
#endif

#include "kigu/common.h"
#include "kigu/color.h"

#include "math/math.h"

//TODOs
// clamp axes to edge of camera (when out of frame or always)

struct Graph{
    //TODO this should be vec2f64 (vec2d)
    vec2      cameraPosition{0,0}; //in graph space
	scalar_t  cameraZoom = 5.0; //represets half of the width the camera can see, so if this is 5 and posiiton is 0,0 then you see from -5 to 5

    //Graph properties
     //labels per axis
    char* xAxisLabel = "";
    char* yAxisLabel = "";
    //char* zAxisLabel = "";
    //colors per axis
    //bulk set using set_axes_colors()
    color xAxisColor=Color_White;
    color yAxisColor=Color_White;
    //color zAxisColor=Color_White;
    //TODO scalar_t xAxisScale = 1
	scalar_t gridZoomFit               = 5.0;
	scalar_t gridZoomFitIncrements[3]  = {2.0, 2.5, 2.0};
	u32      gridZoomFitIncrementIndex = 0;
    //bright "major" gridlines
    //bulk set using set_major_gridlines_count()
	u32      xMajorLinesCount       = 12;
	u32      yMajorLinesCount       = 12;
	u32      zMajorLinesCount       = 12;
    //the separation between major gridlines
    //bulk set using set_major_gridlines_increment()
	scalar_t xMajorLinesIncrement   = 1.0;
	scalar_t yMajorLinesIncrement   = 1.0;
	scalar_t zMajorLinesIncrement   = 1.0;
    //amount of minor lines to 
    //bulk set using set_minor_gridlines_count()
	u32      xMinorLinesCount       = 4;
	u32      yMinorLinesCount       = 4;
	u32      zMinorLinesCount       = 4;
    //bulk set using set_major_gridlines_increment()
	scalar_t xMinorLinesIncrement   = 0.2;
	scalar_t yMinorLinesIncrement   = 0.2;
	scalar_t zMinorLinesIncrement   = 0.2;
    //bulk set using set_major_gridlines_increment()
	b32      xShowMajorLines     = true;
	b32      yShowMajorLines     = true;
	b32      zShowMajorLines     = true;
    //bulk set using set_major_gridlines_increment()
	b32      xShowMinorLines     = true;
	b32      yShowMinorLines     = true;
	b32      zShowMinorLines     = true;
    //bulk set using set_axes_coords_visible()
	b32      xShowAxisCoords     = true;
	b32      yShowAxisCoords     = true;
	b32      zShowAxisCoords     = true;
   

    //Graph data


};

// draws a given Graph at position with the given dimensions
// this doesn't draw any decorations, such as a border.
void draw_graph(const Graph& g, vec2 position, vec2 dimensions){
    vec2      cpos = g.cameraPosition;
    scalar_t czoom = g.cameraZoom;

    //TODO get exponent directly from 
    //s32 oom = (1 & 0x1 ? 
    //yep & 0x)

    scalar_t ledge = cpos.x-czoom;
    s32        oom = Math::order_of_magnitude(ledge);
    //round left edge to nearest order of magnitude multiplied by increment 
    scalar_t tentooom = pow(10, oom);
    scalar_t ledgernd = floor(ledge / tentooom) * tentooom * g.xMajorLinesIncrement; 

    


}
