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

			//pos = Nudge(pos, target, randvec2(10));

			//pos += sinf(rand() % 100) * randvec2(10);

			vec2 tdis = (target - pos).normalized();
			pos += tdis * 2;
			pos += vec2(-tdis.y, tdis.x);
			pos = Clamp(pos, vec2::ZERO, DeshWinSize);

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

			if (TIMER_END(walk) > 1000) {
				target = vec2(rand() % DeshWindow->width, rand() % DeshWindow->height);
			}

			if (hist.count) {
				for (int i = 0; i < hist.count - 1; i++) {
					Render::DrawLine2D(hist[i].first, hist[i + 1].first, 2, hist[i].second, 4, vec2::ZERO, DeshWinSize);
				}
			}
		}
		if (TIMER_END(walk) > 1000) TIMER_RESET(walk);
	}
}

//line that interacts with itseld..


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

		step *= 3;

		for (int i = 0; i < (s32)hist.count - 200; i++) {
			vec2 histpos = hist[i].first;
			if ((histpos - pos).mag() < 10 && step.dot(histpos - pos) < 0) {
				vec2 left = hist[Max(i - 10, 0)].first;
				vec2 right = hist[i + 10].first;
				Edge edge(left, right);
				f32 m = 1;
				while (edge.edge_intersect(Edge(pos, pos + step)) || step.dot(histpos - (pos + step)) < -0.5){
					step = Math::vec2RotateByAngle(m, step);
					m += 1;
					if (m > 180) {
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