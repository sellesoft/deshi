#pragma once
#ifndef DESHI_INSTANCEMANAGER_H
#define DESHI_INSTANCEMANAGER_H

#include <vector>
#include <iostream>

//template<class T, class S>
//using Modifier = T(*)(T newvalue, T oldvalue, S indexer);
//
//
//template<class ModifierInputs, class InitializerType, class... InitializerArgs>
//struct InstanceManager {
//
//	std::vector<std::pair<ModifierInputs, InitializerType>> objs;
//
//	void Instance(u32 in, ModifierInputs newin, Modifier<ModifierInputs, InitializerType> c, InitializerType initialize(InitializerArgs...), InitializerArgs... args) {
//		if (in < objs.size()) {
//			objs[in].first = c(newin, objs[in].first, objs[in].second);
//		}
//		else {
//			InitializerType temp = initialize(args...);
//			objs.push_back(std::pair<ModifierInputs, InitializerType>(newin, temp));
//			objs[in].first = c(newin, objs[in].first, objs[in].second);
//		}
//	}
//
//	//for when the initializing function is a member of a class
//	template<class FClass>
//	void Instance(FClass* funcclass, u32 in, ModifierInputs newin, Modifier<ModifierInputs, InitializerType> c, InitializerType FClass::initialize(InitializerArgs...), InitializerArgs... args) {
//		if (in < objs.size()) {
//			objs[in].first = c(newin, objs[in].first, objs[in].second);
//		}
//		else {
//			InitializerType temp = funcclass->initialize(args...);
//			objs.push_back(std::pair<ModifierInputs, InitializerType>(newin, temp));
//			objs[in].first = c(newin, objs[in].first, objs[in].second);
//		}
//	}
//};



#endif