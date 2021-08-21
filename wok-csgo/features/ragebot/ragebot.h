#include "../../globals.h"

struct target_t {
	c_cs_player* m_target;
	int	m_index;

	void store_player(c_cs_player* player, bool store_index, int index) {

		if (store_index == true)
			m_index = index;
		else if (store_index == false)
			m_target = player;
	}
}; // dont use it, i saved that for next upd

class c_ragebot : public c_singleton<c_ragebot> {
public:

	void on_create_move(c_user_cmd* m_cmd);

	bool hitchance(qangle_t angles, float hitchance);

	int find_target();

	void autostop();

	bool is_able_to_shoot();

	bool is_valid_player(c_cs_player* player);

	void reset_player();

	bool get_multipoints(matrix3x4_t bones[], int index, std::vector<vec3_t>& points);

	vec3_t scan(int* hitbox = nullptr, int* est_damage = nullptr);

	bool run(vec3_t point, int index);

	float point_scale(float hitbox_radius, vec3_t pos, vec3_t point, int hitbox);

	matrix3x4_t m_matrix[128];

	c_user_cmd* cmd;

	c_cs_player* m_player;
	int i_index;
};

#define ragebot c_ragebot::instance()