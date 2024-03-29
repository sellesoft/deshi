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
*/
#include "deshi.h"
#include "kigu/array_utils.h"
struct Advert;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @utils
#define TICKS_PER_WORLD_SECOND 1
#define TICKS_PER_WORLD_MINUTE 60
#define TICKS_PER_WORLD_DAY    86400
#define TICKS_PER_WORLD_MONTH  2629740
#define TICKS_PER_WORLD_YEAR   31536000

#define WORLD_WIDTH 32
#define WORLD_HEIGHT 32

enum{
	north,
	east,
	south,
	west
};

vec2i direction_to_movement[] = {
	vec2i{ 0, 1}, //north
	vec2i{ 1, 0}, //east
	vec2i{ 0,-1}, //south
	vec2i{-1, 0}, //west
};

u32 divide_color(u32 color, u32 divisor){
	u32 r = (color >>  0 & 0x000000ff) / divisor;
	u32 g = (color >>  8 & 0x000000ff) / divisor;
	u32 b = (color >> 16 & 0x000000ff) / divisor;
	return PackColorU32(255,b,g,r);
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @entity
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
};StaticAssert(ArrayCount(EntityStrings) == Entity_COUNT);

// color palettes for entities that randomly choose color
u32 EntityColors[Entity_COUNT][7] = {
	0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
	0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa, 0xffaaaaaa,
	0,          0,          0,          0,          0,          0,          0         ,
	0xff709a88, 0xff7ba694, 0xff86b19f, 0xff91bdab, 0xff9cc9b7, 0xffa8d5c3, 0xffb4e1cf,
	0xff3d5f82, 0xff45678a, 0xff4c6e93, 0xff53769b, 0xff5a7ea3, 0xff6286ac, 0xff698eb4,
	0xff595d47, 0xff60644d, 0xff666a54, 0xff6d715a, 0xff747861, 0xff7a7e67, 0xff81856e,
};

typedef struct Entity{
	Node overlap_node; // connection to other entities occupying the same world tile
	Type type;
	u32 color;
	str8 name;
	u64 age;
	vec2i pos;
	Flags flags;
	
	Advert* adverts_array[4]; //TODO make this dynamic
	u64 adverts_count;
	
	union{
		struct{
			b32 evaluating;
			b32 evaluated;
			u32 pressure;
			Node node; // connects the surface of a body of water
		}water;
	};
	
	uiItem* ui;
}Entity;

enum{
	Flags_Water_Evaluated = 1 << 0,
};


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @need/cost
#define MAX_NEED_VALUE 100000
#define MIN_NEED_VALUE      0

enum{
	Need_Bladder,
	Need_Food,
	Need_Health,
	Need_Mating,
	Need_Sleep,
	Need_Water,
	Need_COUNT
};
const str8 NeedStrings[] = {
	STR8("Bladder"),
	STR8("Food"),
	STR8("Health"),
	STR8("Mating"),
	STR8("Sleep"),
	STR8("Water"),
};StaticAssert(ArrayCount(NeedStrings) == Need_COUNT);

const u32 NeedStringsMaxWidth = 7;

typedef struct Need{
	Type type;
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

enum{
	Action_Idle,
	Action_Walk,
	Action_Dig,
	Action_EatLeaf,
	Action_DrinkWater,
	Action_COUNT
};

typedef struct ActionDef{
	Type type;
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
			/*bladder*/ .00f*MAX_NEED_VALUE,
			/*food   */ .00f*MAX_NEED_VALUE,
			/*health */ .00f*MAX_NEED_VALUE,
			/*mating */ .00f*MAX_NEED_VALUE,
			/*sleep  */ .01f*MAX_NEED_VALUE,
			/*water  */ .00f*MAX_NEED_VALUE,
		},
	},
	
	{Action_EatLeaf,
		/*name */ STR8("Eat Leaf"),
		/*time */ 60*TICKS_PER_WORLD_SECOND,
		/*costs*/ {
			/*bladder*/ .30f*MAX_NEED_VALUE,
			/*food   */ .30f*MAX_NEED_VALUE,
			/*health */ .00f*MAX_NEED_VALUE,
			/*mating */ .00f*MAX_NEED_VALUE,
			/*sleep  */ .00f*MAX_NEED_VALUE,
			/*water  */ .10f*MAX_NEED_VALUE,
		},
	},
	
	{Action_DrinkWater,
		/*name */ STR8("Drink Water"),
		/*time */ 30*TICKS_PER_WORLD_SECOND,
		/*costs*/ {
			/*bladder*/ .30f*MAX_NEED_VALUE,
			/*food   */ .00f*MAX_NEED_VALUE,
			/*health */ .00f*MAX_NEED_VALUE,
			/*mating */ .00f*MAX_NEED_VALUE,
			/*sleep  */ .00f*MAX_NEED_VALUE,
			/*water  */ .90f*MAX_NEED_VALUE,
		},
	},
};StaticAssert(ArrayCount(ActionDefinitions) == Action_COUNT);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @agent
enum{
	Race_BlackGardenAntQueen,
	Race_BlackGardenAntMale,
	Race_BlackGardenAntWorker,
	Race_CottonAntQueen,
	Race_CottonAntMale,
	Race_CottonAntMinorWorker,
	Race_CottonAntMajorWorker,
	Race_COUNT
};
str8 RaceStrings[] = {
	STR8("Queen Black Garden Ant"),
	STR8("Male Black Garden Ant"),
	STR8("Worker Black Garden Ant"),
	STR8("Queen Cotton Ant"),
	STR8("Male Cotton Ant"),
	STR8("Minor Worker Cotton Ant"),
	STR8("Major Worker Cotton Ant"),
};StaticAssert(ArrayCount(RaceStrings) == Race_COUNT);
str8 RaceSpeciesStrings[] = {
	STR8("Lasius Niger"),
	STR8("Lasius Niger"),
	STR8("Lasius Niger"),
	STR8("Solenopsis xyloni"),
	STR8("Solenopsis xyloni"),
	STR8("Solenopsis xyloni"),
	STR8("Solenopsis xyloni"),
};StaticAssert(ArrayCount(RaceSpeciesStrings) == Race_COUNT);

