#include "Orb.h"
#include "MeshComp.h"
#include "../../core.h"
#include "../../EntityAdmin.h"
#include "../systems/WorldSystem.h"
#include "../../scene/Model.h"
#include "../../scene/Scene.h"



OrbManager::OrbManager(Mesh* m, EntityAdmin* a, Entity* e) {	
	//create the orbs and their mesh components
	this->admin = a;
	this->entity = e;
	for (int i = 0; i < orbcount; i++) {
		Orb* orb = new Orb(Vector3(10, 10, 10), Vector3::ONE * 0.1 * i, Vector3::ZERO, Vector3::ZERO);
		Mesh* mes = new Mesh(*m);
		MeshComp* mc = new MeshComp(mes);
		orb->mc = mc;
		mc->ENTITY_CONTROL = false;
 		orbs.push_back(orb);
		admin->world->AddAComponentToEntity(admin, entity, mc);

		u32 id = admin->renderer->LoadBaseMesh(mes);
		Model mod;
		mod.mesh = *mes;
		mc->MeshID = id;
		admin->scene->models.push_back(mod);
	}
	name = "OrbManager";
	layer = CL0_PHYSICS;
};

void OrbManager::ReceiveEvent(Event event) {

}

void OrbManager::Update() {
	Time* t = DengTime;

	static bool lerping = false;

	static float timer = 0;
	static float lerptime = 5;

	std::vector<Vector2> vs{
		Vector2(0, 0),
		Vector2(0.30211, 0.32297),
		Vector2(0.56835, 0.73619),
		Vector2(1, 1)
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
			o->ito = Vector3(10 * sin(i + cos(t->totalTime)), 10 * cos(i + cos(i + sin(t->totalTime))), 0);
		}

		if (DengInput->KeyPressed(Key::Y)) {
			o->ito = Vector3(360 * sin(i + cos(t->totalTime)), 360 * cos(i + cos(i + sin(t->totalTime))),  360 * -cos(i + cos(i - sin(t->totalTime))));
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


			//o->ito = Vector3(10 * sin(i + cos(t->totalTime)), 10 * cos(i + cos(i + sin(t->totalTime))), 0);
			//lerping = true;
			//o->posbflerp = o->pos;

			o->pos = o->origpos * Matrix4::RotationMatrixAroundPoint(Vector3::ZERO, o->rot);
			//o->pos.x += sinf((1 / ti) * t->totalTime / 200 + cosf(t->totalTime / 200));
			//o->pos.z += cosf((1 / ti) * t->totalTime / 200 + sinf(t->totalTime / 200));
		}
		else {
			o->pos = o->origpos * Matrix4::RotationMatrixAroundPoint(Vector3::ZERO, Math::lerpv(o->rotbflerp, Vector3::ZERO, timer/lerptime));
			//o->pos = Math::lerpv(o->posbflerp, o->ito, Math::PolynomialCurveInterpolation(vs, timer / lerptime));
			

			o->rot = Vector3::ZERO;
			

		}

		//o->pos.x += 10 * sin(t->totalTime + i);
		o->mc->UpdateMeshTransform(o->pos, Vector3(180 * sin(sin(i) * t->totalTime), 180 * cos(cos(i) * t->totalTime), 0), Vector3::ONE);
	
		//ti += 0.00001;
	}


}