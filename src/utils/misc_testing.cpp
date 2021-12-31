#include "defines.h"
#include "../core/renderer.h"
#include "../core/window.h"
#include "array.h"
#include "geometry/Edge.h"

struct RandDrawObj {
	vec2 pos;
	vec2 target;
	array<pair<vec2, color>> hist;
};

b32 init = 0;
array<RandDrawObj> rdobjs;
TIMER_START(walk);
void random_draw(u32 count) {
	if (!init) {
		init = 1;
		forI(count) {
			rdobjs.add(RandDrawObj{
			vec2(rand() % (u32)DeshWinSize.x, rand() % (u32)DeshWinSize.y),
			vec2(rand() % (u32)DeshWinSize.x, rand() % (u32)DeshWinSize.y) });
			rdobjs.last->hist.add({ rdobjs.last->pos, randcolor });
		}
	}
	else {
		Render::StartNewTwodCmd(4, 0, vec2::ZERO, DeshWinSize);

		for (RandDrawObj& rdo : rdobjs) {
			vec2& pos = rdo.pos;
			vec2& target = rdo.target;
			array<pair<vec2, color>>& hist = rdo.hist;


			//pos += sinf(rand() % 100) * randvec2(10);

			vec2 tdis = (DeshWinSize / 2 - pos).normalized() * 2;
			//pos += tdis * 2;
			pos += vec2(tdis.y, -sin(tdis.x)) * 0.5;

			
			//pos = Nudge(pos, target, randvec2(10) / 10);
			
			//pos = Clamp(pos, vec2::ZERO, DeshWinSize);



			if (pos == target) {
				target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
				hist.clear();
			}
			else if (hist.count) {
				color nu = hist.last->second;
				//nu.r = (nu.r + rand() % 10) % 255;
				nu.g = Nudge((u32)nu.g, (u32)((u32)target.x % (u32)255), (u32)1);
				nu.b = 144;
				hist.add({ pos, nu });
			}

			if (hist.count) {
				for (int i = 0; i < hist.count - 1; i++) {
					Render::DrawLine2D(hist[i].first, hist[i + 1].first, 2, hist[i].second, 4, vec2::ZERO, DeshWinSize);
				}
			}
		}
		if (TIMER_END(walk) > 5000) { 
			TIMER_RESET(walk); 
			f32 i = 0;
			for (RandDrawObj& rdo : rdobjs) {
				vec2& pos = rdo.pos;
				vec2& target = rdo.target;
				array<pair<vec2, color>>& hist = rdo.hist;

				hist.clear();
				hist.add({ vec2(rand() % (u32)DeshWinSize.x, rand() & (u32)DeshWinSize.y), randcolor });
				
			}
		
		}
	}
}



