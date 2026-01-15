#include "ScrollCamera.h"
#include <Novice.h>
#include "Game.h"
#include "const.h" // マップ計算
#include "Vector2.h"
#include "const.h"

// コンストラクタ
ScrollCamera::ScrollCamera() : offset_({0, 0}) {
}

// デストラクタ
ScrollCamera::~ScrollCamera() {
}

// 更新
void ScrollCamera::Update(Vector2& playerPos) {
	// プレイヤーがゲーム面中央に来るようにオフセットを計算
	// offset=プレイヤー位置-画面の半分
	offset_.x = playerPos.x - (kScreenSize.x / 2.0f);
	offset_.y = playerPos.y - targetY_;

	// マップ全体の横幅を計算 (例: 100枚 * 64px = 6400px)
	Vector2 mapWorld = {
		static_cast<float>(kMapWidth * kTileSize),
		static_cast<float>(kMapHeight * kTileSize)
	};

	// スクロールの限界値 (マップの端 - 画面のサイズ)
	Vector2 maxScroll = {
		mapWorld.x - kScreenSize.x,
		mapWorld.y - kScreenSize.y
	};

	// X軸
	if (offset_.x < 0) offset_.x = 0;
	if (offset_.x > maxScroll.x) {
		offset_.x = maxScroll.x;
	}

	// Y軸
	if (offset_.y < 0) offset_.y = 0;
	if(offset_.y > maxScroll.y) {
		offset_.y = maxScroll.y;
	}
}