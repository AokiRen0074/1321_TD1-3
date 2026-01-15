#include "Title.h"
#include <Novice.h>

// コンストラクタ
Title::Title() {
}

// デストラクタ
Title::~Title() {
}

// 更新
void Title::Update() {
}

// 描画
void Title::Draw() {
	Novice::ScreenPrintf(600, 400, "Title");
}