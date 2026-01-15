#pragma once
#include <cmath>
#include <math.h>

// スクリーンの大きさ
/*
const float kScreenWidth_ = 780.0f;
const float kScreenHeight_ = 780.0f;
*/

// ベクトル
class Vector2
{
public:
	float x;
	float y;

	// 引数なし
	Vector2();

	Vector2(float x, float y);

	static Vector2 Nomalize(const Vector2& v);
};