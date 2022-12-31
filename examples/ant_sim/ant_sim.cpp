/* ant_sim deshi example

Index:
@vars
@entity
@need/cost
@action
@agent
@advert
@world
@simulation
@main
*/
#include "deshi.h"
#include "glad/gl.h"
#include "glad/wgl.h"
struct Advert;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@entity
enum{
	Entity_NULL = 0,
	Entity_Agent,
	Entity_Leaf,
	Entity_Dirt,
	Entity_COUNT
};

typedef struct Entity{
	str8 name;
	u64 age;
	vec2i pos;
	Type type;
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
	Need_COUNT
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
};//StaticAssert(ArrayCount(NeedStrings) == Need_COUNT);

typedef struct Need{
	Type type;
	f32 value;
	f32 delta;
	f32 weight; //weight applied when scoring
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
	Action_COUNT
};
str8 ActionStrings[] = {
	STR8("Walk"),
	STR8("Dig"),
};StaticAssert(ArrayCount(ActionStrings) == Action_COUNT);

enum{
	ActionFlags_None = 0,
	ActionFlags_ConsumeActionOnCompletion = (1 << 0),
	ActionFlags_ConsumeOwnerOnCompletion  = (1 << 1),
};

typedef struct ActionDef{
	Type type;
	Flags flags;
	u32 time;
	u32 costs_count;
	Cost* costs_array;
}ActionDef;

typedef struct Action{
	ActionDef* def;
	Entity* owner;
}Action;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@agent
#define AGENT_ADVERT_QUEUE_SIZE 4

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
	Advert* advert_queue[AGENT_ADVERT_QUEUE_SIZE];
	Need* needs_array;
	u32 needs_count;
	u32 padding;
}Agent;
#define AgentFromNode(ptr) CastFromMember(Agent,node,ptr)
#define AgentFromEntity(ptr) CastFromMember(Agent,entity,ptr)


///////////////////////////////////////////////////////////////////////////////////////////////////
//@advert
enum{
	AdvertFlags_None = 0,
	AdvertFlags_ConsumeAdvertOnCompletion = (1 << 0),
	AdvertFlags_ConsumeOwnerOnCompletion  = (1 << 1),
};

typedef struct AdvertDef{
	str8 name;
	Flags flags;
	u32 rangeSq;
	u32 time; //advertised time
	u32 padding;
	Cost* costs_array; //advertised costs
	u32 costs_count;
	u32 actions_count;
	ActionDef* actions_array;
}AdvertDef;

typedef struct Advert{
	AdvertDef* def;
	Entity* owner;
}Advert;

//scales a score value based on the need increase's distance from zero (so the change from 70-80 has less value than 10-30)
//NOTE usually returns a value between -1 and 1
//NOTE the "+ .001f" is just to avoid division by zero
f32 need_attenuation(f32 current, f32 future, f32 weight){
	return weight * ((10.f / (current + .001f)) - (10.f / (future + .001f)));
}

//scales a score value with distance: 100 * score / distance^2
f32 distance_attenuation(f32 score, vec2i pos1, vec2i pos2){
	f32 x = pos2.x - pos1.x;
	f32 y = pos2.y - pos1.y;
	f32 d = x*x + y*y;
	return score / ((d > 1.f) ? d : 1.f);
}

//computes the completion time of an advert based on a given value
f32 compute_completion_time(AdvertDef* advert, f32 weighed_value){
	return (MAX_NEED_VALUE / weighed_value) + advert->time;
}


//score the advert based on its positive/negative costs, completion time, distance, ...
//TODO factor in whether something will become more important due to time (mining for a whole day instead of eating) see Fero::TimeAspect
//TODO weight negative costs heavier
f32 score_advert(Agent* agent, Advert* advert){
	f32 average = 0;
	forX(cost_idx, advert->def->costs_count){ Cost* cost = advert->def->costs_array + cost_idx;
		forX(need_idx, agent->needs_count){ Need* need = agent->needs_array + need_idx;
			if(cost->need == need->type){
				f32 current_score = need->value;
				f32 future_score  = current_score + cost->delta;
				if(future_score > MAX_NEED_VALUE) future_score = MAX_NEED_VALUE;
				
				if(cost->delta > 0){
					average += need_attenuation(current_score, future_score, need->weight);
				}else{
					average += need_attenuation(current_score, future_score, need->weight);
				}
				break;
			}
		}
	}
	
	if(advert->def->costs_count){
		average /= advert->def->costs_count;
	}
	
	return distance_attenuation(average, agent->entity.pos, advert->owner->pos) / advert->def->time;
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
		forI(score_count) Log("ant_sim", best_ads[i]->def->name," ",best_scores[i]);
		Assert(!"An advert should always be chosen if there is a positive one.");
		return 0;
	}
