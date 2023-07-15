// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "other_esp.h"
#include "..\autowall\autowall.h"
#include "..\ragebot\antiaim.h"
#include "..\misc\logs.h"
#include "..\misc\misc.h"
#include "..\lagcompensation\local_animations.h"

bool can_penetrate(weapon_t* weapon)
{
	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return false;

	Vector view_angles;
	m_engine()->GetViewAngles(view_angles);

	Vector direction;
	math::angle_vectors(view_angles, direction);

	CTraceFilter filter;
	filter.pSkip = g_ctx.local();

	trace_t trace;
	util::trace_line(g_ctx.globals.eye_pos, g_ctx.globals.eye_pos + direction * weapon_info->flRange, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);

	if (trace.fraction == 1.0f) //-V550
		return false;

	auto eye_pos = g_ctx.globals.eye_pos;
	auto hits = 1;
	auto damage = (float)weapon_info->iDamage;
	auto penetration_power = weapon_info->flPenetration;

	static auto damageReductionBullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
	static auto damageBulletPenetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

	return autowall::get().handle_bullet_penetration(weapon_info, trace, eye_pos, direction, hits, damage, penetration_power, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat());
}

void otheresp::penetration_reticle()
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.penetration_reticle)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	const auto weapon_info = weapon->get_csweapon_info();
	if (!weapon_info)
		return;

	CTraceFilter filter;
	filter.pSkip = g_ctx.local();

	Vector view_angles;
	m_engine()->GetViewAngles(view_angles);

	Vector direction;
	math::angle_vectors(view_angles, direction);

	trace_t trace;
	util::trace_line(g_ctx.globals.eye_pos, g_ctx.globals.eye_pos + direction * weapon_info->flRange, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);

	if (trace.fraction == 1.0f)
		return;

	auto color = Color(34, 139, 34, 179);

	if (!weapon->is_non_aim() && weapon->m_iItemDefinitionIndex() != WEAPON_TASER && can_penetrate(weapon))
		color = Color(237, 42, 28, 179);

	float angle_z = math::dot_product(Vector(0, 0, 1), trace.plane.normal);
	float invangle_z = math::dot_product(Vector(0, 0, -1), trace.plane.normal);
	float angle_y = math::dot_product(Vector(0, 1, 0), trace.plane.normal);
	float invangle_y = math::dot_product(Vector(0, -1, 0), trace.plane.normal);
	float angle_x = math::dot_product(Vector(1, 0, 0), trace.plane.normal);
	float invangle_x = math::dot_product(Vector(-1, 0, 0), trace.plane.normal);


	if (angle_z > 0.5 || invangle_z > 0.5)
		render::get().filled_rect_world(trace.endpos, Vector2D(5, 5), color, 0);
	else if (angle_y > 0.5 || invangle_y > 0.5)
		render::get().filled_rect_world(trace.endpos, Vector2D(5, 5), color, 1);
	else if (angle_x > 0.5 || invangle_x > 0.5)
		render::get().filled_rect_world(trace.endpos, Vector2D(5, 5), color, 2);
}

void render::filled_rect_world(Vector center, Vector2D size, Color color, int angle) {
	Vector top_left, top_right, bot_left, bot_right;

	switch (angle) {
	case 0: // Z
		top_left = Vector(-size.x, -size.y, 0);
		top_right = Vector(size.x, -size.y, 0);

		bot_left = Vector(-size.x, size.y, 0);
		bot_right = Vector(size.x, size.y, 0);

		break;
	case 1: // Y
		top_left = Vector(-size.x, 0, -size.y);
		top_right = Vector(size.x, 0, -size.y);

		bot_left = Vector(-size.x, 0, size.y);
		bot_right = Vector(size.x, 0, size.y);

		break;
	case 2: // X
		top_left = Vector(0, -size.y, -size.x);
		top_right = Vector(0, -size.y, size.x);

		bot_left = Vector(0, size.y, -size.x);
		bot_right = Vector(0, size.y, size.x);

		break;
	}

	//top line
//    Vector c_top_left = center + add_top_left;
	Vector c_top_left = center + top_left;
	Vector c_top_right = center + top_right;

	//bottom line
	Vector c_bot_left = center + bot_left;
	Vector c_bot_right = center + bot_right;

	Vector m_flTopleft, m_flTopRight, m_flBotLeft, m_flBotRight;
	//your standard world to screen if u need one just grab from a past
	if (math::world_to_screen(c_top_left, m_flTopleft) && math::world_to_screen(c_top_right, m_flTopRight) &&
		math::world_to_screen(c_bot_left, m_flBotLeft) && math::world_to_screen(c_bot_right, m_flBotRight)) {

		Vertex_t vertices[4];
		//static int m_flTexID = g_pSurface->CreateNewTextureID(true);
		static int m_flTexID = m_surface()->CreateNewTextureID(true);
		m_surface()->DrawSetTexture(m_flTexID);
		m_surface()->DrawSetColor(color);

		vertices[0].Init(Vector2D(m_flTopleft.x, m_flTopleft.y));
		vertices[1].Init(Vector2D(m_flTopRight.x, m_flTopRight.y));
		vertices[2].Init(Vector2D(m_flBotRight.x, m_flBotRight.y));
		vertices[3].Init(Vector2D(m_flBotLeft.x, m_flBotLeft.y));

		m_surface()->DrawTexturedPolygon(4, vertices, true);
	}
}

