#pragma once
#ifndef GAME_TRANSFORM_H
#define GAME_TRANSFORM_H

#include "../math/VectorMatrix.h"

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
	
	inline Matrix4 TransformMatrix(){
		return Matrix4::TransformationMatrix(position, rotation, scale);
	}
};

#endif //GAME_TRANSFORM_H