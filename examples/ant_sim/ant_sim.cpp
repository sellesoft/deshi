/* ant simulation

Index:
@entity
@need/cost
@action
@advert
@agent
@main
*/
#include "deshi.h"
#include "stb/stb_ds.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//@entity
typedef struct Entity{
	u64 age;
	union{
		s32 pos[2];
		struct{ s32 x, y; };
	};
}Entity;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@need/cost
#define MAX_NEED_VALUE 100000
#define MIN_NEED_VALUE      0

enum{
	Need_NULL = 0,
	Need_Bladder,
	Need_Food,
	Need_Health,
	Need_Mating,
	Need_Safety,
	Need_Sleep,
	Need_Water,
	Need_COUNT,
};
str8 NeedStrings[] = {
	STR8("NULL"),
	STR8("Bladder"),
	STR8("Food"),
	STR8("Health"),
	STR8("Mating"),
	STR8("Safety"),
	STR8("Sleep"),
	STR8("Water"),
}; StaticAssert(ArrayCount(NeedStrings) == Need_COUNT);

typedef struct Need{
	Type type;
	f32 value;
	f32 delta;
	f32 increase_weight;
	f32 decrease_weight;
}Need;

typedef struct Cost{
	Type need;
	f32 delta;
}Cost;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@action
enum{
	Action_Walk,
	Action_Dig,
};

enum{
	ActionFlags_None = 0,
	ActionFlags_ConsumeActionOnCompletion = (1 << 0),
	ActionFlags_ConsumeOwnerOnCompletion  = (1 << 1),
};

typedef struct Action{
	Entity* owner;
	Type type;
	Flags flags;
	u32 time;
	Cost* costs;
}Action;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@advert
enum{
	AdvertFlags_None = 0,
	AdvertFlags_ConsumeAdvertOnCompletion = (1 << 0),
	AdvertFlags_ConsumeOwnerOnCompletion  = (1 << 1),
};

typedef struct Advert{
	Entity* owner;
	Flags flags;
	u32 time; //advertised time
	Cost* costs; //advertised costs
	Action* actions;
}Advert;

//scales a score value based on the need increase's distance from zero (so the change from 70-80 has less value than 10-30)
//NOTE usually returns a value between -1 and 1
//NOTE the "+ .001f" is just to avoid division by zero
inline f32 need_attenuation(f32 current, f32 future, f32 weight){
	return weight * ((10.f / (current + .001f)) - (10.f / (future + .001f)));
}

//scales a score value with distance: 100 * score / distance^2
inline f32 distance_attenuation(f32 score, f32 pos1[2], f32 pos2[2]){
	f32 x = pos2[0] - pos1[0];
	f32 y = pos2[1] - pos1[1];
	f32 d = x*x + y*y;
	return score / ((d > 1.f) ? d : 1.f);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//@agent
enum{
	Race_Ant,
};

typedef struct Agent{
	Entity entity;
	Type race;
	Need* needs;
}Agent;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@main
int main(int args_count, char** args){
	deshi_init();
	
	
	
	deshi_loop_start();{
		
	}deshi_loop_end();
	
	
	
	deshi_cleanup();
}