typedef struct Agent{
	Node node;
	
	Entity entity;
	Type race;
	
	u32 action_index;
	Advert* active_advert;
	
	Need* needs_array;
	u32 needs_count;
	
	u8* path_nodes; //movement direction steps (north, east, south, west)
	u32 path_count;
	u32 path_index; //progress along path
}Agent;
#define AgentFromNode(ptr) CastFromMember(Agent,node,ptr)
#define AgentFromEntity(ptr) CastFromMember(Agent,entity,ptr)


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @advert
#define MAX_ADVERT_DEF_ACTIONS 4 //arbitrary value, increase if needed
#define ADVERT_COST_DELTA_EPSILON (10)
#define ADVERT_TIME_DELTA_EPSILON (5*TICKS_PER_WORLD_SECOND)

enum{
	Advert_Dig,
	Advert_EatLeaf,
	Advert_DrinkWater,
	Advert_COUNT
};

enum{
	AdvertFlags_None                      = 0,
	AdvertFlags_ConsumeAdvertOnCompletion = (1 << 0),
	AdvertFlags_ConsumeOwnerOnCompletion  = (1 << 1),
};

typedef struct AdvertDef{
	str8 name;
	Flags flags;
	u32 rangeSq;
	f32 costs[Need_COUNT];
	ActionDef* actions[MAX_ADVERT_DEF_ACTIONS];
}AdvertDef;

typedef struct Advert{
	AdvertDef* def;
	Entity* owner;
	Action* actions_array;
	u32 actions_count; //NOTE new actions can be inserted during progress towards advert
	u32 completion_time;
	u32 ongoing_time;
}Advert;

AdvertDef AdvertDefinitions[] = {
	{STR8("Dig"),
		/*flags  */ AdvertFlags_ConsumeOwnerOnCompletion,
		/*rangeSq*/ (u32)-1,
		/*costs  */ {
			/*bladder*/ .00f*MAX_NEED_VALUE,
			/*food   */ .00f*MAX_NEED_VALUE,
			/*health */ .00f*MAX_NEED_VALUE,
			/*mating */ .00f*MAX_NEED_VALUE,
			/*sleep  */ .01f*MAX_NEED_VALUE,
			/*water  */ .00f*MAX_NEED_VALUE,
		},
		/*actions*/ {
			&ActionDefinitions[Action_Dig],
		},
	},
	{STR8("Eat Leaf"),
		/*flags  */ AdvertFlags_ConsumeOwnerOnCompletion,
		/*rangeSq*/ (u32)-1,
		/*costs  */ {
			/*bladder*/ .30f*MAX_NEED_VALUE,
			/*food   */ .30f*MAX_NEED_VALUE,
			/*health */ .00f*MAX_NEED_VALUE,
			/*mating */ .00f*MAX_NEED_VALUE,
			/*sleep  */ .00f*MAX_NEED_VALUE,
			/*water  */ .10f*MAX_NEED_VALUE,
		},
		/*actions*/ {
			&ActionDefinitions[Action_EatLeaf],
		},
	},
	{STR8("Drink Water"),
		/*flags  */ AdvertFlags_ConsumeOwnerOnCompletion,
		/*rangeSq*/ (u32)-1,
		/*costs  */ {
			/*bladder*/ .30f*MAX_NEED_VALUE,
			/*food   */ .00f*MAX_NEED_VALUE,
			/*health */ .00f*MAX_NEED_VALUE,
			/*mating */ .00f*MAX_NEED_VALUE,
			/*sleep  */ .00f*MAX_NEED_VALUE,
			/*water  */ .90f*MAX_NEED_VALUE,
		},
		/*actions*/ {
			&ActionDefinitions[Action_DrinkWater],
		},
	},
};StaticAssert(ArrayCount(AdvertDefinitions) == Advert_COUNT);

//scales a score value based on the need increase's distance from zero (so the change from 70-80 has less value than 10-30)
//NOTE usually returns a value between -1 and 1
//NOTE the "+ .001f" is just to avoid division by zero
f32 need_attenuation(f32 current, f32 future){
	return (10.f / (current + .001f)) - (10.f / (future + .001f));
}

//scales a score value with distance: 100 * score / distance^2
f32 distance_attenuation(f32 score, vec2i pos1, vec2i pos2){
	f32 x = pos2.x - pos1.x;
	f32 y = pos2.y - pos1.y;
	f32 d = x*x + y*y;
	return score / ((d > 1.f) ? d : 1.f);
}