#undef MAX_BEST_ADS
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//@world
#define WORLD_WIDTH 128
#define WORLD_HEIGHT 128

struct{
	Entity** map;
}world;
#define GetEntity(x,y) world.map[x+y*WORLD_WIDTH]

b32 move_entity(Entity* e, vec2i pos){
	if(GetEntity(pos.x,pos.y)) return false;
	GetEntity(e->pos.x,e->pos.y) = 0;
	GetEntity(pos.x,pos.y) = e;
	e->pos = pos;
	return true;
}

Entity* get_entity_under_mouse(){
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//@simulation
u64 ticks;
b32 paused;
Arena* action_def_arena;
Arena* advert_def_arena;
Heap* agents_heap;
Node agents_node;
Advert* adverts_pool;
Entity* entities_pool;

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
	forI(AGENT_ADVERT_QUEUE_SIZE) agent->advert_queue[i] = 0;
	agent->needs_array  = (Need*)(agent+1);
	agent->needs_count  = arg_count;
	Need needs[arg_count] = {args...};
	CopyMemory(agent->needs_array, needs, arg_count*sizeof(Need));
	counts.agents++;
	return agent;
}

Advert* make_advert_def(str8 name, Flags flags, u32 range, u32 time, int cost_count, int action_count, ...){
	NotImplemented;
	counts.adverts++;
	return 0;
}

template<typename... Args>
ActionDef* make_action_def(Type type, Flags flags, u32 time, Args... args){
	constexpr u32 arg_count = sizeof...(Args);
	ActionDef* def = memory_arena_pushT(action_def_arena, ActionDef);
	def->type = type;
	def->flags = flags;
	def->time = time;
	def->costs_count = arg_count;
	Cost costs[arg_count] = {args...};
	CopyMemory(def->costs_array, costs, arg_count*sizeof(Cost));
	counts.actions++;
	return def;	
}

Entity* make_nonagent_entity(vec2i pos, u32 age, Type type){
	Entity* entity = memory_pool_push(entities_pool);
	entity->name = str8{0};
	entity->age  = age;
	entity->pos  = pos;
	entity->type = type;
	counts.entities++;
	counts.entity[type]++;
	return entity;
}

array<Advert*> collect_adverts(Agent* agent){
	array<Advert*> adverts(deshi_temp_allocator);
	for_pool(adverts_pool){
		if(!it->def) continue;
		if(vec2i_distanceToSq(agent->entity.pos,it->owner->pos) <= it->def->rangeSq){
			adverts.add(it);
		}
	}
	return adverts;
}

void tick_agent_needs(Agent* agent){
	for(Need* it = agent->needs_array; it < agent->needs_array+agent->needs_count; ++it){
		it->value += it->delta;
		it->value = Clamp(it->value, MIN_NEED_VALUE, MAX_NEED_VALUE);
	}
}

void tick_agent_adverts(Agent* agent){

}

