#pragma once
#include "Vector2.h"

class ScrollCamera {
public:
	// コンストラクタ
	ScrollCamera();

	// デストラクタ
	~ScrollCamera();

	void Update(Vector2& playerPos);

	// 各ステージのカメラ切り替えのyの高さ
	void SetStageIndex(int index) { 
		if (index >= 0 && index < 3) {
			currentStageIndex_ = index;
			targetY_ = stageYPositions_[index]; 
		}
	}

	int GetStageIndex() const { return currentStageIndex_; }

	float GetStageYPosition(int index) const {
		if (index >= 0 && index < 3) {
			return stageYPositions_[index];
		}
		return 0.0f;
	}

	// スクロールモードの切り替え
	void SetIsScrollMode(bool isScroll) { isScrollMode_ = isScroll; }

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

	// ステージごとの停止位置Y
	float stageYPositions_[3] = { 0.0f, 900.0f, 1920.0f };
	int currentStageIndex_ = 0;

	// プレイヤーを画面のどの高さに維持するか
	float targetY_ = 0.0f;

	bool isScrollMode_ = false;

	// スクロールは3200から
};

