#include "../features.h"

void c_legitbot::on_create_move(c_user_cmd* cmd) {
	if (!interfaces::m_engine->is_in_game() || !interfaces::m_engine->is_connected() || !cfg::get<bool>(FNV1A("legitbot_enable")))
		return;

	if (!globals::m_local || !globals::m_local->is_alive())
		return;

	auto weapon = globals::m_local->get_active_weapon();

	m_cmd = cmd;

	if (!weapon)
		return;

	m_index = find_target();

	if (m_index >= 0)
		m_player = static_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(m_index));
	else
		m_player = nullptr;

	if (m_index >= 0 && m_player)
		run(cmd);
}

bool c_legitbot::is_valid_player(c_cs_player* player) {
	if (!player || player == nullptr)
		return false;

	if (player->get_team() == globals::m_local->get_team())
		return false;

	if (player->is_dormant())
		return false;

	if (!player->is_alive())
		return false;

	return true;
}

int c_legitbot::find_target() {
	int index = -1;

	int distance = 99999;

	qangle_t view_angles;
	interfaces::m_engine->get_view_angles(view_angles);

	auto bounding_box = [this](c_cs_player* player) -> bool {
		auto collideable = player->get_collideable();

		if (!collideable)
			return false;

		const auto obb_min = collideable->obb_mins() + player->get_origin();
		const auto obb_max = collideable->obb_maxs() + player->get_origin();

		vec3_t points[7];

		points[0] = player->get_hitbox_pos(HITBOX_HEAD);
		points[1] = (obb_min + obb_max) * 0.5f;
		points[2] = vec3_t((obb_max.x + obb_min.x) * 0.5f, (obb_max.y + obb_min.y) * 0.5f, obb_min.z);

		points[3] = obb_max;
		points[4] = vec3_t(obb_max.x, obb_min.y, obb_max.z);
		points[5] = vec3_t(obb_min.x, obb_min.y, obb_max.z);
		points[6] = vec3_t(obb_min.x, obb_max.y, obb_max.z);

		for (const auto& point : points) {
			if (autowall->get_damage(point, player) > 0)
				return true;
		}
		return false;
	};

	for (int i = 0; i < 64; i++) {
		c_cs_player* player = static_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(i));

		if (is_valid_player(player)) {
			if (bounding_box(player)) {
				vec3_t diff = globals::m_local->get_origin() - player->get_origin();

				int dist = diff.length();

				if (dist < distance) {
					distance = dist;
					index = i;
				}
			}
		}
	}

	return index;
}

std::vector<vec3_t> c_legitbot::get_shoot_position(c_cs_player* player) {
	std::vector<vec3_t> points = {};

	if (!player)
		return points;

	if (cfg::get<int>(FNV1A("legitbot_hitbox_num")) == 1) {
		points.push_back({ player->get_hitbox_pos(HITBOX_HEAD) });
		points.push_back({ player->get_hitbox_pos(HITBOX_NECK) });
	}
	if (cfg::get<int>(FNV1A("legitbot_hitbox_num")) == 2) {
		points.push_back({ player->get_hitbox_pos(HITBOX_STOMACH) });
		points.push_back({ player->get_hitbox_pos(HITBOX_PELVIS) });
		points.push_back({ player->get_hitbox_pos(HITBOX_CHEST) });
		points.push_back({ player->get_hitbox_pos(HITBOX_LOWER_CHEST) });
		points.push_back({ player->get_hitbox_pos(HITBOX_UPPER_CHEST) });
	}
	if (cfg::get<int>(FNV1A("legitbot_hitbox_num")) == 3) {
		points.push_back({ player->get_hitbox_pos(HITBOX_LEFT_FOREARM) });
		points.push_back({ player->get_hitbox_pos(HITBOX_RIGHT_FOREARM) });
	}
	if (cfg::get<int>(FNV1A("legitbot_hitbox_num")) == 4) {
		points.push_back({ player->get_hitbox_pos(HITBOX_LEFT_FOOT) });
		points.push_back({ player->get_hitbox_pos(HITBOX_RIGHT_FOOT) });
	}

	return points;
}

void c_legitbot::run(c_user_cmd* cmd) {
	auto index = m_player->get_index();

	auto best_fov = cfg::get<float>(FNV1A("legitbot_fov"));
	float fov = 0.f;

	vec3_t best_point = vec3_t(0, 0, 0);

	auto shoot_pos = get_shoot_position(m_player);

	qangle_t fov_angle = qangle_t(0, 0, 0);

	for (auto fov_point : shoot_pos) {
		fov_angle = math::calc_angle(globals::m_local->get_eye_pos(), fov_point);

		fov = math::calc_fov(cmd->m_view_angles, fov_angle);
	}

	if (std::isnan(fov) || fov < best_fov) {
		if (bone_setup->setup_bones_rebuild(m_player, BONE_FLAG_USED_BY_ANYTHING, nullptr)) {
			auto points = get_shoot_position(m_player);

			best_fov = fov;

			int best_damage = 0;

			for (auto point : points) {
				if (point == vec3_t(0, 0, 0))
					continue;

				auto wall_damage = autowall->get_damage(point, m_player);

				if (wall_damage <= 0)
					best_point = vec3_t(0, 0, 0);

				if (wall_damage > best_damage) {
					best_damage = wall_damage;
					best_point = point;
				}
			}
		}

		auto aim_angle = math::calc_angle(globals::m_local->get_eye_pos(), best_point);

		if (cfg::get<float>(FNV1A("legitbot_smooth")) && !cfg::get<bool>(FNV1A("legitbot_silent")))
			aim_angle = globals::m_cur_cmd->m_view_angles + (aim_angle - globals::m_cur_cmd->m_view_angles) / cfg::get<float>(FNV1A("legitbot_smooth"));

		if (cmd->m_buttons.has(IN_ATTACK))
			cmd->m_view_angles = aim_angle;

		if (!cfg::get<bool>(FNV1A("legitbot_silent")))
			interfaces::m_engine->set_view_angles(cmd->m_view_angles);
	}
}