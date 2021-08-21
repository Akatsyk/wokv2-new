#include "../features.h"

bool c_bonesetup::setup_bones_rebuild(c_cs_player* player, int mask, matrix3x4_t* out) {
	auto backup_occlusion_flags = player->get_occlusion_flags();
	auto backup_occlusion_framecount = player->get_occlusion_frame_count();

	auto backup_abs_origin = player->get_abs_origin();

	if (!globals::m_local)
		player->set_abs_origin(player->get_origin());

	player->invalidate_bone_cache();

	player->get_effects().add(0x8);

	player->setup_bones(out, 128, mask, player->get_sim_time());

	player->get_occlusion_flags() = backup_occlusion_flags;
	player->get_occlusion_frame_count() = backup_occlusion_framecount;

	player->get_abs_origin() = backup_abs_origin;

	player->get_effects().remove(0x8);

	return true;
}