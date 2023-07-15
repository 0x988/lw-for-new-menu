// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "nightmode.h"

std::vector <MaterialBackup> materials;

inline void modern_clamp_world(float target, float to, float step)
{
	if (target > to)
		target = std::clamp(target - step, to, 1.f);
	else if (target < to)
		target = std::clamp(target + step, 0.f, to);
}

void nightmode::clear_stored_materials()
{
	materials.clear();
}

void nightmode::modulate(MaterialHandle_t i, IMaterial* material, bool backup = false)
{
	auto name = material->GetTextureGroupName();

	if (strstr(name, crypt_str("World")))
	{
		if (backup)
			materials.emplace_back(MaterialBackup(i, material));

		modern_clamp_world(g_cfg.esp.world_color[0], 255.f, m_globals()->m_frametime / 24.f);
		modern_clamp_world(g_cfg.esp.world_color[1], 255.f, m_globals()->m_frametime / 24.f);
		modern_clamp_world(g_cfg.esp.world_color[2], 255.f, m_globals()->m_frametime / 24.f);
		modern_clamp_world(g_cfg.esp.world_color[3], 255.f, m_globals()->m_frametime / 24.f);

		material->AlphaModulate((float)g_cfg.esp.world_color[3] / 255.0f);
		material->ColorModulate((float)g_cfg.esp.world_color[0] / 255.0f, (float)g_cfg.esp.world_color[1] / 255.0f, (float)g_cfg.esp.world_color[2] / 255.0f);
	}
	else if (strstr(name, crypt_str("StaticProp")))
	{
		if (backup)
			materials.emplace_back(MaterialBackup(i, material));

		material->AlphaModulate((float)g_cfg.esp.props_color.a() / 255.0f);
		material->ColorModulate((float)g_cfg.esp.props_color.r() / 255.0f, (float)g_cfg.esp.props_color.g() / 255.0f, (float)g_cfg.esp.props_color.b() / 255.0f);
	}
}

void nightmode::apply()
{
	if (!materials.empty())
	{
		for (auto i = 0; i < (int)materials.size(); i++) //-V202
			modulate(materials[i].handle, materials[i].material);

		return;
	}

	materials.clear();
	auto materialsystem = m_materialsystem();

	for (auto i = materialsystem->FirstMaterial(); i != materialsystem->InvalidMaterial(); i = materialsystem->NextMaterial(i))
	{
		auto material = materialsystem->GetMaterial(i);

		if (!material)
			continue;

		if (material->IsErrorMaterial())
			continue;

		modulate(i, material, true);
	}
}

void nightmode::remove()
{
	for (auto i = 0; i < materials.size(); i++)
	{
		if (!materials[i].material)
			continue;

		if (materials[i].material->IsErrorMaterial())
			continue;

		materials[i].restore();
		materials[i].material->Refresh();
	}

	materials.clear();
}