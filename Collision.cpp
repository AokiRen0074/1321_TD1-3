#include "Collision.h"
#include <math.h>

/*------------------------------------------
矩形と矩形
------------------------------------------*/
bool Collision::RectRect(
	float ax1, float ay2, float ax2, float ay1,
	float bx1, float by2, float bx2, float by1
) {
	// Aの左 < Bの右 && Aの右 > Bの左 && Aの下 > Bの上 && Aの上 < Bの下
	return
		(ax1 < bx2) &&
		(ax2 > bx1) &&
		(ay2 > by1) &&
		(ay1 < by2);
}

// 座標に計算しなおす
bool Collision::CheckRect(
	const Vector2& posA, float widthA, float heightA,
	const Vector2& posB, float widthB, float heightB
) {
	// 座標(Top-Left)とサイズから、RectRectに必要な4点を計算して渡す
	float ax1 = posA.x;
	float ay1 = posA.y;
	float ax2 = posA.x + widthA;
	float ay2 = posA.y + heightA;

	float bx1 = posB.x;
	float by1 = posB.y;
	float bx2 = posB.x + widthB;
	float by2 = posB.y + heightB;

	return RectRect(ax1, ay2, ax2, ay1, bx1, by2, bx2, by1);
}

/*------------------------------------------
円と円
------------------------------------------*/
bool Collision::CircleCircle(
	const Vector2& aPos, float aRadius,
	const Vector2& bPos, float bRadius
) {
	float dx = aPos.x - bPos.x;
	float dy = aPos.y - bPos.y;
	float r = aRadius + bRadius;

	return (dx * dx + dy * dy) <= (r * r);
}