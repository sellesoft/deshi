/* ant_sim deshi example

Index:
@utils
@vars
@entity
@need
@action
@agent
@advert
@render
@simulation
@ui
@input
@main

TODOs:
advert scoring info
action execution
draw cursor size
*/
#include "deshi.h"
#include "core/baked/fonts.h"
#include "kigu/array_utils.h"
#include "kigu/array.h"
struct Advert;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @utils


#define TICKS_PER_WORLD_SECOND 1
#define TICKS_PER_WORLD_MINUTE 60
#define TICKS_PER_WORLD_DAY    86400
#define TICKS_PER_WORLD_MONTH  2629740
#define TICKS_PER_WORLD_YEAR   31536000

#define WORLD_WIDTH 512
#define WORLD_HEIGHT 512

#define WorldX(index) ((index) % WORLD_WIDTH)
#define WorldY(index) ((index) / WORLD_WIDTH)
#define WorldIndex(x, y) ((y) * WORLD_WIDTH + (x))

#define WORLD_CHUNK_SIZE 16
#define WORLD_CHUNKS_X (WORLD_WIDTH / WORLD_CHUNK_SIZE)
#define WORLD_CHUNKS_Y (WORLD_HEIGHT / WORLD_CHUNK_SIZE)

#define WorldToChunkX(x) ((x) / WORLD_CHUNK_SIZE)
#define WorldToChunkY(y) ((y) / WORLD_CHUNK_SIZE)
#define WorldToChunkIndex(x, y) (WorldToChunkY(y) * WORLD_CHUNKS_X + WorldToChunkX(x))

#define ChunkIndexToChunkX(index) ((index) % WORLD_CHUNKS_X)
#define ChunkIndexToChunkY(index) ((index) / WORLD_CHUNKS_X)

#define ChunkStartX(chunk_x) ((chunk_x) * WORLD_CHUNKS_X)
#define ChunkStartY(chunk_y) ((chunk_y) * WORLD_CHUNKS_Y)
#define ChunkEndX(chunk_x) (ChunkStartX(chunk_x) + WORLD_CHUNK_SIZE - 1)
#define ChunkEndY(chunk_y) (ChunkStartY(chunk_y) + WORLD_CHUNK_SIZE - 1)

typedef u32 Direction;
enum{
	Direction_North,
	Direction_East,
	Direction_South,
	Direction_West,
	Direction_COUNT
};

vec2i direction_to_movement[] = {
	vec2i{ 0, 1}, //north
	vec2i{ 1, 0}, //east
	vec2i{ 0,-1}, //south
	vec2i{-1, 0}, //west
};

u32 divide_color(u32 color, f32 divisor){
	u32 r = (u32)((f32)(color >>  0 & 0x000000ff) / divisor);
	u32 g = (u32)((f32)(color >>  8 & 0x000000ff) / divisor);
	u32 b = (u32)((f32)(color >> 16 & 0x000000ff) / divisor);
	return PackColorU32(r,g,b,255);
}

// Check if the positions are in orthogonally adjacent chunks
// O?O  X = basis
// ?X?  ? = checked
// O?O  O = ignored
b32 in_orthogonal_neighbor_chunk(vec2i pos1, vec2i pos2){
	s32 dx = abs(WorldToChunkX(pos1.x) - WorldToChunkX(pos2.x));
	s32 dy = abs(WorldToChunkY(pos1.y) - WorldToChunkY(pos2.y));
	return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
}

// Check if the positions are in diagonally adjacent chunks
// ?O?  X = basis
// OXO  ? = checked
// ?O?  O = ignored
b32 in_diagonal_neighbor_chunk(vec2i pos1, vec2i pos2){
	s32 dx = abs(WorldToChunkX(pos1.x) - WorldToChunkX(pos2.x));
	s32 dy = abs(WorldToChunkY(pos1.y) - WorldToChunkY(pos2.y));
	return (dx == 1 && dy == 1);
}

