#pragma once
#include "Vector2.h"
#include "Map.h"

class LiftGimmickBlock {
public:
	Vector2 pos_;
	int linkId_; // LDtkで設定したIntegerの値
	float speed_;
	bool isActive_;
	Vector2 startPos_;

	LiftGimmickBlock();
	~LiftGimmickBlock();

	void Initialize();
	void Update();
	void Draw(Vector2 offset);
};

class LiftGimmickButton {
public:

};