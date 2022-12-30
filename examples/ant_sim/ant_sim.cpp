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
struct ActionDef;
struct AdvertDef;
struct Advert;
struct Entity;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@vars
u64 ticks;
b32 paused;
Arena* action_def_arena;
Arena* advert_def_arena;
Heap* agents_heap;
Advert* adverts_pool;
Entity* entities_pool;


///////////////////////////////////////////////////////////////////////////////////////////////////
//@entity
typedef struct Entity{
	str8 name;
	u64 age;
	vec2i pos;
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
};StaticAssert(ArrayCount(NeedStrings) == Need_COUNT);

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
	Entity entity;
	Type race;
	u32 action_index;
	Advert* advert_queue[AGENT_ADVERT_QUEUE_SIZE];
	Need* needs_array;
	u32 needs_count;
	u32 padding;
	Node node;
}Agent;


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



///////////////////////////////////////////////////////////////////////////////////////////////////
//@simulation
Agent* make_agent(Type race, u32 age, vec2i pos, u32 need_count, ...){
	Agent* agent = (Agent*)memory_heap_add_bytes(agents_heap, sizeof(Agent) + need_count*sizeof(Need));
	agent->entity.name = str8{0};
	agent->entity.age  = age;
	agent->entity.pos  = pos;
	agent->race         = race;
	agent->action_index = -1;
	forI(AGENT_ADVERT_QUEUE_SIZE) agent->advert_queue[i] = 0;
	agent->needs_array  = (Need*)(agent+1);
	agent->needs_count  = need_count;
	va_list args;
	va_start(args, need_count);
	forI(need_count) agent->needs_array[i] = Need{va_arg(args,Type), MAX_NEED_VALUE, va_arg(args,f32), 1.0f};
	va_end(args);
	return agent;
}

Advert* make_advert_def(str8 name, Flags flags, u32 range, u32 time, int cost_count, int action_count, ...){
	NotImplemented;
	return 0;
}

Entity* make_nonagent_entity(vec2i pos, u32 age){
	Entity* entity = memory_pool_push(entities_pool);
	entity->name = str8{0};
	entity->age  = age;
	entity->pos  = pos;
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

void tick_agent(Agent* agent){
	agent->entity.age += 1;
	
	//delta needs
	for(Need* it = agent->needs_array; it < agent->needs_array+agent->needs_count; ++it){
		it->value += it->delta;
		it->value = Clamp(it->value, MIN_NEED_VALUE, MAX_NEED_VALUE);
	}
	
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
	u32 screen[1080*1080];
	GLuint screen_idx;
	u32 vao; // handle to vtx array object
	u32 vbo; // handle to vtx buffer object
	u32 ibo; // handle to idx buffer object
	Vertex2* vtxarr; // screen vtx array
	u32* idxarr;
}rendering;
#define GetPixel(x,y) rendering.screen->pixels[x+y*1080]


void flush_screen(){
	//glTexSubImage2D(GL_TEXTURE_2D, );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//@main
int main(int args_count, char** args){
	deshi_init();
	
	//init deshi UI2
	g_ui->base.style.font        = assets_font_create_from_file_bdf(STR8("gohufont-11.bdf"));
	g_ui->base.style.font_height = 11;
	g_ui->base.style.text_color  = Color_White;
	
	
	uiItem* item = uiItemM();
	item->style.size = {20,20};
	item->style.background_color = {50,75,100,255};
	
	{// init screen texture
		// https://stackoverflow.com/questions/24262264/drawing-a-2d-texture-in-opengl
		glGenVertexArrays(1, &rendering.vao);
		glBindVertexArray(rendering.vao);
		
		u32 vtxsize = sizeof(Vertex2)*4;
		u32 idxsize = sizeof(u32)*6;
		
		rendering.vtxarr = (Vertex2*)memalloc(sizeof(Vertex2)*4);
		rendering.vtxarr[0] = {   0,   0}; rendering.vtxarr[0].color = PackColorU32(50,75,100,255);
		rendering.vtxarr[1] = {   0,1080}; rendering.vtxarr[1].color = PackColorU32(50,75,100,255);
		rendering.vtxarr[2] = {1080,   0}; rendering.vtxarr[2].color = PackColorU32(50,75,100,255);
		rendering.vtxarr[3] = {1080,1080}; rendering.vtxarr[3].color = PackColorU32(50,75,100,255);
		
		rendering.idxarr = (u32*)memalloc(sizeof(u32) * 6);
		rendering.idxarr[0] = 0;
		rendering.idxarr[1] = 2;
		rendering.idxarr[2] = 1;
		rendering.idxarr[3] = 2;
		rendering.idxarr[4] = 3;
		rendering.idxarr[5] = 1;
		
		// generate and bind vertex buffers
		glGenBuffers(1, &rendering.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, rendering.vbo);
		glBufferData(GL_ARRAY_BUFFER, vtxsize, 0, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, rendering.vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, rendering.vbo, rendering.vtxarr);
		
		// generate and bind index buffers
		glGenBuffers(1, &rendering.ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendering.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxsize, 0, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendering.ibo);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, rendering.ibo, rendering.idxarr);
		
		// copy quad to buffers
		glBindBuffer(GL_ARRAY_BUFFER,               rendering.vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,       rendering.ibo);
		glBufferSubData(GL_ARRAY_BUFFER,         0, rendering.vbo, rendering.vtxarr);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, rendering.ibo, rendering.idxarr);
		
		// specify vertex packing
		glVertexAttribPointer(0, 3,   GL_FLOAT,         GL_FALSE, sizeof(Vertex2), (void*)offsetof(Vertex2,pos));
		glVertexAttribPointer(1, 2,   GL_FLOAT,         GL_FALSE, sizeof(Vertex2), (void*)offsetof(Vertex2,uv));
		glVertexAttribPointer(2, 4,   GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Vertex2), (void*)offsetof(Vertex2,color));
		glEnableVertexAttribArray(0); glEnableVertexAttribArray(1); glEnableVertexAttribArray(2);
		
		glGenTextures(1, &rendering.screen_idx);
		glBindTexture(GL_TEXTURE_2D, rendering.screen_idx);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1080, 1080, 0, GL_RGBA8UI, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	//init ant_sim storage
	action_def_arena = memory_create_arena(Megabytes(1));
	advert_def_arena = memory_create_arena(Megabytes(1));
	agents_heap = memory_heap_init_bytes(Megabytes(1));
	memory_pool_init(adverts_pool, 1024);
	memory_pool_init(entities_pool, 1024);
	
	deshi_loop_start();{
		//simulate
		if(!paused){
			
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
	}deshi_loop_end();
	
	
	
	deshi_cleanup();
}
