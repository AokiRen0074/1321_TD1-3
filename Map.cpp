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
	for (int i = 0; i < kMaxBlocksType; i++) {
		blockTextures[i] = 0;
	}
}

void Map::Initialize() {
	/*--------------------------
	ここでブロックの画像を貼ってね
	-----------------------------*/
	blockTextures[1] = Novice::LoadTexture("./Images./block.png");
	blockTextures[2] = Novice::LoadTexture("./halfBlock.png");// ハーフブロック用
	blockTextures[3] = Novice::LoadTexture("./halfBlock.png");// ルーター用

	buttonOffTexture = Novice::LoadTexture("./Images/Switch-Push-Blue.png");
	buttonOnTexture = Novice::LoadTexture("./Images/Switch-Blue.png");

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
	waters.clear();
	Beltconveyors.clear();
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
					if (field["__identifier"] == "Integer") {
						// 値が入っていれば取得
						if (!field["__value"].is_null()) {
							linkId = field["__value"];
						}
					}
				}

				// ここにギミックを追加する処理を書く
				if (id == "Door") {
					Door newDoor;
					newDoor.pos = { px, py };
					newDoor.linkId = linkId;
					newDoor.isOpen = false;
					doors.push_back(newDoor);
				}
				else if (id == "Button") {
					ButtonA newButton;
					newButton.pos = { px, py };
					newButton.linkId = linkId;
					newButton.isPressed = false;
					buttons.push_back(newButton);
				}
				else if (id == "Water") {
					Water newWater;
					newWater.pos = { px, py };
					newWater.linkId = linkId;
					newWater.isActive = true;
					waters.push_back(newWater);
				}
				else if (id == "Beltconveyor") {

					Beltconveyor nweBeltconveyor;
					nweBeltconveyor.pos = { px,py };
					nweBeltconveyor.speed = 6.0f;
					nweBeltconveyor.linkId = linkId;
					nweBeltconveyor.isReversed = true;
					Beltconveyors.push_back(nweBeltconveyor);
				}
				else if (id == "Checkpoint") {

					Checkpoint nweCheckpoint;
					nweCheckpoint.pos = { px,py };
					nweCheckpoint.linkId = linkId;
					nweCheckpoint.isActive = true;
					Checkpoints.push_back(nweCheckpoint);

				}
				else if (id == "VanishingFloor") {

					VanishingFloor nweVanishingFloor;
					nweVanishingFloor.pos = { px,py };
					nweVanishingFloor.linkId = linkId;
					nweVanishingFloor.isActive = true;
					VanishingFloors.push_back(nweVanishingFloor);

				}
				else if (id == "Block") {

					Block nweBlocks;
					nweBlocks.pos = { px,py };
					nweBlocks.linkId = linkId;
					nweBlocks.isActive = false;
					Blocks.push_back(nweBlocks);

				}

				if (id == "LiftGimmickBlock") {
					// --- フィールドから値を取得するための変数(デフォルト値を設定) ---
					Vector2 moveLimit = { 0.0f, 0.0f }; // 初期値
					float speed = 2.0f;                // 初期スピード
					// linkId は既に上の階層で取得済みと想定（あるいはここで再度取得）

					// entity["fieldInstances"] の中をループして設定した値を探す
					for (auto& field : entity["fieldInstances"]) {
						std::string fieldName = field["__identifier"];

						if (fieldName == "Integer") { // リンク用ID
							if (!field["__value"].is_null()) {
								linkId = field["__value"];
							}
						}
						else if (fieldName == "MoveX") { // 横移動量
							if (!field["__value"].is_null()) {
								moveLimit.x = field["__value"];
							}
						}
						else if (fieldName == "MoveY") { // 縦移動量
							if (!field["__value"].is_null()) {
								moveLimit.y = field["__value"];
							}
						}
						else if (fieldName == "Speed") { // スピード
							if (!field["__value"].is_null()) {
								speed = field["__value"];
							}
						}
					}

					// --- ここを新しい引数に合わせて修正 ---
					LiftGimmickBlock newLift;
					// 引数：座標, ID, 移動制限(Vector2), スピード(float)
					newLift.Initialize({ px, py }, linkId, moveLimit, speed);
					liftBlocks.push_back(newLift);
				}

				if (id == "LiftGimmickButton") {
					LiftGimmickButton newLiftButton;
					// 初期化（linkIdは既存の読み込み処理で取得済みのものを使用）
					newLiftButton.Initialize({ px, py }, { 32.0f, 32.0f }, linkId);
					liftButtons.push_back(newLiftButton);
				}
			}
		}
	}
}