// Check if the positions are in surrounding adjacent chunks
// ???  X = basis
// ?X?  ? = checked
// ???  O = ignored
b32 in_surrounding_neighbor_chunk(vec2i pos1, vec2i pos2){
	s32 dx = abs(WorldToChunkX(pos1.x) - WorldToChunkX(pos2.x));
	s32 dy = abs(WorldToChunkY(pos1.y) - WorldToChunkY(pos2.y));
	return (dx <= 1 && dy <= 1) && !(dx == 0 && dy == 0);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @entity


typedef u32 EntityType;
enum{
	Entity_NULL = 0,
	Entity_Wall,
	Entity_Agent,
	Entity_Leaf,
	Entity_Dirt,
	Entity_Water,
	Entity_COUNT
};

str8 EntityStrings[] = {
	STR8("NULL"),
	STR8("Wall"),
	STR8("Agent"),
	STR8("Leaf"),
	STR8("Dirt"),
	STR8("Water"),
};
StaticAssert(ArrayCount(EntityStrings) == Entity_COUNT);

typedef Flags EntityFlags;
enum{
	EntityFlags_Water_Evaluated = 1 << 0,
};

// color palettes for entities that randomly choose color
u32 EntityColors[Entity_COUNT][7] = {
	0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
	0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa,
	0,          0,          0,          0,          0,          0,          0         ,
	0xff709a88, 0xff7ba694, 0xff86b19f, 0xff91bdab, 0xff9cc9b7, 0xffa8d5c3, 0xffb4e1cf,
	0xff85a3bc, 0xff8da9c1, 0xff93aec7, 0xff99b4cb, 0xff9fb9d0, 0xffa5bed5, 0xffaac3da,
	0xff9ea18f, 0xffa4a794, 0xffa8ab9a, 0xffadb09f, 0xff747861, 0xffb2b5a4, 0xffbbbeae,
};

#define MAX_ENTITY_INNATE_ADVERTS 4
#define MAX_ENTITY_ADVERTS (MAX_ENTITY_INNATE_ADVERTS + 4)

typedef struct Entity{
	Node overlap_node; // connection to other entities occupying the same world tile
	EntityType type;
	u32 color;
	u64 age;
	vec2i pos;
	EntityFlags flags;
	
	Advert* adverts_array[MAX_ENTITY_ADVERTS];
	u64 adverts_count;
	
	union{
		struct{
			b32 evaluating;
			b32 evaluated;
			u32 pressure;
			Node node; // connects the surface of a body of water
		}water;
	};
}Entity;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @need/cost


#define MAX_NEED_VALUE 100000
#define MIN_NEED_VALUE      0

typedef u32 NeedType;
enum{
	Need_Bladder,
	Need_Food,
	Need_Health,
	Need_Mating,
	Need_Sleep,
	Need_Water,
	Need_Colony,
	Need_COUNT
};

str8 NeedStrings[] = {
	STR8("Bladder"),
	STR8("Food"),
	STR8("Health"),
	STR8("Mating"),
	STR8("Sleep"),
	STR8("Water"),
	STR8("Colony"),
};
StaticAssert(ArrayCount(NeedStrings) == Need_COUNT);

typedef struct Need{
	f32 value;
	f32 delta;
}Need;

void delta_need(Need* need, f32 delta){
	need->value += delta;
	need->value  = Clamp(need->value, MIN_NEED_VALUE, MAX_NEED_VALUE);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @action


#define UNKNOWN_ACTION_COMPLETION_TIME 0

typedef u32 ActionType;
enum{
	Action_Idle,
	Action_Walk,
	Action_Dig,
	Action_EatLeaf,
	Action_DrinkWater,
	Action_COUNT
};

typedef struct ActionDef{
	ActionType type;
	str8 name;
	u32 time;
	f32 costs[Need_COUNT];
}ActionDef;

typedef struct Action{
	ActionDef* def;
	vec2i target;
	f32 progress; //0-1; 1 is complete
}Action;

ActionDef ActionDefinitions[] = {
	{Action_Idle,
		/*name */ STR8("Idle"),
		/*time */ 1*TICKS_PER_WORLD_SECOND,
		/*costs*/ {},
	},
	
	{Action_Walk,
		/*name */ STR8("Walk"),
		/*time */ UNKNOWN_ACTION_COMPLETION_TIME,
		/*costs*/ {},
	},
	
	{Action_Dig,
		/*name */ STR8("Dig"),
		/*time */ 90*TICKS_PER_WORLD_SECOND,
		/*costs*/ {
			/*bladder*/  0.00f*MAX_NEED_VALUE,
			/*food   */  0.00f*MAX_NEED_VALUE,
			/*health */  0.00f*MAX_NEED_VALUE,
			/*mating */  0.00f*MAX_NEED_VALUE,
			/*sleep  */ -0.01f*MAX_NEED_VALUE,
			/*water  */  0.00f*MAX_NEED_VALUE,
			/*colony */  0.00f*MAX_NEED_VALUE,
		},
	},
	
	{Action_EatLeaf,
		/*name */ STR8("Eat Leaf"),
		/*time */ 60*TICKS_PER_WORLD_SECOND,
		/*costs*/ {
			/*bladder*/ -0.30f*MAX_NEED_VALUE,
			/*food   */  0.30f*MAX_NEED_VALUE,
			/*health */  0.00f*MAX_NEED_VALUE,
			/*mating */  0.00f*MAX_NEED_VALUE,
			/*sleep  */  0.00f*MAX_NEED_VALUE,
			/*water  */  0.10f*MAX_NEED_VALUE,
			/*colony */  0.00f*MAX_NEED_VALUE,
		},
	},
	
	{Action_DrinkWater,
		/*name */ STR8("Drink Water"),
		/*time */ 30*TICKS_PER_WORLD_SECOND,
		/*costs*/ {
			/*bladder*/ -0.30f*MAX_NEED_VALUE,
			/*food   */  0.00f*MAX_NEED_VALUE,
			/*health */  0.00f*MAX_NEED_VALUE,
			/*mating */  0.00f*MAX_NEED_VALUE,
			/*sleep  */  0.00f*MAX_NEED_VALUE,
			/*water  */  0.90f*MAX_NEED_VALUE,
			/*colony */  0.00f*MAX_NEED_VALUE,
		},
	},
};
StaticAssert(ArrayCount(ActionDefinitions) == Action_COUNT);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @agent


typedef u32 AgentType;
enum{
	Agent_BlackGardenAntQueen,
	Agent_BlackGardenAntMale,
	Agent_BlackGardenAntWorker,
	Agent_CottonAntQueen,
	Agent_CottonAntMale,
	Agent_CottonAntMinorWorker,
	Agent_CottonAntMajorWorker,
	Agent_COUNT
};

str8 AgentStrings[] = {
	STR8("Queen Black Garden Ant"),
	STR8("Male Black Garden Ant"),
	STR8("Worker Black Garden Ant"),
	STR8("Queen Cotton Ant"),
	STR8("Male Cotton Ant"),
	STR8("Minor Worker Cotton Ant"),
	STR8("Major Worker Cotton Ant"),
};
StaticAssert(ArrayCount(AgentStrings) == Agent_COUNT);

str8 AgentSpeciesStrings[] = {
	STR8("Lasius Niger"),
	STR8("Lasius Niger"),
	STR8("Lasius Niger"),
	STR8("Solenopsis xyloni"),
	STR8("Solenopsis xyloni"),
	STR8("Solenopsis xyloni"),
	STR8("Solenopsis xyloni"),
};
StaticAssert(ArrayCount(AgentSpeciesStrings) == Agent_COUNT);

Need AgentNeedDefinitions[][Need_COUNT] = {
	/*Agent_BlackGardenAntQueen*/ { //TODO(caj) queen ants
		/*bladder*/ {MAX_NEED_VALUE, -0.010f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*food   */ {MAX_NEED_VALUE, -0.010f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*health */ {MAX_NEED_VALUE, -0.010f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*mating */ {MAX_NEED_VALUE, -0.010f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*sleep  */ {MAX_NEED_VALUE, -0.011f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY}, //90 times per day
		/*water  */ {MAX_NEED_VALUE, -0.010f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*colony */ {MAX_NEED_VALUE, -0.010f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
	},
	/*Agent_BlackGardenAntMale*/ { //TODO(caj) male ants
		/*bladder*/ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*food   */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*health */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*mating */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*sleep  */ {MAX_NEED_VALUE, -0.006f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY}, //170 times per day
		/*water  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*colony */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
	},
	/*Agent_BlackGardenAntWorker*/ { //TODO(caj) worker ants
		/*bladder*/ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*food   */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*health */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*mating */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY}, //NOTE worker ants don't mate
		/*sleep  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY}, //250 times per day
		/*water  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*colony */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
	},
	/*Agent_CottonAntQueen*/ { //TODO(caj) queen ants
		/*bladder*/ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*food   */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*health */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*mating */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*sleep  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*water  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*colony */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
	},
	/*Agent_CottonAntMale*/ { //TODO(caj) male ants
		/*bladder*/ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*food   */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*health */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*mating */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*sleep  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*water  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*colony */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
	},
	/*Agent_CottonAntMinorWorker*/ { //TODO(caj) worker ants
		/*bladder*/ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*food   */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*health */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*mating */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY}, //NOTE worker ants don't mate
		/*sleep  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*water  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*colony */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
	},
	/*Agent_CottonAntMajorWorker*/ { //TODO(caj) worker ants
		/*bladder*/ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*food   */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*health */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*mating */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY}, //NOTE worker ants don't mate
		/*sleep  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*water  */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
		/*colony */ {MAX_NEED_VALUE, -0.000f*MAX_NEED_VALUE/TICKS_PER_WORLD_DAY},
	},
};
StaticAssert(ArrayCount(AgentNeedDefinitions[0]) == Need_COUNT);
StaticAssert(ArrayCount(AgentNeedDefinitions) == Agent_COUNT);

typedef struct Agent{
	Entity entity;
	AgentType type;
	
	u32 action_index;
	Advert* active_advert;
	
	Need needs[Need_COUNT];
	
	u8* path_nodes; //movement direction steps (north, east, south, west)
	u32 path_count;
	u32 path_index; //progress along path
}Agent;
#define AgentFromNode(ptr) CastFromMember(Agent,node,ptr)
#define AgentFromEntity(ptr) CastFromMember(Agent,entity,ptr)


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @advert


#define MAX_ADVERT_DEF_ACTIONS (4) //arbitrary value, increase if needed
#define ADVERT_COST_DELTA_EPSILON (10)
#define ADVERT_TIME_DELTA_EPSILON (5*TICKS_PER_WORLD_SECOND)
#define MIN_ADVERT_SELECTION_SCORE (1.0f)

typedef u32 AdvertType;
enum{
	Advert_NULL,
	Advert_Dig,
	Advert_EatLeaf,
	Advert_DrinkWater,
	Advert_COUNT
};

typedef Flags AdvertDefFlags;
enum{
	AdvertDefFlags_None                      = 0,
	AdvertDefFlags_ConsumeAdvertOnCompletion = (1 << 0),
	AdvertDefFlags_ConsumeOwnerOnCompletion  = (1 << 1),
};

typedef struct AdvertDef{
	str8 name;
	AdvertDefFlags flags;
	u32 range_sq;
	f32 costs[Need_COUNT];
	ActionDef* actions[MAX_ADVERT_DEF_ACTIONS];
}AdvertDef;

typedef struct Advert{
	AdvertDef* def;
	Entity* owner;
	Action actions_array[MAX_ADVERT_DEF_ACTIONS];
	u32 actions_count;
	u32 completion_time;
	u32 ongoing_time;
}Advert;

AdvertDef AdvertDefinitions[] = {
	{STR8("NULL"),
		/*flags   */ AdvertDefFlags_None,
		/*range_sq*/ (u32)-1,
		/*costs   */ {0},
		/*actions */ {0},
	},
	{STR8("Dig"),
		/*flags   */ AdvertDefFlags_ConsumeOwnerOnCompletion,
		/*range_sq*/ (u32)-1,
		/*costs   */ {
			/*bladder*/  0.00f*MAX_NEED_VALUE,
			/*food   */  0.00f*MAX_NEED_VALUE,
			/*health */  0.00f*MAX_NEED_VALUE,
			/*mating */  0.00f*MAX_NEED_VALUE,
			/*sleep  */ -0.01f*MAX_NEED_VALUE,
			/*water  */  0.00f*MAX_NEED_VALUE,
			/*colony */  0.00f*MAX_NEED_VALUE,
		},
		/*actions*/ {
			&ActionDefinitions[Action_Dig],
		},
	},
	{STR8("Eat Leaf"),
		/*flags   */ AdvertDefFlags_ConsumeOwnerOnCompletion,
		/*range_sq*/ (u32)-1,
		/*costs   */ {
			/*bladder*/ -0.30f*MAX_NEED_VALUE,
			/*food   */  0.30f*MAX_NEED_VALUE,
			/*health */  0.00f*MAX_NEED_VALUE,
			/*mating */  0.00f*MAX_NEED_VALUE,
			/*sleep  */  0.00f*MAX_NEED_VALUE,
			/*water  */  0.10f*MAX_NEED_VALUE,
			/*colony */  0.00f*MAX_NEED_VALUE,
		},
		/*actions*/ {
			&ActionDefinitions[Action_EatLeaf],
		},
	},
	{STR8("Drink Water"),
		/*flags   */ AdvertDefFlags_ConsumeOwnerOnCompletion,
		/*range_sq*/ (u32)-1,
		/*costs   */ {
			/*bladder*/ -0.30f*MAX_NEED_VALUE,
			/*food   */  0.00f*MAX_NEED_VALUE,
			/*health */  0.00f*MAX_NEED_VALUE,
			/*mating */  0.00f*MAX_NEED_VALUE,
			/*sleep  */  0.00f*MAX_NEED_VALUE,
			/*water  */  0.90f*MAX_NEED_VALUE,
			/*colony */  0.00f*MAX_NEED_VALUE,
		},
		/*actions*/ {
			&ActionDefinitions[Action_DrinkWater],
		},
	},
};
StaticAssert(ArrayCount(AdvertDefinitions) == Advert_COUNT);

#define AdvertTypeFromAdvertDef(def) ((AdvertType)((def) - AdvertDefinitions))

AdvertType EntityInnateAdverts[][MAX_ENTITY_INNATE_ADVERTS] = {
	/*Entity_NULL */ {},
	/*Entity_Wall */ {},
	/*Entity_Agent*/ {},
	/*Entity_Leaf */ {Advert_EatLeaf},
	/*Entity_Dirt */ {Advert_Dig},
	/*Entity_Water*/ {Advert_DrinkWater},
};
StaticAssert(ArrayCount(EntityInnateAdverts) == Entity_COUNT);

//scales a score value based on the need increase's distance from zero (so the change from 70-80 has less value than 10-30)
//NOTE usually returns a value between -1 and 1
//NOTE the "+ .001f" is just to avoid division by zero
f32 need_attenuation(f32 current, f32 future){
	return (10.0f / (current + .001f)) - (10.0f / (future + .001f));
}

//scales a score value with distance: 100 * score / distance^2
f32 distance_attenuation(f32 score, vec2i pos1, vec2i pos2){
	f32 x = pos2.x - pos1.x;
	f32 y = pos2.y - pos1.y;
	f32 d = x*x + y*y;
	return score / ((d > 1.0f) ? d : 1.0f);
}

// Add this near the top of the file, with other global variables
f32 g_negative_cost_weight = 1.5f;

//score the advert based on its positive/negative costs, completion time, distance, ...
f32 score_advert(Agent* agent, Advert* advert){
	f32 average = 0;
	forX(need_type, Need_COUNT){
		f32 current_score = agent->needs[need_type].value;
		f32 future_score  = current_score + advert->def->costs[need_type];
		if(future_score > MAX_NEED_VALUE) future_score = MAX_NEED_VALUE;
		if(future_score < 0) future_score = 0;
		
		if(advert->def->costs[need_type] > 0){
			average += need_attenuation(current_score, future_score);
		}else{
			average += g_negative_cost_weight * need_attenuation(current_score, future_score);
		}
	}
	average /= Need_COUNT;
	
	//TODO(delle) better factor in whether something will become more important due to time (mining for a whole day instead of eating) see Fero::TimeAspect
	return distance_attenuation(average, agent->entity.pos, advert->owner->pos) / advert->completion_time;
}

Advert* select_advert(Agent* agent, Advert** adverts, u32 adverts_count){
#define MAX_BEST_ADS 3
	Advert* best_ads[MAX_BEST_ADS] = {0};
	f32 best_scores[MAX_BEST_ADS] = {0};
	u32 score_count = 0;
	
	//score the adverts and get the best three
	For(adverts, adverts_count){
		f32 score = score_advert(agent, *it);
		if(score < 0) continue; //skip ads that have no benefits
		
		Advert* compare_ad = *it;
		f32 compare_score = score;
		forI(MAX_BEST_ADS){
			if(best_ads[i]){
				if(compare_score > best_scores[i]){
					Advert* temp_ad = best_ads[i];
					f32 temp_score = best_scores[i];
					best_ads[i] = compare_ad;
					best_scores[i] = compare_score;
					compare_ad = temp_ad;
					compare_score = temp_score;
				}
			}else{
				best_ads[i] = compare_ad;
				best_scores[i] = compare_score;
				score_count += 1;
				break;
			}
		}
	}
	
	//choose from the best three scores by normalizing them into a range and choosing randomly in that range
	if(score_count == 0 || best_scores[0] < MIN_ADVERT_SELECTION_SCORE){
		return 0; //idle if nothing is worth doing
	}else{
		if(score_count == 1){
			return best_ads[0];
		}
		
		f32 scores_sum = 0;
		forI(score_count){
			scores_sum += best_scores[i];
			best_scores[i] = scores_sum;
		}
		
		f32 r = ((f32)rand() / (f32)RAND_MAX) * scores_sum;
		forI(score_count){
			if(best_scores[i] >= r){
				return best_ads[i];
			}
		}
		
		//error printing
		LogE("ant_sim","Unchosen advert scores: ");
		forI(score_count) LogE("ant_sim","\t",best_ads[i]->def->name," ",best_scores[i]);
		Assert(!"An advert should always be chosen if there is a positive one.");
		return 0;
	}
#undef MAX_BEST_ADS
}

void add_action(Advert* advert, u32 action_index, ActionDef* action_def, vec2i target){
	//TODO(delle) implement add_action 
	//advert->actions_count += 1;
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @render


struct{
	struct{
		u32* data;
		Texture* texture;
	}foreground;
	
	struct{
		u32* data;
		Texture* texture;
	}background;
	
	vec2i visual_size;
	vec2i visual_position;
}rendering;

u32 get_pixelfg(u32 x, u32 y){	
	Assert(x >= 0 && y >= 0 && x <= WORLD_WIDTH && y <= WORLD_HEIGHT);
	return rendering.foreground.data[WorldIndex(x, y)];
}

b32 set_pixelfg(u32 x, u32 y, u32 val){
	if(x < 0 || y < 0 || x > WORLD_WIDTH || y > WORLD_HEIGHT) return 0;
	rendering.foreground.data[WorldIndex(x, y)] = val;
	return 1;
}

u32 get_pixelbg(u32 x, u32 y){	
	Assert(x >= 0 && y >= 0 && x <= WORLD_WIDTH && y <= WORLD_HEIGHT);
	return rendering.background.data[WorldIndex(x, y)];
}

b32 set_pixelbg(u32 x, u32 y, u32 val){
	if(x < 0 || y < 0 || x > WORLD_WIDTH || y > WORLD_HEIGHT) return 0;
	rendering.background.data[WorldIndex(x, y)] = val;
	return 1;
}

void setup_rendering(){
	rendering.foreground.data = (u32*)memalloc(sizeof(u32)*WORLD_WIDTH*WORLD_HEIGHT);
	rendering.foreground.texture = assets_texture_create_from_memory(rendering.foreground.data, STR8("world_foreground"),
																	 WORLD_WIDTH, WORLD_HEIGHT,
																	 ImageFormat_RGBA, TextureType_TwoDimensional, TextureFilter_Nearest,
																	 TextureAddressMode_ClampToTransparent, false);
	
	rendering.background.data = (u32*)memalloc(sizeof(u32)*WORLD_WIDTH*WORLD_HEIGHT);
	rendering.background.texture = assets_texture_create_from_memory(rendering.background.data, STR8("world_background"),
																	 WORLD_WIDTH, WORLD_HEIGHT,
																	 ImageFormat_RGBA, TextureType_TwoDimensional, TextureFilter_Nearest,
																	 TextureAddressMode_ClampToTransparent, false);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @simulation


typedef u32 WeatherType;
enum{
	Weather_Clear,
	Weather_Cloudy,
	Weather_Rain,
	Weather_Thunderstorm,
	Weather_Snow,
};

typedef u32 ToolType;
enum{
	Tool_Navigate,
	Tool_Draw,
	Tool_Erase,
};

struct{
	Entity** map;
	
	struct{
		WeatherType type;
		s32 wind_strength;
		s32 temperature; // celsius
	}weather;
}world;

struct{
	ToolType tool;
	struct{
		EntityType entity_type;
		AgentType agent_type;
	}drawing;
	Entity* break_on_me;
	Entity* selected_entity;
	Advert* selected_advert;
	u64 ticks;
	u32 ticks_per_second;
	f64 tick_delta;
	f64 tick_accumulator;
	b32 paused;
	b32 step;
}sim;

struct{
	u32 entity[Entity_COUNT]; // count of each entity
	u32 entities; // total entities
	u32 agent[Agent_COUNT];
	u32 agents; // sub count of entities
	u32 actions;
	u32 advert[Advert_COUNT];
	u32 adverts;
}counts = {0};

Advert* adverts_pool;
Entity* entities_pool;
Agent* agents_pool;

void change_tool(ToolType tool){
	switch(tool){
		case Tool_Navigate:{
			g_ui->keys.drag_item = Mouse_LEFT;
		}break;
		case Tool_Draw:{
			g_ui->keys.drag_item = Mouse_MIDDLE;
		}break;
		case Tool_Erase:{
			g_ui->keys.drag_item = Mouse_MIDDLE;
		}break;
	}
	sim.tool = tool;
}

Entity* get_entity(vec2i pos){
	if(pos.x < 0 || pos.y < 0 || pos.x > WORLD_WIDTH || pos.y > WORLD_HEIGHT) return 0;
	return world.map[WorldIndex(pos.x, pos.y)];
}
FORCE_INLINE Entity* get_entity(u32 x, u32 y){ return get_entity(Vec2i(x,y)); }

b32 set_entity(vec2i pos, Entity* entity){
	if(pos.x < 0 || pos.y < 0 || pos.x > WORLD_WIDTH || pos.y > WORLD_HEIGHT) return false;
	world.map[WorldIndex(pos.x, pos.y)] = entity;
	set_pixelfg(pos.x,pos.y, (entity) ? entity->color : 0);
	return true;
}
FORCE_INLINE b32 set_entity(u32 x, u32 y, Entity* entity){ return set_entity(Vec2i(x,y), entity); }

b32 move_entity(Entity* e, vec2i pos){
	if(get_entity(pos.x,pos.y)) return false;
	set_entity(e->pos,0);
	set_entity(pos,e);
	e->pos = pos;
	return true;
}
FORCE_INLINE b32 move_entity(Entity* e, u32 x, u32 y){ return move_entity(e,Vec2i(x,y)); }

void swap_entities(Entity* e0, Entity* e1){
	auto pos0 = e0->pos;
	auto pos1 = e1->pos;
	set_entity(pos0, e1);
	set_entity(pos1, e0);
	e1->pos = pos0;
	e0->pos = pos1;
}

//walks the path between two positions to see if there are any entities inbetween
b32 line_of_sight(vec2i start, vec2i end){
	vec2i between;
	while(true){
		between = vec2i_subtract(end, start);
		
		if(between.x == 0 && between.y == 0){
			return true;
		}
		
		if(abs(between.x) > abs(between.y)){
			if      (between.x > 0){
				start.x += 1;
			}else if(between.x < 0){
				start.x -= 1;
			}
		}else{
			if      (between.y > 0){
				start.y += 1;
			}else if(between.y < 0){
				start.y -= 1;
			}
		}
		
		if(get_entity(start) != 0){
			return false;
		}
	}
}

//pathfind from agent position to target position using A* algorithm on the raw grid
//!ref: https://www.redblobgames.com/pathfinding/a-star/introduction.html#astar
void generate_path(Agent* agent, vec2i target){
	persist Allocator deshi_allocator_no_release{
		deshi__memory_generic_allocate_allocator,
		Allocator_ReleaseMemory_Noop,
		deshi__memory_generic_reallocate_allocator
	};
	
	//remove the agent's previous path
	memory_zfree(agent->path_nodes);
	agent->path_nodes = 0;
	agent->path_count = 0;
	
	//early out if already at the target
	if(vec2i_equal(agent->entity.pos, target)){
		return;
	}
	
	//BEGIN_ALGORITHM: A* with uniform tile movement cost
	b32 valid_path = false;
	map<vec2i,u8> directions(deshi_temp_allocator); //<pos,direction> (direction = previous path tile -> this tile)
	arrayT<pair<vec2i,u32>> frontier(128,deshi_temp_allocator); //priority stack (lowest prio -> highest prio)
	frontier.add({agent->entity.pos,0});
	
	//iterate frontier (tiles not yet assigned a direction towards the path start)
	while(frontier.count){
		//travel along the frontier stack
		vec2i current = frontier.pop().first;
		
		//path is valid if the target is reached
		if(vec2i_equal(current, target)){
			valid_path = true;
			break;
		}
		
		//iterate neighbors
		forX(direction,4){
			vec2i next = vec2i_add(current, direction_to_movement[direction]);
			if(!directions.has(next)){
				//insert to frontier based on priority (so that travelling the frontier goes towards the target)
				u32 priority = abs(target.x - next.x) + abs(target.y - next.y); //manhattan distance on square grid
				u32 frontier_idx = 0;
				while(frontier_idx < frontier.count){
					if(priority <= frontier[frontier_idx].second){
						frontier.insert({next,priority}, frontier_idx);
						break;
					}
					frontier_idx += 1;
				}
				if(frontier_idx == frontier.count){
					frontier.add({next,priority});
				}
				
				//store the direction
				directions.add(next, direction);
			}
		}
	}
	//END_ALGORITHM: A* with uniform tile movement cost
	
	//create the path and give it to the agent
	if(valid_path){
		vec2i current = target;
		arrayT<u8> path(32,&deshi_allocator_no_release);
		while(!vec2i_equal(current, agent->entity.pos)){
			u8 direction = *directions.at(current);
			path.add(direction);
			current = vec2i_add(current, direction_to_movement[(direction + 2) % 4]); //add 2 and wrap if over 4 to go to opposite direction
		}
		reverse(path.data, path.count);
		
		agent->path_nodes = path.data;
		agent->path_count = path.count;
		agent->path_index = 0;
	}
}

Advert* make_advert(AdvertType type, Entity* owner, vec2i target){
	Assert(type < Advert_COUNT);
	AdvertDef* def = &AdvertDefinitions[type];
	Advert* advert = memory_pool_push(adverts_pool);
	advert->def = def;
	advert->owner = owner;
	forI(MAX_ADVERT_DEF_ACTIONS){
		if(def->actions[i] == 0) break;
		
		advert->actions_array[i].def = def->actions[i];
		advert->actions_array[i].target = target;
		advert->actions_array[i].progress = 0;
		
		advert->completion_time += def->actions[i]->time;
		advert->actions_count   += 1;
	}
	
	if(owner){
		Assert(owner->adverts_count < MAX_ENTITY_ADVERTS);
		owner->adverts_array[owner->adverts_count] = advert;
		owner->adverts_count += 1;
	}
	
	counts.adverts += 1;
	counts.advert[type] += 1;
	return advert;
}

Agent* make_agent(AgentType type, u32 age, vec2i pos){
	Assert(type < Agent_COUNT);
	Agent* agent = (Agent*)memory_pool_push(agents_pool);
	agent->entity.type   = Entity_Agent;
	agent->entity.age    = age;
	agent->entity.pos    = pos;
	agent->type          = type;
	agent->action_index  = -1;
	agent->active_advert = 0;
	CopyMemory(agent->needs, AgentNeedDefinitions[type], Need_COUNT*sizeof(Need));

	counts.agents += 1;
	counts.agent[type] += 1;
	counts.entities += 1;
	counts.entity[Entity_Agent] += 1;
	return agent;
}

Entity* make_entity(EntityType type, vec2i pos, u32 age){
	Assert(type < Entity_COUNT);
	Entity* entity = memory_pool_push(entities_pool);
	entity->type = type;
	entity->age  = age;
	entity->pos  = pos;
	
	for(u32 i = 0; i < MAX_ENTITY_INNATE_ADVERTS; i += 1){
		AdvertType advert_type = EntityInnateAdverts[type][i];
		if (advert_type != Advert_NULL){
			make_advert(advert_type, entity, vec2i{});
		}
	}
	
	counts.entities += 1;
	counts.entity[type] += 1;
	return entity;
}

void delete_advert(Advert* advert)
{
	if(advert->owner){
		for(u32 i = 0; i < advert->owner->adverts_count; i += 1){
			if(advert->owner->adverts_array[i] == advert){
				if(i != advert->owner->adverts_count-1){
					advert->owner->adverts_array[i] = advert->owner->adverts_array[advert->owner->adverts_count-1];
				}
				advert->owner->adverts_array[advert->owner->adverts_count-1] = 0;
				advert->owner->adverts_count -= 1;
				break;
			}
		}
	}
	
	counts.adverts -= 1;
	counts.advert[AdvertTypeFromAdvertDef(advert->def)] -= 1;
	memory_pool_delete(adverts_pool, advert);
}

void delete_entity(Entity* entity){
	Assert(entity != 0);
	
	//TODO(delle) handle entity->overlap_node
	if(entity == get_entity(entity->pos)){
		set_entity(entity->pos, 0);
	}
	
	for(u32 i = 0; i < entity->adverts_count; i += 1){
		counts.adverts -= 1;
		counts.advert[AdvertTypeFromAdvertDef(entity->adverts_array[i]->def)] -= 1;
		memory_pool_delete(adverts_pool, entity->adverts_array[i]);
	}
	
	counts.entity[entity->type] -= 1;
	counts.entities -= 1;
	
	if(entity->type == Entity_Agent){
		Agent* agent = AgentFromEntity(entity);
		counts.agent[agent->type] -= 1;
		counts.agents -= 1;
		memory_pool_delete(agents_pool, agent);
	}else{
		memory_pool_delete(entities_pool, entity);
	}
}

Advert** collect_adverts(Agent* agent, u32* out_count){
#define MAX_COLLECTED_ADVERTS 64
	Assert(agent != 0);
	Assert(out_count != 0);
	
	static Advert* adverts[MAX_COLLECTED_ADVERTS];
	u32 count = 0;
	
	//check the agent's current chunk
	s32 agent_chunk_x = WorldToChunkX(agent->entity.pos.x);
	s32 agent_chunk_y = WorldToChunkY(agent->entity.pos.y);
	s32 agent_chunk_world_index = WorldIndex(ChunkStartX(agent_chunk_x), ChunkStartY(agent_chunk_y));
	for(s32 i = agent_chunk_world_index; i < WORLD_CHUNK_SIZE*WORLD_CHUNK_SIZE; i += 1){
		Entity* entity = world.map[i];
		if(entity != 0){
			Assert(entity->type != Entity_NULL);
			s32 dist_sq = vec2i_distanceToSq(agent->entity.pos, entity->pos);
			for(s32 i = 0; i < entity->adverts_count; i++){
				if(dist_sq <= entity->adverts_array[i]->def->range_sq){
					Assert(count < MAX_COLLECTED_ADVERTS);
					adverts[count++] = entity->adverts_array[i];
					if(count >= MAX_COLLECTED_ADVERTS) goto end_collection;
				}
			}
		}
	}
	
	//randomize the order of the chunks checked
	s32 chunk_offsets[8][2] = { {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1} };
	forI(8){
		s32 random_index = rand() % 8;
		s32 temp_offset_x = chunk_offsets[i][0];
		s32 temp_offset_y = chunk_offsets[i][1];
		chunk_offsets[i][0] = chunk_offsets[random_index][0];
		chunk_offsets[i][1] = chunk_offsets[random_index][1];
		chunk_offsets[random_index][0] = temp_offset_x;
		chunk_offsets[random_index][1] = temp_offset_y;
	}
	
	//check the surrounding chunks
	//NOTE: this isn't ideal since it always starts searching at the top right of the chunk
	for(s32 i = 0; i < 8; i += 1){
		s32 chunk_x = agent_chunk_x + chunk_offsets[i][0];
		s32 chunk_y = agent_chunk_y + chunk_offsets[i][1];
		if(chunk_x >= 0 && chunk_y >= 0 && chunk_x < WORLD_CHUNKS_X && chunk_y < WORLD_CHUNKS_Y){
			s32 chunk_world_index = WorldIndex(ChunkStartX(chunk_x), ChunkStartY(chunk_y));
			for(s32 i = chunk_world_index; i < WORLD_CHUNK_SIZE*WORLD_CHUNK_SIZE; i += 1){
				Entity* entity = world.map[i];
				if(entity != 0){
					Assert(entity->type != Entity_NULL);
					s32 dist_sq = vec2i_distanceToSq(agent->entity.pos, entity->pos);
					for(s32 i = 0; i < entity->adverts_count; i++){
						if(dist_sq <= entity->adverts_array[i]->def->range_sq){
							Assert(count < MAX_COLLECTED_ADVERTS);
							adverts[count++] = entity->adverts_array[i];
							if(count >= MAX_COLLECTED_ADVERTS) goto end_collection;
						}
					}
				}
			}
		}
	}
	
end_collection:
	*out_count = count;
	return adverts;
}

void perform_actions(Agent* agent){
	if(agent->active_advert == 0) return;
	
	//perform action
	Advert* advert = agent->active_advert;
	Action* action = &advert->actions_array[agent->action_index];
	switch(action->def->type){
		case Action_Idle:{
			action->progress += (1.0f / (f32)action->def->time);
		}break;
		
		case Action_Walk:{
			//check if a path has already been generated
			if(agent->path_nodes == 0){
				//at destination
				if(vec2i_equal(agent->entity.pos, action->target)){
					action->progress = 1.0f;
				}
				
				//generate a path if one doesnt exist yet and consume the tick doing it (wait)
				generate_path(agent, action->target);
			}else{
				//move to next nav node
				vec2i next_node = agent->entity.pos + direction_to_movement[agent->path_nodes[agent->path_index]];
				if(move_entity(&agent->entity, next_node)){
					agent->path_index += 1;
				}else{
					//entity in the way of the path, so generate new path or stack on top if same agent
					Entity* entity_in_way = get_entity(next_node);
					if((entity_in_way->type == Entity_Agent) && (AgentFromEntity(entity_in_way)->type == agent->type)){
						//TODO(delle) handle entity stacking
					}else{
						generate_path(agent, action->target);
					}
				}
				
				//at destination
				if(vec2i_equal(agent->entity.pos, action->target)){
					action->progress = 1.0f;
				}
			}
		}break;
		
		case Action_Dig:{
			Entity* target_entity = get_entity(action->target);
			
			//too far from target
			if(vec2i_distanceToSq(agent->entity.pos, action->target) > 1){
				//if line of sight and the dirt is gone, skip the action; else, walk to the dirt
				if(line_of_sight(agent->entity.pos, action->target) && target_entity && target_entity->type != Entity_Dirt){
					agent->action_index += 1;
				}else{
					//TODO(delle) handle pathing to dig once we switch to graph navigation (to allow travelling to closest point and not knowing if the path is valid)
					action->progress = 1.0f;
					
					//Path* path = generate_path(agent, action->target);
					//vec2i closest_point = path->nodes_array[path->nodes_count-1].pos;
					//add_action(advert, agent->action_index, &ActionDefinitions[Action_Walk], closest_point);
					//perform_actions(agent);
					//return;
				}
			}
			
			if(target_entity && target_entity->type == Entity_Dirt){
				set_entity(action->target, 0);
				//update_navgraph(); //TODO(delle) smarter modification of the navgraph
			}
			
			//TODO(delle) adding dirt to inventory to drag it
		}break;
		
		case Action_EatLeaf:{
			//TODO(delle) leaf eating
			action->progress = 1.0f;
		}break;
		
		case Action_DrinkWater:{
			//TODO(delle) water drinking
			action->progress = 1.0f;
		}break;
		
		default:{
			if(action->def->type < Action_COUNT){
				LogE("ant_sim","Unhandled action type: ",action->def->name);
			}else{
				LogE("ant_sim","Unhandled action type: ",action->def->type);
			}
		}break;
	}
	
	//if action completed
	if(action->progress >= 1.0f){
		//award costs
		forX(need_type, Need_COUNT){
			if(abs(advert->def->costs[need_type] - action->def->costs[need_type]) > ADVERT_COST_DELTA_EPSILON){
				//TODO(delle) make a memory if adverted completion reward didnt match actual
			}
			
			delta_need(&agent->needs[need_type], advert->def->costs[need_type]);
		}
		
		//next action
		agent->action_index += 1;
	}
	
	//if advert completed
	if(agent->action_index >= advert->actions_count){
		if(abs((int)(advert->completion_time - advert->ongoing_time)) > ADVERT_TIME_DELTA_EPSILON){
			//TODO(delle) make a memory if adverted completion time didnt match actual
		}
		
		//remove advert from queue
		agent->active_advert = 0;
		
		//delete the advert
		if(HasFlag(advert->def->flags, AdvertDefFlags_ConsumeOwnerOnCompletion)){
			delete_entity(advert->owner);
		}
		if(HasFlag(advert->def->flags, AdvertDefFlags_ConsumeAdvertOnCompletion)){
			delete_advert(advert);
		}
	}
}

//TODO(sushi) leaf decay: https://encycolorpedia.com/d57835
void eval_leaf(Entity* e){
	if(e->age >= 2*TICKS_PER_WORLD_DAY)
	{
		Assert(e->adverts_count > 0);
		Assert(e->adverts_array[0] != 0);
		Assert(AdvertTypeFromAdvertDef(e->adverts_array[0]->def) == Advert_EatLeaf);
		
		forI(e->adverts_count){
			delete_advert(e->adverts_array[i]);
		}
		
		forI(MAX_ENTITY_INNATE_ADVERTS){
			AdvertType advert_type = EntityInnateAdverts[Entity_Dirt][i];
			if (advert_type != Advert_NULL){
				make_advert(advert_type, e, vec2i{});
			}
		}
		
		e->type = Entity_Dirt;
		e->age = 0;
		e->color = EntityColors[Entity_Dirt][rand()%7];
		set_pixelfg(e->pos.x, e->pos.y, e->color);
		return;
	}
	
	if(!e->pos.y || get_entity(e->pos.x,e->pos.y-1)){
		// stupid way to try and get leaves to spread out 
		vec2i search = e->pos;
		while(search.y < WORLD_HEIGHT && get_entity(search.x,search.y)){
			search.y += 1;
		}
		
		if(search.y - e->pos.y > 1){
			if(e->pos.x == 0 && !get_entity(e->pos.x+1, e->pos.y)){
				move_entity(e, {e->pos.x+1, e->pos.y});
			}else if(e->pos.x == WORLD_WIDTH-1 && !get_entity(e->pos.x-1, e->pos.y)){
				move_entity(e, {e->pos.x-1, e->pos.y});
			}else{
				Entity* l = get_entity(e->pos.x-1, e->pos.y);
				Entity* r = get_entity(e->pos.x+1, e->pos.y);
				if(!l&&!r) move_entity(e, {e->pos.x+(rand()%2?1:-1), e->pos.y});
				else if(l) move_entity(e, {e->pos.x+1, e->pos.y});
				else if(r) move_entity(e, {e->pos.x-1, e->pos.y});
			}
		}
		
		if(search.x != e->pos.x){
			forI(search.y - e->pos.y - 1){
				Entity* above = get_entity(search.x, e->pos.y + i + 1);
				if(above && above->type == Entity_Leaf){
					move_entity(above, {above->pos.x, above->pos.y-1});
				}
			}
		}
	}else{
		if(e->age % 5) return;
		vec2i nupos = e->pos;
		nupos.y--;
		s32 r = rand() % 3 + world.weather.wind_strength; 
		nupos.x += r;
		nupos.x = Clamp(nupos.x, 0, WORLD_WIDTH);
		nupos.y = Clamp(nupos.y, 0, WORLD_HEIGHT-1);
		move_entity(e, nupos);
	}
}

void eval_water(Entity* e){
	if(!e->pos.y || get_entity(e->pos.x,e->pos.y-1)){
		if(e->pos.x == 0 && !get_entity(e->pos.x+1,e->pos.y-1)){
			move_entity(e,{e->pos.x+1,e->pos.y-1});
		}else if(e->pos.x == WORLD_WIDTH-1 && !get_entity(e->pos.x-1,e->pos.y-1)){
			move_entity(e,{e->pos.x-1,e->pos.y-1});
		}else{
			Entity* bl = get_entity(e->pos.x-1,e->pos.y-1);
			Entity* br = get_entity(e->pos.x+1,e->pos.y-1);
			if(br&&bl){
				Entity* l = get_entity(e->pos.x-1,e->pos.y);
				Entity* r = get_entity(e->pos.x+1,e->pos.y);
				move_entity(e,{e->pos.x+(rand()%2?1:-1),e->pos.y});
				if(!l&&!r) move_entity(e,{e->pos.x+(rand()%2?1:-1),e->pos.y});
				else if(l) move_entity(e,{e->pos.x+1,e->pos.y});
				else if(r) move_entity(e,{e->pos.x-1,e->pos.y});
			} 
			else if(!bl&&!br) move_entity(e,{e->pos.x+(rand()%2?1:-1),e->pos.y-1});
			else if(bl) move_entity(e,{e->pos.x+1,e->pos.y-1});
			else if(br) move_entity(e,{e->pos.x-1,e->pos.y-1});
		}
	}else{
		vec2i nupos = e->pos;
		nupos.y--;
		//TODO(sushi) this needs to check along the line of movement, not just across x
		forI(abs(world.weather.wind_strength)/2){
			u32 move = (world.weather.wind_strength>0?1:-1);
			if(get_entity(nupos.x+move,e->pos.y-1)) break;
			nupos.x += move;
		}
		//nupos.x += world.weather.wind_strength / 2;
		nupos.x = Clamp(nupos.x, 0, WORLD_WIDTH);
		nupos.y = Clamp(nupos.y, 0, WORLD_HEIGHT-1);
		move_entity(e, nupos);
	}
	
	// TODO(sushi) better water sim so that water equilizes
	// if(e->water.evaluated == sim.ticks % 2) return;
	// e->water.evaluated = sim.ticks % 2;
	
	// if(sim.break_on_me == e){
	// 	DebugBreakpoint;
	// 	sim.break_on_me = 0;
	// } 
	
	// Entity* t = get_entity(e->pos.x,e->pos.y+1);
	// Entity* r = get_entity(e->pos.x+1,e->pos.y);
	// Entity* b = get_entity(e->pos.x,e->pos.y-1);
	// Entity* l = get_entity(e->pos.x-1,e->pos.y);
	
	// if(b || !e->pos.y){
	// 	//if there is an entity below us, or we are at the bottom of the world
	// 	if(t && t->type == Entity_Water){
	// 		// if there is an entity above us and it is water, we try to evaluate it so that
	// 		// it knows its water pressure
	// 		eval_water(t);
	// 		if(t->pos == vec2i{e->pos.x,e->pos.y+1}){
	// 			// if the water has not moved, we set this water's pressure to its pressure plus 1
	// 			e->water.pressure = t->water.pressure + 1;
	// 		}
	// 	}
	
	// 	if(r && r->type == Entity_Water){
	
	// 	}
	// }
	
	
	
	// if(b || !e->pos.y){
	// 	if(t && t->type == Entity_Water){
	// 		eval_water(t);
	// 		if(t->pos == vec2i{e->pos.x,e->pos.y+1}){
	// 			e->water.pressure = t->water.pressure + 1;
	// 		}else{
	// 			e->water.pressure = 0;
	// 			t = 0;
	// 		}
	// 	}else{
	// 		e->water.pressure = 0;
	// 	}
	
	// 	if(r && r->type == Entity_Water){
	// 		eval_water(r);
	// 		if(r->pos == vec2i{e->pos.x+1,e->pos.y}){
	// 			if(r->water.pressure > e->water.pressure){
	// 				e->water.pressure += r->water.pressure;
	// 			}
	// 		}else{
	// 			r = 0;
	// 		}
	// 	}
	
	// 	if(l && l->type == Entity_Water){
	// 		eval_water(l);
	// 		if(l->pos == vec2i{e->pos.x-1,e->pos.y}){
	// 			if(l->water.pressure > e->water.pressure){
	// 				e->water.pressure += l->water.pressure;
	// 			}
	// 		}else{
	// 			l = 0;
	// 		}
	// 	}
	
	// 	if(b->type == Entity_Water){
	// 		eval_water(b);
	// 		if(b->pos == vec2i{e->pos.x,e->pos.y-1}){
	// 			// if(b->water.pressure > e->water.pressure + 1){
	// 			// 	e->water.pressure = b->water.pressure - 1;
	// 			// }
	// 		}else{
	// 			b = 0;
	// 		}
	// 	}
	
	// 	if(!t && b){
	// 		if(b->type == Entity_Water && b->water.pressure > e->water.pressure + 1){
	// 			move_entity(e,e->pos.x,e->pos.y+1);
	// 		}else if(b->type != Entity_Water && e->water.pressure){
	// 			move_entity(e,e->pos.x,e->pos.y+1);
	// 		}
	// 	}else if(e->pos.x == 0 && !get_entity(e->pos.x+1,e->pos.y-1)){
	// 		move_entity(e,{e->pos.x+1,e->pos.y-1});
	// 	}else if(e->pos.x == WORLD_WIDTH-1 && !get_entity(e->pos.x-1,e->pos.y-1)){
	// 		move_entity(e,{e->pos.x-1,e->pos.y-1});
	// 	}else if(l && !r && e->water.pressure){
	// 		move_entity(e,e->pos.x+1,e->pos.y);
	// 	}else if(r && !l && e->water.pressure){
	// 		move_entity(e,e->pos.x-1,e->pos.y);
	// 	}else if (!r && !l && e->water.pressure){
	// 		move_entity(e,e->pos.x+(rand()%2?1:-1),e->pos.y);
	// 	}else{
	// 		Entity* bl = get_entity(e->pos.x-1,e->pos.y-1);
	// 		Entity* br = get_entity(e->pos.x+1,e->pos.y-1);
	// 		// if(br&&bl){
	// 		// 	Entity* l = get_entity(e->pos.x-1,e->pos.y);
	// 		// 	Entity* r = get_entity(e->pos.x+1,e->pos.y);
	// 		// 	move_entity(e,{e->pos.x+(rand()%2?1:-1),e->pos.y});
	// 		// 	if(!l&&!r) 
	// 		// 	else if(l) move_entity(e,{e->pos.x+1,e->pos.y});
	// 		// 	else if(r) move_entity(e,{e->pos.x-1,e->pos.y});
	// 		// } 
	// 		if(!bl&&!br) move_entity(e,{e->pos.x+(rand()%2?1:-1),e->pos.y-1});
	// 		else if(bl) move_entity(e,{e->pos.x+1,e->pos.y-1});
	// 		else if(br) move_entity(e,{e->pos.x-1,e->pos.y-1});
	// 	}
	// }else{
	// 	vec2i nupos = e->pos;
	// 	nupos.y--;
	// 		//TODO(sushi) this needs to check along the line of movement, not just across x
	// 	forI(abs(world.weather.wind_strength)/2){
	// 		u32 move = (world.weather.wind_strength>0?1:-1);
	// 		if(get_entity(nupos.x+move,e->pos.y-1)) break;
	// 		nupos.x += move;
	// 	}
	// 	//nupos.x += world.weather.wind_strength / 2;
	// 	nupos.x = Clamp(nupos.x, 0, WORLD_WIDTH);
	// 	nupos.y = Clamp(nupos.y, 0, WORLD_HEIGHT-1);
	// 	move_entity(e, nupos);
	// }
}

void setup_simulation(){
	//init RNG
	srand(12345);
	
	//init storage
	world.map = (Entity**)memalloc(sizeof(Entity*) * WORLD_WIDTH * WORLD_HEIGHT);
	memory_pool_init(agents_pool, 1024);
	memory_pool_init(entities_pool, 1024);
	memory_pool_init(adverts_pool, 1024);
	
	//init weather
	world.weather.type = Weather_Clear;
	world.weather.wind_strength = 0;
	
	//blue background
	forI(WORLD_HEIGHT*WORLD_WIDTH){
		rendering.background.data[i] = PackColorU32(178,219,243,255);
	}
	
	//generate terrain foreground and background
	//TODO(sushi) initialize world by spawning all items in mid air so they fall down and it looks cool
	s32 vel = 0;
	s32 pos = WORLD_HEIGHT / 2;
	forI(WORLD_WIDTH){
		if(i%WORLD_WIDTH/(rand() % 8 + 8)){
			u32 mag = 0;
			if     (rand()%16==0){ mag = 8; }
			else if(rand()% 8==0){ mag = 6; }
			else if(rand()% 4==0){ mag = 4; }
			else                   mag = 2;
			
			if(pos < u32(WORLD_HEIGHT/6.0f)) vel = rand() % mag+1;
			else if(pos > u32(5.0f*WORLD_HEIGHT/6.0f)) vel = -(rand() % mag + 1);
			else vel = rand() % (mag+1) - mag/2;
		}
		pos += vel;
		pos = Clamp(pos,0, WORLD_HEIGHT-1);
		
		u32 color = EntityColors[Entity_Dirt][rand()%7];
		forX(j,pos){
			if(rand()%2 == 0) color = EntityColors[Entity_Dirt][rand()%7];
			Entity* e = make_entity(Entity_Dirt, {i,j}, 0);
			e->color = color;
			set_entity(i,j,e);
			
			u32 bg_color = divide_color(EntityColors[Entity_Dirt][rand()%7], 1.5);
			set_pixelbg(i,j,bg_color);
		}
	}
	
	sim.tool = Tool_Navigate;
	sim.break_on_me = 0;
	sim.selected_entity = 0;
	sim.selected_advert = 0;
	sim.ticks = 0;
	sim.ticks_per_second = 10;
	sim.tick_delta = 1.0 / (f64)sim.ticks_per_second;
	sim.tick_accumulator = 0.0;
	sim.paused = false;
	sim.step = false;
}

void update_simulation(){
	if(sim.paused && !sim.step){
		return;
	}
	
	if(sim.step){
		sim.step = false;
		sim.tick_accumulator += sim.tick_delta + M_EPSILON;
	}else{
		sim.tick_accumulator += g_time->deltaTime / 1000.0;
	}
	
	while(sim.tick_accumulator - sim.tick_delta > (f64)M_EPSILON){
		//agents tick needs and choose adverts
		for_pool(agents_pool){
			Agent* agent = it;
			if(agent->entity.type == Entity_NULL){
				continue;
			}
			
			forX(need_type, Need_COUNT){
				delta_need(&agent->needs[need_type], agent->needs[need_type].delta);
			}
			
			if(agent->active_advert == 0){
				u32 adverts_count;
				Advert** adverts = collect_adverts(agent, &adverts_count);
				agent->active_advert = select_advert(agent, adverts, adverts_count);
			}
		}
		
		//agents perform actions (after all agents decide what to do)
		for_pool(agents_pool){
			if(it->entity.type != Entity_NULL){
				perform_actions(it);
			}
		}
		
		//update entities
		for_pool(entities_pool){
			if(it == sim.break_on_me){
				sim.break_on_me = 0;
				DebugBreakpoint;
			}
			switch(it->type){
				case Entity_Leaf: eval_leaf(it); break;
				case Entity_Water: eval_water(it); break;
			}
			it->age += 1;
		}
		
		//spawn more leaves
		if(counts.entity[Entity_Leaf] < 50){
			u32 add = (50 - counts.entity[Entity_Leaf]) +  rand() % 10; 
			forI(add){
				vec2i pos = {rand() % WORLD_WIDTH, WORLD_HEIGHT-1};
				while(get_entity(pos.x,pos.y) && pos.y) pos.y -= 1;
				if(!pos.y){ add--; continue; } // we somehow failed to place the leaf anywhere in the random column, whatever
				Entity* e = make_entity(Entity_Leaf, pos, 0);
				e->color = EntityColors[Entity_Leaf][rand()%7];
				set_entity(pos.x,pos.y,e);
			}
		}
		
		//weather
		switch(world.weather.type){
			case Weather_Clear:{
				//do nothing
			}break;
			case Weather_Rain:{
				forI(rand() % 10){
					vec2i pos = {rand() % WORLD_WIDTH, WORLD_HEIGHT-1};
					Entity* e = make_entity(Entity_Water, pos, 0);
					e->color = EntityColors[Entity_Water][rand()%7];
					set_entity(pos.x,pos.y,e);
				}
			}break;
		}
		
		sim.tick_accumulator -= sim.tick_delta;
		sim.ticks += 1;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @ui


struct{
	uiItem* universe;
	uiItem* world;
	uiItem* background; // NOTE(sushi) the foreground is a child of the background so that it follows it and always draws over
	uiItem* foreground;
	uiItem* info;
	uiItem* agent_collected_adverts_header;
}ui;

// returns a boolean indicating if the mouse is actually over the world 
pair<vec2i, b32> get_tile_under_mouse(){
	vec2 mouse_pos = input_mouse_position();
	if(mouse_pos.x > ui.foreground->pos_screen.x + ui.foreground->size.x || mouse_pos.x < ui.foreground->pos_screen.x ||
	   mouse_pos.y > ui.foreground->pos_screen.y + ui.foreground->size.y || mouse_pos.y < ui.foreground->pos_screen.y ||
	   !ui.foreground->height || !ui.foreground->width){
		return {{0},0};
	}
	vec2i local_pos;
	local_pos.x = mouse_pos.x - ui.foreground->pos_screen.x;
	local_pos.y = mouse_pos.y - ui.foreground->pos_screen.y;
	local_pos.x = floor(f32(local_pos.x) / f32(ui.foreground->width) * WORLD_WIDTH);
	local_pos.y = floor(f32(local_pos.y) / f32(ui.foreground->height) * WORLD_HEIGHT);
	local_pos.y = WORLD_HEIGHT-1 - local_pos.y;
	return {local_pos, 1};
}

Entity* get_entity_under_mouse(){
	auto [pos, ok] = get_tile_under_mouse();
	if(!ok) return 0; 
	return get_entity(pos.x, pos.y);
}

// allocates a temporary string
str8 aligned_text(u32 rows, u32 columns, arrayT<str8> texts){
	dstr8 build;
	dstr8_init(&build, {0}, deshi_temp_allocator);
	
	u32* max = (u32*)StackAlloc(sizeof(u32)*columns);
	memset(max, 0, sizeof(u32)*columns);
	
	forI(rows*columns){
		u32 len = texts[i].count;
		if(len > max[i%columns]){
			max[i%columns] = len;
		}
	}
	
	forI(rows*columns){
		u32 len = max[i%columns];
		dstr8_grow(&build, len);
		memcpy(build.str+build.count, texts[i].str, texts[i].count);
		memset(build.str+build.count+texts[i].count, ' ', len-texts[i].count);
		build.count += len;
		if(i%columns == columns-1 && i != rows*columns-1){
			dstr8_append(&build, STR8("\n"));
		}
	}
	
	return build.fin;
}

void setup_ui(){
	Font* default_font = assets_font_get_by_name(STR8("baked_gohufont_11_bdf"));
	if(!default_font){
		default_font = assets_font_create_from_memory_bdf(baked_font_gohufont_11_bdf.str, baked_font_gohufont_11_bdf.count, STR8("baked_gohufont_11_bdf"));
	}
	
	g_ui->base.style.font        = default_font;
	g_ui->base.style.font_height = 11;
	g_ui->base.style.text_color  = Color_White;
	
	uiStyle header_style = {0};
	header_style.display = display_horizontal;
	header_style.sizing = size_percent_x | size_auto_y;
	header_style.width = 100/*percent*/;
	header_style.padding_top = 1/*pixels*/;
	header_style.padding_bottom = 1/*pixels*/;
	header_style.margin_top = 5/*pixels*/;
	header_style.margin_bottom = 5/*pixels*/;
	header_style.background_color = Color_DarkCyan;
	
	uiItem* main = ui_begin_item(0);{ main->id = STR8("ant_sim.main");
		main->style.sizing = size_percent;
		main->style.size = {100,100};
		main->style.display = display_horizontal | display_flex;
		
		ui.universe = ui_begin_item(0);{ ui.universe->id = STR8("ant_sim.universe");
			ui.universe->style.sizing = size_flex | size_percent_y;
			ui.universe->style.size = {6, 100};
			ui.universe->style.background_color = {5,5,5,255};
			
			//TODO multiple views on the world
			ui.world = ui_begin_item(0);{ ui.world->id = STR8("ant_sim.universe.world");
				ui.world->style.sizing = size_auto;
				ui.world->style.positioning = pos_draggable_fixed;
				ui.world->style.padding = {10,10,10,10};
				ui.world->style.border_style = border_solid;
				ui.world->style.border_width = 4;
				ui.world->style.border_color = Color_White;
				
				ui.background = ui_begin_item(0);{ ui.background->id = STR8("ant_sim.universe.world.background");
					ui.background->style.background_image = rendering.background.texture;
					ui.background->style.background_color = Color_White;
					f32 initial_size = Min(0.6f * (f32)g_window->width - 28, (f32)g_window->height - 28);
					ui.background->style.size = {initial_size, initial_size};
					ui.background->style.hover_passthrough = true;
					
					ui.foreground = ui_begin_item(0);{ ui.foreground->id = STR8("ant_sim.universe.world.foreground");
						ui.foreground->style.background_image = rendering.foreground.texture;
						ui.foreground->style.background_color = Color_White;
						ui.foreground->style.sizing = size_percent;
						ui.foreground->style.size = {100,100};
						ui.foreground->style.hover_passthrough = true;
					}ui_end_item();
				}ui_end_item();
			}ui_end_item();
		}ui_end_item();
		
		ui.info = ui_begin_item(0);{ ui.info->id = STR8("ant_sim.info");
			ui.info->style.sizing = size_flex | size_percent_y;
			ui.info->style.size = {4, 100};
			ui.info->style.padding = {10,10,10,10};
			ui.info->style.background_color = {20,20,20,255};
			
			uiItem* tools_header = ui_begin_collapsing_header(STR8("Tools"), false, 0);{ tools_header->id = STR8("ant_sim.info.tools");
				//TODO replace with combo box
				uiItem* tool_select = ui_begin_item(0);{ tool_select->id = STR8("ant_sim.info.tools.tool_select");
					tool_select->style.background_color = Color_DarkGrey;
					tool_select->style.sizing = size_auto;
					tool_select->style.padding = {2,2,2,2};
					tool_select->style.display = display_horizontal;
					
					//TODO(sushi) add visual feedback to these buttons
					uiItem* navigate = ui_begin_item(0);{ navigate->id = STR8("ant_sim.info.tools.tool_select.navigate");
						navigate->style.sizing = size_auto;
						navigate->style.padding = {2,2,2,2};
						
						ui_make_text(STR8("Nav"), 0)->style.hover_passthrough = true;
						
						navigate->action_trigger = action_act_mouse_released;
						navigate->action = [](uiItem* item){
							change_tool(Tool_Navigate);
						};
						
						navigate->update_trigger = action_act_always;
						navigate->__update = [](uiItem* item){
							if(sim.tool == Tool_Navigate){
								item->style.background_color = Color_DarkRed;
							}else{
								item->style.background_color = {0};
							}
						};
					}ui_end_item();
					
					uiItem* draw = ui_begin_item(0);{ draw->id = STR8("ant_sim.info.tools.tool_select.draw");
						draw->style.sizing = size_auto;
						draw->style.padding = {2,2,2,2};
						
						ui_make_text(STR8("Draw"), 0)->style.hover_passthrough = true;
						
						draw->action_trigger = action_act_mouse_released;
						draw->action = [](uiItem* item){
							change_tool(Tool_Draw);
						};
						
						draw->update_trigger = action_act_always;
						draw->__update = [](uiItem* item){
							if(sim.tool == Tool_Draw){
								item->style.background_color = Color_DarkRed;
							}else{
								item->style.background_color = {0};
							}
						};
					}ui_end_item();

					uiItem* erase = ui_begin_item(0);{ erase->id = STR8("ant_sim.info.tools.tool_select.erase");
						erase->style.sizing = size_auto;
						erase->style.padding = {2,2,2,2};
						
						ui_make_text(STR8("Erase"), 0)->style.hover_passthrough = true;
						
						erase->action_trigger = action_act_mouse_released;
						erase->action = [](uiItem* item){
							change_tool(Tool_Erase);
						};
						
						erase->update_trigger = action_act_always;
						erase->__update = [](uiItem* item){
							if(sim.tool == Tool_Erase){
								item->style.background_color = Color_DarkRed;
							}else{
								item->style.background_color = {0};
							}
						};
					}ui_end_item();
				}ui_end_item();
				
				uiItem* break_button = ui_begin_item(0);{ //TODO replace with button widget
					break_button->id = STR8("ant_sim.info.tools.break_button");
					break_button->style.sizing = size_auto;
					break_button->style.background_color = Color_VeryDarkCyan;
					break_button->style.border_color = Color_White;
					break_button->style.border_width = 1;
					break_button->style.border_style = border_solid;
					break_button->style.padding = {2,2,2,2};
					
					ui_make_text(STR8("break on click"), 0)->style.hover_passthrough = true;
					
					break_button->action_trigger = action_act_mouse_released;
					break_button->action = [](uiItem* item){
						if(item->action_trigger == action_act_mouse_released){
							item->action_trigger = action_act_always;
							text_clear_and_replace(&((uiText*)item->node.first_child)->text, STR8("breaking (esc to cancel)"));
							item->style.background_color = Color_Red;
						}else if(input_lmouse_pressed()){
							Entity* e = get_entity_under_mouse();
							if(e){
								sim.break_on_me = e;
								item->action_trigger = action_act_mouse_released;
								text_clear_and_replace(&((uiText*)item->node.first_child)->text, STR8("break on click"));
								item->style.background_color = Color_VeryDarkCyan;
							}
						}else if(key_pressed(Key_ESCAPE)){
							item->action_trigger = action_act_mouse_released;
							text_clear_and_replace(&((uiText*)item->node.first_child)->text, STR8("break on click"));
							item->style.background_color = Color_VeryDarkCyan;
						}
					};
				}ui_end_item();
				
				//TODO replace with list widget
				uiItem* entity_list_wrapper = ui_begin_item(0);{ entity_list_wrapper->id = STR8("ant_sim.info.tools.entity_list_wrapper");
					entity_list_wrapper->style.sizing = size_percent_x | size_auto_y;
					entity_list_wrapper->style.width = 100/*percent*/;
					entity_list_wrapper->style.padding = {5,5,5,5};
					entity_list_wrapper->style.background_color = Color_DarkGrey;
					
					uiItem* entity_list = ui_begin_item(0);{
						entity_list->id = STR8("ant_sim.info.tools.entity_list_wrapper.entity_list");
						entity_list->style.width = 100/*percent*/;
						entity_list->style.sizing = size_auto_y | size_percent_x;
						
						forI(Entity_COUNT){
							if(i == Entity_Agent) continue; //handle agents below
							
							uiItem* item = ui_begin_item(0);{
								item->id = to_dstr8v(deshi_allocator, "ant_sim.info.tools.entity_list_wrapper.entity_list.", EntityStrings[i]).fin;
								item->userVar = i;
								item->style.background_color = Color_VeryDarkCyan;
								item->style.margin_bottom = 2;
								item->style.content_align = {0.0,0.5};
								item->style.width = 100/*percent*/;
								item->style.sizing = size_auto_y | size_percent_x;
								item->style.padding_left = 1;
								
								ui_make_text(EntityStrings[i], 0)->style.hover_passthrough = true;
								
								item->action_trigger = action_act_mouse_released;
								item->action = [](uiItem* item){
									sim.drawing.entity_type = item->userVar;
									sim.drawing.agent_type = Agent_COUNT;
									item->dirty = true;
								};
								
								item->update_trigger = action_act_always;
								item->__update = [](uiItem* item){
									if(sim.drawing.entity_type == item->userVar){
										item->style.background_color = Color_DarkRed;
									}else{
										item->style.background_color = Color_VeryDarkCyan;
									}
								};
							}ui_end_item();
						}
						
						forI(Agent_COUNT){
							uiItem* item = ui_begin_item(0);{ item->id = to_dstr8v(deshi_allocator, "ant_sim.info.tools.entity_list_wrapper.entity_list.", AgentStrings[i]).fin;
								item->userVar = i;
								item->style.background_color = Color_VeryDarkCyan;
								item->style.margin_bottom = 2;
								item->style.content_align = {0.0,0.5};
								item->style.width = 100/*percent*/;
								item->style.sizing = size_auto_y | size_percent_x;
								item->style.padding_left = 1;
								
								ui_make_text(AgentStrings[i], 0)->style.hover_passthrough = true;
								
								item->action_trigger = action_act_mouse_released;
								item->action = [](uiItem* item){
									sim.drawing.entity_type = Entity_Agent;
									sim.drawing.agent_type = item->userVar;
									item->dirty = true;
								};
								
								item->update_trigger = action_act_always;
								item->__update = [](uiItem* item){
									if(sim.drawing.entity_type == Entity_Agent && sim.drawing.agent_type == item->userVar){
										item->style.background_color = Color_DarkRed;
									}else{
										item->style.background_color = Color_VeryDarkCyan;
									}
								};
							}ui_end_item();
						}
					}ui_end_item();
				}ui_end_item();
			}ui_end_collapsing_header();
			
			uiItem* keybinds_header = ui_begin_collapsing_header(STR8("Keybinds"), false, 0);{ keybinds_header->id = STR8("ant_sim.info.keybinds");
				ui_make_text(aligned_text(6,3,{
					STR8("pause"),    STR8(" - "), STR8("space"),
					STR8("step"),     STR8(" - "), STR8("lctrl + space"),
					STR8("navigate"), STR8(" - "), STR8("lshift + n"),
					STR8("draw"),     STR8(" - "), STR8("lshift + d"),
					STR8("erase"),    STR8(" - "), STR8("lshift + e"),
					STR8("select"),   STR8(" - "), STR8("right click"),
				}), 0)->id = STR8("ant_sim.info.keybinds_header.text");
			}ui_end_collapsing_header();
			
			uiItem* simulation_header = ui_begin_collapsing_header(STR8("Simulation"), false, 0);{ simulation_header->id = STR8("ant_sim.info.simulation");
				uiItem* simulation_ticks = ui_make_text(to_dstr8v(deshi_temp_allocator, STR8("ticks: "), sim.ticks).fin, 0);
				simulation_ticks->id = STR8("ant_sim.info.simulation.ticks");
				simulation_ticks->update_trigger = action_act_always;
				simulation_ticks->__update = [](uiItem* item){
					text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, STR8("ticks: "), sim.ticks).fin);
				};
				
				uiItem* simulation_paused = ui_make_text(to_dstr8v(deshi_temp_allocator, STR8("paused: "), ((sim.paused) ? STR8("true") : STR8("false"))).fin, 0);
				simulation_paused->id = STR8("ant_sim.info.simulation.paused");
				simulation_paused->update_trigger = action_act_always;
				simulation_paused->__update = [](uiItem* item){
					text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, STR8("paused: "), ((sim.paused) ? STR8("true") : STR8("false"))).fin);
				};
				
				uiItem* settings_header = ui_begin_collapsing_header(STR8("Settings"), false, 0);{ settings_header->id = STR8("ant_sim.info.simulation.settings");
					uiItem* negative_cost_wrapper = ui_begin_item(0);{ negative_cost_wrapper->id = STR8("ant_sim.info.simulation.settings.negative_cost_wrapper");
						negative_cost_wrapper->style.sizing = size_auto;
						negative_cost_wrapper->style.height = g_ui->base.style.font_height + 2;
						negative_cost_wrapper->style.padding = {2,2,2,2};
						negative_cost_wrapper->style.display = display_horizontal;
						
						ui_make_text(STR8("Negative Cost Weight: "), 0)->style.hover_passthrough = true;
						
						uiItem* slider = ui_make_slider_f32(1.0f, 3.0f, &g_negative_cost_weight, 0);
						slider->id = STR8("ant_sim.info.simulation.settings.negative_cost_wrapper.slider");
						slider->style.size = {200, (f32)g_ui->base.style.font_height};
						slider->style.background_color = Color_DarkGrey;
						
						uiItem* value_text = ui_make_text(to_dstr8v(deshi_temp_allocator, g_negative_cost_weight).fin, 0);
						value_text->style.pos = {4, 0};
						
						slider->userVar = (u64)value_text;
						slider->action_trigger = action_act_hash_change;
						slider->action = [](uiItem* item){
							uiItem* value_text = (uiItem*)item->userVar;
							text_clear_and_replace(&ui_get_text(value_text)->text, to_dstr8v(deshi_temp_allocator, g_negative_cost_weight).fin);
						};
					}ui_end_item();
					
					uiItem* tps_wrapper = ui_begin_item(0);{ tps_wrapper->id = STR8("ant_sim.info.simulation.settings.tps_wrapper");
						tps_wrapper->style.sizing = size_auto;
						tps_wrapper->style.height = g_ui->base.style.font_height + 2;
						tps_wrapper->style.padding = {2,2,2,2};
						tps_wrapper->style.display = display_horizontal;
						
						ui_make_text(STR8("Ticks Per Second: "), 0)->style.hover_passthrough = true;
						
						uiItem* slider = ui_make_slider_u32(1, 1000, &sim.ticks_per_second, 0);
						slider->id = STR8("ant_sim.info.simulation.settings.tps_wrapper.slider");
						slider->style.size = {200, (f32)g_ui->base.style.font_height};
						slider->style.background_color = Color_DarkGrey;
						
						uiItem* value_text = ui_make_text(to_dstr8v(deshi_temp_allocator, sim.ticks_per_second).fin, 0);
						value_text->style.pos = {4, 0};
						
						slider->userVar = (u64)value_text;
						slider->action_trigger = action_act_hash_change;
						slider->action = [](uiItem* item){
							uiItem* value_text = (uiItem*)item->userVar;
							sim.tick_delta = 1.0 / (f64)sim.ticks_per_second;
							text_clear_and_replace(&ui_get_text(value_text)->text, to_dstr8v(deshi_temp_allocator, sim.ticks_per_second).fin);
						};
					}ui_end_item();
				}ui_end_collapsing_header();
			}ui_end_collapsing_header();
			
			uiItem* hovered_header = ui_begin_collapsing_header(STR8("Hovered"), false, 0);{ hovered_header->id = STR8("ant_sim.info.hovered");
				uiItem* hovered_entity = ui_make_text(STR8("entity: nothing"), 0);
				hovered_entity->id = STR8("ant_sim.info.hovered.entity");
				hovered_entity->update_trigger = action_act_always;
				hovered_entity->__update = [](uiItem* item){
					Entity* hovered = get_entity_under_mouse();
					if(hovered){
						text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, STR8("entity: "), EntityStrings[hovered->type]).fin);
					}else{
						text_clear_and_replace(&ui_get_text(item)->text, STR8("entity: nothing"));
					}
				};
				
				uiItem* hovered_tile = ui_make_text(STR8("tile: nothing"), 0);
				hovered_tile->id = STR8("ant_sim.info.hovered.tile");
				hovered_tile->update_trigger = action_act_always;
				hovered_tile->__update = [](uiItem* item){
					auto [tile_pos,valid_tile] = get_tile_under_mouse();
					if(valid_tile){
						text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, STR8("tile: "), tile_pos).fin);
					}else{
						text_clear_and_replace(&ui_get_text(item)->text, STR8("tile: nothing"));
					}
				};
				
				uiItem* hovered_ui = ui_make_text(STR8("ui: nothing"), 0);
				hovered_ui->id = STR8("ant_sim.info.hovered.ui");
				hovered_ui->update_trigger = action_act_always;
				hovered_ui->__update = [](uiItem* item){
					if(g_ui->hovered){
						text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, STR8("ui: "), g_ui->hovered->id).fin);
					}else{
						text_clear_and_replace(&ui_get_text(item)->text, STR8("ui: nothing"));
					}
				};
			}ui_end_collapsing_header();
			
			uiItem* selected_entity_header = ui_begin_collapsing_header(STR8("Selected Entity"), false, 0);{ selected_entity_header->id = STR8("ant_sim.info.selected_entity");
				{ //// Entity Info ////
					uiItem* entity_type = ui_make_text(STR8("type: ERROR"), 0);
					entity_type->id = STR8("ant_sim.info.selected_entity.entity.type");
					entity_type->update_trigger = action_act_always;
					entity_type->__update = [](uiItem* item){
						if(sim.selected_entity){
							text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, "type: ", EntityStrings[sim.selected_entity->type]).fin);
						}else{
							text_clear_and_replace(&ui_get_text(item)->text, STR8("nothing selected"));
						}
					};
					
					uiItem* entity_age = ui_make_text(STR8("age: ERROR"), 0);
					entity_age->id = STR8("ant_sim.info.selected_entity.entity.age");
					entity_age->style.display = display_hidden;
					entity_age->update_trigger = action_act_always;
					entity_age->__update = [](uiItem* item){
						if(sim.selected_entity){
							item->style.display = display_vertical;
							text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, "age: ", sim.selected_entity->age).fin);
						}else{
							item->style.display = display_hidden;
						}
					};
					
					uiItem* entity_pos = ui_make_text(STR8("pos: ERROR"), 0);
					entity_pos->id = STR8("ant_sim.info.selected_entity.entity.pos");
					entity_pos->style.display = display_hidden;
					entity_pos->update_trigger = action_act_always;
					entity_pos->__update = [](uiItem* item){
						if(sim.selected_entity){
							item->style.display = display_vertical;
							text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, "pos: ", sim.selected_entity->pos).fin);
						}else{
							item->style.display = display_hidden;
						}
					};
					
					uiItem* entity_adverts = ui_make_text(STR8("adverts: 0"), 0);
					entity_adverts->id = STR8("ant_sim.info.selected_entity.entity.adverts");
					entity_adverts->style.display = display_hidden;
					entity_adverts->update_trigger = action_act_always;
					entity_adverts->__update = [](uiItem* item){
						if(sim.selected_entity){
							item->style.display = display_vertical;
							if(sim.selected_entity->adverts_count == 0){
								text_clear_and_replace(&ui_get_text(item)->text, STR8("adverts: 0"));
							}else{
								dstr8 needs_builder;
								dstr8_init(&needs_builder, STR8("adverts: "), deshi_temp_allocator);
								dstr8_append(&needs_builder, to_dstr8(sim.selected_entity->adverts_count, deshi_temp_allocator));
								dstr8_append(&needs_builder, STR8("\n"));
								forI(sim.selected_entity->adverts_count){
									dstr8_append(&needs_builder, to_dstr8v(deshi_temp_allocator, i, STR8("_"), sim.selected_entity->adverts_array[i]->def->name, STR8("\n")).fin);
								}
								text_clear_and_replace(&ui_get_text(item)->text, dstr8_peek(&needs_builder));
							}
						}else{
							item->style.display = display_hidden;
						}
					};
				} //// Entity Info ////
				
				uiItem* agent_info = ui_begin_item(0);{ agent_info->id = STR8("ant_sim.info.selected_entity.agent");
					agent_info->style.sizing = size_percent_x | size_auto_y;
					agent_info->style.width = 100/*percent*/;
					agent_info->style.display = display_hidden;
					agent_info->update_trigger = action_act_always;
					agent_info->__update = [](uiItem* item){
						if(sim.selected_entity && sim.selected_entity->type == Entity_Agent){
							item->style.display = display_vertical;
						}else{
							item->style.display = display_hidden;
						}
					};
					
					uiItem* agent_race = ui_make_text(STR8("race: ERROR"), 0);
					agent_race->id = STR8("ant_sim.info.selected_entity.agent.race");
					agent_race->update_trigger = action_act_always;
					agent_race->__update = [](uiItem* item){
						text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, "race: ", AgentStrings[AgentFromEntity(sim.selected_entity)->type]).fin);
					};
					
					uiItem* agent_species = ui_make_text(STR8("species: ERROR"), 0);
					agent_species->id = STR8("ant_sim.info.selected_entity.agent.species");
					agent_species->update_trigger = action_act_always;
					agent_species->__update = [](uiItem* item){
						text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, "species: ", AgentSpeciesStrings[AgentFromEntity(sim.selected_entity)->type]).fin);
					};
					
					uiItem* agent_needs = ui_begin_collapsing_header(STR8("Needs"), false, 0);{ agent_needs->id = STR8("ant_sim.info.selected_entity.agent.needs");
						ui_get_collapsing_header(agent_needs)->content->style.background_color = Color_DarkGrey;
						forX(need_type, Need_COUNT){
							uiItem* need_item = ui_make_text(STR8(""), 0);
							need_item->userVar = (u64)need_type;
							need_item->style.margin_bottom = 2/*pixels*/;
							need_item->__update = [](uiItem* item){
								Agent* agent = AgentFromEntity(sim.selected_entity);
								NeedType need_type = (NeedType)item->userVar;
								
								dstr8 need_text;
								dstr8_init(&need_text, STR8("["), deshi_temp_allocator);
								f32 need_percent = (agent->needs[need_type].value / MAX_NEED_VALUE) * 100.0f;
								u32 need_percent_whole = (u32)need_percent;
								u32 dash_count = (u32)(need_percent / 10.0f);
								u32 space_count = 10 - dash_count;
								forI(dash_count) dstr8_append(&need_text, STR8("-"));
								forI(space_count) dstr8_append(&need_text, STR8(" "));
								dstr8_append(&need_text, STR8("] ("));
								dstr8_append(&need_text, to_dstr8(need_percent_whole, deshi_temp_allocator));
								dstr8_append(&need_text, STR8("%) "));
								dstr8_append(&need_text, NeedStrings[need_type]);
								
								text_clear_and_replace(&ui_get_text(item)->text, need_text.fin);
							};
						}
					}ui_end_collapsing_header();
					
					ui.agent_collected_adverts_header = ui_make_collapsing_header(STR8("Collected Adverts"), false, 0);
					ui.agent_collected_adverts_header->id = STR8("ant_sim.info.selected_entity.agent.collected_adverts");
					ui_get_collapsing_header(ui.agent_collected_adverts_header)->content->style.background_color = Color_DarkGrey;
				}ui_end_item();
				
				uiItem* water_info = ui_begin_item(0);{ water_info->id = STR8("ant_sim.info.selected_entity.water");
					water_info->style.sizing = size_percent_x | size_auto_y;
					water_info->style.width = 100/*percent*/;
					water_info->style.display = display_hidden;
					water_info->update_trigger = action_act_always;
					water_info->__update = [](uiItem* item){
						if(sim.selected_entity && sim.selected_entity->type == Entity_Water){
							item->style.display = display_vertical;
						}else{
							item->style.display = display_hidden;
						}
					};
					
					uiItem* water_pressure = ui_make_text(STR8("pressure: ERROR"), 0);
					water_pressure->id = STR8("ant_sim.info.selected_entity.water.pressure");
					water_pressure->update_trigger = action_act_always;
					water_pressure->__update = [](uiItem* item){
						text_clear_and_replace(&ui_get_text(item)->text, to_dstr8v(deshi_temp_allocator, "pressure: ", sim.selected_entity->water.pressure).fin);
					};
				}ui_end_item();
			}ui_end_collapsing_header(/*Selected Entity*/);
		}ui_end_item();
	}ui_end_item();
}

