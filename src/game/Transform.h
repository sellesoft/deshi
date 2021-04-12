#pragma once
#ifndef GAME_TRANSFORM_H
#define GAME_TRANSFORM_H

#include "../math/VectorMatrix.h"
#include "../math/Math.h"

struct Transform {
	Vector3 position = Vector3::ZERO;
	Vector3 rotation = Vector3::ZERO;
	Vector3 scale    = Vector3::ONE;
	
	Vector3 prevPosition = Vector3::ZERO;
	Vector3 prevRotation = Vector3::ZERO;
	
	Transform(){};
	Transform(Vector3 position, Vector3 rotation, Vector3 scale){
		this->position = position;
		this->rotation = rotation;
		this->scale = scale;
		prevPosition = position;
		prevRotation = rotation;
	}
	
	inline Vector3 Up(){
		return (Vector3::UP * Matrix4::RotationMatrix(rotation)).normalized();
	}
	
	inline Vector3 Right(){
		return (Vector3::RIGHT * Matrix4::RotationMatrix(rotation)).normalized();
	}
	
	inline Vector3 Forward(){
		return (Vector3::FORWARD * Matrix4::RotationMatrix(rotation)).normalized();
	}

	inline void AlignUp(Vector3 up) {
		rotation = Vector3::UP * Math::LookAtMatrix(position, up);
	}

	inline void AlignRight(Vector3 right) {
		rotation = Vector3::RIGHT * Math::LookAtMatrix(position, right);
	}

	inline void AlignForward(Vector3 forward) {
		rotation = Vector3::FORWARD * Math::LookAtMatrix(position, forward);
	}
	
	inline Matrix4 TransformMatrix(){
		return Matrix4::TransformationMatrix(position, rotation, scale);
	}

	inline std::string Save() {
		return TOSTRING(
			"pos: ", position, "\n",
			"rot: ", rotation, "\n",
			"scale: ", scale, "\n"
		);
	}

	inline void Load() {

	}
};

#endif //GAME_TRANSFORM_H