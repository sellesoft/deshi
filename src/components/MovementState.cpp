#include "MovementState.h"

#include "../EntityAdmin.h"

MovementState::MovementState() {
	movementState = MOVEMENT_NOCLIP | MOVEMENT_FLYING;

	//not sure where i want this yet
	layer = CL0_COMMAND;
}

MovementState::MovementState(uint32 state) {
	movementState = state;

	layer = CL0_COMMAND;
}

void MovementState::Update() {

}
