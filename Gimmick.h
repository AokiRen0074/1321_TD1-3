#pragma once
#include "Vector2.h"
// #include "Map.h"
// #include "Player.h"

class Player;

class LiftGimmickBlock {
public:
	Vector2 pos_;
	int linkId_; // LDtkで設定したIntegerの値
	float speed_;
	bool isActive_; // ボタンが押されて処理が動くか
	bool isRunning_; // 動いている最中か
	bool isReturning_;
	Vector2 startPos_;
	Vector2 size_ = {64.0f, 64.0f};

	// どこまで動くか
	Vector2 moveLimit_;

	LiftGimmickBlock();
	~LiftGimmickBlock();

	void Initialize(Vector2 pos, int linkId, Vector2 moveLimit, float speed);

	void Update();
	void Draw(Vector2 offset);

	void CheckCollision(Player& player);
};

class LiftGimmickButton {
public:
	Vector2 pos_;
	int linkId_; // LDtkで設定したIntegerの値
	Vector2 size_ = {32.0f, 32.0f};
	bool isPressed_;

	LiftGimmickButton();
	~LiftGimmickButton();

	void Initialize(Vector2 pos, Vector2 size, int linkId);
	void Update();
	void Draw(Vector2 offset);

	void CheckCollision(Player& player);
};