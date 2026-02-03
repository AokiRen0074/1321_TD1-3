#include "Router.h"
#include "Map.h"
#include <vector>    
#include"Novice.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h> // rand用

Router::Router(int id, int mapData[kMapHeight][kMapWidth]) {
	InitRouter(id, mapData);
	frameCounter = 0; 
	// 画像を配列に読み込む
	electricGraphHandles[0] = Novice::LoadTexture("./Images/DenkiBiribiri1.png");
	electricGraphHandles[1] = Novice::LoadTexture("./Images/DenkiBiribiri2.png");
	electricGraphHandles[2] = Novice::LoadTexture("./Images/DenkiBiribiri3.png");
	electricGraphHandles[3] = Novice::LoadTexture("./Images/DenkiBiribiri4.png");
}


void Router::InitRouter(int id, int mapData[kMapHeight][kMapWidth]) {
	id_ = id;
	int idCount = 0;
	router_.radius = 500.0f;
	router_.bigRadius = 1300.0f;
	router_.pos = { -100000.0f,-100000.0f };

	switch (id_)
	{

	case 0://ルーターの効果範囲の設定
		router_.radius = 1000.0f;
		router_.bigRadius = 1300.0f;

		break;
	case 3://ルーターの効果範囲の設定
		router_.radius = 600.0f;
		router_.bigRadius = 1300.0f;

		break;

		case 6:
			router_.radius = 600.0f;
			router_.bigRadius = 1300.0f;
			break;

	default:
		break;
	}

	for (int y = 0; y < kMapHeight; y++) {
		for (int x = 0; x < kMapWidth; x++) {
			if (mapData[y][x] == 3) {
				if (idCount == id_) {
					router_.pos.x = (float)x * kTileSize + (kTileSize / 2.0f);
					router_.pos.y = (float)y * kTileSize + (kTileSize / 2.0f);
					return;
				}
				idCount++;
			}
		}
	}
}

void Router::UpdateRouter(int mapData[kMapHeight][kMapWidth]) {
	int idCount = 0;
	frameCounter++;

	for (int y = 0; y < kMapHeight; y++) {
		for (int x = 0; x < kMapWidth; x++) {
			if (mapData[y][x] == 3) {
				if (idCount == id_) {
					// 配列の添字(x, y)にチップサイズを掛けて座標に変換
					router_.pos.x = (float)x * kTileSize + (kTileSize / 2.0f); // 中心に合わせるなら半分足す
					router_.pos.y = (float)y * kTileSize + (kTileSize / 2.0f);
					return;
				}
				idCount++;
			}
		}
	}

}

//void Router::DrawRouter(Vector2 offset) {
//	//ルーターのデバック表示
//
//	Novice::DrawEllipse(
//		(int)(router_.pos.x - offset.x), (int)(router_.pos.y - offset.y),
//		(int)router_.bigRadius, (int)router_.bigRadius,
//		0.0f,
//		0xFF00FF0f,
//		kFillModeSolid
//	);
//
//	Novice::DrawEllipse(
//		(int)(router_.pos.x - offset.x), (int)(router_.pos.y - offset.y),
//		(int)router_.radius, (int)router_.radius,
//		0.0f,
//		0xFFFFFF10,
//		kFillModeSolid
//	);
//
//	
//}

//void Router::DrawRouter(Vector2 offset) {
//	// 1. 本来あるべき「スケール」を計算
//	// 画像そのものが 64px なので、直径(radius * 2)を 64 で割る
//	const float kBaseSize = 64.0f;
//	float currentScale = (router_.radius * 2.0f) / kBaseSize;
//
//	// 2. 中心座標
//	float centerX = router_.pos.x - offset.x;
//	float centerY = router_.pos.y - offset.y;
//
//	// 3. 描画開始位置（左上）の計算
//	// 中心から「半径（拡大後のサイズ）」を引く
//	float drawX = centerX - router_.radius;
//	float drawY = centerY - router_.radius;
//
//	// 4. アニメーション（4枚の配列のインデックスを切り替え）
//	int animeIndex = (frameCounter / 10) % 4;
//
//	// --- 描画 ---
//
//	// デバッグ用の円（これと画像が重なるか確認）
//	Novice::DrawEllipse(
//		(int)centerX, (int)centerY,
//		(int)router_.radius, (int)router_.radius,
//		0.0f, 0xFFFFFF33, kFillModeSolid
//	);
//
//	// 画像の描画（DrawSpriteを使用）
//	if (electricGraphHandles[animeIndex] > 0) {
//		Novice::DrawSprite(
//			(int)drawX, (int)drawY,
//			electricGraphHandles[animeIndex], // 今のフレームの画像
//			currentScale, currentScale,       // 縦横均等に拡大
//			0.0f,
//			0xFFFFFFFF
//		);
//	}
//}

void Router::DrawRouter(Vector2 offset) {
	// 中心座標
	float centerX = router_.pos.x - offset.x;
	float centerY = router_.pos.y - offset.y;

	// --- ビリビリ線の描画設定 ---
	const int kSegments = 40; // 円を何分割するか（多いほど滑らか）
	const float kAmplitude = 20.0f; // 揺れの大きさ（ビリビリの激しさ）

	// 分割点ごとの座標を格納する配列
	Vector2 points[kSegments + 1];

	for (int i = 0; i <= kSegments; i++) {
		// 現在の角度（ラジアン）
		float theta = (2.0f * (float)M_PI * i) / (float)kSegments;

		// 半径にランダムな揺れを加える
		// frameCounterを使うと、毎フレーム形が変わってバチバチ動く
		float noise = (float)(rand() % 100) / 100.0f * kAmplitude;
		float r = router_.radius + noise;

		points[i].x = centerX + cosf(theta) * r;
		points[i].y = centerY + sinf(theta) * r;
	}

	// --- 点と点を線でつなぐ ---
	for (int i = 0; i < kSegments; i++) {
		Novice::DrawLine(
			(int)points[i].x, (int)points[i].y,
			(int)points[i + 1].x, (int)points[i + 1].y,
			0x00FFFFFF // 白色の線
		);
	}

	// デバッグ用の静かな円も薄く描いておくと範囲がわかりやすい
	Novice::DrawEllipse(
		(int)centerX, (int)centerY,
		(int)router_.radius, (int)router_.radius,
		0.0f, 0xFFFFFF11, kFillModeWireFrame
	);
}