void Map::Update(Player& player) {
	// リフトボタンの判定
	for (auto& button : liftButtons) {
		button.CheckCollision(player);
	}

	// リフトの更新
	for (auto& lift : liftBlocks) {
		bool linkedButtonIsPressed = false;

		for (auto& button : liftButtons) {
			if (lift.linkId_ == button.linkId_ && button.isPressed_) {
				linkedButtonIsPressed = true;
				break;
			}
		}

		// ボタンが押されている間だけリフトをアクティブにする
		lift.isActive_ = linkedButtonIsPressed;

		lift.Update();
		lift.CheckCollision(player); // ここで各リフトとプレイヤーを判定
	}

	// --- ブロックとベルトの連動処理 ---
// --- ブロックとベルトの連動処理 ---
	for (auto& block : Blocks) {
		// LDtk読み込み時にfalseにしているので、デバッグ用に一旦チェックを外すか、
		// LDtk側で初期状態を制御できるようにしてください。
		// if (!block.isActive) continue; 

		for (auto& blet : Beltconveyors) {
		

	

			// 【重要】当たり判定：ブロックの底面がベルトの上面に触れているか
			if (block.isActive) {

				if (block.linkId == 50) {
					// ベルトの向きに合わせて移動
					if (blet.isReversed) {
						block.pos.x -= blet.speed*0.05f;
					}
					else {
						block.pos.x += blet.speed;
					}
				}

			}
		}
	}
}

