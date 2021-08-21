#include "../features.h"

void c_fakelag::on_create_move() { // example of fakelag, u can recode it and make adapive fakelags(diff origin's)
	if (!cfg::get<bool>(FNV1A("fakelag_enable")))
		return;

	int max_to_choke = 14;

	if (globals::m_cur_cmd->m_buttons.has(IN_ATTACK)) {
		globals::m_packet = true;

		max_to_choke = 1;

		return;
	}

	int tick_to_choke = cfg::get<int>(FNV1A("fakelag_amount"));

	if (tick_to_choke <= 1)
		tick_to_choke = 1;

	if (tick_to_choke > max_to_choke)
		tick_to_choke = max_to_choke;

	globals::m_packet = interfaces::m_client_state->m_choked_commands >= tick_to_choke;
}