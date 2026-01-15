#include "Map.h"
#include <Novice.h>
#include <fstream>
#include "json.hpp" 
#include "Vector2.h"

using json = nlohmann::json;

Map::Map() {
	// 初期化（全部0にする）
	for (int y = 0; y < kMapHeight; y++) {
		for (int x = 0; x < kMapWidth; x++) {
			mapData[y][x] = 0;
		}
	}

	// 画像配列の初期化
	for (int i = 0;i < kMaxBlocksType;i++) {
		blockTextures[i] = 0;
	}
}

void Map::Initialize() {
	/*--------------------------
	ここでブロックの画像を貼ってね
	-----------------------------*/
	blockTextures[1] = Novice::LoadTexture("./block.png");
	blockTextures[2] = Novice::LoadTexture("./halfBlock.png");// ハーフブロック用
	blockTextures[3] = Novice::LoadTexture("./halfBlock.png");// ルーター用

}

// LDtk読み込み
void Map::LoadMapFromLDtk(const char* fileName, const std::vector<std::string>& layerNames) {
	std::ifstream file(fileName);
	if (!file.is_open()) return;

	json data;
	file >> data;

	// いったんマップをクリア
	for (int y = 0; y < kMapHeight; y++) {
		for (int x = 0; x < kMapWidth; x++) {
			mapData[y][x] = 0;
		}
	}

	auto& level = data["levels"][0];
	auto& layers = level["layerInstances"];

	doors.clear();
	buttons.clear();

	// LDtkの全レイヤーをチェック
	for (auto& layer : layers) {
		std::string currentLayerName = layer["__identifier"];
		std::string layerType = layer["__type"];

		// 読み込みたいレイヤー名リストの中に、今のレイヤー名が含まれているか確認
		bool isTargetLayer = false;
		for (const std::string& target : layerNames) {
			if (currentLayerName == target) {
				isTargetLayer = true;
				break;
			}
		}

		// 対象のレイヤーならデータを読み込む
		if (isTargetLayer) {
			auto& csvData = layer["intGridCsv"];
			for (int i = 0; i < csvData.size(); i++) {
				int id = csvData[i];

				// 0(空気)じゃない場合のみ書き込む
				// これにより、「Ground」の上に「Item」が重なっていても消えずに合成されます
				if (id != 0) {
					int x = i % kMapWidth;
					int y = i / kMapWidth;
					if (y < kMapHeight && x < kMapWidth) {
						mapData[y][x] = id;
					}
				}
			}
		}

		if (layerType == "Entities") {
			auto& entities = layer["entityInstances"];

			for (auto& entity : entities) {
				std::string id = entity["__identifier"]; // "Door" とか "Button" とか

				// 座標取得 (LDtkは左上が原点)
				float px = (float)entity["px"][0];
				float py = (float)entity["px"][1];

				// --- フィールド（Integer型）の値を取得する処理 ---
				int linkId = 0; // デフォルト値

				// "fieldInstances" の中から、設定したIntegerフィールドを探す
				for (auto& field : entity["fieldInstances"]) {
					// LDtkで設定したフィールド名（例: "LinkID"）
					if (field["__identifier"] == "LinkID") {
						// 値が入っていれば取得
						if (!field["__value"].is_null()) {
							linkId = field["__value"];
						}
					}
				}

				if (id == "Door") {
					Door newDoor;
					newDoor.pos = { px, py };
					newDoor.linkId = linkId;
					newDoor.isOpen = false;
					doors.push_back(newDoor);
				} else if (id == "Button") {
					ButtonA newButton;
					newButton.pos = { px, py };
					newButton.linkId = linkId;
					newButton.isPressed = false;
					buttons.push_back(newButton);
				}
			}

		}




	}
}

void Map::Draw(Vector2 offset) {
	for (int y = 0; y < kMapHeight; y++) {
		for (int x = 0; x < kMapWidth; x++) {

			int id = mapData[y][x];
			if (id > 0 && id < kMaxBlocksType && blockTextures[id] != 0) {

				Novice::DrawSprite(
					(int)(x * kTileSize - offset.x),
					(int)(y * kTileSize - offset.y),
					blockTextures[id], 
					1.0f, 1.0f, 0.0f, 0xFFFFFFFF
				);
			}

			//ルーターのデバック表示
			if (mapData[y][x] == 3) {
			
				Novice::DrawBox(
					(int)(x * kTileSize - offset.x), 
					(int)(y * kTileSize - offset.y),
					kTileSize, kTileSize,
					0.0f,
					RED,
					kFillModeSolid
				);

			}

		}

		for (const auto& door : doors) {
			// 描画座標の計算（ワールド座標 - カメラオフセット）
			int drawX = (int)(door.pos.x - offset.x);
			int drawY = (int)(door.pos.y - offset.y);

			// 開いているかどうかの色分け（開いたら緑、閉じてたら青）
			// ※ Noviceで定義されている色定数を使っています
			unsigned int color = door.isOpen ? GREEN : BLUE;

			Novice::DrawBox(
				drawX, drawY,
				kTileSize, kTileSize, // タイルと同じ大きさ
				0.0f,
				color,
				kFillModeSolid
			);

			// デバッグ用: リンクIDを画面左上に表示して確認したい場合
			// Novice::ScreenPrintf(0, 0, "Door ID:%d at (%d,%d)", door.linkId, (int)door.pos.x, (int)door.pos.y);
		}


		// ボタンの描画
		// (Map.hで定義した buttons リストをループ)
		for (const auto& button : buttons) {
			// 描画座標の計算
			int drawX = (int)(button.pos.x - offset.x);
			int drawY = (int)(button.pos.y - offset.y);

			// 押されているかどうかの色分け（押されたら黄色、未踏なら赤）
			unsigned int color = button.isPressed ? BLACK : RED;

			// ボタンは少し小さく表示して、ドアと区別しやすくする
			int btnSize = kTileSize / 2;
			int offsetSize = (kTileSize - btnSize) / 2; // 中央寄せ用のオフセット

			Novice::DrawBox(
				drawX + offsetSize, drawY + offsetSize, // 少しずらして中央に
				btnSize, btnSize,
				0.0f,
				color,
				kFillModeSolid
			);

			// デバッグ用: リンクIDを確認したい場合
			// Novice::ScreenPrintf(0, 20, "Button ID:%d", button.linkId);
		}

		// Map.cpp の Draw内に追加してデバッグ
		Novice::ScreenPrintf(0, 100, "MapData[10][10]: %d", mapData[10][10]);
	}
}