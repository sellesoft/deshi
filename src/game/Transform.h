#pragma once
#ifndef GAME_TRANSFORM_H
#define GAME_TRANSFORM_H

#include "../math/VectorMatrix.h"

struct Transform {
	vec3 position;
	vec3 rotation;
	vec3 scale;
	
	vec3 prevPosition;
	vec3 prevRotation;
	
	Transform(){
		position = vec3::ZERO;
		rotation = vec3::ZERO;
		scale    = vec3::ONE;
		prevPosition = vec3::ZERO;
		prevRotation = vec3::ZERO;
	}
	
	Transform(vec3 _position, vec3 _rotation, vec3 _scale){
		position = _position;
		rotation = _rotation;
		scale    = _scale;
		prevPosition = position;
		prevRotation = rotation;
	}
	
	inline vec3 Up(){
		return vec3::UP * mat4::RotationMatrix(rotation);
	}
	
	inline vec3 Right(){
		return vec3::RIGHT * mat4::RotationMatrix(rotation);
	}
	
	inline vec3 Forward(){
		return vec3::FORWARD * mat4::RotationMatrix(rotation);
	}
	
	inline mat4 TransformMatrix(){
		return mat4::TransformationMatrix(position, rotation, scale);
	}
};

#endif //GAME_TRANSFORM_H