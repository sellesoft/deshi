/* ant_sim deshi example

Index:
@utils
@vars
@entity
@need
@action
@agent
@advert
@world
@pathfinding
@simulation
@main
*/
#include "deshi.h"
struct Advert;
struct Path;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@utils
#define TICKS_PER_WORLD_SECOND 1
#define TICKS_PER_WORLD_MINUTE 60
#define TICKS_PER_WORLD_DAY    86400
#define TICKS_PER_WORLD_MONTH  2629740
#define TICKS_PER_WORLD_YEAR   31536000

u32 divide_color(u32 color, u32 divisor){
	u32 r = (color >>  0 & 0x000000ff) / divisor;
	u32 g = (color >>  8 & 0x000000ff) / divisor;
	u32 b = (color >> 16 & 0x000000ff) / divisor;
	return PackColorU32(255,b,g,r);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//@entity
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

	union{
		struct{
			b32 evaluating;
			b32 evaluated;
			u32 pressure;
		}water;
	};

	uiItem* ui;
}Entity;

enum{
	Flags_Water_Evaluated = 1 << 0,
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//@need/cost
#define MAX_NEED_VALUE 100000
#define MIN_NEED_VALUE      0

enum{
	Need_Bladder,
	Need_Food,
	Need_Health,
	Need_Mating,
	Need_Sleep,
	Need_Water,
	//Need_PADDING, //NOTE only necessary when there is an odd number of needs
	Need_COUNT
};
str8 NeedStrings[] = {
	STR8("Bladder"),
	STR8("Food"),
	STR8("Health"),
	STR8("Mating"),
	STR8("Sleep"),
	STR8("Water"),
	//STR8("PADDING"),
};StaticAssert(ArrayCount(NeedStrings) == Need_COUNT);

typedef struct Need{
	Type type;
	f32 value;
	f32 delta;
	u32 padding;
}Need;

void delta_need(Need* need, f32 delta){
	need->value += delta;
	need->value  = Clamp(need->value, MIN_NEED_VALUE, MAX_NEED_VALUE);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//@action
typedef struct ActionDef{
	str8 name;
	f32 costs[Need_COUNT];
}ActionDef;

typedef struct Action{
	ActionDef* def;
	vec2i target;
	u32 completion_time;
	f32 progress; //0-1; 1 is complete
}Action;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@agent
#define AGENT_ADVERT_QUEUE_SIZE 2

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
	Entity* entity;
	Type race;
	u32 action_index;
	Advert* adverts_queue[AGENT_ADVERT_QUEUE_SIZE];
	Need* needs_array;
	u32 needs_count;
	u32 inventory_size;
	Entity** inventory_array;
	Path* path;
}Agent;
#define AgentFromNode(ptr) CastFromMember(Agent,node,ptr)
#define AgentFromEntity(ptr) CastFromMember(Agent,entity,ptr)


///////////////////////////////////////////////////////////////////////////////////////////////////
//@advert
#define ADVERT_COST_DELTA_EPSILON (10)
#define ADVERT_TIME_DELTA_EPSILON (5*TICKS_PER_WORLD_SECOND)

enum{
	AdvertFlags_None                      = 0,
	AdvertFlags_ConsumeAdvertOnCompletion = (1 << 0),
	AdvertFlags_ConsumeOwnerOnCompletion  = (1 << 1),
};

typedef struct AdvertDef{
	str8 name;
	Flags flags;
	u32 padding;
	f32 costs[Need_COUNT];
	u32 rangeSq;
	u32 actions_count;
	ActionDef** actions_array;
}AdvertDef;

typedef struct Advert{
	AdvertDef* def;
	Entity* owner;
	Action* actions_array;
	u32 actions_count; //NOTE new actions can be inserted during progress towards advert
	u32 completion_time;
	u32 ongoing_time;
	u32 padding;
}Advert;

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
	return distance_attenuation(average, agent->entity->pos, advert->owner->pos) / advert->completion_time;
}