void Map::Draw(Vector2 offset) {

	const float kScreenWidth = 1980.0f;
	const float kScreenHeight = 1080.0f;

	// ========================================================
	//  最適化処理（カリング）
	// ========================================================

	// 1. 描画開始位置（左上）を計算
	// カメラ位置(offset)をタイルサイズで割って、何番目のタイルから描けばいいか求める
	int startX = (int)(offset.x / kTileSize);
	int startY = (int)(offset.y / kTileSize);

	// 2. 描画終了位置（右下）を計算
	// 画面に入りきるタイル数 ＋ 少しの余裕(+2) を足す
	// ※ +2 はスクロールした瞬間に端っこが消えないようにするための予備です
	int endX = startX + (int)(kScreenWidth / kTileSize) + 2;
	int endY = startY + (int)(kScreenHeight / kTileSize) + 2;

	// 3. マップの範囲外（マイナスや最大値オーバー）にアクセスしないように補正
	// これを忘れるとゲームがクラッシュします！
	if (startX < 0) startX = 0;
	if (startY < 0) startY = 0;
	if (endX > kMapWidth) endX = kMapWidth;
	if (endY > kMapHeight) endY = kMapHeight;


	// ========================================================
	//  描画ループ
	//  （0からではなく、計算した startX, startY から回します）
	// ========================================================
	for (int y = startY; y < endY; y++) {
		for (int x = startX; x < endX; x++) {

			int id = mapData[y][x];

			// --- 通常ブロックの描画 ---
			if (id > 0 && id < kMaxBlocksType && blockTextures[id] != 0) {
				Novice::DrawSprite(
					(int)(x * kTileSize - offset.x),
					(int)(y * kTileSize - offset.y),
					blockTextures[id],
					1.0f, 1.0f, 0.0f, 0xFFFFFFFF
				);
			}

			// --- ルーターのデバッグ表示 (ID: 3) ---
			if (id == 3) {
				Novice::DrawBox(
					(int)(x * kTileSize - offset.x),
					(int)(y * kTileSize - offset.y),
					kTileSize, kTileSize,
					0.0f,
					RED,
					kFillModeSolid
				);
			}

			// --- 追加ブロック (ID: 4) ---
			if (id == 4) {
				Novice::DrawBox(
					(int)(x * kTileSize - offset.x),
					(int)(y * kTileSize - offset.y),
					kTileSize, kTileSize,
					0.0f,
					0xFFFFFFFF,
					kFillModeSolid
				);
			}
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
			kTileSize, kTileSize * 2, // タイルと同じ大きさ
			0.0f,
			color,
			kFillModeSolid
		);

		// デバッグ用: リンクIDを画面左上に表示して確認したい場合
		// Novice::ScreenPrintf(0, 0, "Door ID:%d at (%d,%d)", door.linkId, (int)door.pos.x, (int)door.pos.y);
	}


	// ボタンの描画
	// (Map.hで定義した buttons リストをループ)
	// ---------------------------------------------
	// ボタン（ドアスイッチ）の描画
	// ---------------------------------------------
	for (const auto& btn : buttons) {
		// 画面上の描画位置を計算
		int drawX = (int)(btn.pos.x - offset.x);
		int drawY = (int)(btn.pos.y - offset.y);

		// ★ここで画像を切り替える！
		// btn.isPressed が true なら ON画像、false なら OFF画像 を選ぶ
		int currentTex;

		if (btn.isPressed) {
			currentTex = buttonOffTexture;  // 押されてる！
		} else {
			currentTex = buttonOnTexture; // まだ押されてない
		}

		// 画像を描画 (DrawBox ではなく DrawSprite を使う)
		// ※画像サイズと当たり判定サイズが違う場合は、第4,5引数のスケール(1.0f)で調整してください
		Novice::DrawSprite(drawX, drawY, currentTex, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);


		// (デバッグ用) IDを表示したければ残してもOK
		// Novice::ScreenPrintf(drawX, drawY - 20, "ID:%d", btn.linkId);
	}

	// リフトギミックブロックの描画
	for (auto& lift : liftBlocks) {
		lift.Draw(offset);
	}

	// リフトギミックボタンの描画
	for (auto& button : liftButtons) {
		button.Draw(offset);
	}

	// (Map.hで定義した buttons リストをループ)
	for (const auto& water : waters) {
		// 描画座標の計算
		int drawX = (int)(water.pos.x - offset.x);
		int drawY = (int)(water.pos.y - offset.y);

		// 押されているかどうかの色分け（押されたら黄色、未踏なら赤）
		unsigned int color = water.isActive ? 0x00FF00FF : 0x00FF0000;

		// ボタンは少し小さく表示して、ドアと区別しやすくする
		int btnSize = kTileSize;

		Novice::DrawBox(
			drawX, drawY, // 少しずらして中央に
			btnSize * 8, btnSize,
			0.0f,
			color,
			kFillModeSolid
		);

		// デバッグ用: リンクIDを確認したい場合
		// Novice::ScreenPrintf(0, 20, "Button ID:%d", button.linkId);
	}


	// (Map.hで定義した belt リストをループ)
	for (const auto& Beltconveyor : Beltconveyors) {
		// 描画座標の計算
		int drawX = (int)(Beltconveyor.pos.x - offset.x);
		int drawY = (int)(Beltconveyor.pos.y - offset.y);

		// 押されているかどうかの色分け（押されたら黄色、未踏なら赤）
		unsigned int color = Beltconveyor.isReversed ? 0x00FF00FF : 0x0000FFFF;

		// ボタンは少し小さく表示して、ドアと区別しやすくする
		int btnSize = kTileSize;

		Novice::DrawBox(
			drawX - 5, drawY, // 少しずらして中央に
			btnSize * 16, btnSize,
			0.0f,
			color,
			kFillModeSolid
		);

		// デバッグ用: リンクIDを確認したい場合
		// Novice::ScreenPrintf(0, 20, "Button ID:%d", button.linkId);
	}


	for (const auto& Checkpoint : Checkpoints) {
		// 描画座標の計算
		int drawX = (int)(Checkpoint.pos.x - offset.x);
		int drawY = (int)(Checkpoint.pos.y - offset.y);

		// 押されているかどうかの色分け（押されたら黄色、未踏なら赤）
		unsigned int color = Checkpoint.isActive ? 0x000000FF : 0xFF0000FF;

		// ボタンは少し小さく表示して、ドアと区別しやすくする
		int btnSize = kTileSize;

		Novice::DrawBox(
			drawX, drawY, // 少しずらして中央に
			btnSize * 2, btnSize / 2,
			0.0f,
			color,
			kFillModeSolid
		);

		// デバッグ用: リンクIDを確認したい場合
		// Novice::ScreenPrintf(0, 20, "Button ID:%d", button.linkId);
	}


	for (const auto& VanishingFloor : VanishingFloors) {
		// 描画座標の計算
		int drawX = (int)(VanishingFloor.pos.x - offset.x);
		int drawY = (int)(VanishingFloor.pos.y - offset.y);

		// 押されているかどうかの色分け（押されたら黄色、未踏なら赤）
		unsigned int color = VanishingFloor.isActive ? 0xFFFF00FF : 0x000000FF;

		// ボタンは少し小さく表示して、ドアと区別しやすくする
		int btnSize = kTileSize;

		Novice::DrawBox(
			drawX, drawY, // 少しずらして中央に
			btnSize * 4, btnSize,
			0.0f,
			color,
			kFillModeSolid
		);

		// デバッグ用: リンクIDを確認したい場合
		// Novice::ScreenPrintf(0, 20, "Button ID:%d", button.linkId);
	}


	for (const auto& Block : Blocks) {
		// 描画座標の計算
		int drawX = (int)(Block.pos.x - offset.x);
		int drawY = (int)(Block.pos.y - offset.y);

		// 押されているかどうかの色分け（押されたら黄色、未踏なら赤）
		unsigned int color = Block.isActive ? 0xFFFF00FF : 0x00FF00FF;

		// ボタンは少し小さく表示して、ドアと区別しやすくする
		int btnSize = kTileSize;

		Novice::DrawBox(
			drawX, drawY, // 少しずらして中央に
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



