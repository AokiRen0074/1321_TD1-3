#include "Gimmick.h"

LiftGimmickBlock::LiftGimmickBlock() {
}

LiftGimmickBlock::~LiftGimmickBlock() {
}

void LiftGimmickBlock::Initialize() {
	pos_ = {0.0f, 0.0f};
	linkId_ = 0;
	isActive_ = false;
	speed_ = 2.0f;
	startPos_ = pos_;
}