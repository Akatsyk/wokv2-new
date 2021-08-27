#include "../../globals.h"

class c_legitbot : public c_singleton<c_legitbot> {
public:

	void on_create_move(c_user_cmd* cmd);
	int find_target();

	bool is_valid_player(c_cs_player* player);

	std::vector<vec3_t> get_shoot_position(c_cs_player* player);

	void run(c_user_cmd* cmd);

	c_cs_player* m_player;
	int m_index;
	c_user_cmd* m_cmd;
};

#define legitbot c_legitbot::instance()