void update_ui(){
	ui_begin_immediate_branch(0);{
		persist Type anchor = anchor_top_left;
		uiItem* window = ui_begin_item(0);{ window->id = STR8("ant_sim.info_window");
			if(ui_item_hovered(window,false)) anchor = (anchor+1) % (anchor_bottom_left+1);
			window->style.positioning = pos_absolute;
			window->style.anchor = anchor;
			window->style.sizing = size_auto;
			window->style.background_color = Color_DarkGrey;
			ui_make_text(to_dstr8v(deshi_temp_allocator, (int)F_AVG(100,1000/DeshTime->deltaTime)," fps").fin, 0)->id = STR8("ant_sim.info_window.fps");
			if(sim.paused){ 
				uiItem* pausebox = ui_begin_item(0);{
					pausebox->style.sizing = size_auto;
					pausebox->style.padding = {2,2,2,2};
					pausebox->style.background_color = color(255*(sin(1.5*DeshTime->totalTime/1000 + cos(1.5*DeshTime->totalTime/1000))+1)/2, 0, 0, 255);
					ui_make_text(STR8("paused"), 0)->id = STR8("ant_sim.info_window.paused");
				}ui_end_item();
			}
		}ui_end_item();
	}ui_end_immediate_branch();
	
	ui_begin_immediate_branch(ui_get_collapsing_header(ui.agent_collected_adverts_header)->content);{
		if(!sim.paused){
			sim.selected_advert = 0;
			ui_make_text(STR8("pause simulation to view collected adverts"), 0);
		}else{
			Agent* agent = AgentFromEntity(sim.selected_entity);
			u32 advert_count;
			Advert** adverts = collect_adverts(agent, &advert_count);
			
			for (u32 i = 0; i < advert_count; ++i) {
				Advert* advert = adverts[i];
				f32 score = score_advert(agent, advert);
				
				uiItem* advert_item = ui_begin_item(0);{
					advert_item->id = STR8("ant_sim.info.selected_entity.agent.collected_adverts.advert_item");
					advert_item->style.sizing = size_percent_x | size_auto_y;
					advert_item->style.width = 100/*percent*/;
					advert_item->style.margin_bottom = 2/*pixels*/;
					advert_item->style.padding_left = 1/*pixels*/;
					advert_item->style.content_align = {0.0,0.5};
					advert_item->style.background_color = Color_VeryDarkCyan;
					
					dstr8 advert_text = to_dstr8v(deshi_temp_allocator, advert->def->name, " ", advert->owner->pos, " Score: ", score);
					ui_make_text(advert_text.fin, 0)->style.hover_passthrough = true;
					
					advert_item->userVar = (u64)advert;
					advert_item->action_trigger = action_act_mouse_released;
					advert_item->action = [](uiItem* item){
						sim.selected_advert = (Advert*)item->userVar;
					};
				}ui_end_item();
			}
		}
	}ui_end_immediate_branch();
	
	if(sim.selected_advert){
		ui_begin_immediate_branch(0);
		uiItem* breakdown = ui_begin_item(0);{ breakdown->id = STR8("ant_sim.advert_breakdown");
			breakdown->style.sizing = size_auto;
			breakdown->style.positioning = pos_draggable_fixed;
			breakdown->style.pos = {100, 100};
			breakdown->style.padding = {5,5,5,5};
			breakdown->style.border_style = border_solid;
			breakdown->style.border_width = 1;
			breakdown->style.border_color = Color_White;
			breakdown->style.background_color = Color_DarkGrey;
			
			Agent* agent = AgentFromEntity(sim.selected_entity);
			Advert* advert = sim.selected_advert;
			
			ui_make_text(to_dstr8v(deshi_temp_allocator, "Advert Breakdown: ", advert->def->name).fin, 0);
			
			f32 total_score = 0;
			forX(need_type, Need_COUNT){
				f32 current_score = agent->needs[need_type].value;
				f32 future_score  = current_score + advert->def->costs[need_type];
				if(future_score > MAX_NEED_VALUE) future_score = MAX_NEED_VALUE;
				if(future_score < 0) future_score = 0;
				
				f32 need_score;
				if(advert->def->costs[need_type] > 0){
					need_score = need_attenuation(current_score, future_score);
				}else{
					need_score = g_negative_cost_weight * need_attenuation(current_score, future_score);
				}
				total_score += need_score;
				
				dstr8 need_text = to_dstr8v(deshi_temp_allocator, NeedStrings[need_type], ": ", need_score);
				ui_make_text(need_text.fin, 0);
			}
			
			dstr8 total_text = to_dstr8v(deshi_temp_allocator, "Total (before distance): ", total_score / Need_COUNT);
			ui_make_text(total_text.fin, 0);
			
			f32 distance_factor = vec2i_distanceToSq(agent->entity.pos, advert->owner->pos);
			f32 final_score = (total_score / Need_COUNT) / distance_factor;
			
			dstr8 final_text = to_dstr8v(deshi_temp_allocator, "Final Score: ", final_score);
			ui_make_text(final_text.fin, 0);
			
			uiItem* close_button = ui_begin_item(0);{
				close_button->style.sizing = size_auto;
				close_button->style.padding = {5,5,5,5};
				close_button->style.margin_top = 10;
				close_button->style.background_color = Color_DarkRed;
				
				ui_make_text(STR8("Close"), 0)->style.hover_passthrough = true;
				
				close_button->action_trigger = action_act_mouse_released;
				close_button->action = [](uiItem* item){
					sim.selected_advert = 0;
				};
			}ui_end_item();
		}ui_end_item();
		ui_end_immediate_branch();
	}
	
	assets_texture_update(rendering.background.texture, vec2i{0,0}, vec2i{WORLD_WIDTH,WORLD_HEIGHT});
	assets_texture_update(rendering.foreground.texture, vec2i{0,0}, vec2i{WORLD_WIDTH,WORLD_HEIGHT});
	
	//zooming (even though ui handles moving the world for us, we still need to handle zooming)
	vec2 mouse_pos = input_mouse_position();
	if(DeshInput->scrollY != 0 && ui_item_hovered(ui.universe, hovered_child)){
		vec2 current_size = ui.background->style.size;
		vec2 local_mouse = mouse_pos - ui.world->pos_screen;
		ui.background->style.size.x += DeshInput->scrollY / 10.0 * current_size.x;
		ui.background->style.size.y += DeshInput->scrollY / 10.0 * current_size.y;
		ui.world->style.pos.x -= local_mouse.x * ((ui.background->style.size.x / current_size.x) - 1);
		ui.world->style.pos.y -= local_mouse.y * ((ui.background->style.size.y / current_size.y) - 1);
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @input


void update_input(){
	if(key_pressed(Key_SPACE | InputMod_None)) sim.paused = !sim.paused;
	if(key_pressed(Key_SPACE | InputMod_Lctrl)) sim.step = true;
	
	if(key_pressed(Key_N | InputMod_Lshift)) change_tool(Tool_Navigate);
	if(key_pressed(Key_D | InputMod_Lshift)) change_tool(Tool_Draw);
	if(key_pressed(Key_E | InputMod_Lshift)) change_tool(Tool_Erase);
	
	if(input_rmouse_released()) sim.selected_entity = get_entity_under_mouse();
	
	switch(sim.tool){
		case Tool_Navigate:{
			//do nothing
		}break;
		case Tool_Draw:{
			if(input_lmouse_pressed() || (input_lmouse_down() && input_mods_down(InputMod_Lshift))){
				auto [pos, ok] = get_tile_under_mouse();
				if(!ok || sim.break_on_me || get_entity(pos.x, pos.y)) break;
				
				if(sim.drawing.entity_type != Entity_Agent){
					Entity* e = make_entity(sim.drawing.entity_type, pos, 0);
					e->color = EntityColors[sim.drawing.entity_type][rand()%7];
					set_entity(pos.x,pos.y, e);
				}else{
					Agent* a = make_agent(sim.drawing.agent_type, 1, pos);
					a->entity.color = 0xff000000 | (rand() & 0xffffff); // Random color for agents
					set_entity(pos.x, pos.y, &a->entity);
				}
			}
		}break;
		case Tool_Erase:{
			if(input_lmouse_down() || (input_lmouse_down() && input_mods_down(InputMod_Lshift))){
				auto [pos, ok] = get_tile_under_mouse();
				if(!ok || sim.break_on_me) break;
				
				Entity* e = get_entity(pos.x, pos.y);
				if(e){
					if(e == sim.selected_entity){
						sim.selected_entity = 0;
					}
					delete_entity(e);
				}
			}
		}break;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @main


int main(int args_count, char** args){
	deshi_init_specify("ant_sim",Megabytes(256),Megabytes(256));
	
	setup_rendering();
	setup_simulation();
	setup_ui();
	
	deshi_loop_start();{
		update_input();
		update_simulation();
		update_ui();
	}deshi_loop_end();
	
	deshi_cleanup();
}
