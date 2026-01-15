#pragma once
#include "Vector2.h"

class ScrollCamera {
public:
	// コンストラクタ
	ScrollCamera();

	// デストラクタ
	~ScrollCamera();

	void Update(Vector2& playerPos);

	// ゲッター
	Vector2 GetOffset() const {
		return offset_;
	} // 描画時にこの値を引き算しちゃうわよ～

	  // 後から注視点を変えられるようにセッターを用意
	void SetTargetY(float y) { targetY_ = y; }
private:
	// 描画をずらす量
	Vector2 offset_;

	// ゲーム画面（表示領域）の幅
	const Vector2 kScreenSize = {
		1400.0f,
		1080.0f
	}; 

	// プレイヤーを画面のどの高さに維持するか
	float targetY_ = 800.0f;
};

