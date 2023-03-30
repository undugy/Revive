#pragma once
#include"util/vec3.h"
namespace CONST_VALUE
{
	const Vector3 g_ground_base_pos{ 2400.f, 300.f, 2850.f };
	const Vector3 PLAYER_SPAWN_POINT[3]{
	{2350.0f,300.0f,3150.0f},
	{2450.0f,300.0f,3150.0f},
	{2400.0f,300.0f,3150.0f}
	};
	const int ROUND_TIME = 30000;
	const int HEAL_TIME = 1000;
	const int ROUND_MAX = 3;
	const float HEURISTICS = 50.f;
	const int ATTACK_INTERVAL = 1000;
}