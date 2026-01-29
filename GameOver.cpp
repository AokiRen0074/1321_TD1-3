#include "GameOver.h"
#include <Novice.h>
#include <vector>
#include <math.h>

//void GameOver::Reset() {
//	retryFlag = false;
//	internalTimer = 0;
//}

void GameOver::Update(char keys[], char preKeys[]) {
	internalTimer++;

	if (keys[DIK_RETURN] && !preKeys[DIK_RETURN]) {
		retryFlag = true;
	}
}

void GameOver::Draw() {
	// 背景を少し暗く
	Novice::DrawBox(0, 0, 1400, 1080, 0.0f, 0x000000AA, kFillModeSolid);

	// 4秒（約240フレーム）かけてアニメーション
	const float kAnimDuration = 240.0f;
	float globalProgress = (float)internalTimer / kAnimDuration;
	if (globalProgress > 1.0f) globalProgress = 1.0f;

	// 基準位置とサイズ
	float cx = 1400.0f / 2.0f - 300.0f; // 画面中央より少し左から開始
	float cy = 1080.0f / 2.0f;
	float size = 60.0f;     // 文字の大きさ
	float spacing = 140.0f; // 文字の間隔

	// ---------------------------------------------------------
	// 線が伸びて文字になる描画関数 (ラムダ式)
	// ---------------------------------------------------------
	auto DrawAnimatedChar = [&](char c, float x, float y, float s, float progress, unsigned int color) {

		// 線の「始点」と「終点」のリスト
		struct LineSeg {
			float lx, ly, rx, ry;
		};
		std::vector<LineSeg> segments;

		// 文字の形を定義（一筆書き順）
		switch (c) {
			case 'G':
				segments = {
					{x + s, y - s, x - s, y - s}, // 上
					{x - s, y - s, x - s, y + s}, // 左
					{x - s, y + s, x + s, y + s}, // 下
					{x + s, y + s, x + s, y},     // 右下
					{x + s, y, x, y}      // 中
				};
				break;
			case 'A':
				segments = {
					{x - s, y + s, x, y - s}, // 左斜め上
					{x, y - s, x + s, y + s}, // 右斜め下
					{x - s / 2, y, x + s / 2, y}  // 横棒
				};
				break;
			case 'M':
				segments = {
					{x - s, y + s, x - s, y - s}, // 左縦
					{x - s, y - s, x, y},     // 斜め下
					{x, y, x + s, y - s}, // 斜め上
					{x + s, y - s, x + s, y + s}  // 右縦
				};
				break;
			case 'E':
				segments = {
					{x + s, y - s, x - s, y - s}, // 上
					{x - s, y - s, x - s, y + s}, // 左縦
					{x - s, y + s, x + s, y + s}, // 下
					{x - s, y, x + s / 2, y}  // 中
				};
				break;
			case 'O':
				segments = {
					{x - s, y - s, x + s, y - s}, // 上
					{x + s, y - s, x + s, y + s}, // 右
					{x + s, y + s, x - s, y + s}, // 下
					{x - s, y + s, x - s, y - s}  // 左
				};
				break;
			case 'V':
				segments = {
					{x - s, y - s, x, y + s}, // 左斜め下
					{x, y + s, x + s, y - s}  // 右斜め上
				};
				break;
			case 'R':
				segments = {
					{x - s, y + s, x - s, y - s}, // 左縦
					{x - s, y - s, x + s, y - s}, // 上
					{x + s, y - s, x + s, y},     // 右丸み
					{x + s, y, x - s, y},     // 中
					{x - s, y, x + s, y + s}  // 斜め払い
				};
				break;
		}

		// --- 線を徐々に描く計算 ---
		float totalSegs = (float)segments.size();
		float progressPerSeg = 1.0f / totalSegs;

		for (int i = 0; i < segments.size(); i++) {
			// この線を描き始めるタイミング
			float startThreshold = i * progressPerSeg;
			// この線を描き終わるタイミング
			float endThreshold = (i + 1) * progressPerSeg;

			// まだ描く時間じゃないならスキップ
			if (progress < startThreshold) continue;

			// 完全に描き終わっている場合
			if (progress >= endThreshold) {
				Novice::DrawLine((int)segments[i].lx, (int)segments[i].ly, (int)segments[i].rx, (int)segments[i].ry, color);
			}
			// 描いている途中（補間）
			else {
				// 現在の線の中での進行度 (0.0 ~ 1.0)
				float localT = (progress - startThreshold) / progressPerSeg;

				float currentX = segments[i].lx + (segments[i].rx - segments[i].lx) * localT;
				float currentY = segments[i].ly + (segments[i].ry - segments[i].ly) * localT;

				Novice::DrawLine((int)segments[i].lx, (int)segments[i].ly, (int)currentX, (int)currentY, color);
			}
		}
		};

	// ---------------------------------------------------------
	// 文字を描画（ウェーブさせる）
	// ---------------------------------------------------------
	const char* str = "GAMEOVER";
	int len = 8;
	unsigned int col = RED; // 文字色

	for (int i = 0; i < len; i++) {
		// 文字ごとにタイミングをずらす (0.08秒ずつ遅らせる)
		float charStart = i * 0.08f;
		// 1文字が完成するのにかかる時間割合 (全体の40%)
		float charDuration = 0.4f;

		// 全体の進行度から、この文字の進行度を計算
		float localProgress = (globalProgress - charStart) / charDuration;

		if (localProgress < 0.0f) localProgress = 0.0f;
		if (localProgress > 1.0f) localProgress = 1.0f;

		// 表示位置（GAMEOVER の O以降は少し隙間をあける）
		float offsetX = (float)i * spacing;
		if (i >= 4) offsetX += 40.0f;

		DrawAnimatedChar(str[i], cx + offsetX, cy, size, localProgress, col);
	}

	if (internalTimer > 180) {
		Novice::ScreenPrintf(850, 800, "Press ENTER to Retry");
	}
}
