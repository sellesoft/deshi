#pragma once
#ifndef DESHI_IMGUI_H
#define DESHI_IMGUI_H

#include "../math/vector.h"
#include "../utils/Color.h"

#include "../external/imgui/imgui.h" //includes <float.h>,<stdarg.h>,<stddef.h>,<string.h>
#undef Max
#undef Min 
#include "../external/imgui/imgui_internal.h" //includes <stdio.h>,<stdlib.h>,<math.h>,<limits.h>
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Min(a, b) (((a) < (b)) ? (a) : (b))

namespace DeshiImGui{
	
	void Init();
	void Cleanup();
	void NewFrame();
	
}; //namespace DeshiImGui

namespace ImGui{
	
	static ImVec4 ColorToImVec4(color _color) {
		return ImVec4((float)_color.r / 255.0f, (float)_color.g / 255.0f, (float)_color.b / 255.0f, _color.a / 255.0f);
	}
	
	static ImVec2 vec2ToImVec2(vec2 v){
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
	
	static bool Inputvec2(const char* id, vec2* vecPtr, bool inputUpdate = false) {
		ImGui::SetNextItemWidth(-FLT_MIN);
		if(inputUpdate) {
			return ImGui::InputFloat2(id, (float*)vecPtr); 
		} else {
			return ImGui::InputFloat2(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
		}
	}
	
	static bool Inputvec3(const char* id, vec3* vecPtr, bool inputUpdate = false) {
		ImGui::SetNextItemWidth(-FLT_MIN);
		if(inputUpdate) {
			return ImGui::InputFloat3(id, (float*)vecPtr); 
		} else {
			return ImGui::InputFloat3(id, (float*)vecPtr, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue); 
		}
	}
	
	static bool Inputvec4(const char* id, vec4* vecPtr, bool inputUpdate = false) {
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
	
	static void HelpMarker(const char* symbol, const char* desc){
		ImGui::TextDisabled(symbol);
		if (ImGui::IsItemHovered()){
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
	
}; //namespace ImGui

#endif //DESHI_IMGUI_H