#include "../hooks.h"

void __stdcall hooks::client_mode::override_view::fn(view_setup_t* view) {
	static const auto original = m_client_mode->get_original<T>(index);

	original(view);
}