Advert* select_advert(Agent* agent, Advert* adverts, u32 adverts_count){
#define MAX_BEST_ADS 3
	Advert* best_ads[MAX_BEST_ADS] = {0};
	f32 best_scores[MAX_BEST_ADS] = {0};
	u32 score_count = 0;
	
	//score the adverts and get the best three
	forX(advert_idx, adverts_count){ Advert* advert = adverts+advert_idx;
		f32 score = score_advert(agent, advert);
		if(score < 0) continue; //skip ads that have no benefits
		
		Advert* compare_ad = advert;
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


///////////////////////////////////////////////////////////////////////////////////////////////////
//@world
#define WORLD_WIDTH 512
#define WORLD_HEIGHT 512

enum{
	Weather_Clear,
	Weather_Cloudy,
	Weather_Rain,
	Weather_Thunderstorm,
	Weather_Snow,
};

struct{
	Entity** map;

	struct{
		Type type;
		s32 wind_strength;
		s32 temperature; // celsius
	}weather;

}world;

Entity* get_entity(u32 x, u32 y){
	if(x < 0 || y < 0 || x > WORLD_WIDTH || y > WORLD_HEIGHT) return 0;
	return world.map[x+y*WORLD_WIDTH];
}

Entity* get_entity(vec2i pos){
	return get_entity(pos.x, pos.y);
}

b32 set_pixelfg(u32 x,u32 y,u32 val);
b32 set_entity(u32 x, u32 y, Entity* entity){
	if(x < 0 || y < 0 || x > WORLD_WIDTH || y > WORLD_HEIGHT) return 0;
	world.map[x+y*WORLD_WIDTH] = entity;
	if(entity) set_pixelfg(x,y,entity->color);
	else set_pixelfg(x,y,0);
	return 1;
}

b32 set_entity(vec2i pos, Entity* entity){
	return set_entity(pos.x,pos.y,entity);
}

b32 move_entity(Entity* e, s32 x, s32 y){
	if(get_entity(x,y)) return 0;
	set_entity(e->pos,0);
	set_entity(x,y,e);
	e->pos = {x,y};
	return 1;
}

b32 move_entity(Entity* e, vec2i pos){
	return move_entity(e,pos.x,pos.y);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//@pathfinding
typedef struct NavNode{
	vec2i pos;
	NavNode* edges_array;
	u32 edges_count;
	u32 padding;
}NavNode;

typedef struct Path{
	NavNode* nodes_array;
	u32 nodes_count;
	u32 current_index;
}Path;

//pathfind from agents starting position (inclusive) to target position (inclusive) using A* algorithm
//if no valid path is found, returns best path towards target
Path* generate_path(Agent* agent, vec2i target){ //TODO(delle) implement pathfinding
	//find empty point closest to target (along line between)
	
	//A* pathfind to that closest point
	
	//cache the pathing data
	
	//return the path
	return 0;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//@simulation
Arena* action_def_arena;
Arena* advert_def_arena;
Heap* agents_heap;
Node agents_node;
Advert* adverts_pool;
Entity* entities_pool;

enum{
	DrawTool_Draw_Square,
	DrawTool_Erase_Square,
};

struct{
	Type mode;
	struct{
		Type tool;
		Type entity_type;
	}drawing;
	Entity* break_on_me;
	u64 ticks;
	b32 paused;
	b32 step;
}sim;

struct{
	u32 entity[Entity_COUNT]; // count of each entity
	u32 entities; // total entities
	u32 agents; // sub count of entities
	u32 actions;
	u32 adverts; 
}counts = {0};

template<typename... Args>
Agent* make_agent(Type race, u32 age, vec2i pos, Args... args){
	constexpr u32 arg_count = sizeof...(Args);
	Agent* agent = (Agent*)memory_heap_add_bytes(agents_heap, sizeof(Agent) + arg_count*sizeof(Need));
	agent->entity.name  = str8{0};
	agent->entity.age   = age;
	agent->entity.pos   = pos;
	agent->race         = race;
	agent->action_index = -1;
	forI(AGENT_ADVERT_QUEUE_SIZE) agent->adverts_queue[i] = 0;
	agent->needs_array  = (Need*)(agent+1);
	agent->needs_count  = arg_count;
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
	counts.entities++;
	counts.entity[type]++;
	return entity;
}

array<Advert*> collect_adverts(Agent* agent){
	array<Advert*> adverts(deshi_temp_allocator);
	for_pool(adverts_pool){
		if(!it->def) continue;
		if(vec2i_distanceToSq(agent->entity->pos,it->owner->pos) <= it->def->rangeSq){
			adverts.add(it);
		}
	}
	return adverts;
}

void tick_agent_needs(Agent* agent){
	For(agent->needs_array, agent->needs_count){
		delta_need(it, it->delta);
	}
}

void tick_agent_adverts(Agent* agent){
	//advance the advert actions
	b32 new_advert = false;
	
	//select an advert to add to the queue
	if(new_advert){
		
	}
}

void tick_agent_actions(Agent* agent){
	if(agent->adverts_queue[0] == 0) return;
	
	//perform action
	Advert* advert = agent->adverts_queue[0];
	Action* action = &advert->actions_array[agent->action_index];
	/*
	switch(action->def->type){
		case Action_Walk:{
			//move to next nav node pos
			NavNode* nav_node = &agent->path->nodes_array[agent->path->current_index];
			if(move_entity(agent->entity, nav_node->pos)){
				agent->path.current_index += 1;
			}else{
				//entity in the way of the path, so generate new path or stack on top if same race
				Entity* entity_in_way = get_entity(nav_node->pos);
				if((entity_in_way->type == Entity_Agent) && (AgentFromEntity(entity_in_way)->race == agent->race)){
					//TODO(delle) handle entity stacking
				}else{
					Path* path = generate_path(agent, action->target);
					if(path->nodes_count == 0){ //no valid path
						//TODO(delle) handle no valid path (dig or wait)
						return;
					}
				}
			}
			
			//at destination
			if(vec2i_equal(agent->entity->pos, action->target)){
				action->progress = 1.0f;
			}
		}break;
		case Action_Dig:{
			//too far from target, insert a walk action
			if(vec2i_distanceToSq(agent->entity->pos, action->target) > 1){
				Path* path = generate_path(agent, action->target);
				vec2i closest_point = path->nodes_array[path->nodes_count-1].pos;
				add_action(advert, agent->action_index, &ActionDefinitions[Action_Walk], closest_point);
				tick_agent_actions(agent);
				return;
			}
			
			//TODO(delle) digging
			//TODO(delle) adding dirt to inventory
		}break;
		default:{
			if(action->def->type < Action_COUNT){
				LogE("ant_sim","Unhandled action type: ",ActionStrings[action->def->type]);
			}else{
				LogE("ant_sim","Unhandled action type: ",action->def->type);
			}
		}break;
	}
	*/
	
	//action completed
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
	
	//advert completed
	if(agent->action_index >= advert->actions_count){
		if(abs((int)(advert->completion_time - advert->ongoing_time)) > ADVERT_TIME_DELTA_EPSILON){
			//TODO(delle) make a memory if adverted completion time didnt match actual
		}
		
		//remove advert from queue
		forI(AGENT_ADVERT_QUEUE_SIZE-1){
			agent->adverts_queue[i] = agent->adverts_queue[i+1];
		}
		agent->adverts_queue[AGENT_ADVERT_QUEUE_SIZE-1] = 0;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//@render
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

struct{
	uiItem* main;
	uiItem* worldwin;
	uiItem* worldholder;
	uiItem* background; // NOTE(sushi) the foreground is a child of the background so that it follows it and always draws over
	uiItem* foreground;
	uiItem* info;
	uiItem* draw_menu;
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

enum{
	Mode_Navigate,
	Mode_Draw,
};

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

// allocates a temporary string
str8 aligned_text(u32 rows, u32 columns, array<str8> texts){
	str8b build;
	str8_builder_init(&build, {0}, deshi_temp_allocator);

	u32* max = (u32*)StackAlloc(sizeof(u32)*columns);
	memset(max, 0, sizeof(u32)*columns);

	forI(rows*columns) {
		u32 len = texts[i].count;
		if(len > max[i%rows]) max[i%rows] = len;
	}

	forI(rows*columns) {
		u32 len = max[i%rows];
		str8_builder_grow(&build, len);
		memcpy(build.str+build.count, texts[i].str, texts[i].count);
		memset(build.str+build.count+texts[i].count, ' ', len-texts[i].count);
		build.count += len;
		if(i%columns == columns-1)  str8_builder_append(&build, STR8("\n"));
	}

	return build.fin;
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
	if(e->water.evaluated == sim.ticks % 2) return;
	e->water.evaluated = sim.ticks % 2;

	if(sim.break_on_me == e){
		DebugBreakpoint;
		sim.break_on_me = 0;
	} 
	
	Entity* t = get_entity(e->pos.x,e->pos.y+1);
	Entity* r = get_entity(e->pos.x+1,e->pos.y);
	Entity* b = get_entity(e->pos.x,e->pos.y-1);
	Entity* l = get_entity(e->pos.x-1,e->pos.y);

	if(b || !e->pos.y){
		if(t && t->type == Entity_Water){
			eval_water(t);
			if(t->pos == vec2i{e->pos.x,e->pos.y+1}){
				e->water.pressure = t->water.pressure + 1;
			}else{
				e->water.pressure = 0;
				t = 0;
			}
		}else{
			e->water.pressure = 0;
		}

		if(r && r->type == Entity_Water){
			eval_water(r);
			if(r->pos == vec2i{e->pos.x+1,e->pos.y}){
				if(r->water.pressure > e->water.pressure){
					e->water.pressure += r->water.pressure;
				}
			}else{
				r = 0;
			}
		}

		if(l && l->type == Entity_Water){
			eval_water(l);
			if(l->pos == vec2i{e->pos.x-1,e->pos.y}){
				if(l->water.pressure > e->water.pressure){
					e->water.pressure += l->water.pressure;
				}
			}else{
				l = 0;
			}
		}

		if(b->water.pressure > e->water.pressure + 1){
			e->water.pressure = b->water.pressure - 1;
		}

		if(!t && e->water.pressure){
			move_entity(e,e->pos.x,e->pos.y+1);
		}else if(e->pos.x == 0 && !get_entity(e->pos.x+1,e->pos.y-1)){
			move_entity(e,{e->pos.x+1,e->pos.y-1});
		}else if(e->pos.x == WORLD_WIDTH-1 && !get_entity(e->pos.x-1,e->pos.y-1)){
			move_entity(e,{e->pos.x-1,e->pos.y-1});
		}else if(l && !r && e->water.pressure){
			move_entity(e,e->pos.x+1,e->pos.y);
		}else if(r && !l && e->water.pressure){
			move_entity(e,e->pos.x-1,e->pos.y);
		}else if (!r && !l && e->water.pressure){
			move_entity(e,e->pos.x+(rand()%2?1:-1),e->pos.y);
		}else{
			Entity* bl = get_entity(e->pos.x-1,e->pos.y-1);
			Entity* br = get_entity(e->pos.x+1,e->pos.y-1);
			// if(br&&bl){
			// 	Entity* l = get_entity(e->pos.x-1,e->pos.y);
			// 	Entity* r = get_entity(e->pos.x+1,e->pos.y);
			// 	move_entity(e,{e->pos.x+(rand()%2?1:-1),e->pos.y});
			// 	if(!l&&!r) 
			// 	else if(l) move_entity(e,{e->pos.x+1,e->pos.y});
			// 	else if(r) move_entity(e,{e->pos.x-1,e->pos.y});
			// } 
			if(!bl&&!br) move_entity(e,{e->pos.x+(rand()%2?1:-1),e->pos.y-1});
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
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//@main
int main(int args_count, char** args){
	deshi_init();
	
	srand(13535153135);

	//init deshi ui2
	g_ui->base.style.font        = assets_font_create_from_file_bdf(STR8("gohufont-11.bdf"));
	g_ui->base.style.font_height = 11;
	g_ui->base.style.text_color  = Color_White;
	
	//init ant_sim storage
	world.map = (Entity**)memalloc(sizeof(Entity*) * WORLD_WIDTH * WORLD_HEIGHT);
	action_def_arena = memory_create_arena(Megabytes(1));
	advert_def_arena = memory_create_arena(Megabytes(1));
	agents_heap = memory_heap_init_bytes(Megabytes(1));
	memory_pool_init(adverts_pool, 1024);
	memory_pool_init(entities_pool, 1024);
	
	//load definitions
	//TODO(delle) definition loading
	
	//init ant_sim rendering
	rendering.foreground.data = (u32*)memalloc(sizeof(u32)*WORLD_WIDTH*WORLD_HEIGHT);
	rendering.foreground.texture = assets_texture_create_from_memory(
		rendering.foreground.data, 
		STR8("world_foreground"), 
		WORLD_WIDTH, WORLD_HEIGHT,
		ImageFormat_RGBA,
		TextureType_2D,
		TextureFilter_Nearest,
		TextureAddressMode_ClampToTransparent,
		0
	);
	rendering.background.data = (u32*)memalloc(sizeof(u32)*WORLD_WIDTH*WORLD_HEIGHT);
	rendering.background.texture = assets_texture_create_from_memory(
		rendering.background.data,
		STR8("world_background"),
		WORLD_WIDTH,WORLD_HEIGHT,
		ImageFormat_RGBA,
		TextureType_2D,
		TextureFilter_Nearest,
		TextureAddressMode_ClampToTransparent,
		0
	);
	
	ui.main = uiItemB();{
		ui.main->id = STR8("ant_sim.main");
		ui.main->style.background_color = {20,20,20,255};
		ui.main->style.sizing = size_percent;
		ui.main->style.size = {100,100};
		ui.main->style.display = display_flex | display_horizontal;
		ui.worldwin = uiItemB();{
			ui.worldwin->id = STR8("ant_sim.main.worldwin");
			ui.worldwin->style.sizing = size_percent_y | size_flex;
			ui.worldwin->style.size = {512, 100};
			ui.worldwin->style.background_color = {5,5,5,255};
			ui.worldholder = uiItemB();{
				ui.worldholder->style.sizing = size_auto;
				ui.worldholder->style.positioning = pos_draggable_fixed;
				ui.worldholder->style.padding = {10,10,10,10};
				ui.worldholder->style.border_style = border_solid;
				ui.worldholder->style.border_width = 4;
				ui.worldholder->style.border_color = Color_White;
				ui.background = uiItemB();{
					ui.background->id = STR8("ant_sim.main.background");
					ui.background->style.background_image = rendering.background.texture;
					ui.background->style.background_color = Color_White;
					ui.background->style.size = {100,100};
					ui.background->style.hover_passthrough = 1;
					ui.foreground = uiItemB();{
						ui.foreground->id = STR8("ant_sim.main.foreground");
						ui.foreground->style.background_image = rendering.foreground.texture;
						ui.foreground->style.background_color = Color_White;
						ui.foreground->style.sizing = size_percent;
						ui.foreground->style.size = {100,100};
						ui.foreground->style.hover_passthrough = 1;
					}uiItemE();
				}uiItemE();
			}uiItemE();
		}uiItemE();
		ui.info = uiItemB();{
			ui.info->id = STR8("ant_sim.main.info");
			ui.info->style.sizing = size_percent;
			ui.info->style.size = {40, 100};
			ui.info->style.padding = {10,10,10,10};
			uiItem* tool_select = uiItemB();{
				tool_select->id = STR8("ant_sim.main.info.tool_select");
				tool_select->style.background_color = Color_DarkGrey;
				tool_select->style.sizing = size_auto;
				tool_select->style.padding = {2,2,2,2};
				tool_select->style.display = display_horizontal;
				//TODO(sushi) add visual feedback to these buttons
				uiItem* navigate = uiItemB();{
					navigate->id = STR8("ant_sim.main.info.tool_select.navigate");
					navigate->style.sizing = size_auto;
					navigate->style.padding = {2,2,2,2};
					uiTextML("Nav")->style.hover_passthrough = 1;
					navigate->action = [](uiItem* item){
						change_mode(Mode_Navigate);
					}; navigate->action_trigger = action_act_mouse_released;
					navigate->__update = [](uiItem* item){
						if(sim.mode == Mode_Navigate){
							item->style.background_color = Color_DarkRed;
						}else{
							item->style.background_color = {0};
						}
					}; navigate->update_trigger = action_act_always;
				}uiItemE();
				uiItem* draw = uiItemB();{
					draw->id = STR8("ant_sim.main.info.tool_select.draw");
					draw->style.sizing = size_auto;
					draw->style.padding = {2,2,2,2};
					uiTextML("Draw")->style.hover_passthrough = 1;
					draw->action = [](uiItem* item){
						change_mode(Mode_Draw);
					}; draw->action_trigger = action_act_mouse_released;
					draw->__update = [](uiItem* item){
						if(sim.mode == Mode_Draw){
							item->style.background_color = Color_DarkRed;
						}else{
							item->style.background_color = {0};
						}
					}; draw->update_trigger = action_act_always;
				}uiItemE();
			}uiItemE();
			uiItem* break_button = uiItemB();{
				break_button->id = STR8("ant_sim.main.info.break_button");
				break_button->style.sizing = size_auto;
				break_button->style.background_color = Color_VeryDarkCyan;
				break_button->style.border_color = Color_White;
				break_button->style.border_width = 1;
				break_button->style.border_style = border_solid;
				break_button->style.padding = {2,2,2,2};
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
				break_button->action_trigger = action_act_mouse_released;
				uiTextML("break on click")->style.hover_passthrough=1;
			}uiItemE();
			ui.draw_menu = uiItemB();{
				ui.draw_menu->id = STR8("ant_sim.main.info.draw_menu");
				ui.draw_menu->style.sizing = size_percent_x | size_auto_y;
				ui.draw_menu->style.size = {100,0};
				ui.draw_menu->style.padding = {5,5,5,5};
				ui.draw_menu->style.background_color = Color_DarkGrey;
				uiItem* entity_list = uiItemB();{
					entity_list->id = STR8("ant_sim.main.info.draw_menu.entity_list");
					entity_list->style.sizing = size_auto;
					entity_list->style.content_align = {0.5, 0};
					forI(Entity_COUNT){
						uiItem* item = uiItemB();
						item->id = ToString8(deshi_allocator, "ant_sim.main.info.draw_menu.entity_list.", EntityStrings[i]);
						item->userVar = i;
						item->style.background_color = Color_VeryDarkCyan;
						item->style.margin_bottom = 2;
						item->style.content_align = {0.5,0.5};
						item->style.sizing = size_auto;
						uiTextM(EntityStrings[i])->style.hover_passthrough = 1;
						item->action = [](uiItem* item){
							sim.drawing.entity_type = item->userVar;
							item->dirty = 1;
						}; item->action_trigger = action_act_mouse_released;
						item->__update = [](uiItem* item){
							if(sim.drawing.entity_type == item->userVar){
								item->style.background_color = Color_DarkRed;
							}else{
								item->style.background_color = Color_VeryDarkCyan;
							}
						}; item->update_trigger = action_act_always;
						uiItemE();
					}
				}uiItemE();
			}uiItemE();
			uiTextML("keys:");
			uiTextM(aligned_text(3,3,{
				STR8("pause"), STR8("- "), STR8("space"),
				STR8("draw"), STR8("-"), STR8("lshift + d"),
				STR8("navigate "), STR8("-"), STR8("lshift + n")
 			}));
		}uiItemE();
	}uiItemE();

	sim.mode = Mode_Navigate;
	//sim.paused = 1;

	world.weather.type = Weather_Clear;
	world.weather.wind_strength = 0;

	// TODO(sushi) initialize world by spawning all items in mid air so they fall down and it looks cool

	s32 vel = 0;
	s32 pos = WORLD_HEIGHT / 2;
	
	// randomly generate terrain
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

	while(platform_update()){
		//simulate
		if(!sim.paused || sim.step){
			sim.step = 0;

			//agents tick needs and choose adverts
			for_node(agents_node.next){
				tick_agent_needs(AgentFromNode(it));
				tick_agent_adverts(AgentFromNode(it));
			}
			
			//agents perform actions (after all agents decide what to do)
			for_node(agents_node.next){
				tick_agent_actions(AgentFromNode(it));
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

			// weather
			switch(world.weather.type){
				case Weather_Clear:{
					
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

		Entity* hovered = get_entity_under_mouse();
		
		//update ui
		uiImmediateB();{
			persist Type anchor = anchor_top_left;
			uiItem* window = uiItemB();
			if(ui_item_hovered(window,false)) anchor = (anchor+1) % (anchor_bottom_left+1);
			window->style.positioning      = pos_absolute;
			window->style.anchor           = anchor;
			window->style.sizing           = size_auto;
			window->style.background_color = Color_DarkGrey;
			window->id                     = STR8("ant_sim.info_window");
			uiTextM(ToString8(deshi_temp_allocator, (int)F_AVG(100,1000/DeshTime->deltaTime)," fps"));
			uiItemE();
		}uiImmediateE();

		render_update_texture(rendering.background.texture, vec2i{0,0}, vec2i{WORLD_WIDTH,WORLD_HEIGHT});
		render_update_texture(rendering.foreground.texture, vec2i{0,0}, vec2i{WORLD_WIDTH,WORLD_HEIGHT});

		if(key_pressed(Key_SPACE | InputMod_None)) sim.paused = !sim.paused;
		if(key_pressed(Key_SPACE | InputMod_Lctrl)) sim.step = 1;


		if(key_pressed(Key_N | InputMod_Lshift)) change_mode(Mode_Navigate);
		if(key_pressed(Key_D | InputMod_Lshift)) change_mode(Mode_Draw);

		switch(sim.mode){
			case Mode_Navigate:{
				
			}break;
			case Mode_Draw:{
				if(key_pressed(Key_1)) sim.drawing.tool = DrawTool_Draw_Square;
				if(key_pressed(Key_2)) sim.drawing.tool = DrawTool_Erase_Square;

				if(hovered) break;

				auto [pos, ok] = get_tile_under_mouse();
				if(!ok || sim.break_on_me) break;

				switch(sim.drawing.tool){
					case DrawTool_Draw_Square:{
						if(input_lmouse_down()){
							Entity* e = make_entity(sim.drawing.entity_type, pos, 0);
							e->color = EntityColors[sim.drawing.entity_type][rand()%7];
							e->name = EntityStrings[sim.drawing.entity_type];
							set_entity(pos.x,pos.y, e);
						}
					}break;
				}

				

			}break;
		}

		// even though ui handles moving the world for us, we still need to handle zooming
		vec2 cursize = ui.background->style.size;
		vec2 local_mouse = input_mouse_position() - ui.worldholder->pos_screen;
		ui.background->style.size.x += DeshInput->scrollY / 10.0 * cursize.x;
		ui.background->style.size.y += DeshInput->scrollY / 10.0 * cursize.y;
		vec2 diff = ui.background->style.size - cursize;
		if(DeshInput->scrollY){
			ui.worldholder->style.pos.x -= local_mouse.x * ((ui.background->style.size.x / cursize.x) - 1);
			ui.worldholder->style.pos.y -= local_mouse.y * ((ui.background->style.size.y / cursize.y) - 1);
		}

		uiImmediateBP(ui.info);{
			if(hovered){
				uiTextM(ToString8(deshi_temp_allocator, "hovered: ", hovered->name));
				if(hovered->type == Entity_Water){
					uiTextM(ToString8(deshi_temp_allocator, "pressure: ", hovered->water.pressure));
				}
			} 
			else  uiTextM(ToString8(deshi_temp_allocator, "hovered: nothing"));
			auto [pos,ok] = get_tile_under_mouse();
			if(ok) uiTextM(ToString8(deshi_temp_allocator, pos));

			if(g_ui->hovered) uiTextM(ToString8(deshi_temp_allocator, "hovered ui: ", g_ui->hovered->id));

			if(sim.paused){ 
				uiItem* pausebox = uiItemB();{
					pausebox->style.anchor = anchor_bottom_right;
					pausebox->style.sizing = size_auto;
					pausebox->style.padding = {2,2,2,2};
					pausebox->style.background_color = color(255*(sin(1.5*DeshTime->totalTime/1000 + cos(1.5*DeshTime->totalTime/1000))+1)/2, 0, 0, 255);
					uiTextML("paused");
				}uiItemE();
			
			}

		}uiImmediateE();
		


		console_update();
		UI::Update();
		ui_update();
		render_update();
		logger_update();
		deshi__memory_temp_clear();
	}
	
	deshi_cleanup();
}
