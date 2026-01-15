#pragma once
#include "Vector2.h"

class Collision {
public:
    // 矩形同士の当たり判定 (AABB)
	static bool RectRect(
		float ax1, float ay2, float ax2, float ay1,
		float bx1, float by2, float bx2, float by1
	);

	// 矩形同士の当たり判定 (座標版) RectRect->CheckRect
	static bool CheckRect(
		const Vector2& posA, float widthA, float heightA,
		const Vector2& posB, float widthB, float heightB
	);

	// 円と円
	static bool CircleCircle(
		const Vector2& aPos, float aRadius,
		const Vector2& bPos, float bRadius
	);
};