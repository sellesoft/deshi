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
	Entity_Water,
	Entity_COUNT
};

// color palettes for entities that randomly choose color
u32 entity_colors[Entity_COUNT][7] = {
	0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff,
	0,          0,          0,          0,          0,          0,          0         ,
	0xff709a88, 0xff7ba694, 0xff86b19f, 0xff91bdab, 0xff9cc9b7, 0xffa8d5c3, 0xffb4e1cf,
	0xff3d5f82, 0xff45678a, 0xff4c6e93, 0xff53769b, 0xff5a7ea3, 0xff6286ac, 0xff698eb4,
	0xff595d47, 0xff60644d, 0xff666a54, 0xff6d715a, 0xff747861, 0xff7a7e67, 0xff81856e,
};	

u32 divide_color(u32 color, u32 divisor){
	u32 r = (color >>  0 & 0x000000ff) / divisor;
	u32 g = (color >>  8 & 0x000000ff) / divisor;
	u32 b = (color >> 16 & 0x000000ff) / divisor;
	return PackColorU32(255,b,g,r);
}

typedef struct Entity{
	Node overlap_node; // connection to other entities occupying the same world tile
	str8 name;
	u64 age;
	vec2i pos;
	Type type;
	u32 color;
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

b32 set_pixel(u32 x,u32 y,u32 val);
b32 set_entity(u32 x, u32 y, Entity* entity){
	if(x < 0 || y < 0 || x > WORLD_WIDTH || y > WORLD_HEIGHT) return 0;
	world.map[x+y*WORLD_WIDTH] = entity;
	if(entity) set_pixel(x,y,entity->color);
	else set_pixel(x,y,0);
	return 1;
}

b32 move_entity(Entity* e, vec2i pos){
	if(get_entity(pos.x,pos.y)) return false;
	set_entity(e->pos.x,e->pos.y,0);
	set_entity(pos.x,pos.y,e);
	e->pos = pos;
	return true;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//@simulation

Arena* action_def_arena;
Arena* advert_def_arena;
Heap* agents_heap;
Node agents_node;
Advert* adverts_pool;
Entity* entities_pool;

struct{
	Type mode;
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

u32 get_pixel(u32 x, u32 y) {	
	Assert(x >= 0 && y >= 0 && x <= WORLD_WIDTH && y <= WORLD_HEIGHT);
	return rendering.screen[x+y*WORLD_WIDTH];
}

b32 set_pixel(u32 x, u32 y, u32 val) {
	if(x < 0 || y < 0 || x > WORLD_WIDTH || y > WORLD_HEIGHT) return 0;
	rendering.screen[x+y*WORLD_WIDTH] = val;
	return 1;
}

struct{
	uiItem* main;
	uiItem* worldwin;
	uiItem* worldholder;
	uiItem* worldtex;
	uiItem* info;
}ui;

// returns a boolean indicating if the mouse is actually over the world 
pair<vec2i, b32> get_tile_under_mouse(){
	vec2 mouse_pos = input_mouse_position();
	if(mouse_pos.x > ui.worldtex->pos_screen.x + ui.worldtex->size.x || mouse_pos.x < ui.worldtex->pos_screen.x ||
	   mouse_pos.y > ui.worldtex->pos_screen.y + ui.worldtex->size.y || mouse_pos.y < ui.worldtex->pos_screen.y ||
	   !ui.worldtex->height || !ui.worldtex->width){
		return {{0},0};
	}
	vec2i local_pos;
	local_pos.x = mouse_pos.x - ui.worldtex->pos_screen.x;
	local_pos.y = mouse_pos.y - ui.worldtex->pos_screen.y;
	local_pos.x = floor(f32(local_pos.x) / f32(ui.worldtex->width) * WORLD_WIDTH);
	local_pos.y = floor(f32(local_pos.y) / f32(ui.worldtex->height) * WORLD_HEIGHT);
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
				u32 col = get_pixel(above->pos.x, above->pos.y);
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//@main
int main(int args_count, char** args){
	deshi_init();
	
	srand(13535153135);

	//init deshi UI2
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
				ui.worldtex = uiItemB();{
					ui.worldtex->id = STR8("ant_sim.main.worldtex");
					ui.worldtex->style.background_image = rendering.texture;
					ui.worldtex->style.background_color = {255,255,255,255};
					ui.worldtex->style.size = {100,100};
					ui.worldtex->style.hover_passthrough = 1;
				}uiItemE();
			}uiItemE();
		}uiItemE();
		ui.info = uiItemB();{
			ui.info->id = STR8("ant_sim.main.infowin");
			ui.info->style.sizing = size_percent_x | size_percent_y;
			ui.info->style.size = {40, 100};
			ui.info->style.padding = {5,5,5,5};
			uiItem* break_button = uiItemB();{
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

	world.weather.type = Weather_Rain;
	world.weather.wind_strength = 0;

	// TODO(sushi) initialize world by spawning all items in mid air so they fall down and it looks cool

	s32 vel = 0;
	s32 pos = WORLD_HEIGHT / 2;

	forI(WORLD_WIDTH){
		if(i%WORLD_WIDTH/(rand() % 8 + 8)){
			u32 mag = 0;
			if     (rand()%32==0) { mag = 8; }
			else if(rand()%16==0) { mag = 6; }
			else if(rand()% 8==0) { mag = 4; }
			else                    mag = 2;

			if(pos < u32(WORLD_HEIGHT/6.f)) vel = rand() % mag+1;
			else if(pos > u32(5.f*WORLD_HEIGHT/6.f)) vel = -(rand() % mag + 1);
			else vel = rand() % (mag+1) - mag/2;
		}
		pos += vel;
		pos = Clamp(pos,0, WORLD_HEIGHT-1);

		u32 color = entity_colors[Entity_Dirt][rand()%7];
		forX(j,pos){
			if(rand()%2==0) color = entity_colors[Entity_Dirt][rand()%7];
			Entity* e = make_nonagent_entity({i,j}, 0, Entity_Dirt);
			e->color = divide_color(color, 2);
			set_entity(i,j,e);
			e->name = STR8("dirt");
		}
	}

	while(platform_update()){
		//simulate
		if(!sim.paused || sim.step){
			sim.step = 0;

			//tick agents
			for_node(agents_node.next){
				tick_agent(AgentFromNode(it));
			}

			for_pool(entities_pool){
				if(it == sim.break_on_me) sim.break_on_me = 0, DebugBreakpoint;
				switch(it->type){
					case Entity_Leaf: eval_leaf(it); break;
					case Entity_Dirt:{
						if(!it->pos.y || get_entity(it->pos.x,it->pos.y-1)) break;
						move_entity(it, vec2i{it->pos.x,it->pos.y-1});
					}break;
					case Entity_Water:{
						if(!it->pos.y || get_entity(it->pos.x,it->pos.y-1)){

							if(it->pos.x == 0 && !get_entity(it->pos.x+1,it->pos.y-1)){
								move_entity(it,{it->pos.x+1,it->pos.y-1});
							}else if(it->pos.x == WORLD_WIDTH-1 && !get_entity(it->pos.x-1,it->pos.y-1)){
								move_entity(it,{it->pos.x-1,it->pos.y-1});
							}else{
								Entity* bl = get_entity(it->pos.x-1,it->pos.y-1);
								Entity* br = get_entity(it->pos.x+1,it->pos.y-1);
								if(br&&bl){
									Entity* l = get_entity(it->pos.x-1,it->pos.y);
									Entity* r = get_entity(it->pos.x+1,it->pos.y);
									move_entity(it,{it->pos.x+(rand()%2?1:-1),it->pos.y});
									if(!l&&!r) move_entity(it,{it->pos.x+(rand()%2?1:-1),it->pos.y});
									else if(l) move_entity(it,{it->pos.x+1,it->pos.y});
									else if(r) move_entity(it,{it->pos.x-1,it->pos.y});
								} 
								else if(!bl&&!br) move_entity(it,{it->pos.x+(rand()%2?1:-1),it->pos.y-1});
								else if(bl) move_entity(it,{it->pos.x+1,it->pos.y-1});
								else if(br) move_entity(it,{it->pos.x-1,it->pos.y-1});
							}
						}else{
							vec2i nupos = it->pos;
							nupos.y--;
								//TODO(sushi) this needs to check along the line of movement, not just across x
							forI(abs(world.weather.wind_strength)/2){
								u32 move = (world.weather.wind_strength>0?1:-1);
								if(get_entity(nupos.x+move,it->pos.y-1)) break;
								nupos.x += move;
							}
							//nupos.x += world.weather.wind_strength / 2;
							nupos.x = Clamp(nupos.x, 0, WORLD_WIDTH);
							nupos.y = Clamp(nupos.y, 0, WORLD_HEIGHT-1);
							move_entity(it, nupos);
						}
					}break;
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
					Entity* e = make_nonagent_entity(pos, 0, Entity_Leaf);
					e->name = STR8("leaf");
					e->color = entity_colors[Entity_Leaf][rand()%7];
					set_entity(pos.x,pos.y,e);
				}
			}

			// weather
			switch(world.weather.type){
				case Weather_Rain:{
					forI(rand() % 10){
						vec2i pos = {rand() % WORLD_WIDTH, WORLD_HEIGHT-1};
						Entity* e = make_nonagent_entity(pos, 0, Entity_Water);
						e->name = STR8("water");
						e->color = entity_colors[Entity_Water][rand()%7];
						set_entity(pos.x,pos.y,e);
					}
				}break;
			}
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

		render_update_texture(rendering.texture, vec2i{0,0}, vec2i{WORLD_WIDTH,WORLD_HEIGHT});

		if(key_pressed(Key_SPACE | InputMod_None)) sim.paused = !sim.paused;
		if(key_pressed(Key_SPACE | InputMod_Lctrl)) sim.step = 1;


		if(key_pressed(Key_N | InputMod_Lshift)) change_mode(Mode_Navigate);
		if(key_pressed(Key_D | InputMod_Lshift)) change_mode(Mode_Draw);

		switch(sim.mode){
			case Mode_Navigate:{
				// even though ui handles moving the world for us, we still need to handle zooming
				vec2 cursize = ui.worldtex->style.size;
				vec2 local_mouse = input_mouse_position() - ui.worldholder->pos_screen;
				ui.worldtex->style.size.x += DeshInput->scrollY / 10.0 * cursize.x;
				ui.worldtex->style.size.y += DeshInput->scrollY / 10.0 * cursize.y;
				vec2 diff = ui.worldtex->style.size - cursize;
				if(DeshInput->scrollY){
					ui.worldholder->style.pos.x -= local_mouse.x * ((ui.worldtex->style.size.x / cursize.x) - 1);
					ui.worldholder->style.pos.y -= local_mouse.y * ((ui.worldtex->style.size.y / cursize.y) - 1);
				}

				

				//break on clicked entity	
				// /* if(hovered && input_lmouse_pressed()){
				// 	sim.break_on_me = hovered;
				// } */
				
			}break;
			case Mode_Draw:{
				if(auto [pos, ok] = get_tile_under_mouse(); ok && input_lmouse_down()){
					Entity* e = make_nonagent_entity(pos, 0, Entity_Leaf);
					e->color = entity_colors[Entity_Leaf][rand()%7];
					e->name = STR8("leaf");
					set_entity(pos.x,pos.y, e);
				}
			}break;
		}

		
		uiImmediateBP(ui.info);{
			if(hovered) uiTextM(ToString8(deshi_temp_allocator, "hovered: ", hovered->name));
			else  uiTextM(ToString8(deshi_temp_allocator, "hovered: nothing"));
			auto [pos,ok] = get_tile_under_mouse();
			if(ok) uiTextM(ToString8(deshi_temp_allocator, pos));
			
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
