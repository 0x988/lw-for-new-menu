// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "prediction_system.h"

void engineprediction::store_netvars()
{
	auto data = &netvars_data[m_clientstate()->iCommandAck % MULTIPLAYER_BACKUP]; //-V807

	data->tickbase = g_ctx.local()->m_nTickBase(); //-V807

	data->m_aimPunchAngle = g_ctx.local()->m_aimPunchAngle();
	data->m_aimPunchAngleVel = g_ctx.local()->m_aimPunchAngleVel();

	data->m_viewPunchAngle = g_ctx.local()->m_viewPunchAngle();
	data->m_vecViewOffset = g_ctx.local()->m_vecViewOffset();

	data->m_velocity_modifier = g_ctx.local()->m_flVelocityModifier();
}

void engineprediction::restore_netvars()
{
	auto data = &netvars_data[(m_clientstate()->iCommandAck - 1) % MULTIPLAYER_BACKUP]; //-V807

	if (data->tickbase != g_ctx.local()->m_nTickBase()) //-V807
		return;

	auto aim_punch_angle_delta = g_ctx.local()->m_aimPunchAngle() - data->m_aimPunchAngle;
	auto aim_punch_angle_vel_delta = g_ctx.local()->m_aimPunchAngleVel() - data->m_aimPunchAngleVel;
	auto view_punch_angle_delta = g_ctx.local()->m_viewPunchAngle() - data->m_viewPunchAngle;
	auto view_offset_delta = g_ctx.local()->m_vecViewOffset() - data->m_vecViewOffset;
	auto modifier_delta = g_ctx.local()->m_flVelocityModifier() - data->m_velocity_modifier;

	if (fabs(aim_punch_angle_delta.x) < 0.03125f &&
		fabs(aim_punch_angle_delta.y) < 0.03125f &&
		fabs(aim_punch_angle_delta.z) < 0.03125f)
		g_ctx.local()->m_aimPunchAngle() = data->m_aimPunchAngle;

	if (fabs(aim_punch_angle_vel_delta.x) < 0.03125f &&
		fabs(aim_punch_angle_vel_delta.y) < 0.03125f &&
		fabs(aim_punch_angle_vel_delta.z) < 0.03125f)
		g_ctx.local()->m_aimPunchAngleVel() = data->m_aimPunchAngleVel;

	if (fabs(view_punch_angle_delta.x) < 0.03125f &&
		fabs(view_punch_angle_delta.y) < 0.03125f &&
		fabs(view_punch_angle_delta.z) < 0.03125f)
		g_ctx.local()->m_viewPunchAngle() = data->m_viewPunchAngle;

	if (fabs(view_offset_delta.x) < 0.03125f &&
		fabs(view_offset_delta.y) < 0.03125f &&
		fabs(view_offset_delta.z) < 0.03125f)
		g_ctx.local()->m_vecViewOffset() = data->m_vecViewOffset;

	if (std::abs(modifier_delta) < 0.03125f)
		g_ctx.local()->m_flVelocityModifier() = data->m_velocity_modifier;
}

void engineprediction::prestart()
{
	if (g_ctx.globals.last_velocity_modifier < 1.f)
		*(bool*)((uintptr_t)m_prediction() + 0x24) = true;

	bool valid{ m_clientstate()->iDeltaTick > 0 };
	int start = m_clientstate()->nLastCommandAck;
	int stop = m_clientstate()->nLastOutgoingCommand + m_clientstate()->iChokedCommands;

	m_prediction()->Update(m_clientstate()->iDeltaTick, true, start, stop);
}

void engineprediction::setup()
{
	if (prediction_data.prediction_stage != SETUP)
		return;

	backup_data.flags = g_ctx.local()->m_fFlags(); //-V807
	backup_data.velocity = g_ctx.local()->m_vecVelocity();

	prediction_data.old_curtime = m_globals()->m_curtime; //-V807
	prediction_data.old_frametime = m_globals()->m_frametime;

	m_globals()->m_curtime = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase);
	m_globals()->m_frametime = m_prediction()->EnginePaused ? 0.0f : m_globals()->m_intervalpertick;

	prediction_data.prediction_stage = PREDICT;
}