//line that interacts with itself
b32 rwainit = 0;
RandDrawObj rwa;
TIMER_START(rwawalk);
TIMER_START(histreset);
void random_walk_avoid() {
	if (!rwainit) {
		srand(time(0));
		rwainit = 1;
		rwa.pos = vec2(rand() % (u32)DeshWinSize.x, rand() % (u32)DeshWinSize.y);
		rwa.target = vec2(rand() % (u32)DeshWinSize.x, rand() % (u32)DeshWinSize.y);
		rwa.hist.add({ rwa.pos, randcolor });
	}
	else {
		Render::StartNewTwodCmd(4, 0, vec2::ZERO, DeshWinSize);

		vec2& pos = rwa.pos;
		vec2& target = rwa.target;
		array<pair<vec2, color>>& hist = rwa.hist;

		vec2 tdis = (target - pos).normalized();
		vec2 step = tdis + vec2(-tdis.y, tdis.x);

		step *= 4;

		for (int i = 0; i < (s32)hist.count - 200; i++) {
			vec2 histpos = hist[i].first;
			if ((histpos - pos).mag() < 100 && step.dot(histpos - pos) < 0) {
				vec2 left = hist[Max(i - 1, 0)].first;
				vec2 right = hist[i + 1].first;
				Edge edge(left, right);
				f32 m = 1;
				while (edge.edge_intersect(Edge(pos, pos + step)) || step.dot(histpos - (pos + step)) < -0.1){
					step = Math::vec2RotateByAngle(m, step);
					m += 1;
					if (m > 270) {
						break;
					}
				}
			}
		}

		pos += step;

		//pos = Clamp(pos, vec2::ZERO, DeshWinSize);

		if (pos == target) {
			target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
			hist.clear();
		}
		else {
			color nu = hist.last->second;
			//nu.r = (nu.r + rand() % 10) % 255;
			nu.g = Nudge((u32)nu.g, (u32)((u32)target.x % (u32)255), (u32)1);
			nu.b = 144;
			hist.add({ pos, nu });
		}

		if (TIMER_END(rwawalk) > 100) {
			TIMER_RESET(rwawalk);
			target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
			
		}

		if (TIMER_END(histreset) > 5000) {
			TIMER_RESET(histreset);
			color last = hist.last->second;
			hist.clear();
			hist.add({ rwa.pos, last });
		}

		if (hist.count) {
			for (int i = 0; i < hist.count - 1; i++) {
				Render::DrawLine2D(hist[i].first, hist[i + 1].first, 2, hist[i].second, 4, vec2::ZERO, DeshWinSize);
			}
		}
		
	}
}

void vector_field() {
	f32 precision = 100;
	static f32 zoom = 20;
	if (DeshInput->ScrollDown()) zoom++;
	if (DeshInput->ScrollUp()) zoom--;


	static vec2 postrack(0, 0);
	static array<vec2> hist;
	static vec2 mpl(0,0);
	static TIMER_START(vft);


	vec2 mp = DeshInput->mousePos;

	static b32 ringed = 0;
	static u32 idx = 0;
	static b32 tracking = 1;
	if (DeshInput->LMouseDown() && mpl != DeshInput->mousePos) {
		postrack = DeshInput->mousePos;
		mpl = DeshInput->mousePos;
		ringed = 0;
		idx = 0;
		hist.clear();
	}

	f32 time = DeshTotalTime;

	static vec2 target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
	
	auto step = [&](vec2 pos) {
		vec2 tc = (DeshWinSize / 2 - pos).normalized() / zoom;
		f32 x = tc.x, y = tc.y;
		//vec2 tm = mp - pos;// - ;
		return vec2(y, -x);
	};

	if (TIMER_END(vft) > 1000) {
		TIMER_RESET(vft);
		target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
	}

	static f32 maxmag = 0;

	Render::StartNewTwodCmd(4, 0, vec2::ZERO, DeshWinSize);
	for (u32 i = 0; i < precision; i++) {

		for (u32 o = 0; o < precision; o++) {
			vec2 pos(i / precision * DeshWinSize.x+precision/DeshWinSize.x , o / precision * DeshWinSize.y+precision/ DeshWinSize.y);
			vec2 s = step(pos);
			vec2 pos2 = pos + s.normalized() * 10;

			f32 ang = Math::AngBetweenVectors(pos2 - pos, vec2(0, 1));
			maxmag = Max(maxmag, s.mag());

			//Render::DrawCircle2D(pos, 4, 10, Color_Red, 4, vec2::ZERO, DeshWinSize);
			Render::DrawLine2D(pos, pos2, 2, color(25,  Clamp(255 * s.mag() / maxmag, 0.f, 255.f), 125), 4, vec2::ZERO, DeshWinSize);
			
		
		}
	}

	

	if (tracking) {
		if (Math::PointInRectangle(postrack, vec2::ZERO, DeshWinSize)) {
			postrack += step(postrack);
			hist.add(postrack);
		}
		for (int i = 0; i < hist.count - 1; i++) {
			Render::DrawLine2D(hist[i], hist[i + 1], 2, Color_Red, 4, vec2::ZERO, DeshWinSize);
		}
	}


}