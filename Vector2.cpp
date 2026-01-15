#include "Vector2.h"

Vector2::Vector2()
	: x(0.0f)
	, y(0.0f) {
}

Vector2::Vector2(float x, float y)
	: x(x)
	, y(y) {
}

Vector2 Vector2::Nomalize(const Vector2& v)
{
	//長さを計算する
	float length = std::sqrt(v.x * v.x + v.y * v.y);

	//長さがゼロか確認する (0除算を防ぐため)
	if (length == 0.0f) {

		// 長さがゼロの場合は (0, 0) を返す
		return Vector2(0.0f, 0.0f);
	}

	//各成分を長さで割って新しいベクトルを返す
	return Vector2(v.x / length, v.y / length);
}