void otheresp::indicators()
{
	if (!g_cfg.menu.keybinds)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	int r = g_cfg.menu.menu_theme.r();
	int g = g_cfg.menu.menu_theme.g();
	int b = g_cfg.menu.menu_theme.b();
	int a = g_cfg.menu.menu_theme.a();

	int x = g_cfg.menu.keybind_x;
	int y = g_cfg.menu.keybind_y;

	int offset = 1;

	render::get().rect_filled(x + 2, y / 2 + 10, 100, 5 + 15 * offset, { 10, 10, 10, 215 });

	render::get().gradient(x + 52, y / 2 + 10, 50, 2, Color(r, g, b, a), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
	render::get().gradient(x + 2, y / 2 + 10, 50, 2, Color(0, 0, 0, 0), Color(r, g, b, a), GRADIENT_HORIZONTAL);

	render::get().text(fonts[INDICATORFONT], x + 30, y / 2 + 14, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Keybinds");

	if (misc::get().double_tap_key)
	{
		render::get().text(fonts[INDICATORFONT], x + 24, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Double tap");
		offset = offset + 1;
	}

	if (misc::get().hide_shots_key)
	{
		render::get().text(fonts[INDICATORFONT], x + 26, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Hide shots");
		offset = offset + 1;
	}

	if (key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon))
	{
		render::get().text(fonts[INDICATORFONT], x + 29, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Min. dmg");
		offset = offset + 1;
	}

	if (key_binds::get().get_key_bind_state(17))
	{
		render::get().text(fonts[INDICATORFONT], x + 23, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Thirdperson");
		offset = offset + 1;
	}

	if (key_binds::get().get_key_bind_state(21))
	{
		render::get().text(fonts[INDICATORFONT], x + 28, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Slow walk");
		offset = offset + 1;
	}

	if (key_binds::get().get_key_bind_state(20))
	{
		render::get().text(fonts[INDICATORFONT], x + 25, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Fake duck");
		offset = offset + 1;
	}

	if (key_binds::get().get_key_bind_state(19))
	{
		render::get().text(fonts[INDICATORFONT], x + 25, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Edge jump");
		offset = offset + 1;
	}

	if (key_binds::get().get_key_bind_state(18))
	{
		render::get().text(fonts[INDICATORFONT], x + 26, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Quick Peek");
		offset = offset + 1;
	}

	if (key_binds::get().get_key_bind_state(16))
	{
		render::get().text(fonts[INDICATORFONT], x + 20, y / 2 + 15 + 15 * offset, { 255, 255, 255, 255 }, HFONT_CENTERED_NONE, "Desync Invert");
		offset = offset + 1;
	}
}

void otheresp::draw_indicators()
{
	if (!g_ctx.local()->is_alive()) //-V807
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto h = height - 325;

	for (auto& indicator : m_indicators)
	{
		render::get().text(fonts[INDICATORFONT], 27, h, indicator.m_color, HFONT_CENTERED_Y, indicator.m_text.c_str());
		h -= 25;
	}

	m_indicators.clear();
}

void otheresp::hitmarker_paint()
{
	if (!g_cfg.esp.hitmarker[0] && !g_cfg.esp.hitmarker[1])
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (!g_ctx.local()->is_alive())
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (hitmarker.hurt_time + 0.7f > m_globals()->m_curtime)
	{
		if (g_cfg.esp.hitmarker[0])
		{
			static int width, height;
			m_engine()->GetScreenSize(width, height);

			auto alpha = (int)((hitmarker.hurt_time + 0.7f - m_globals()->m_curtime) * 255.0f);
			hitmarker.hurt_color.SetAlpha(alpha);

			auto offset = 7.0f - (float)alpha / 255.0f * 7.0f;

			render::get().line(width / 2 + 5 + offset, height / 2 - 5 - offset, width / 2 + 12 + offset, height / 2 - 12 - offset, hitmarker.hurt_color);
			render::get().line(width / 2 + 5 + offset, height / 2 + 5 + offset, width / 2 + 12 + offset, height / 2 + 12 + offset, hitmarker.hurt_color);
			render::get().line(width / 2 - 5 - offset, height / 2 + 5 + offset, width / 2 - 12 - offset, height / 2 + 12 + offset, hitmarker.hurt_color);
			render::get().line(width / 2 - 5 - offset, height / 2 - 5 - offset, width / 2 - 12 - offset, height / 2 - 12 - offset, hitmarker.hurt_color);
		}

		if (g_cfg.esp.hitmarker[1])
		{
			Vector world;

			if (math::world_to_screen(hitmarker.point, world))
			{
				auto alpha = (int)((hitmarker.hurt_time + 0.7f - m_globals()->m_curtime) * 255.0f);
				hitmarker.hurt_color.SetAlpha(alpha);

				auto offset = 7.0f - (float)alpha / 255.0f * 7.0f;

				render::get().line(world.x + 5 + offset, world.y - 5 - offset, world.x + 12 + offset, world.y - 12 - offset, hitmarker.hurt_color);
				render::get().line(world.x + 5 + offset, world.y + 5 + offset, world.x + 12 + offset, world.y + 12 + offset, hitmarker.hurt_color);
				render::get().line(world.x - 5 - offset, world.y + 5 + offset, world.x - 12 - offset, world.y + 12 + offset, hitmarker.hurt_color);
				render::get().line(world.x - 5 - offset, world.y - 5 - offset, world.x - 12 - offset, world.y - 12 - offset, hitmarker.hurt_color);
			}
		}
	}
}

void otheresp::damage_marker_paint()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		if (damage_marker[i].hurt_time + 2.0f > m_globals()->m_curtime)
		{
			Vector screen;

			if (!math::world_to_screen(damage_marker[i].position, screen))
				continue;

			auto alpha = (int)((damage_marker[i].hurt_time + 2.0f - m_globals()->m_curtime) * 127.5f);
			damage_marker[i].hurt_color.SetAlpha(alpha);

			render::get().text(fonts[DAMAGE_MARKER], screen.x, screen.y, damage_marker[i].hurt_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, "%i", damage_marker[i].damage);
		}
	}
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device);

void otheresp::spread_crosshair(LPDIRECT3DDEVICE9 device)
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.show_spread)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (weapon->is_non_aim())
		return;

	int w, h;
	m_engine()->GetScreenSize(w, h);

	draw_circe((float)w * 0.5f, (float)h * 0.5f, g_ctx.globals.inaccuracy * 500.0f, 50, D3DCOLOR_RGBA(g_cfg.esp.show_spread_color.r(), g_cfg.esp.show_spread_color.g(), g_cfg.esp.show_spread_color.b(), g_cfg.esp.show_spread_color.a()), D3DCOLOR_RGBA(0, 0, 0, 0), device);
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device)
{
	LPDIRECT3DVERTEXBUFFER9 g_pVB2 = nullptr;
	std::vector <CUSTOMVERTEX2> circle(resolution + 2);

	circle[0].x = x;
	circle[0].y = y;
	circle[0].z = 0.0f;

	circle[0].rhw = 1.0f;
	circle[0].color = color2;

	for (auto i = 1; i < resolution + 2; i++)
	{
		circle[i].x = (float)(x - radius * cos(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].y = (float)(y - radius * sin(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].z = 0.0f;

		circle[i].rhw = 1.0f;
		circle[i].color = color;
	}

	device->CreateVertexBuffer((resolution + 2) * sizeof(CUSTOMVERTEX2), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB2, nullptr); //-V107

	if (!g_pVB2)
		return;

	void* pVertices;

	g_pVB2->Lock(0, (resolution + 2) * sizeof(CUSTOMVERTEX2), (void**)&pVertices, 0); //-V107
	memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(CUSTOMVERTEX2));
	g_pVB2->Unlock();

	device->SetTexture(0, nullptr);
	device->SetPixelShader(nullptr);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	device->SetStreamSource(0, g_pVB2, 0, sizeof(CUSTOMVERTEX2));
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);

	g_pVB2->Release();
}

void otheresp::automatic_peek_indicator()
{
	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	static auto position = ZERO;

	if (!g_ctx.globals.start_position.IsZero())
		position = g_ctx.globals.start_position;

	if (position.IsZero())
		return;

	static auto alpha = 4.0f;

	if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18) || alpha)
	{
		if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
			alpha += 9.0f * m_globals()->m_frametime; //-V807
		else
			alpha -= 9.0f * m_globals()->m_frametime;

		alpha = math::clamp(alpha, 0.0f, 1.0f);
		render::get().Draw3DFilledCircle(position, alpha * 20.f, g_ctx.globals.fired_shot ? Color(40, 220, 5, (int)(alpha * 135.0f)) : Color(200, 200, 200, (int)(alpha * 125.0f)));
		Vector screen;

		if (math::world_to_screen(position, screen))
		{
			static auto offset = 30.0f;

			if (!g_ctx.globals.fired_shot)
			{
				static auto switch_offset = false;

				if (offset <= 30.0f || offset >= 55.0f)
					switch_offset = !switch_offset;

				offset += switch_offset ? 22.0f * m_globals()->m_frametime : -22.0f * m_globals()->m_frametime;
				offset = math::clamp(offset, 30.0f, 55.0f);
			}
		}
	}
}