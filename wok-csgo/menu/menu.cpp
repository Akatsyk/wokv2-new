#include "menu.h"

void c_menu::on_paint() {
	if (!(input::m_blocked = input::get_key<TOGGLE>(VK_INSERT)))
		return;

	ImGui::SetNextWindowPos(ImGui::GetIO().DisplaySize / 2.f, ImGuiCond_Once, ImVec2(0.5f, 0.5f));

	ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_Once);

	if (ImGui::Begin(_("wok sdk v2"), 0, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse)) {
		ImGui::Checkbox(_("ragebot enable"), &cfg::get<bool>(FNV1A("ragebot_enable")));
		ImGui::Checkbox(_("autoshoot enable"), &cfg::get<bool>(FNV1A("autoshoot_enable")));
		ImGui::Checkbox(_("autoscope enable"), &cfg::get<bool>(FNV1A("autoscope_enable")));
		ImGui::Checkbox(_("autostop enable"), &cfg::get<bool>(FNV1A("autostop_enable")));

		ImGui::SliderInt(_("min damage"), &cfg::get<int>(FNV1A("min_damage")), 0, 120);
		ImGui::SliderFloat(_("hitchane amount"), &cfg::get<float>(FNV1A("hitchance_amount")), 0.f, 100.f);

		if (ImGui::Button(_("save"))) {
			cfg::save();
		}

		if (ImGui::Button(_("load"))) {
			cfg::load();
		}
	}
	ImGui::End();
}