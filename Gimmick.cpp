#include "Gimmick.h"

LiftGimmickBlock::LiftGimmickBlock() {
}

LiftGimmickBlock::~LiftGimmickBlock() {
}

void LiftGimmickBlock::Initialize() {

	isActive_ = false;
	speed_ = 2.0f;
	startPos_ = pos_;
}