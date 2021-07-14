#pragma once
#ifndef DESHI_IMGUI_H
#define DESHI_IMGUI_H

#include "../external/imgui/imgui.h" //includes <float.h>,<stdarg.h>,<stddef.h>,<string.h>
#include "../external/imgui/imgui_internal.h" //includes <stdio.h>,<stdlib.h>,<math.h>,<limits.h>

#include "../math/Vector.h"
#include "../utils/Color.h"

namespace DeshiImGui{
	
	void init();
	void cleanup();
	void newFrame();
	
}; //namespace DeshiImGui

namespace ImGui{
	
	static ImVec4 ColorToImVec4(Color color) {
		return ImVec4((float)color.r / 255.0f, (float)color.g / 255.0f, (float)color.b / 255.0f, color.a / 255.0f);
	}
	
	static ImVec2 Vector2ToImVec2(Vector2 v){
		return ImVec2(v.x, v.y);
	}
	
	static void TextCentered(const char* text){
		size_t text_length = strlen(text);
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text, text+text_length).x) / 2.f);
		ImGui::TextEx(text, text+text_length);
	}
	
    static void CopyButton(const char* text) {
        if(ImGui::Button("Copy")){ ImGui::LogToClipboard(); ImGui::LogText(text); ImGui::LogFinish(); }
    }
    
    static bool InputVector2(const char* id, Vector2* vecPtr, bool inputUpdate = false) {
        ImGui::SetNextItemWidth(-FLT_MIN);
        if(inputUpdate) {
            return ImGui::InputFloat2(id, (float*)vecPtr); 
        } else {
            return ImGui::InputFloat2(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        }
    }
    
    static bool InputVector3(const char* id, Vector3* vecPtr, bool inputUpdate = false) {
        ImGui::SetNextItemWidth(-FLT_MIN);
        if(inputUpdate) {
            return ImGui::InputFloat3(id, (float*)vecPtr); 
        } else {
            return ImGui::InputFloat3(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue); 
        }
    }
    
    static bool InputVector4(const char* id, Vector4* vecPtr, bool inputUpdate = false) {
        ImGui::SetNextItemWidth(-FLT_MIN);
        if(inputUpdate) {
            return ImGui::InputFloat4(id, (float*)vecPtr);
        } else {
            return ImGui::InputFloat4(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue); 
        }
    }
	
	static bool SliderUInt32(const char* label, u32* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0){
		return SliderScalar(label, ImGuiDataType_U32, v, &v_min, &v_max, format, flags);
	}
	
}; //namespace ImGui

#endif //DESHI_IMGUI_H