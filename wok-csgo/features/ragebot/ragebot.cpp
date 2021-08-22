#include "../features.h"

void c_ragebot::on_create_move(c_user_cmd* m_cmd) {
	if (!interfaces::m_engine->is_in_game() || !interfaces::m_engine->is_connected() || !cfg::get<bool>(FNV1A("ragebot_enable")))
		return;

	if (!globals::m_local || !globals::m_local->is_alive())
		return;

	auto weapon = globals::m_local->get_active_weapon();

	cmd = m_cmd;

	if (!weapon)
		return;

	i_index = find_target();

	if (i_index >= 0)
		m_player = static_cast<c_cs_player*>(interfaces::m_entity_list->get_client_entity(i_index));
	else
		m_player = nullptr;

	if (i_index >= 0 && m_player) {
		auto aim_point = scan();
		run(aim_point, i_index);
	}

	if (cmd->m_buttons.has(IN_ATTACK))
		cmd->m_view_angles -= globals::m_local->get_aim_punch_angle() * 2.f;
}

bool c_ragebot::is_valid_player(c_cs_player* player) {
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

int c_ragebot::find_target() {
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

		points[0] = player->get_bone_pos(0);
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

void c_ragebot::reset_player() {
	i_index = -1;
}

bool c_ragebot::hitchance(qangle_t angles, float hitchance) {
	auto weapon = globals::m_local->get_active_weapon();
	auto weapon_index = weapon->get_item_definition_index();

	if (!weapon)
		return false;

	vec3_t forward, right, up;
	math::angle_vectors(angles, &forward, &right, &up);

	int hits = 0;
	int needed_hits = static_cast<int>(255 * (hitchance / 100.f));

	float spread = engine_prediction->get_spread();
	float inaccuracy = engine_prediction->get_inaccuracy();

	vec3_t src = globals::m_local->get_eye_pos();

	for (int i = 0; i < 255; i++) {
		float a = math::random_float(0.f, 1.f);
		float b = math::random_float(0.f, math::m_pi * 2.f);
		float c = math::random_float(0.f, 1.f);
		float d = math::random_float(0.f, math::m_pi * 2.f);
		float inacc = a * inaccuracy;
		float spr = c * spread;

		if (weapon_index == WEAPON_R8_REVOLVER) {
			if (cmd->m_buttons.has(IN_ATTACK2)) {
				a = 1.f - a * a;
				c = 1.f - c * c;
			}
		}

		vec3_t spread_view((cos(b) * inacc) + (cos(d) * spr), (sin(b) * inacc) + (sin(d) * spr), 0);
		vec3_t direction;

		direction.x = forward.x + (spread_view.x * right.x) + (spread_view.y + up.x);
		direction.y = forward.y + (spread_view.x * right.y) + (spread_view.y + up.y);
		direction.z = forward.z + (spread_view.x * right.z) + (spread_view.y * up.z);
		direction.normalized();

		qangle_t view_angles_spread;
		vec3_t view_forward;

		math::vector_angles(direction, up, view_angles_spread);
		view_angles_spread.normalized();

		math::angle_vectors(view_angles_spread, &view_forward);

		view_forward.normalize();

		view_forward = src + (view_forward * weapon->get_cs_weapon_data()->m_range);

		c_game_trace trace;
		ray_t ray(src, view_forward);

		interfaces::m_trace_system->clip_ray_to_entity(ray, MASK_SHOT | CONTENTS_GRATE, m_player, &trace);

		if (trace.m_hit_entity == m_player)
			hits++;

		if (static_cast<int>((static_cast<float>(hits) / 255) * 100.f) >= hitchance)
			return true;

		if ((255 - i + hits) < needed_hits)
			return false;
	}

	return false;
}

bool c_ragebot::get_multipoints(matrix3x4_t bones[], int index, std::vector<vec3_t>& points) {
	points.clear();

	const model_t* model = m_player->get_model();

	if (!model)
		return false;

	studiohdr_t* hdr = interfaces::m_model_info->get_studio_model(model);

	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->get_hitbox_set(m_player->hitbox_set());

	auto bbox = set->get_hitbox(index);

	if (!bbox)
		return false;

	matrix3x4_t rot_matrix;

	math::angle_matrix(qangle_t(bbox->m_rotation.x, bbox->m_rotation.y, bbox->m_rotation.z), rot_matrix);

	matrix3x4_t matrix;

	math::concat_transform(bones[bbox->m_bone], rot_matrix, matrix);

	vec3_t origin = matrix.GetOrigin();

	vec3_t center = (bbox->m_obb_min + bbox->m_obb_max) / 2.f;

	float scale = point_scale(bbox->m_radius, globals::m_local->get_eye_pos(), center, index);

	if (!m_player->get_flags().has(FL_ONGROUND)) // meme inair fix lol
		scale = 0.7f;

	if (bbox->m_radius <= 0.f) {
		if (index == HITBOX_RIGHT_FOOT || index == HITBOX_LEFT_FOOT) {
			float d1 = (bbox->m_obb_min.z - center.z) * 0.875f;

			if (index == HITBOX_LEFT_FOOT)
				d1 *= -1.f;

			points.push_back({ center.x, center.y, center.z + d1 });
			{

				float d2 = (bbox->m_obb_min.x - center.x) * scale;
				float d3 = (bbox->m_obb_max.x - center.x) * scale;

				points.push_back({ center.x + d2, center.y, center.z });

				points.push_back({ center.x + d3, center.y, center.z });
			}
		}

		if (points.empty())
			return false;

		for (auto& p : points) {
			p = { p.dot_product(matrix[0]), p.dot_product(matrix[1]), p.dot_product(matrix[2]) };

			p += origin;
		}
	}
	else {
		float r = bbox->m_radius * scale;
		float br = bbox->m_radius * scale;

		vec3_t center = (bbox->m_obb_min + bbox->m_obb_max) / 2.f;

		if (index == HITBOX_HEAD) {
			points.push_back(center);

			constexpr float rotation = 0.70710678f;

			points.push_back({ bbox->m_obb_max.x + (rotation * r), bbox->m_obb_max.y + (-rotation * r), bbox->m_obb_max.z });

			points.push_back({ bbox->m_obb_max.x, bbox->m_obb_max.y, bbox->m_obb_max.z + r });

			points.push_back({ bbox->m_obb_max.x, bbox->m_obb_max.y, bbox->m_obb_max.z - r });

			points.push_back({ bbox->m_obb_max.x, bbox->m_obb_max.y - r, bbox->m_obb_max.z });
		}

		else if (index == HITBOX_STOMACH) {
			points.push_back(center);

			points.push_back({ center.x, bbox->m_obb_max.y - br, center.z });
		}

		else if (index == HITBOX_PELVIS || index == HITBOX_UPPER_CHEST) {
			points.push_back({ center.x, bbox->m_obb_max.y - r, center.z });
		}

		else if (index == HITBOX_LOWER_CHEST || index == HITBOX_CHEST) {
			points.push_back(center);


			points.push_back({ center.x, bbox->m_obb_max.y - r, center.z });
		}

		else if (index == HITBOX_RIGHT_CALF || index == HITBOX_LEFT_CALF) {
			points.push_back(center);

			points.push_back({ bbox->m_obb_max.x - (bbox->m_radius / 2.f), bbox->m_obb_max.y, bbox->m_obb_max.z });
		}

		else if (index == HITBOX_RIGHT_THIGH || index == HITBOX_LEFT_THIGH) {
			points.push_back(center);
		}

		else if (index == HITBOX_RIGHT_UPPER_ARM || index == HITBOX_LEFT_UPPER_ARM) {
			points.push_back({ bbox->m_obb_max.x + bbox->m_radius, center.y, center.z });
		}

		if (points.empty())
			return false;

		for (auto& p : points)
			math::vector_transform(p, bones[bbox->m_bone], p); // vec3_t -> i_transform or rotate ik dont tell me pls ;(
	}

	return true;
}

bool c_ragebot::is_able_to_shoot() {
	auto weapon = globals::m_local->get_active_weapon();

	if (!weapon)
		return false;

	if (weapon->get_item_definition_index() == WEAPON_C4)
		return false;

	auto time = TICKS_TO_TIME(globals::m_local->get_tick_base()); // used predicted curtime :0

	if (cmd->m_weapon_select)
		return false;

	if (globals::m_local->get_next_attack() > time || weapon->get_next_primary_attack() > time || weapon->get_next_secondary_attack() > time) {
		if (weapon->get_item_definition_index() == WEAPON_R8_REVOLVER)
			return true;
		else
			return false;
	}

	return true;
}

std::vector<vec3_t> c_ragebot::get_hitbox(c_cs_player* player) {
	std::vector<vec3_t> points = {};

	if (!player)
		return points;

	if (cfg::get<int>(FNV1A("hitbox_num")) == 1) {
		points.push_back({ player->get_hitbox_pos(HITBOX_HEAD) });
		points.push_back({ player->get_hitbox_pos(HITBOX_NECK) });
	}
	if (cfg::get<int>(FNV1A("hitbox_num")) == 2) {
		points.push_back({ player->get_hitbox_pos(HITBOX_STOMACH) });
		points.push_back({ player->get_hitbox_pos(HITBOX_PELVIS) });
		points.push_back({ player->get_hitbox_pos(HITBOX_CHEST) });
		points.push_back({ player->get_hitbox_pos(HITBOX_LOWER_CHEST) });
		points.push_back({ player->get_hitbox_pos(HITBOX_UPPER_CHEST) });
	}
	if (cfg::get<int>(FNV1A("hitbox_num")) == 3) {
		points.push_back({ player->get_hitbox_pos(HITBOX_LEFT_FOREARM) });
		points.push_back({ player->get_hitbox_pos(HITBOX_RIGHT_FOREARM) });
	}
	if (cfg::get<int>(FNV1A("hitbox_num")) == 4) {
		points.push_back({ player->get_hitbox_pos(HITBOX_LEFT_FOOT) });
		points.push_back({ player->get_hitbox_pos(HITBOX_RIGHT_FOOT) });
	}

	return points;
}

vec3_t c_ragebot::scan(int* hitbox, int* est_damage) {
	auto index = m_player->get_index();

	bone_setup->setup_bones_rebuild(m_player, BONE_FLAG_USED_BY_ANYTHING, m_matrix);

	if (m_matrix == nullptr)
		return vec3_t(0, 0, 0);

	static int hitscan_mode;

	auto points = get_hitbox(m_player);

	int best_damage = 0;
	vec3_t best_point = vec3_t(0, 0, 0);

	for (auto point : points) {
		if (!get_multipoints(m_matrix, index, points))
			continue;

		auto wall_damage = autowall->get_damage(point, m_player);

		if (wall_damage <= 0)
			return best_point;

		if (wall_damage > best_damage && wall_damage >= cfg::get<int>(FNV1A("min_damage"))) {
			best_damage = wall_damage;
			best_point = point;
		}
	}

	if (est_damage)
		*est_damage = best_damage;

	return best_point;
}

bool c_ragebot::run(vec3_t point, int index) {
	auto aim_angle = math::calc_angle(globals::m_local->get_eye_pos(), point);

	bool hitchaned = hitchance(aim_angle, cfg::get<float>(FNV1A("hitchance_amount")));

	float server_time = TICKS_TO_TIME(globals::m_local->get_tick_base());
	bool can_shoot = is_able_to_shoot();

	if (hitchaned && can_shoot) {
		if (cfg::get<bool>(FNV1A("autoshoot_enable"))) {
			cmd->m_buttons.add(IN_ATTACK);
		}

		if (cmd->m_buttons.has(IN_ATTACK)) {
			cmd->m_view_angles = aim_angle;
		}
	}
	else {
		if (cfg::get<bool>(FNV1A("autoscope_enable")) && globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_weapon_type == WEAPON_TYPE_SNIPER && !globals::m_local->is_scoped()) {
			cmd->m_buttons.add(IN_ATTACK2);
		}
	}

	if (cfg::get<bool>(FNV1A("autostop_enable"))) {
		static auto min_velocity = 0.f;

		min_velocity = globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_max_speed_alt * .34f;

		if (globals::m_local->get_velocity().length() >= min_velocity && can_shoot && globals::m_local->get_flags().has(FL_ONGROUND) && !globals::m_local->get_active_weapon()->get_cs_weapon_data()->m_weapon_type == WEAPON_TYPE_KNIFE)
			autostop();
	}

	return true;
}

void c_ragebot::autostop() {
	qangle_t direction;
	qangle_t real_view;

	math::vector_angles(globals::m_local->get_velocity(), direction);
	interfaces::m_engine->get_view_angles(real_view);

	direction.y = real_view.y - direction.y;

	vec3_t forward;
	math::angle_vectors(direction, &forward);

	static auto cl_forwardspeed = interfaces::m_cvar_system->find_var(FNV1A("cl_forwardspeed"));
	static auto cl_sidespeed = interfaces::m_cvar_system->find_var(FNV1A("cl_sidespeed"));

	auto negative_forward_speed = -cl_forwardspeed->get_float() * 2.0;
	auto negative_side_speed = -cl_sidespeed->get_float() * 2.0;

	auto negative_forward_direction = forward * negative_forward_speed;
	auto negative_side_direction = forward * negative_side_speed;

	cmd->m_move.x = negative_forward_direction.x;
	cmd->m_move.y = negative_side_direction.y;
}

float c_ragebot::point_scale(float radius, vec3_t pos, vec3_t point, int hitbox) {
	auto weapon = globals::m_local->get_active_weapon();

	float point_scale;
	float scale;

	if (!weapon)
		return 0.f;

	if (!cfg::get<bool>(FNV1A("point_scale"))) {
		scale = 0.1f;
	}
	else {
		if (hitbox == HITBOX_HEAD)
			scale = cfg::get<float>(FNV1A("head_scale"));
		else if (hitbox == HITBOX_CHEST || hitbox == HITBOX_STOMACH || hitbox == HITBOX_PELVIS || hitbox == HITBOX_UPPER_CHEST)
			scale = cfg::get<float>(FNV1A("body_scale"));
		else
			scale = 5.f;
	}

	point_scale = (((scale / 100.0) * 0.69999999) + 0.2) * radius;

	return std::fminf(radius * 0.9f, point_scale);
}