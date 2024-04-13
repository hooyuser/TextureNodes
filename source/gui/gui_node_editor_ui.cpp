#include "gui_node_editor_ui.h"
#include "gui_node_editor.h"

#include <imgui.h>




auto constexpr pin_color_visitor = Overloaded {
	[](const TextureIdData&)       { return ImColor(227,  52,  47); },
	[](const FloatData&)           { return ImColor(246, 153,  63); },
	[](const IntData&)             { return ImColor(255, 237,  74); },
	[](const BoolData&)            { return ImColor(56,  193, 114); },
	[](const Color4Data&)          { return ImColor(77,  192, 181); },
	[](const EnumData&)            { return ImColor(52,  144, 220); },
	[](const ColorRampData&)       { return ImColor(101, 116, 205); },
	[](const FloatTextureIdData&)  { return ImColor(149,  97, 226); },
	[](const Color4TextureIdData&) { return ImColor(246, 109, 155); },
};

void NodeEditorUIManager::draw_node_pin(const ImVec2& center, const PinVariant& pin_data) const {
	auto const draw_list = ImGui::GetWindowDrawList();
	ImColor output_pin_color = std::visit(pin_color_visitor, pin_data);
	draw_list->AddCircle(  // Draw output pin circle
		center,
		pin_radius,
		output_pin_color,
		24,
		1.8
	);
	output_pin_color.Value.w *= 0.62745; 
	draw_list->AddCircleFilled(
		center,
		pin_radius,
		output_pin_color,
		24
	);
}




