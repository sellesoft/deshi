#pragma once
#ifndef GAME_TRANSFORM_H
#define GAME_TRANSFORM_H

#include "../math/VectorMatrix.h"

struct Transform {
	Vector3 position;
	Vector3 rotation;
	Vector3 scale;
	
	Vector3 prevPosition;
	Vector3 prevRotation;
	
	Transform(){
		position = Vector3::ZERO;
		rotation = Vector3::ZERO;
		scale    = Vector3::ONE;
		prevPosition = Vector3::ZERO;
		prevRotation = Vector3::ZERO;
	}
	
	Transform(Vector3 _position, Vector3 _rotation, Vector3 _scale){
		position = _position;
		rotation = _rotation;
		scale    = _scale;
		prevPosition = position;
		prevRotation = rotation;
	}
	
	inline Vector3 Up(){
		return Vector3::UP * Matrix4::RotationMatrix(rotation);
	}
	
	inline Vector3 Right(){
		return Vector3::RIGHT * Matrix4::RotationMatrix(rotation);
	}
	
	inline Vector3 Forward(){
		return Vector3::FORWARD * Matrix4::RotationMatrix(rotation);
	}
	
	inline Matrix4 TransformMatrix(){
		return Matrix4::TransformationMatrix(position, rotation, scale);
	}
};

#endif //GAME_TRANSFORM_H