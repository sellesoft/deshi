#include "../../EntityAdmin.h"

struct Player;
struct Movement;
struct AudioListener;
struct Collider;
struct MeshComp;
struct Physics;

struct PlayerEntity : public Entity {
	Player* player;
	Movement* movement;
	AudioListener* listener;
	Collider* collider;
	MeshComp* mesh;
	Physics* physics;




};