void engineprediction::predict(CUserCmd* m_pcmd)
{
	if (prediction_data.prediction_stage != PREDICT)
		return;

	const auto backup_footsteps = m_cvar()->FindVar(crypt_str("sv_footsteps"))->GetInt();
	m_cvar()->FindVar(crypt_str("sv_footsteps"))->m_nFlags &= ~(1 << 14);
	m_cvar()->FindVar(crypt_str("sv_footsteps"))->m_nFlags &= ~(1 << 8);
	m_cvar()->FindVar(crypt_str("sv_footsteps"))->SetValue(0);

	bool valid{ m_clientstate()->iDeltaTick > 0 };
	int start = m_clientstate()->nLastCommandAck;
	int stop = m_clientstate()->nLastOutgoingCommand + m_clientstate()->iChokedCommands;

	m_prediction()->Update(m_clientstate()->iDeltaTick, valid, start, stop);

	if (!prediction_data.prediction_random_seed)
		prediction_data.prediction_random_seed = *reinterpret_cast <int**> (util::FindSignature(crypt_str("client.dll"), crypt_str("A3 ? ? ? ? 66 0F 6E 86")) + 0x1);

	*prediction_data.prediction_random_seed = MD5_PseudoRandom(m_pcmd->m_command_number) & INT_MAX;

	if (!prediction_data.prediction_player)
		prediction_data.prediction_player = *reinterpret_cast <int**> (util::FindSignature(crypt_str("client.dll"), crypt_str("89 35 ? ? ? ? F3 0F 10 48")) + 0x2);

	*prediction_data.prediction_player = reinterpret_cast <int> (g_ctx.local());

	m_movehelper()->set_host(g_ctx.local());
	m_gamemovement()->StartTrackPredictionErrors(g_ctx.local());

	static auto m_nImpulse = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_nImpulse"));
	static auto m_nButtons = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_nButtons"));
	static auto m_afButtonLast = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_afButtonLast"));
	static auto m_afButtonPressed = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_afButtonPressed"));
	static auto m_afButtonReleased = util::find_in_datamap(g_ctx.local()->GetPredDescMap(), crypt_str("m_afButtonReleased"));

	if (m_pcmd->m_impulse)
		*reinterpret_cast<uint32_t*>(uint32_t(g_ctx.local()) + m_nImpulse) = m_pcmd->m_impulse;

	CMoveData move_data;
	memset(&move_data, 0, sizeof(CMoveData));

	m_prediction()->SetupMove(g_ctx.local(), m_pcmd, m_movehelper(), &move_data);
	m_gamemovement()->ProcessMovement(g_ctx.local(), &move_data);

	move_data.m_bGameCodeMovedPlayer = false;

	m_prediction()->FinishMove(g_ctx.local(), m_pcmd, &move_data);
	m_movehelper()->process_impacts();
	m_gamemovement()->FinishTrackPredictionErrors(g_ctx.local());
	m_movehelper()->set_host(nullptr);

	prediction_data.prediction_stage = FINISH;
}

void engineprediction::update_viewmodel_data()
{
	auto viewmodel = g_ctx.local()->m_hViewModel().Get();

	if (viewmodel)
	{
		viewmodel_data.weapon = viewmodel->m_hWeapon().Get();

		viewmodel_data.viewmodel_index = viewmodel->m_nViewModelIndex();
		viewmodel_data.sequence = viewmodel->m_nSequence();
		viewmodel_data.animation_parity = viewmodel->m_nAnimationParity();

		viewmodel_data.cycle = viewmodel->m_flCycle();
		viewmodel_data.animation_time = viewmodel->m_flAnimTime();
	}
}

void engineprediction::finish()
{
	if (prediction_data.prediction_stage != FINISH)
		return;

	m_gamemovement()->FinishTrackPredictionErrors(g_ctx.local());
	m_movehelper()->set_host(nullptr);

	*prediction_data.prediction_random_seed = -1;
	*prediction_data.prediction_player = 0;

	m_globals()->m_curtime = prediction_data.old_curtime;
	m_globals()->m_frametime = prediction_data.old_frametime;
}