void tick_agent(Agent* agent){
	//delta needs
	
	
	//advance the advert actions
	b32 new_advert = false;
	
	//select an advert to add to the queue
	if(new_advert){
	}
	
	//action perform
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//@render

struct{
	u32* screen;
	Texture* texture; // texture representing the world
	vec2i visual_size;
	vec2i visual_position;
}rendering;
#define GetPixel(x,y) rendering.screen[x+(WORLD_HEIGHT-y*WORLD_WIDTH)]

///////////////////////////////////////////////////////////////////////////////////////////////////
//@main
int main(int args_count, char** args){
	deshi_init();
	
	srand(1235123);

	//init deshi UI2
	g_ui->base.style.font        = assets_font_create_from_file_bdf(STR8("gohufont-11.bdf"));
	g_ui->base.style.font_height = 11;
	g_ui->base.style.text_color  = Color_White;


	//init ant_sim storage
	action_def_arena = memory_create_arena(Megabytes(1));
	advert_def_arena = memory_create_arena(Megabytes(1));
	agents_heap = memory_heap_init_bytes(Megabytes(1));
	memory_pool_init(adverts_pool, 1024);
	memory_pool_init(entities_pool, 1024);
	world.map = (Entity**)memalloc(sizeof(Entity*) * WORLD_WIDTH * WORLD_HEIGHT);

	//init ant_sim rendering
	rendering.screen = (u32*)memalloc(sizeof(u32)*WORLD_WIDTH*WORLD_HEIGHT);
	rendering.texture = assets_texture_create_from_memory(
		rendering.screen, 
		STR8("world_screen"), 
		WORLD_WIDTH, WORLD_HEIGHT,
		ImageFormat_RGBA,
		TextureType_2D,
		TextureFilter_Nearest,
		TextureAddressMode_ClampToTransparent,
		0
	);

	uiItem* main = uiItemB();{
		main->id = STR8("ant_sim.main");
		main->style.background_color = {20,20,20,255};
		main->style.sizing = size_percent;
		main->style.size = {100,100};
		main->style.display = display_flex;
		uiItem* worldwin = uiItemB();{
			worldwin->id = STR8("ant_sim.main.worldwin");
			worldwin->style.sizing = size_percent_y;
			worldwin->style.size = {512, 100};
			worldwin->style.background_color = {5,5,5,255};
			uiItem* worldtex = uiItemB();{
				worldtex->id = STR8("ant_sim.main.worldtex");
				worldtex->style.background_image = rendering.texture;
				worldtex->style.background_color = {255,255,255,255};
				worldtex->style.sizing = size_percent_x | size_square | size_auto_y;
				worldtex->style.size = {100,0};
				worldtex->style.positioning = pos_draggable_fixed;
				worldtex->style.border_width = 1;
			}uiItemE();
		}uiItemE();
		uiItem* infowin = uiItemB();{
			infowin->id = STR8("ant_sim.main.infowin");
			infowin->style.sizing = size_flex | size_percent_y;
			infowin->style.size = {1, 100};
			uiTextML("test");
		}uiItemE();
	}uiItemE();



	// initialize world by spawning all items in mid air so they fall down and it looks cool

	while(platform_update()){
		//simulate
		if(!paused){
			//tick agents
			for_node(agents_node.next){
				tick_agent(AgentFromNode(it));
			}

			//spawn more leaves
			if(counts.entity[Entity_Leaf] < 50){
				u32 add = (50 - counts.entity[Entity_Leaf]) +  rand() % 10; 
				forI(add){
					vec2i pos = {rand() % WORLD_WIDTH, WORLD_HEIGHT};
					while(GetEntity(pos.x,pos.y) && pos.y) pos.y -= 1;
					if(!pos.y){ add--; continue; } // we somehow failed to place the leaf anywhere in the random column, whatever
					Entity* e = make_nonagent_entity(pos, 0, Entity_Leaf);
					GetEntity(pos.x,pos.y) = e;
				}
			}

			for_pool(entities_pool){
				switch(it->type){
					case Entity_Leaf:{
						if(it->age % (rand() % 50 + 1)) break;
						GetPixel(it->pos.x,it->pos.y) = PackColorU32(0,0,0,0);
						vec2i nupos = it->pos;
						nupos.y--;
						u32 r = rand() % 3; 
						if(r == 1) nupos.x += 1;
						else if(r == 2) nupos.x -= 1;
						GetPixel(it->pos.x,it->pos.y) = PackColorU32(255,0,255,0);
					}break;
					case Entity_Dirt:{
						if(!it->pos.y || GetEntity(it->pos.x,it->pos.y-1)) break;
						GetPixel(it->pos.x,it->pos.y) = 0;
						move_entity(it, vec2i{it->pos.x,it->pos.y-1});
						GetPixel(it->pos.x,it->pos.y) = 0xff13458b;
					}break;
				}
				it->age++;
			}
		}
		
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

		// for_pool(entities_pool){
		// 	switch(it->type){
		// 		case Entity_Agent:{
		// 			Agent* agent = CastFromMember(Agent, entity, it);
		// 			switch(agent->race){
		// 				case Race_BlackGardenAntQueen:
		// 				case Race_BlackGardenAntMale:
		// 				case Race_BlackGardenAntWorker: {
		// 					GetPixel(it->pos.x,it->pos.y) = PackColorU32(15,15,15,255);
		// 				}break;
		// 				case Race_CottonAntQueen: 
		// 				case Race_CottonAntMajorWorker:
		// 				case Race_CottonAntMinorWorker:
		// 				case Race_CottonAntMale:{
		// 					GetPixel(it->pos.x,it->pos.y) = PackColorU32(100,15,15,255);
		// 				}break;
		// 			}
		// 		}break;
		// 	}
		// }
		
		// forI(WORLD_WIDTH*WORLD_HEIGHT) {
		// 	rendering.screen[i] = PackColorU32((u32)floor(255.0*i/(WORLD_WIDTH*WORLD_HEIGHT)),50,50,255);
		// }
		ui_debug();


		render_update_texture(rendering.texture, vec2i{0,0}, vec2i{WORLD_WIDTH,WORLD_HEIGHT});
		//render_texture_flat2(rendering.texture, vec2{0,0}, Vec2(DeshWindow->height,DeshWindow->height), 1);
		
		counts.entity[Entity_Leaf]--;

		console_update();
		UI::Update();
		ui_update();
		render_update();
		logger_update();
		deshi__memory_temp_clear();
	}

	
	
	
	deshi_cleanup();
}
