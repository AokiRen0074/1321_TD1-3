#include "Router.h"
#include "Map.h"
#include <vector>    
#include"Novice.h"
Router::Router(int id, int mapData[kMapHeight][kMapWidth]) {
	InitRouter(id, mapData);
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

void Router::DrawRouter(Vector2 offset) {
	//ルーターのデバック表示

	Novice::DrawEllipse(
		(int)(router_.pos.x - offset.x), (int)(router_.pos.y - offset.y),
		(int)router_.bigRadius, (int)router_.bigRadius,
		0.0f,
		0xFF00FF0f,
		kFillModeSolid
	);

	Novice::DrawEllipse(
		(int)(router_.pos.x - offset.x), (int)(router_.pos.y - offset.y),
		(int)router_.radius, (int)router_.radius,
		0.0f,
		0xFFFFFF10,
		kFillModeSolid
	);
}