//score the advert based on its positive/negative costs, completion time, distance, ...
f32 score_advert(Agent* agent, Advert* advert){
	f32 average = 0;
	ForX(need, agent->needs_array, agent->needs_count){
		f32 current_score = need->value;
		f32 future_score  = current_score + advert->def->costs[need->type];
		if(future_score > MAX_NEED_VALUE) future_score = MAX_NEED_VALUE;
		
		if(advert->def->costs[need->type] > 0){
			average += need_attenuation(current_score, future_score);
		}else{
			//TODO(delle) weight negative costs heavier
			average += need_attenuation(current_score, future_score);
		}
	}
	average /= agent->needs_count;
	
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
				score_count++;
				break;
			}
		}
	}
	
	//choose from the best three scores by normalizing them into a range and choosing randomly in that range
	if(score_count == 0){
		return 0;
	}else if(score_count == 1){
		return best_ads[0];
	}else{
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

u32 get_pixelfg(u32 x, u32 y) {	
	Assert(x >= 0 && y >= 0 && x <= WORLD_WIDTH && y <= WORLD_HEIGHT);
	return rendering.foreground.data[x+y*WORLD_WIDTH];
}

b32 set_pixelfg(u32 x, u32 y, u32 val) {
	if(x < 0 || y < 0 || x > WORLD_WIDTH || y > WORLD_HEIGHT) return 0;
	rendering.foreground.data[x+y*WORLD_WIDTH] = val;
	return 1;
}

u32 get_pixelbg(u32 x, u32 y) {	
	Assert(x >= 0 && y >= 0 && x <= WORLD_WIDTH && y <= WORLD_HEIGHT);
	return rendering.background.data[x+y*WORLD_WIDTH];
}

b32 set_pixelbg(u32 x, u32 y, u32 val) {
	if(x < 0 || y < 0 || x > WORLD_WIDTH || y > WORLD_HEIGHT) return 0;
	rendering.background.data[x+y*WORLD_WIDTH] = val;
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
enum{
	Weather_Clear,
	Weather_Cloudy,
	Weather_Rain,
	Weather_Thunderstorm,
	Weather_Snow,
};

enum{
	DrawTool_Draw_Square,
	DrawTool_Erase_Square,
};

enum{
	Mode_Navigate,
	Mode_Draw,
};

struct{
	Entity** map;
	
	struct{
		Type type;
		s32 wind_strength;
		s32 temperature; // celsius
	}weather;
}world;

struct{
	Type mode;
	struct{
		Type tool;
		Type entity_type;
		Type agent_race;
	}drawing;
	Entity* break_on_me;
	Entity* selected_entity;
	Advert* selected_advert;
	u64 ticks;
	b32 paused;
	b32 step;
}sim;

struct{
	u32 entity[Entity_COUNT]; // count of each entity
	u32 entities; // total entities
	u32 agents; // sub count of entities
	u32 actions;
	u32 advert[Advert_COUNT];
	u32 adverts;
}counts = {0};

Heap* agents_heap;
Node agents_node;
Advert* adverts_pool;
Entity* entities_pool;

void change_mode(Type mode){
	switch(mode){
		case Mode_Navigate:{
			g_ui->keys.drag_item = Mouse_LEFT;
		}break;
		case Mode_Draw:{
			g_ui->keys.drag_item = Mouse_MIDDLE;
		}break;
	}
	sim.mode = mode;
}

Entity* get_entity(vec2i pos){
	if(pos.x < 0 || pos.y < 0 || pos.x > WORLD_WIDTH || pos.y > WORLD_HEIGHT) return 0;
	return world.map[pos.x + pos.y * WORLD_WIDTH];
}
FORCE_INLINE Entity* get_entity(u32 x, u32 y){ return get_entity(Vec2i(x,y)); }

b32 set_entity(vec2i pos, Entity* entity){
	if(pos.x < 0 || pos.y < 0 || pos.x > WORLD_WIDTH || pos.y > WORLD_HEIGHT) return false;
	world.map[pos.x + pos.y * WORLD_WIDTH] = entity;
	if(entity) set_pixelfg(pos.x,pos.y,entity->color);
	else set_pixelfg(pos.x,pos.y,0);
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

void swap_entities(Entity* e0, Entity* e1) {
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

Advert* make_advert(Type type, Flags flags, Entity* owner, vec2i target){
	Assert(type < Advert_COUNT);
	AdvertDef* def = &AdvertDefinitions[type];
	Advert* advert = memory_pool_push(adverts_pool);
	advert->def = def;
	advert->owner = owner;
	advert->actions_array = (Action*)memory_alloc(MAX_ADVERT_DEF_ACTIONS*sizeof(Action));
	forI(MAX_ADVERT_DEF_ACTIONS){
		if(def->actions[i] == 0) break;
		
		advert->actions_array[i].def = def->actions[i];
		advert->actions_array[i].target = target;
		advert->actions_array[i].progress = 0;
		
		advert->completion_time += def->actions[i]->time;
		advert->actions_count   += 1;
	}
	
	if(owner){
		Assert(owner->adverts_count < 4); //TODO make adverts_array dynamic
		owner->adverts_array[owner->adverts_count] = advert;
		owner->adverts_count += 1;
	}
	
	counts.adverts += 1;
	counts.advert[type] += 1;
	return advert;
}

template<typename... Args>
Agent* make_agent(Type race, u32 age, vec2i pos, Args... args){
	constexpr u32 arg_count = sizeof...(Args);
	Agent* agent = (Agent*)memory_heap_add_bytes(agents_heap, sizeof(Agent) + arg_count*sizeof(Need));
	agent->entity.name   = str8{0};
	agent->entity.age    = age;
	agent->entity.pos    = pos;
	agent->race          = race;
	agent->action_index  = -1;
	agent->active_advert = 0;
	agent->needs_array   = (Need*)(agent+1);
	agent->needs_count   = arg_count;
	Need needs[arg_count] = {args...};
	CopyMemory(agent->needs_array, needs, arg_count*sizeof(Need));
	counts.agents++;
	return agent;
}

Entity* make_entity(Type type, vec2i pos, u32 age){
	Entity* entity = memory_pool_push(entities_pool);
	entity->type = type;
	entity->age  = age;
	entity->pos  = pos;
	entity->name = str8{0};
	switch(type){
		case Entity_Leaf:{
			make_advert(Advert_EatLeaf, AdvertFlags_ConsumeOwnerOnCompletion, entity, vec2i{});
		}break;
		case Entity_Dirt:{
			make_advert(Advert_Dig, AdvertFlags_ConsumeOwnerOnCompletion, entity, vec2i{});
		}break;
		case Entity_Water:{
			make_advert(Advert_DrinkWater, AdvertFlags_ConsumeOwnerOnCompletion, entity, vec2i{});
		}break;
	}
	counts.entities++;
	counts.entity[type]++;
	return entity;
}

void delete_entity(Entity* entity){
	//TODO(delle) handle entity->overlap_node
	if(entity == get_entity(entity->pos)){
		set_entity(entity->pos, 0);
	}
	
	if(entity->type == Entity_Agent){
		Agent* agent = AgentFromEntity(entity);
		memory_heap_remove(agents_heap, agent);
	}else{
		//TODO(sushi) handle entity->ui
		
		entity->type = Entity_NULL;
		memory_pool_delete(entities_pool, entity);
	}
}

arrayT<Advert*> collect_adverts(Agent* agent){
	arrayT<Advert*> adverts(deshi_temp_allocator);
	for_pool(adverts_pool){
		if(!it->def) continue;
		if(vec2i_distanceToSq(agent->entity.pos,it->owner->pos) <= it->def->rangeSq){
			adverts.add(it);
		}
	}
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
					//entity in the way of the path, so generate new path or stack on top if same race
					Entity* entity_in_way = get_entity(next_node);
					if((entity_in_way->type == Entity_Agent) && (AgentFromEntity(entity_in_way)->race == agent->race)){
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
		ForX(need, agent->needs_array, agent->needs_count){
			if(abs(advert->def->costs[need->type] - action->def->costs[need->type]) > ADVERT_COST_DELTA_EPSILON){
				//TODO(delle) make a memory if adverted completion reward didnt match actual
			}
			
			delta_need(need, advert->def->costs[need->type]);
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
		if(HasFlag(advert->def->flags, AdvertFlags_ConsumeOwnerOnCompletion)){
			delete_entity(advert->owner);
		}
		if(HasFlag(advert->def->flags, AdvertFlags_ConsumeAdvertOnCompletion)){
			//TODO(delle) handle deletion
		}
	}
}

// NOTE(sushi) this is separated into another function because i call it again on leaves that are above leaves that are moved because there are too many above it
// https://encycolorpedia.com/d57835
void eval_leaf(Entity* e){
	if(!e->pos.y || get_entity(e->pos.x,e->pos.y-1)){
		// stupid way to try and get leaves to spread out 
		vec2i search = e->pos;
		while(search.y < WORLD_HEIGHT && get_entity(search.x,search.y)) search.y++;
		if(search.y-e->pos.y > 1){
			if(e->pos.x == 0 && !get_entity(e->pos.x+1,e->pos.y)){
				move_entity(e,{e->pos.x+1,e->pos.y});
			}else if(e->pos.x == WORLD_WIDTH-1 && !get_entity(e->pos.x-1,e->pos.y)){
				move_entity(e,{e->pos.x-1,e->pos.y});
			}else{
				Entity* l = get_entity(e->pos.x-1,e->pos.y);
				Entity* r = get_entity(e->pos.x+1,e->pos.y);
				if(!l&&!r) move_entity(e,{e->pos.x+(rand()%2?1:-1),e->pos.y});
				else if(l) move_entity(e,{e->pos.x+1,e->pos.y});
				else if(r) move_entity(e,{e->pos.x-1,e->pos.y});
			}
		}
		if(search.x != e->pos.x){
			forI(search.y - e->pos.y - 1){
				Entity* above = get_entity(search.x, e->pos.y + i + 1);
				if(!above) continue;
				u32 col = get_pixelfg(above->pos.x, above->pos.y);
				move_entity(above, {above->pos.x, above->pos.y-1});
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
	srand(13535153135);
	
	//init storage
	world.map = (Entity**)memalloc(sizeof(Entity*) * WORLD_WIDTH * WORLD_HEIGHT);
	agents_heap = memory_heap_init_bytes(Megabytes(1));
	memory_pool_init(adverts_pool, 1024);
	memory_pool_init(entities_pool, 1024);
	
	//init weather
	world.weather.type = Weather_Clear;
	world.weather.wind_strength = 0;
	
	//generate terrain
	//TODO(sushi) initialize world by spawning all items in mid air so they fall down and it looks cool
	s32 vel = 0;
	s32 pos = WORLD_HEIGHT / 2;
	forI(WORLD_WIDTH){
		if(i%WORLD_WIDTH/(rand() % 8 + 8)){
			u32 mag = 0;
			if     (rand()%16==0) { mag = 8; }
			else if(rand()% 8==0) { mag = 6; }
			else if(rand()% 4==0) { mag = 4; }
			else                    mag = 2;
			
			if(pos < u32(WORLD_HEIGHT/6.f)) vel = rand() % mag+1;
			else if(pos > u32(5.f*WORLD_HEIGHT/6.f)) vel = -(rand() % mag + 1);
			else vel = rand() % (mag+1) - mag/2;
		}
		pos += vel;
		pos = Clamp(pos,0, WORLD_HEIGHT-1);
		
		u32 color = EntityColors[Entity_Dirt][rand()%7];
		forX(j,pos){
			if(rand()%2==0) color = EntityColors[Entity_Dirt][rand()%7];
			Entity* e = make_entity(Entity_Dirt, {i,j}, 0);
			e->color = color;
			set_entity(i,j,e);
			e->name = STR8("dirt");
			set_pixelbg(i,j,e->color);
		}
		forX(j,WORLD_HEIGHT-pos){
			set_pixelbg(i,pos+j,0xffcd7f07);
		}
	}
	
	sim.mode = Mode_Navigate;
	//sim.paused = 1;
}

void update_simulation(){
	if(!sim.paused || sim.step){
		sim.step = false;
		
		//agents tick needs and choose adverts
		for_node(agents_node.next){
			Agent* agent = AgentFromNode(it);
			
			ForX(need, agent->needs_array, agent->needs_count){
				delta_need(need, need->delta);
			}
			
			if(agent->active_advert == 0){
				arrayT<Advert*> adverts = collect_adverts(agent);
				agent->active_advert = select_advert(agent, adverts.data, adverts.count);
			}
		}
		
		//agents perform actions (after all agents decide what to do)
		for_node(agents_node.next){
			perform_actions(AgentFromNode(it));
		}
		
		//update entities
		for_pool(entities_pool){
			//if(it == sim.break_on_me) sim.break_on_me = 0, DebugBreakpoint;
			switch(it->type){
				case Entity_Leaf: eval_leaf(it); break;
				case Entity_Dirt:{
					if(!it->pos.y || get_entity(it->pos.x,it->pos.y-1)) break;
					move_entity(it, vec2i{it->pos.x,it->pos.y-1});
				}break;
				case Entity_Water: eval_water(it); break;
			}
			it->age++;
		}
		
		//spawn more leaves
		if(counts.entity[Entity_Leaf] < 50){
			u32 add = (50 - counts.entity[Entity_Leaf]) +  rand() % 10; 
			forI(add){
				vec2i pos = {rand() % WORLD_WIDTH, WORLD_HEIGHT-1};
				while(get_entity(pos.x,pos.y) && pos.y) pos.y -= 1;
				if(!pos.y){ add--; continue; } // we somehow failed to place the leaf anywhere in the random column, whatever
				Entity* e = make_entity(Entity_Leaf, pos, 0);
				e->name = STR8("leaf");
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
					e->name = STR8("water");
					e->color = EntityColors[Entity_Water][rand()%7];
					set_entity(pos.x,pos.y,e);
				}
			}break;
		}
		sim.ticks++;
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
	uiItem* entity_list;
	uiItem* hover_container;
	struct{
		uiItem* header;
		uiItem* container;
		
		struct{
			uiItem* header;
			b32 collapsed;
			
			uiItem* list;
			Advert* selected;
		}adverts;
	}entity;
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
	
	forI(rows*columns) {
		u32 len = texts[i].count;
		if(len > max[i%rows]) max[i%rows] = len;
	}
	
	forI(rows*columns) {
		u32 len = max[i%rows];
		dstr8_grow(&build, len);
		memcpy(build.str+build.count, texts[i].str, texts[i].count);
		memset(build.str+build.count+texts[i].count, ' ', len-texts[i].count);
		build.count += len;
		if(i%columns == columns-1)  dstr8_append(&build, STR8("\n"));
	}
	
	return build.fin;
}

void setup_ui(){
	g_ui->base.style.font        = assets_font_create_from_path_bdf(STR8("gohufont-11.bdf"));
	g_ui->base.style.font_height = 11;
	g_ui->base.style.text_color  = Color_White;
	
	uiItem* main = ui_begin_item(0);{ main->id = STR8("ant_sim.main");
		main->style.sizing = size_percent;
		main->style.size = {100,100};
		main->style.display = display_flex | display_horizontal;
		main->style.background_color = {20,20,20,255};
		
		ui.universe = ui_begin_item(0);{ ui.universe->id = STR8("ant_sim.universe");
			ui.universe->style.sizing = size_flex | size_percent_y;
			ui.universe->style.size = {512, 100};
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
					ui.background->style.size = {100,100};
					ui.background->style.hover_passthrough = 1;
					
					ui.foreground = ui_begin_item(0);{ ui.foreground->id = STR8("ant_sim.universe.world.foreground");
						ui.foreground->style.background_image = rendering.foreground.texture;
						ui.foreground->style.background_color = Color_White;
						ui.foreground->style.sizing = size_percent;
						ui.foreground->style.size = {100,100};
						ui.foreground->style.hover_passthrough = 1;
					}ui_end_item();
				}ui_end_item();
			}ui_end_item();
		}ui_end_item();
		
		ui.info = ui_begin_item(0);{ ui.info->id = STR8("ant_sim.info");
			ui.info->style.sizing = size_percent;
			ui.info->style.size = {40, 100};
			ui.info->style.padding = {10,10,10,10};
			
			//TODO replace with tab widget
			uiItem* tool_select = ui_begin_item(0);{ tool_select->id = STR8("ant_sim.info.tool_select");
				tool_select->style.background_color = Color_DarkGrey;
				tool_select->style.sizing = size_auto;
				tool_select->style.padding = {2,2,2,2};
				tool_select->style.display = display_horizontal;
				
				//TODO(sushi) add visual feedback to these buttons
				uiItem* navigate = ui_begin_item(0);{ navigate->id = STR8("ant_sim.info.tool_select.navigate");
					navigate->style.sizing = size_auto;
					navigate->style.padding = {2,2,2,2};
					
					ui_make_text(STR8("Nav"), 0)->style.hover_passthrough = 1;
					
					navigate->action_trigger = action_act_mouse_released;
					navigate->action = [](uiItem* item){
						change_mode(Mode_Navigate);
					};
					
					navigate->update_trigger = action_act_always;
					navigate->__update = [](uiItem* item){
						if(sim.mode == Mode_Navigate){
							item->style.background_color = Color_DarkRed;
						}else{
							item->style.background_color = {0};
						}
					};
				}ui_end_item();
				
				uiItem* draw = ui_begin_item(0);{ draw->id = STR8("ant_sim.info.tool_select.draw");
					draw->style.sizing = size_auto;
					draw->style.padding = {2,2,2,2};
					
					ui_make_text(STR8("Draw"), 0)->style.hover_passthrough = 1;
					
					draw->action_trigger = action_act_mouse_released;
					draw->action = [](uiItem* item){
						change_mode(Mode_Draw);
					};
					
					draw->update_trigger = action_act_always;
					draw->__update = [](uiItem* item){
						if(sim.mode == Mode_Draw){
							item->style.background_color = Color_DarkRed;
						}else{
							item->style.background_color = {0};
						}
					};
				}ui_end_item();
			}ui_end_item();
			
			uiItem* break_button = ui_begin_item(0);{ //TODO replace with button widget
				break_button->id = STR8("ant_sim.info.break_button");
				break_button->style.sizing = size_auto;
				break_button->style.background_color = Color_VeryDarkCyan;
				break_button->style.border_color = Color_White;
				break_button->style.border_width = 1;
				break_button->style.border_style = border_solid;
				break_button->style.padding = {2,2,2,2};
				
				ui_make_text(STR8("break on click"), 0)->style.hover_passthrough = 1;
				
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
			ui.entity_list = ui_begin_item(0);{ ui.entity_list->id = STR8("ant_sim.info.entity_list");
				ui.entity_list->style.sizing = size_percent_x | size_auto_y;
				ui.entity_list->style.width = 100/*percent*/;
				ui.entity_list->style.padding = {5,5,5,5};
				ui.entity_list->style.background_color = Color_DarkGrey;
				
				uiItem* entity_list = ui_begin_item(0);{
					entity_list->id = STR8("ant_sim.info.draw_menu.entity_list");
					entity_list->style.width = 100/*percent*/;
					entity_list->style.sizing = size_auto_y | size_percent_x;
					forI(Entity_COUNT){
						if(i == Entity_Agent) continue; //handle agent races below
						
						uiItem* item = ui_begin_item(0);{
							item->id = to_dstr8v(deshi_allocator, "ant_sim.info.draw_menu.entity_list.", EntityStrings[i]).fin;
							item->userVar = i;
							item->style.background_color = Color_VeryDarkCyan;
							item->style.margin_bottom = 2;
							item->style.content_align = {0.0,0.5};
							item->style.width = 100/*percent*/;
							item->style.sizing = size_auto_y | size_percent_x;
							item->style.padding_left = 1;
							
							ui_make_text(EntityStrings[i], 0)->style.hover_passthrough = 1;
							
							item->action_trigger = action_act_mouse_released;
							item->action = [](uiItem* item){
								sim.drawing.entity_type = item->userVar;
								sim.drawing.agent_race = Race_COUNT;
								item->dirty = 1;
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
					
					forI(Race_COUNT){
						uiItem* item = ui_begin_item(0);{ item->id = to_dstr8v(deshi_allocator, "ant_sim.info.draw_menu.entity_list.Agent.", RaceStrings[i]).fin;
							item->userVar = i;
							item->style.background_color = Color_VeryDarkCyan;
							item->style.margin_bottom = 2;
							item->style.content_align = {0.0,0.5};
							item->style.width = 100/*percent*/;
							item->style.sizing = size_auto_y | size_percent_x;
							item->style.padding_left = 1;
							
							ui_make_text(RaceStrings[i], 0)->style.hover_passthrough = 1;
							
							item->action_trigger = action_act_mouse_released;
							item->action = [](uiItem* item){
								sim.drawing.entity_type = Entity_Agent;
								sim.drawing.agent_race = item->userVar;
								item->dirty = 1;
							};
							
							item->update_trigger = action_act_always;
							item->__update = [](uiItem* item){
								if(sim.drawing.entity_type == Entity_Agent && sim.drawing.agent_race == item->userVar){
									item->style.background_color = Color_DarkRed;
								}else{
									item->style.background_color = Color_VeryDarkCyan;
								}
							};
						}ui_end_item();
					}
				}ui_end_item();
			}ui_end_item();
			
			ui_make_text(STR8("Keys ---------------"), 0); //TODO replace with header widget
			ui_make_text(aligned_text(3,3,{
										  STR8("pause"),    STR8(" - "), STR8("space"),
										  STR8("draw"),     STR8(" - "), STR8("lshift + d"),
										  STR8("navigate"), STR8(" - "), STR8("lshift + n"),
										  STR8("select"),   STR8(" - "), STR8("right click"),
									  }), 0)->id = STR8("ant_sim.info.keys");
			
			ui_make_text(STR8("Hovered ---------------"), 0); //TODO replace with header widget
			ui.hover_container = ui_begin_item(0);{ ui.hover_container->id = STR8("ant_sim.info.hover_container");
				ui.hover_container->style.sizing = size_percent_x | size_auto_y;
				ui.hover_container->style.width = 100/*percent*/;
				ui.hover_container->style.padding_left = 5/*pixels*/;
			}ui_end_item();
			
			ui_make_text(STR8("Selected Entity ---------------"), 0); //TODO replace with header widget
			ui.entity.header = ui_begin_item(0);{ ui.entity.header->id = STR8("ant_sim.info.entity.header");
				ui.entity.header->style.sizing = size_percent_x | size_auto_y;
				ui.entity.header->style.width = 100/*percent*/;
				
				ui.entity.container = ui_begin_item(0);{ ui.entity.container->id = STR8("ant_sim.info.entity.container");
					ui.entity.container->style.sizing = size_percent_x | size_auto_y;
					ui.entity.container->style.width = 100/*percent*/;
					ui.entity.container->style.padding_left = 5/*pixels*/;
				}ui_end_item();
				
				//TODO replace with header widget
				ui.entity.adverts.collapsed = false;
				ui.entity.adverts.header = ui_begin_item(0);{ ui.entity.adverts.header->id = STR8("ant_sim.info.entity.adverts.header");
					ui.entity.adverts.header->style.sizing = size_percent_x | size_auto_y;
					ui.entity.adverts.header->style.width = 100/*percent*/;
					ui.entity.adverts.header->style.padding_left = 5/*pixels*/;
					ui.entity.adverts.header->style.background_color = Color_DarkGrey;
					
					ui_make_text(STR8("Adverts ---------------"), 0)->style.hover_passthrough = 1;
					
					ui.entity.adverts.header->action_trigger = action_act_mouse_released;
					ui.entity.adverts.header->action = [](uiItem* item){
						ToggleBool(ui.entity.adverts.collapsed);
						if(HasFlag(ui.entity.adverts.list->style.display, display_hidden)){
							RemoveFlag(ui.entity.adverts.list->style.display, display_hidden);
						}else{
							AddFlag(ui.entity.adverts.list->style.display, display_hidden);
						}
					};
				}ui_end_item();
				
				ui.entity.adverts.list = ui_begin_item(0);{ ui.entity.adverts.list->id = STR8("ant_sim.info.entity.adverts.body");
					ui.entity.adverts.list->style.sizing = size_percent_x | size_auto_y;
					ui.entity.adverts.list->style.width = 100/*percent*/;
					ui.entity.adverts.list->style.padding_left = 5/*pixels*/;
					ui.entity.adverts.list->style.background_color = Color_DarkGrey;
				}ui_end_item();
			}ui_end_item();
		}ui_end_item();
	}ui_end_item();
}

void update_ui(){
	Entity* hovered = get_entity_under_mouse();
	
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
					pausebox->style.anchor = anchor_bottom_right;
					pausebox->style.sizing = size_auto;
					pausebox->style.padding = {2,2,2,2};
					pausebox->style.background_color = color(255*(sin(1.5*DeshTime->totalTime/1000 + cos(1.5*DeshTime->totalTime/1000))+1)/2, 0, 0, 255);
					ui_make_text(STR8("paused"), 0)->id = STR8("ant_sim.info_window.paused");
				}ui_end_item();
			}
		}ui_end_item();
	}ui_end_immediate_branch();
	
	assets_texture_update(rendering.background.texture, vec2i{0,0}, vec2i{WORLD_WIDTH,WORLD_HEIGHT});
	assets_texture_update(rendering.foreground.texture, vec2i{0,0}, vec2i{WORLD_WIDTH,WORLD_HEIGHT});
	
	//zooming (even though ui handles moving the world for us, we still need to handle zooming)
	vec2 cursize = ui.background->style.size;
	vec2 local_mouse = input_mouse_position() - ui.world->pos_screen;
	ui.background->style.size.x += DeshInput->scrollY / 10.0 * cursize.x;
	ui.background->style.size.y += DeshInput->scrollY / 10.0 * cursize.y;
	vec2 diff = ui.background->style.size - cursize;
	if(DeshInput->scrollY){
		ui.world->style.pos.x -= local_mouse.x * ((ui.background->style.size.x / cursize.x) - 1);
		ui.world->style.pos.y -= local_mouse.y * ((ui.background->style.size.y / cursize.y) - 1);
	}
	
	ui_begin_immediate_branch(ui.hover_container);{
		ui_make_text(to_dstr8v(deshi_temp_allocator, STR8("entity: "), (hovered) ? hovered->name : STR8("nothing")).fin, 0)->id = STR8("ant_sim.info.hovered_container.entity");
		
		auto [tile_pos,valid_tile] = get_tile_under_mouse();
		if(valid_tile){
			ui_make_text(to_dstr8v(deshi_temp_allocator, STR8("tile  : "), tile_pos).fin, 0)->id = STR8("ant_sim.info.hovered_container.tile");
		}else{
			ui_make_text(STR8("tile  : nothing"), 0)->id = STR8("ant_sim.info.hovered_container.tile");
		}
		
		ui_make_text(to_dstr8v(deshi_temp_allocator, STR8("ui    : "), (g_ui->hovered) ? g_ui->hovered->id : STR8("nothing")).fin, 0)->id = STR8("ant_sim.info.hovered_container.ui");
	}ui_end_immediate_branch();
	
	ui_begin_immediate_branch(ui.entity.container);{
		if(sim.selected_entity){
			ui_make_text(to_dstr8v(deshi_temp_allocator, "name: ", sim.selected_entity->name).fin, 0)->id = STR8("ant_sim.info.entity.name");
			ui_make_text(to_dstr8v(deshi_temp_allocator, "type: ", EntityStrings[sim.selected_entity->type]).fin, 0)->id = STR8("ant_sim.info.entity.type");
			ui_make_text(to_dstr8v(deshi_temp_allocator, "age : ", sim.selected_entity->age).fin, 0)->id = STR8("ant_sim.info.entity.age");
			ui_make_text(to_dstr8v(deshi_temp_allocator, "pos : ", sim.selected_entity->pos).fin, 0)->id = STR8("ant_sim.info.entity.pos");
			
			switch(sim.selected_entity->type){
				case Entity_Agent:{
					Agent* agent = AgentFromEntity(sim.selected_entity);
					ui_make_text(to_dstr8v(deshi_temp_allocator, "species: ", RaceSpeciesStrings[agent->race]).fin, 0)->id = STR8("ant_sim.info.entity.agent.species");
					ui_make_text(to_dstr8v(deshi_temp_allocator, "race   : ", RaceStrings[agent->race]).fin, 0)->id = STR8("ant_sim.info.entity.agent.race");
					ui_make_text(STR8("Needs ----------"), 0)->id = STR8("ant_sim.info.entity.agent.needs_header");
					
					dstr8 needs_builder;
					dstr8_init(&needs_builder, str8{}, deshi_temp_allocator);
					ForX(need,agent->needs_array,agent->needs_count){
						dstr8_append(&needs_builder, STR8("["));
						f32 need_percent = need->value / MAX_NEED_VALUE;
						u32 need_percent_whole = (u32)(need_percent * 100.f);
						u32 dash_count = (u32)(need_percent / 10.f);
						u32 space_count = 10 - dash_count;
						forI(dash_count) dstr8_append(&needs_builder, STR8("-"));
						forI(space_count) dstr8_append(&needs_builder, STR8(" "));
						dstr8_append(&needs_builder, STR8("] ("));
						dstr8_append(&needs_builder, to_dstr8(need_percent_whole, deshi_temp_allocator));
						dstr8_append(&needs_builder, STR8("%) "));
						dstr8_append(&needs_builder, NeedStrings[need->type]);
						dstr8_append(&needs_builder, STR8("\n"));
					}
					ui_make_text(dstr8_peek(&needs_builder), 0)->id = STR8("ant_sim.info.entity.agent.needs_list");
				}break;
				case Entity_Water:{
					ui_make_text(to_dstr8v(deshi_temp_allocator, "pressure: ", sim.selected_entity->water.pressure).fin, 0)->id = STR8("ant_sim.info.entity.water.pressure");
				}break;
			}
		}else{
			ui_make_text(STR8("nothing selected"), 0)->id = STR8("ant_sim.info.entity.nothing");
		}
	}ui_end_immediate_branch();
	
	ui_begin_immediate_branch(ui.entity.adverts.list);{
		if(sim.selected_entity){
			if(sim.selected_entity->adverts_count == 0){
				ui_make_text(STR8("no adverts"), 0);
			}else{
				forX(advert_idx,sim.selected_entity->adverts_count){
					Advert* advert = sim.selected_entity->adverts_array[advert_idx];
					
					uiItem* item = ui_begin_item(0);{
						str8 advert_name = to_dstr8v(deshi_temp_allocator, advert_idx, STR8("_"), advert->def->name).fin;
						item->id = to_dstr8v(deshi_temp_allocator, STR8("ant_sim.info.entity.adverts.list."), advert_name).fin;
						item->userVar = (u64)advert;
						item->style.background_color = Color_VeryDarkCyan;
						item->style.margin_bottom = 2;
						item->style.content_align = {0.0,0.5};
						item->style.sizing = size_auto_y | size_percent_x;
						item->style.width = 100/*percent*/;
						item->style.padding_left = 1;
						
						ui_make_text(advert_name, 0)->style.hover_passthrough = 1;
						
						item->action_trigger = action_act_mouse_released;
						item->action = [](uiItem* item){
							ui.entity.adverts.selected = (Advert*)item->userVar;
							item->dirty = 1;
						};
						
						item->update_trigger = action_act_always;
						item->__update = [](uiItem* item){
							if(ui.entity.adverts.selected == (Advert*)item->userVar){
								item->style.background_color = Color_DarkRed;
							}else{
								item->style.background_color = Color_VeryDarkCyan;
							}
						};
					}ui_end_item();
				}
			}
		}
	}ui_end_immediate_branch();
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @input
void update_input(){
	if(key_pressed(Key_SPACE | InputMod_None)) sim.paused = !sim.paused;
	if(key_pressed(Key_SPACE | InputMod_Lctrl)) sim.step = true;
	
	if(key_pressed(Key_N | InputMod_Lshift)) change_mode(Mode_Navigate);
	if(key_pressed(Key_D | InputMod_Lshift)) change_mode(Mode_Draw);
	
	if(input_rmouse_released()) sim.selected_entity = get_entity_under_mouse();
	
	switch(sim.mode){
		case Mode_Navigate:{
			
		}break;
		case Mode_Draw:{
			if(key_pressed(Key_1)) sim.drawing.tool = DrawTool_Draw_Square;
			if(key_pressed(Key_2)) sim.drawing.tool = DrawTool_Erase_Square;
			
			if(get_entity_under_mouse()) break;
			
			auto [pos, ok] = get_tile_under_mouse();
			if(!ok || sim.break_on_me) break;
			
			switch(sim.drawing.tool){
				case DrawTool_Draw_Square:{
					if(input_lmouse_down()){
						if(sim.drawing.entity_type != Entity_Agent){
							Entity* e = make_entity(sim.drawing.entity_type, pos, 0);
							e->color = EntityColors[sim.drawing.entity_type][rand()%7];
							e->name = EntityStrings[sim.drawing.entity_type];
							set_entity(pos.x,pos.y, e);
						}else{
							//TODO make agents
						}
					}
				}break;
				case DrawTool_Erase_Square:{
					//TODO entity/agent deletion
				}break;
			}
		}break;
	}
}


//-////////////////////////////////////////////////////////////////////////////////////////////////
//// @main
int main(int args_count, char** args){
	deshi_init_specify("ant_sim",Megabytes(256),Megabytes(512));
	
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
