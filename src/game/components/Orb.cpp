#include "Orb.h"
#include "MeshComp.h"
#include "../admin.h"
#include "../../core/renderer.h"
#include "../../core/time.h"
#include "../../core/storage.h"
#include "../../core/console.h"
#include "../../utils/debug.h"
#include "../../math/math.h"

OrbManager::OrbManager(){
	layer = ComponentLayer_Physics;
	type = ComponentType_OrbManager;
	
	model = Storage::NullModel();
	orbcount = 100;
};

OrbManager::OrbManager(Model* m, int _orbcount){
	layer = ComponentLayer_Physics;
	type = ComponentType_OrbManager;
	
	model = m;
	orbcount = _orbcount;
};

void OrbManager::Init(){
	//create the orbs and their mesh components
	for (int i = 0; i < orbcount; i++) {
		Orb* orb = new Orb(vec3(10, 10, 10), vec3::ONE * 0.1 * i, vec3::ZERO, vec3::ZERO);
		
		ModelInstance* mc = new ModelInstance(model);
		orb->mc = mc;
		entity->AddComponent(mc);
		orbs.push_back(orb);
	}
}

void OrbManager::ReceiveEvent(Event event) {
	
}

void OrbManager::Update() {
	Time* t = DengTime;
	
	persist bool lerping = false;
	persist float timer = 0;
	persist float lerptime = 2;
	
	std::vector<vec2> vs{
		vec2(0, 0),
		vec2(0.30211, 0.32297),
		vec2(0.56835, 0.73619),
		vec2(1, 1)
	};
	
	if (lerping) {
		timer += t->deltaTime;
		if (timer > lerptime) {
			lerping = false;
			timer = 0;
		}
	}
	
	float ti = 1;
	for (int i = 0; i < orbs.size(); i++) {
		
		
		Orb* o = orbs[i];
		
		if (DengInput->KeyPressed(Key::T)) {
			o->ito = vec3(10 * sin(i + cos(t->totalTime)), 10 * cos(i + cos(i + sin(t->totalTime))), 0);
		}
		
		if (DengInput->KeyPressed(Key::Y)) {
			o->ito = vec3(360 * sin(i + cos(t->totalTime)), 360 * cos(i + cos(i + sin(t->totalTime))),  360 * -cos(i + cos(i - sin(t->totalTime))));
			lerping = true;
			o->rotbflerp = o->rot;
			o->posbflerp = o->pos;
		}
		
		
		if (!lerping) {
			o->rotacc = Math::clamp((o->ito - o->rot), -100.f, 100.f);
			o->rotvel += o->rotacc * t->deltaTime;
			o->rotvel = Math::clamp(o->rotvel, -100.f, 100.f);
			o->rot += o->rotvel * t->deltaTime;
			
			//o->acc = Math::clamp((o->ito - o->pos) * (o->ito - o->pos).mag() * 1000, -100000.f, 100000.f);
			//o->vel += o->acc * t->deltaTime;
			//o->vel = Math::clamp(o->vel, -50.f, 50.f);
			//o->pos += o->vel * t->deltaTime;
			
			
			//o->ito = vec3(10 * sin(i + cos(t->totalTime)), 10 * cos(i + cos(i + sin(t->totalTime))), 0);
			//lerping = true;
			//o->posbflerp = o->pos;
			
			o->pos = o->origpos * mat4::RotationMatrixAroundPoint(vec3::ZERO, o->rot);
			//o->pos.x += sinf((1 / ti) * t->totalTime / 200 + cosf(t->totalTime / 200));
			//o->pos.z += cosf((1 / ti) * t->totalTime / 200 + sinf(t->totalTime / 200));
		}
		else {
			o->pos = o->origpos * mat4::RotationMatrixAroundPoint(vec3::ZERO, Math::lerpv(o->rotbflerp, vec3::ZERO, timer/lerptime));
			//o->pos = Math::lerpv(o->posbflerp, o->ito, Math::PolynomialCurveInterpolation(vs, timer / lerptime));
			
			
			o->rot = vec3::ZERO;
			
			
		}
		
		//o->pos.x += 10 * sin(t->totalTime + i);
		//o->mc->UpdateMeshTransform(o->pos, vec3(180 * sin(sin(i) * t->totalTime), 180 * cos(cos(i) * t->totalTime), 0), vec3::ONE);
		//o->mc->UpdateMeshTransform(o->pos, vec3::ZERO, vec3::ONE);
		
		//ti += 0.00001;
	}
	
	
}

std::string OrbManager::SaveTEXT(){
	return TOSTDSTRING("\n>orb manager"
					"\n");
}

void OrbManager::LoadDESH(Admin* admin, const char* data, u32& cursor, u32 count){
	ERROR("OrbManager::Load not setup");
}