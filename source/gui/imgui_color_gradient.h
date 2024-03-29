//
//  imgui_color_gradient.h
//  imgui extension
//
//  Created by David Gallardo on 11/06/16.

/*
 
 Usage:
 
 ::GRADIENT DATA::
 ImGradient gradient;
 
 ::BUTTON::
 if(ImGui::GradientButton(&gradient))
 {
    //set show editor flag to true/false
 }
 
 ::EDITOR::
 static ImGradientMark* draggingMark = nullptr;
 static ImGradientMark* selectedMark = nullptr;
 
 bool updated = ImGui::GradientEditor(&gradient, draggingMark, selectedMark);
 
 ::GET A COLOR::
 float color[3];
 gradient.getColorAt(0.3f, color); //position from 0 to 1
 
 ::MODIFY GRADIENT WITH CODE::
 gradient.getMarks().clear();
 gradient.addMark(0.0f, ImColor(0.2f, 0.1f, 0.0f));
 gradient.addMark(0.7f, ImColor(120, 200, 255));
 
 ::WOOD BROWNS PRESET::
 gradient.getMarks().clear();
 gradient.addMark(0.0f, ImColor(0xA0, 0x79, 0x3D));
 gradient.addMark(0.2f, ImColor(0xAA, 0x83, 0x47));
 gradient.addMark(0.3f, ImColor(0xB4, 0x8D, 0x51));
 gradient.addMark(0.4f, ImColor(0xBE, 0x97, 0x5B));
 gradient.addMark(0.6f, ImColor(0xC8, 0xA1, 0x65));
 gradient.addMark(0.7f, ImColor(0xD2, 0xAB, 0x6F));
 gradient.addMark(0.8f, ImColor(0xDC, 0xB5, 0x79));
 gradient.addMark(1.0f, ImColor(0xE6, 0xBF, 0x83));
 
 */

#pragma once

#include "imgui.h"
#include <json.hpp>

#include <list>
using json = nlohmann::json;

struct ImGradientMark {
    float color[4];
    float position; //0 to 1
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ImGradientMark, color, position)

class ImGradient {
public:
    //ImGradient() = default;
    ImGradient(float* lookup);
    ~ImGradient();
    
    void getColorAt(float position, float* color) const;
    void insert_mark(float position, ImColor color);
    void addMark(float position, ImColor color);
    void removeMark(ImGradientMark* mark);
    void clear_marks() {
        for (ImGradientMark* mark : m_marks) {
            delete mark;
        }
        m_marks.clear();
    }
    void refreshCache();
    std::list<ImGradientMark*> & getMarks(){ return m_marks; }

private:
    float* m_cachedValues;  //float[256 * 4] m_cachedValues;
    std::list<ImGradientMark*> m_marks;

    void computeColorAt(float position, float* color) const;

    friend inline void to_json(json& j, const ImGradient& p);
};

inline void to_json(json& j, const ImGradient& p) {
    for (auto iter : p.m_marks) {
        j.emplace_back(*iter);
    }
}

inline void from_json(const json& j, ImColor& p) {
    j[0].get_to(p.Value.x);
    j[1].get_to(p.Value.y);
    j[2].get_to(p.Value.z);
    j[3].get_to(p.Value.w);
};

namespace ImGui {
    bool GradientButton(const char* label, ImGradient* gradient, float max_width = 250.0f, float height = 25.0f);
    
    bool GradientEditor(const char* label, ImGradient* gradient,
                        ImGradientMark* & draggingMark,
                        ImGradientMark* & selectedMark);
}
