#include "Map.h"
#include <Novice.h>
#include <fstream>
#include "json.hpp" 
#include "Vector2.h"
#include "Game.h"

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
	blockTextures[3] = Novice::LoadTexture("./Images./ruta.png");// <-だれかルーター書いて；；


	buttonOffTexture = Novice::LoadTexture("./Images/Switch-Push-Blue.png");
	buttonOnTexture = Novice::LoadTexture("./Images/Switch-Blue.png");

	// タイトル
	texTitle = Novice::LoadTexture("./Images/title.png");

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
	Checkpoints.clear();
	Blocks.clear();
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
					newDoor.openRatio = 0.0f;
					doors.push_back(newDoor);
				} else if (id == "Button") {
					ButtonA newButton;
					newButton.pos = { px, py };
					newButton.linkId = linkId;
					newButton.isPressed = false;
					buttons.push_back(newButton);
				} else if (id == "Water") {
					Water newWater;
					newWater.pos = { px, py };
					newWater.linkId = linkId;
					newWater.isActive = true;
					waters.push_back(newWater);
				} else if (id == "Beltconveyor") {

					Beltconveyor nweBeltconveyor;
					nweBeltconveyor.pos = { px,py };
					nweBeltconveyor.speed = 6.0f;
					nweBeltconveyor.linkId = linkId;
					nweBeltconveyor.isReversed = true;
					Beltconveyors.push_back(nweBeltconveyor);
				} else if (id == "Checkpoint") {

					Checkpoint nweCheckpoint;
					nweCheckpoint.pos = { px,py };
					nweCheckpoint.linkId = linkId;
					nweCheckpoint.isActive = false;
					Checkpoints.push_back(nweCheckpoint);

				} else if (id == "VanishingFloor") {

					VanishingFloor nweVanishingFloor;
					nweVanishingFloor.pos = { px,py };
					nweVanishingFloor.linkId = linkId;
					nweVanishingFloor.isActive = true;
					VanishingFloors.push_back(nweVanishingFloor);

				} else if (id == "Block") {

					Block nweBlocks;
					nweBlocks.pos = { px,py };
					nweBlocks.STpos = { px,py };
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
						} else if (fieldName == "MoveX") { // 横移動量
							if (!field["__value"].is_null()) {
								moveLimit.x = field["__value"];
							}
						} else if (fieldName == "MoveY") { // 縦移動量
							if (!field["__value"].is_null()) {
								moveLimit.y = field["__value"];
							}
						} else if (fieldName == "Speed") { // スピード
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

		// ドアの開閉アニメーション
		for (auto& door : doors) {
			float speed = 0.1f; // ドアが開く速さ

			if (door.isOpen) {
		
				door.openRatio += speed;
				if (door.openRatio > 1.0f) door.openRatio = 1.0f;
			} else {
				door.openRatio -= speed;
				if (door.openRatio < 0.0f) {
					door.openRatio = 0.0f;
				}
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

				if (block.linkId == 77) {
					// ベルトの向きに合わせて移動
					if (blet.linkId==100) {
						block.pos.x -= blet.speed*0.01f;
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

	// ========================================================
	// ★修正版：タイトル画像の描画（変数名をユニークに変更）
	// ========================================================

	// タイトル用の座標計算
	float titleWorldX = 250.0f;
	float titleWorldY = 30.0f;

	// 変数名を 'titleDrawX', 'titleDrawY' に変更して被りを回避
	int titleDrawX = (int)(titleWorldX - offset.x);
	int titleDrawY = (int)(titleWorldY - offset.y);

	// 画面内判定
	if (titleDrawX > -1000 && titleDrawX < (int)kScreenWidth && titleDrawY > -500 && titleDrawY < (int)kScreenHeight) {

		if (texTitle != -1) {
			static float titleLogoTimer = 0.0f; // 変数名変更
			titleLogoTimer += 0.05f;

			// エフェクト設定
			float titleScale = 1.0f;
			titleScale += sinf(titleLogoTimer * 2.0f) * 0.02f;

			float titleGlitchX = 0.0f;
			float titleGlitchY = 0.0f;
			if (rand() % 100 < 5) {
				titleGlitchX = (float)(rand() % 10 - 5);
				titleGlitchY = (float)(rand() % 6 - 3);
			}

			// 色定義（変数名変更）
			unsigned int titleCMain = 0x00FFFFFF;
			unsigned int titleCGlow = 0x00AAAAFF;

			// --- 1. グロー（発光）描画 ---
			Novice::SetBlendMode(kBlendModeAdd);
			Novice::DrawSprite(
				(int)(titleDrawX + titleGlitchX - 4), (int)(titleDrawY + titleGlitchY - 4),
				texTitle,
				titleScale * 1.02f, titleScale * 1.02f,
				0.0f,
				titleCGlow & 0xFFFFFF88
			);
			Novice::DrawSprite(
				(int)(titleDrawX + titleGlitchX + 4), (int)(titleDrawY + titleGlitchY + 4),
				texTitle,
				titleScale * 1.02f, titleScale * 1.02f,
				0.0f,
				titleCGlow & 0xFFFFFF88
			);
			Novice::SetBlendMode(kBlendModeNormal);

			// --- 2. 本体描画 ---
			Novice::DrawSprite(
				(int)(titleDrawX + titleGlitchX), (int)(titleDrawY + titleGlitchY),
				texTitle,
				titleScale, titleScale,
				0.0f,
				titleCMain
			);
		}

		
	}





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
				Novice::DrawSprite(
					(int)(x * kTileSize - offset.x),
					(int)(y * kTileSize - offset.y),
					blockTextures[id],
					1.0f, 1.0f, 0.0f, 0xFFFFFFFF
				);
			}

			// --- 追加ブロック (ID: 4) - 床から突き出るスパイク/クラッシャー ---
			if (id == 4) {
				int drawX = (int)(x * kTileSize - offset.x);
				int drawY = (int)(y * kTileSize - offset.y);
				int w = kTileSize;
				int h = kTileSize;

				unsigned int cBody = 0x555555FF;      // 本体
				unsigned int cShadow = 0x222222FF;    // 影
				unsigned int cHighlight = 0x888888FF; // ハイライト
				unsigned int cBlade = 0xAAAAAAFF;     // 刃

				// 1. 本体部分（下側に配置）
				int bodyH = (int)(h * 0.65f);
				int bodyY = drawY + (h - bodyH); // Y座標を下にずらす

				// ベース
				Novice::DrawBox(drawX, bodyY, w, bodyH, 0.0f, cBody, kFillModeSolid);
				Novice::DrawBox(drawX, bodyY, w, bodyH, 0.0f, cShadow, kFillModeWireFrame);

				// ディテール（溝やボルト）
				Novice::DrawLine(drawX, bodyY + 1, drawX + w - 1, bodyY + 1, cHighlight);
				Novice::DrawLine(drawX, bodyY + bodyH / 2, drawX + w, bodyY + bodyH / 2, cShadow);

				int boltSize = 4;
				Novice::DrawBox(drawX + 4, bodyY + bodyH - 8, boltSize, boltSize, 0.0f, cShadow, kFillModeSolid);
				Novice::DrawBox(drawX + w - 8, bodyY + bodyH - 8, boltSize, boltSize, 0.0f, cShadow, kFillModeSolid);


				// 2. 刃部分（上側に配置・上向き△）
				int toothCount = 3;
				float toothW = (float)w / toothCount;
				int toothH = h - bodyH;
				int startYA = drawY; // 一番上から描画開始

				for (int i = 0; i < toothCount; ++i) {
					int currentX = drawX + (int)(i * toothW);

					// 上向きの三角形（スパイク）
					Novice::DrawTriangle(
						currentX + (int)(toothW / 2), startYA,           // 上先端
						currentX, startYA + toothH,                      // 左下
						currentX + (int)toothW, startYA + toothH,        // 右下
						cBlade,
						kFillModeSolid
					);
					// 輪郭線
					Novice::DrawTriangle(
						currentX + (int)(toothW / 2), startYA,
						currentX, startYA + toothH,
						currentX + (int)toothW, startYA + toothH,
						cShadow,
						kFillModeWireFrame
					);
					// ハイライト（中央線）
					Novice::DrawLine(
						currentX + (int)(toothW / 2), startYA + 2,
						currentX + (int)(toothW / 2), startYA + toothH,
						cHighlight
					);
				}
			}
		}
	}


	// ==========================================
	// チュートリアル看板：移動 & ジャンプ
	// ==========================================

	// --- 1. 共通で使う描画関数（ラムダ式） ---

	// 文字パターンを大幅に追加したバージョン
	auto DrawDotText = [&](float x, float y, const char* str, float size, unsigned int color) {
		int startX = (int)x;
		int startY = (int)y;
		int spacing = (int)(size * 6.0f); // 文字間隔

		for (int k = 0; str[k] != '\0'; k++) {
			char c = str[k];
			if (c == ' ') continue;

			int px = startX + k * spacing;
			int py = startY;
			int p[5][5] = { 0 };

			// 大文字・数字・記号のパターン定義
			switch (c) {
				// アルファベット (A-Z)
			case 'A': p[0][2] = 1; p[1][1] = 1; p[1][3] = 1; p[2][0] = 1; p[2][4] = 1; p[3][0] = 1; p[3][1] = 1; p[3][2] = 1; p[3][3] = 1; p[3][4] = 1; p[4][0] = 1; p[4][4] = 1; break;
			case 'B': p[0][0] = 1; p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][0] = 1; p[1][4] = 1; p[2][0] = 1; p[2][1] = 1; p[2][2] = 1; p[2][3] = 1; p[3][0] = 1; p[3][4] = 1; p[4][0] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'C': p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][0] = 1; p[1][4] = 1; p[2][0] = 1; p[3][0] = 1; p[3][4] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'D': p[0][0] = 1; p[0][1] = 1; p[0][2] = 1; p[1][0] = 1; p[1][3] = 1; p[2][0] = 1; p[2][4] = 1; p[3][0] = 1; p[3][3] = 1; p[4][0] = 1; p[4][1] = 1; p[4][2] = 1; break;
			case 'E': p[0][0] = 1; p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][0] = 1; p[2][0] = 1; p[2][1] = 1; p[2][2] = 1; p[3][0] = 1; p[4][0] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'F': p[0][0] = 1; p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][0] = 1; p[2][0] = 1; p[2][1] = 1; p[2][2] = 1; p[3][0] = 1; p[4][0] = 1; break;
			case 'G': p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][0] = 1; p[2][0] = 1; p[2][3] = 1; p[2][4] = 1; p[3][0] = 1; p[3][4] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'H': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][4] = 1; p[2][0] = 1; p[2][1] = 1; p[2][2] = 1; p[2][3] = 1; p[2][4] = 1; p[3][0] = 1; p[3][4] = 1; p[4][0] = 1; p[4][4] = 1; break;
			case 'I': p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][2] = 1; p[2][2] = 1; p[3][2] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'J': p[0][4] = 1; p[1][4] = 1; p[2][4] = 1; p[3][0] = 1; p[3][4] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'K': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][3] = 1; p[2][0] = 1; p[2][1] = 1; p[2][2] = 1; p[3][0] = 1; p[3][3] = 1; p[4][0] = 1; p[4][4] = 1; break;
			case 'L': p[0][0] = 1; p[1][0] = 1; p[2][0] = 1; p[3][0] = 1; p[4][0] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'M': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][1] = 1; p[1][3] = 1; p[1][4] = 1; p[2][0] = 1; p[2][2] = 1; p[2][4] = 1; p[3][0] = 1; p[3][4] = 1; p[4][0] = 1; p[4][4] = 1; break;
			case 'N': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][1] = 1; p[1][4] = 1; p[2][0] = 1; p[2][2] = 1; p[2][4] = 1; p[3][0] = 1; p[3][3] = 1; p[3][4] = 1; p[4][0] = 1; p[4][4] = 1; break;
			case 'O': p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][0] = 1; p[1][4] = 1; p[2][0] = 1; p[2][4] = 1; p[3][0] = 1; p[3][4] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'P': p[0][0] = 1; p[0][1] = 1; p[0][2] = 1; p[1][0] = 1; p[1][3] = 1; p[2][0] = 1; p[2][1] = 1; p[2][2] = 1; p[3][0] = 1; p[4][0] = 1; break;
			case 'Q': p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][0] = 1; p[1][4] = 1; p[2][0] = 1; p[2][4] = 1; p[3][0] = 1; p[3][2] = 1; p[3][4] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'R': p[0][0] = 1; p[0][1] = 1; p[0][2] = 1; p[1][0] = 1; p[1][3] = 1; p[2][0] = 1; p[2][1] = 1; p[2][2] = 1; p[3][0] = 1; p[3][2] = 1; p[4][0] = 1; p[4][3] = 1; break;
			case 'S': p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[1][0] = 1; p[2][1] = 1; p[2][2] = 1; p[3][4] = 1; p[4][0] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'T': p[0][0] = 1; p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[0][4] = 1; p[1][2] = 1; p[2][2] = 1; p[3][2] = 1; p[4][2] = 1; break;
			case 'U': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][4] = 1; p[2][0] = 1; p[2][4] = 1; p[3][0] = 1; p[3][4] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; break;
			case 'V': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][4] = 1; p[2][0] = 1; p[2][4] = 1; p[3][1] = 1; p[3][3] = 1; p[4][2] = 1; break;
			case 'W': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][4] = 1; p[2][0] = 1; p[2][2] = 1; p[2][4] = 1; p[3][0] = 1; p[3][1] = 1; p[3][3] = 1; p[3][4] = 1; p[4][0] = 1; p[4][4] = 1; break;
			case 'X': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][4] = 1; p[2][2] = 1; p[3][0] = 1; p[3][4] = 1; p[4][0] = 1; p[4][4] = 1; break;
			case 'Y': p[0][0] = 1; p[0][4] = 1; p[1][0] = 1; p[1][4] = 1; p[2][2] = 1; p[3][2] = 1; p[4][2] = 1; break;
			case 'Z': p[0][0] = 1; p[0][1] = 1; p[0][2] = 1; p[0][3] = 1; p[0][4] = 1; p[1][3] = 1; p[2][2] = 1; p[3][1] = 1; p[4][0] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; p[4][4] = 1; break;

				// 記号 (プログラム用)
			case '!': p[0][2] = 1; p[1][2] = 1; p[2][2] = 1; p[4][2] = 1; break;
			case ';': p[1][2] = 1; p[3][2] = 1; p[4][1] = 1; break; // セミコロン
			case '(': p[0][3] = 1; p[1][2] = 1; p[2][2] = 1; p[3][2] = 1; p[4][3] = 1; break; // 左カッコ
			case ')': p[0][1] = 1; p[1][2] = 1; p[2][2] = 1; p[3][2] = 1; p[4][1] = 1; break; // 右カッコ
			case '_': p[4][0] = 1; p[4][1] = 1; p[4][2] = 1; p[4][3] = 1; p[4][4] = 1; break; // アンダーバー
			case '-': p[2][0] = 1; p[2][1] = 1; p[2][2] = 1; p[2][3] = 1; p[2][4] = 1; break; // ハイフン
			case '>': p[0][0] = 1; p[1][1] = 1; p[2][2] = 1; p[3][1] = 1; p[4][0] = 1; break; // 大なり
			case '<': p[0][4] = 1; p[1][3] = 1; p[2][2] = 1; p[3][3] = 1; p[4][4] = 1; break; // 小なり
			case '/': p[0][4] = 1; p[1][3] = 1; p[2][2] = 1; p[3][1] = 1; p[4][0] = 1; break; // スラッシュ
			}

			// 描画
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 5; j++) {
					if (p[i][j] == 1) {
						Novice::DrawBox((int)(px + j * size), (int)(py + i * size), (int)size, (int)size, 0.0f, color, kFillModeSolid);
					}
				}
			}
		}
		};

	// キーを描画する関数
	auto DrawKey = [&](float x, float y, char keySymbol, bool isPressed, float widthRatio) {
		float baseSize = 80.0f;
		float w = baseSize * widthRatio;
		float h = baseSize;
		float depth = 15.0f;

		unsigned int cBody = isPressed ? 0x666677FF : 0x333344FF;
		unsigned int cSide = 0x111122FF;
		unsigned int cLine = isPressed ? 0x00FFFFFF : 0x008888FF;

		float dy = y + (isPressed ? depth : 0);

		// 側面
		if (!isPressed) Novice::DrawBox((int)x, (int)(y + h), (int)w, (int)depth, 0.0f, cSide, kFillModeSolid);
		// 上面
		Novice::DrawBox((int)x, (int)dy, (int)w, (int)h, 0.0f, cBody, kFillModeSolid);
		Novice::DrawBox((int)x, (int)dy, (int)w, (int)h, 0.0f, cLine, kFillModeWireFrame);
		// 内枠
		Novice::DrawBox((int)(x + 4), (int)(dy + 4), (int)(w - 8), (int)(h - 8), 0.0f, cLine, kFillModeWireFrame);

		// キーの文字 (A, D, S=Spaceなど)
		// 簡易的にDrawDotTextを流用せず、これまで通りのライン描画で見やすくする
		int cx = (int)(x + w / 2);
		int cy = (int)(dy + h / 2);

		if (keySymbol == 'A') {
			Novice::DrawLine(cx, cy - 20, cx - 15, cy + 20, cLine);
			Novice::DrawLine(cx, cy - 20, cx + 15, cy + 20, cLine);
			Novice::DrawLine(cx - 8, cy + 5, cx + 8, cy + 5, cLine);
		} else if (keySymbol == 'D') {
			Novice::DrawLine(cx - 10, cy - 20, cx - 10, cy + 20, cLine);
			Novice::DrawLine(cx - 10, cy - 20, cx + 5, cy - 20, cLine);
			Novice::DrawLine(cx - 10, cy + 20, cx + 5, cy + 20, cLine);
			Novice::DrawLine(cx + 5, cy - 20, cx + 15, cy, cLine);
			Novice::DrawLine(cx + 5, cy + 20, cx + 15, cy, cLine);
		} else if (keySymbol == 'S') { // Space Bar
			Novice::DrawLine(cx - 40, cy + 10, cx + 40, cy + 10, cLine);
			Novice::DrawLine(cx - 40, cy + 10, cx - 40, cy - 5, cLine);
			Novice::DrawLine(cx + 40, cy + 10, cx + 40, cy - 5, cLine);
		}
		};

	// 矢印を描画する関数
	auto DrawArrow = [&](float x, float y, int dir) { // dir 0:左, 1:右, 2:上
		unsigned int c = 0xFFFF00FF;
		int bx = (int)x;
		int by = (int)y;

		if (dir == 0) { // 左
			Novice::DrawTriangle(bx, by + 20, bx + 30, by, bx + 30, by + 40, c, kFillModeSolid);
			Novice::DrawBox(bx + 30, by + 10, 40, 20, 0.0f, c, kFillModeSolid);
		} else if (dir == 1) { // 右
			Novice::DrawBox(bx, by + 10, 40, 20, 0.0f, c, kFillModeSolid);
			Novice::DrawTriangle(bx + 70, by + 20, bx + 40, by, bx + 40, by + 40, c, kFillModeSolid);
		} else if (dir == 2) { // 上 (ジャンプ)
			Novice::DrawTriangle(bx + 20, by, bx, by + 30, bx + 40, by + 30, c, kFillModeSolid);
			Novice::DrawBox(bx + 10, by + 30, 20, 40, 0.0f, c, kFillModeSolid);
		}
		};



	static int tutTimer = 0;
	tutTimer++;

	// --- [A][D] MOVE ---
	float moveX = 350.0f;
	float moveY = 650.0f;

	if (moveX - offset.x > -400 && moveX - offset.x < 1500) {
		float dx = moveX - offset.x;
		float dy = moveY - offset.y;
		bool pressA = (tutTimer / 60) % 2 == 0;

		// 文字 "MOVE"
		DrawDotText(dx + 20, dy - 80, "MOVE", 10.0f, 0xFFFFFFDD);

		// キー
		DrawKey(dx, dy, 'A', pressA, 1.0f);
		DrawKey(dx + 100, dy, 'D', !pressA, 1.0f);

		// 矢印
		if (pressA) DrawArrow(dx - 90, dy + 20, 0); // 左
		else        DrawArrow(dx + 200, dy + 20, 1); // 右
	}

	// --- [SPACE] JUMP ---
	float jumpX = 2000.0f;
	float jumpY = 450.0f;

	if (jumpX - offset.x > -400 && jumpX - offset.x < 1500) {
		float dx = jumpX - offset.x;
		float dy = jumpY - offset.y;
		bool pressSpace = (tutTimer / 60) % 2 == 0;

		// 文字 "JUMP"
		DrawDotText(dx + 20, dy - 80, "JUMP", 10.0f, 0xFFFFFFDD);

		// スペースキー (横長 widthRatio=3.0)
		DrawKey(dx, dy, 'S', pressSpace, 3.5f);

		// 上矢印 (キーの上に配置)
		if (pressSpace) {
			// キーの上から飛び出すイメージ
			float arrowY = dy - 20 - (tutTimer % 20); // 少し上に動く
			DrawArrow(dx + 120, arrowY, 2);
		}
	}
	// --- [LETS COMMAND!] 看板（UI誘導アニメーション版） ---
	float letsX = 3200.0f;
	float letsY = 350.0f;

	if (letsX - offset.x > -800 && letsX - offset.x < 1500) {
		float dx = letsX - offset.x;
		float dy = letsY - offset.y;

		// 1. 背景とテキスト
		unsigned int cText = 0xFFFFFFDD;
		unsigned int cLine = 0x00AAAAFF;

		Novice::DrawBox((int)dx - 20, (int)dy - 100, 800, 240, 0.0f, 0x000000CC, kFillModeSolid);
		Novice::DrawBox((int)dx - 20, (int)dy - 100, 800, 240, 0.0f, cLine, kFillModeWireFrame);

		DrawDotText(dx, dy - 60, "LETS", 12.0f, cText);
		DrawDotText(dx, dy + 40, "COMMAND !", 12.0f, cText);


	}



	// ==========================================
	// ドアの描画（重厚なブラストドア風）
	// ==========================================
	for (const auto& door : doors) {
		int drawX = (int)(door.pos.x - offset.x);
		int drawY = (int)(door.pos.y - offset.y);
		int w = kTileSize;
		int h = kTileSize * 2;

		// --- カラーパレット ---
		unsigned int cVoid = 0x050510FF; // 開いた奥の暗闇
		unsigned int cFrame = 0x333344FF; // 外枠
		unsigned int cDoor = 0x555566FF; // ドア本体（鉄板）
		unsigned int cShadow = 0x222233FF; // 影・溝
		unsigned int cLight = 0x777788FF; // ハイライト
		unsigned int cWarn1 = 0xDDAA00FF; // 警告色（黄色）
		unsigned int cWarn2 = 0x222200FF; // 警告色（黒）
		unsigned int cLock = 0x00FFFFFF; // ロックランプ（シアン）

		// 1. 背景（ドアが開いたときに見える奥）
		Novice::DrawBox(drawX, drawY, w, h, 0.0f, cVoid, kFillModeSolid);
		// 奥のパイプや配線（ディテール）
		Novice::DrawBox(drawX + 8, drawY, 4, h, 0.0f, 0x002200FF, kFillModeSolid);
		Novice::DrawBox(drawX + w - 12, drawY, 4, h, 0.0f, 0x002200FF, kFillModeSolid);

		// 2. ドア枠（動かない部分）
		int frameW = 4;
		// 左右の柱
		Novice::DrawBox(drawX, drawY, frameW, h, 0.0f, cFrame, kFillModeSolid);
		Novice::DrawBox(drawX + w - frameW, drawY, frameW, h, 0.0f, cFrame, kFillModeSolid);
		// 上の鴨居
		Novice::DrawBox(drawX, drawY, w, frameW * 2, 0.0f, cFrame, kFillModeSolid);

		// 3. ドア本体（可動部分）
		float currentHeight = h * (1.0f - door.openRatio);

		// 完全に消えず、少し上に残る演出（格納された感）
		if (door.isOpen && currentHeight < 8.0f) currentHeight = 8.0f;

		if (currentHeight > 0) {
			int doorBottomY = drawY + (int)currentHeight; // ドアの下端座標

			// --- 鉄板本体 ---
			// 枠の内側に描画
			int innerX = drawX + frameW;
			int innerW = w - frameW * 2;
			Novice::DrawBox(innerX, drawY, innerW, (int)currentHeight, 0.0f, cDoor, kFillModeSolid);

			// 質感（枠線とハイライト）
			Novice::DrawBox(innerX, drawY, innerW, (int)currentHeight, 0.0f, cShadow, kFillModeWireFrame);
			Novice::DrawLine(innerX + 2, drawY, innerX + 2, doorBottomY, cLight); // 左ハイライト

			// --- ディテール：横方向の補強リブ（溝） ---
			for (int i = 20; i < h; i += 20) {
				if (i < currentHeight - 10) {
					Novice::DrawLine(innerX, drawY + i, innerX + innerW, drawY + i, cShadow);
					Novice::DrawLine(innerX, drawY + i + 1, innerX + innerW, drawY + i + 1, cLight);
				}
			}

			// --- 警告ストライプ（下端） ---
			// ドアの下の方に「トラ柄」を入れると工業用ドアっぽくなります
			int stripeH = 16;
			if (currentHeight > stripeH) {
				int stripeY = doorBottomY - stripeH;
				Novice::DrawBox(innerX, stripeY, innerW, stripeH, 0.0f, cWarn1, kFillModeSolid);

				// 斜め線（黒）
				for (int i = -20; i < innerW; i += 10) {
					int x1 = innerX + i;
					int x2 = x1 + 6;
					int y1 = stripeY + stripeH;
					int y2 = stripeY;

					// はみ出しクリッピング簡易版（枠内に収める描画）
					// ※厳密なクリッピングは複雑になるので、枠線で上書きして隠す手法をとります
					if (x1 >= innerX && x2 <= innerX + innerW) {
						Novice::DrawTriangle(x1, y1, x2, y2, x1 + 4, y1, cWarn2, kFillModeSolid);
						Novice::DrawTriangle(x2, y2, x1 + 4, y1, x2 + 4, y2, cWarn2, kFillModeSolid);
					}
				}
				// 仕上げに枠線で縁取り（はみ出し隠し）
				Novice::DrawBox(innerX, stripeY, innerW, stripeH, 0.0f, cShadow, kFillModeWireFrame);
			}

			// --- 中央のロック機構 / ハンドル ---
			// ドアの真ん中あたりに描画
			int handleY = drawY + h / 2;
			if (handleY < doorBottomY - 10) {
				int handleSize = 14;
				int hx = drawX + w / 2;

				// ベース
				Novice::DrawBox(hx - handleSize / 2, handleY - handleSize / 2, handleSize, handleSize, 0.0f, cFrame, kFillModeSolid);
				Novice::DrawBox(hx - handleSize / 2, handleY - handleSize / 2, handleSize, handleSize, 0.0f, cShadow, kFillModeWireFrame);

				// ランプ（ロック状態なら赤、開錠中なら緑や消灯）
				// ここではシンプルに「ドアの象徴」としてシアンに光らせます
				unsigned int centerCol = (door.openRatio > 0.0f) ? 0x00FF00FF : cLock;
				Novice::DrawBox(hx - 2, handleY - 2, 4, 4, 0.0f, centerCol, kFillModeSolid);
			}
		}
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

	// (Map.hで定義した waters リストをループ)
	// ★ここから書き換え（電気ビリビリ版）

	// アニメーション用タイマー（乱数のシード代わりにもなる）
	static int elecTimer = 0;
	elecTimer++;

	for (const auto& water : waters) {
		// 描画座標
		int drawX = (int)(water.pos.x - offset.x);
		int drawY = (int)(water.pos.y - offset.y);

		// サイズ（前のコードに合わせて横幅8ブロック分としていますが、必要に応じて変えてください）
		int w = kTileSize * 8;
		int h = kTileSize;

		// OFF（isActive == false）なら描画しない
		if (!water.isActive) continue;

		// --- カラーパレット ---
		// 電気の色（シアン〜白）
		unsigned int cElecCore = 0xFFFFFFFF;    // 中心の白（コア）
		unsigned int cElecGlow = 0x00FFFFCC;    // 周りの輝き（シアン、少し透明）

		// 1. 背景の明滅（電気エネルギーが充満している感じ）
		// sin波でアルファ値（透明度）を高速に揺らす
		int alpha = 60 + (int)(sinf(elecTimer * 0.5f) * 40.0f); // 20~100くらいの透明度
		if (alpha < 0) alpha = 0;
		unsigned int cBg = (0x00FFFF00) | alpha; // RGB=シアン, A=変動

		Novice::DrawBox(drawX, drawY, w, h, 0.0f, cBg, kFillModeSolid);


		// 2. 枠線のビリビリ
		// 枠の色を高速で切り替えて、不安定な感じを出す
		unsigned int cFrame = (elecTimer % 4 < 2) ? cElecGlow : WHITE;
		Novice::DrawBox(drawX, drawY, w, h, 0.0f, cFrame, kFillModeWireFrame);


		// 3. 放電（稲妻ライン）の描画 
		// ランダムなジグザグ線を横方向に走らせる
		int numBolts = 2; // 稲妻の本数

		for (int k = 0; k < numBolts; k++) {
			// 稲妻の始点（左端のどこか）
			float currentX = (float)drawX;
			float currentY = (float)drawY + (rand() % h);

			// 右端までジグザグに進むループ
			while (currentX < drawX + w) {
				// 次の点の座標（Xは10~30px進む、Yはランダムに振れる）
				float nextX = currentX + (rand() % 20 + 10);
				if (nextX > drawX + w) nextX = (float)(drawX + w); // はみ出し防止

				// Y座標の振れ幅（大きくすると激しい）
				float nextY = currentY + (rand() % 30 - 15);

				// 上下からはみ出さないように制限
				if (nextY < drawY) nextY = (float)drawY;
				if (nextY > drawY + h) nextY = (float)(drawY + h);

				// 線を描く（太く見せるために少しずらして2回描く）
				Novice::DrawLine((int)currentX, (int)currentY, (int)nextX, (int)nextY, cElecCore);
				Novice::DrawLine((int)currentX, (int)currentY + 1, (int)nextX, (int)nextY + 1, cElecGlow);

				// 次の始点へ更新
				currentX = nextX;
				currentY = nextY;
			}
		}

		// 4. バチバチする火花パーティクル（簡易的な点描画）
		// ランダムな位置に四角い火花を散らす
		int numSparks = 4;
		for (int i = 0; i < numSparks; i++) {
			int sx = drawX + (rand() % w);
			int sy = drawY + (rand() % h);
			int size = rand() % 3 + 2; // 2~4px
			Novice::DrawBox(sx, sy, size, size, 0.0f, cElecCore, kFillModeSolid);
		}
	}
	// ★書き換えここまで

		// デバッグ用: リンクIDを確認したい場合
		// Novice::ScreenPrintf(0, 20, "Button ID:%d", button.linkId);




	// アニメーション用のタイマー
	static float beltAnimTimer = 0.0f;
	beltAnimTimer += 1.0f;

	// --- カラーパレット（前回のデザインを維持） ---
	const unsigned int kColorFrameDark = 0x333344FF;  // フレーム暗部
	const unsigned int kColorFrameLight = 0x666677FF; // フレーム明部
	const unsigned int kColorBeltBase = 0x222222FF;   // ベルト地
	const unsigned int kColorTreadDark = 0x444444FF;  // トレッド暗部
	const unsigned int kColorTreadLight = 0x777777FF; // トレッド明部
	const unsigned int kColorRollerRim = 0x888899FF;  // 車輪の縁
	const unsigned int kColorRollerHub = 0x222233FF;  // 車輪の軸

	for (const auto& Beltconveyor : Beltconveyors) {
		float drawX = Beltconveyor.pos.x - offset.x;
		float drawY = Beltconveyor.pos.y - offset.y;
		float width = kTileSize * 16.0f;
		float height = kTileSize;
		float rollerRadius = height / 2.0f;

		// 逆回転ならマイナス、通常ならプラス
		float direction = Beltconveyor.isReversed ? -1.0f : 1.0f;
		float currentSpeed = Beltconveyor.speed * 0.8f;

		// =================================================
		// 1. 支持フレーム（一番奥）
		// =================================================
		Novice::DrawBox((int)drawX, (int)drawY, (int)width, (int)height, 0.0f, kColorFrameDark, kFillModeSolid);
		Novice::DrawBox((int)drawX, (int)drawY, (int)width, 4, 0.0f, kColorFrameLight, kFillModeSolid);
		Novice::DrawBox((int)drawX, (int)(drawY + height - 4), (int)width, 4, 0.0f, BLACK, kFillModeSolid);

		// =================================================
		// 2. ベルト表面（真ん中）
		// =================================================
		// 車輪の少し内側からベルトを描画（車輪と馴染ませる）
		float beltMargin = rollerRadius * 0.5f;
		float beltSurfaceY = drawY + 4.0f;
		float beltSurfaceH = height - 8.0f;
		float beltStartX = drawX + beltMargin;
		float beltEndX = drawX + width - beltMargin;
		float beltSurfaceW = beltEndX - beltStartX;

		// 地の色
		Novice::DrawBox((int)beltStartX, (int)beltSurfaceY, (int)beltSurfaceW, (int)beltSurfaceH, 0.0f, kColorBeltBase, kFillModeSolid);

		// 流れるトレッド
		float treadSpacing = 48.0f;
		float treadWidth = 24.0f;
		float animOffset = fmodf(beltAnimTimer * currentSpeed * direction, treadSpacing);
		if (animOffset < 0) animOffset += treadSpacing;

		for (float i = -treadSpacing; i < beltSurfaceW + treadSpacing; i += treadSpacing) {
			float treadX = beltStartX + i + animOffset;

			// 範囲チェック
			if (treadX + treadWidth < beltStartX || treadX > beltEndX) continue;

			float drawStartX = max(treadX, beltStartX);
			float drawEndX = min(treadX + treadWidth, beltEndX);
			float drawW = drawEndX - drawStartX;

			if (drawW > 0) {
				Novice::DrawBox((int)drawStartX, (int)beltSurfaceY, (int)drawW, (int)beltSurfaceH, 0.0f, kColorTreadDark, kFillModeSolid);
				float highlightX = (direction > 0) ? drawStartX : drawEndX - 2;
				Novice::DrawBox((int)highlightX, (int)beltSurfaceY, 2, (int)beltSurfaceH, 0.0f, kColorTreadLight, kFillModeSolid);
			}
		}

		// ベルト全体の影
		Novice::DrawBox((int)drawX, (int)(drawY + height / 2.0f), (int)width, (int)(height / 2.0f), 0.0f, 0x00000044, kFillModeSolid);

		// =================================================
		// 3. 回転する車輪（一番手前！）
		// =================================================
		// これを最後に描くことで、車輪がベルトの上に表示され、回転がはっきり見えます
		auto DrawRotatingRoller = [&](float centerX, float centerY) {
			// 車輪のベース
			Novice::DrawEllipse((int)centerX, (int)centerY, (int)rollerRadius, (int)rollerRadius, 0.0f, kColorRollerRim, kFillModeSolid);
			Novice::DrawEllipse((int)centerX, (int)centerY, (int)(rollerRadius * 0.6f), (int)(rollerRadius * 0.6f), 0.0f, kColorRollerHub, kFillModeSolid);
			Novice::DrawEllipse((int)centerX, (int)centerY, (int)rollerRadius, (int)rollerRadius, 0.0f, BLACK, kFillModeWireFrame);

			// 回転アニメーション（スポーク）
			float angleBase = beltAnimTimer * currentSpeed * direction * 0.05f;
			for (int i = 0; i < 4; i++) { // 4本に増やして回転を分かりやすく
				float angle = angleBase + (i * 2.0f * 3.14159f / 4.0f);
				float endX = centerX + cosf(angle) * (rollerRadius * 0.85f);
				float endY = centerY + sinf(angle) * (rollerRadius * 0.85f);
				// スポークを目立つ色で描画
				Novice::DrawLine((int)centerX, (int)centerY, (int)endX, (int)endY, kColorFrameLight);
			}
			};

		// 左車輪
		DrawRotatingRoller(drawX + rollerRadius, drawY + rollerRadius);
		// 右車輪
		DrawRotatingRoller(drawX + width - rollerRadius, drawY + rollerRadius);
	}
	// ★書き換えここまで

	for (const auto& Checkpoint : Checkpoints) {
		int drawX = (int)(Checkpoint.pos.x - offset.x);
		int drawY = (int)(Checkpoint.pos.y - offset.y);

		// --- 1. カラー設定（未起動：赤 / 起動：SFグリーン） ---
		unsigned int mainColor;
		unsigned int beamColor;

		if (Checkpoint.isActive) {
			// スキャン済み：フルパワー・ネオングリーン
			mainColor = 0x39FF14FF;
			beamColor = 0x39FF1444;
		}
		else {
			// 未スキャン：アラート・レッド（少し深みのある赤）
			mainColor = 0xFF3030FF;
			beamColor = 0xFF000033;
		}

		int baseW = kTileSize * 2;
		int centerX = drawX + baseW / 2;
		int bottomY = drawY + kTileSize / 2;

		// --- 2. 上昇エフェクト（緑のときだけ高密度に） ---
		if (Checkpoint.isActive) {
			for (int i = 0; i < 5; i++) {
				float pSeed = (float)((tutTimer * 2 + i * 30) % 120) / 120.0f;
				int px = centerX + (int)(cosf(pSeed * 10.0f) * 18.0f);
				int py = bottomY - (int)(pSeed * 80.0f);
				Novice::DrawBox(px, py, 2, 2, 0.0f, mainColor, kFillModeSolid);
			}
		}

		// --- 3. 土台 ---
		// 枠線をmainColorにすることで、赤か緑に光ります
		Novice::DrawBox(drawX, bottomY - 4, baseW, 8, 0.0f, 0x111111FF, kFillModeSolid);
		Novice::DrawBox(drawX, bottomY - 4, baseW, 8, 0.0f, mainColor, kFillModeWireFrame);

		// --- 4. ホログラムスキャンライン ---
		float animTimer = (float)(tutTimer % 60) / 60.0f;
		// 起動前はラインを少なくして「待機中」感を出す
		int numLines = Checkpoint.isActive ? 6 : 2;

		for (int i = 0; i < numLines; i++) {
			float lineProgress = fmodf(animTimer + (i * (1.0f / numLines)), 1.0f);
			int lineY = bottomY - (int)(lineProgress * 60.0f);
			int alpha = (int)((1.0f - lineProgress) * 160.0f);

			// 未起動時はラインを短く、起動後は幅広に
			int lineW = Checkpoint.isActive ? 22 : 12;
			Novice::DrawLine(centerX - lineW, lineY, centerX + lineW, lineY, (mainColor & 0xFFFFFF00) | alpha);
		}

		// --- 5. 中央の浮遊コア ---
		int coreY = bottomY - 30 + (int)(sinf(tutTimer * 0.12f) * 5.0f);
		if (Checkpoint.isActive) {
			// 起動後：ひし形が高速回転
			Novice::DrawBox(centerX - 5, coreY - 5, 10, 10, tutTimer * 0.15f, mainColor, kFillModeSolid);
			Novice::DrawBox(centerX - 9, coreY - 9, 18, 18, -tutTimer * 0.1f, mainColor, kFillModeWireFrame);
		}
		else {
			// 起動前：警告を示す「！」のような縦型菱形
			Novice::DrawBox(centerX - 3, coreY - 6, 6, 12, 0.0f, mainColor, kFillModeSolid);
		}

		// --- 6. 垂直ビーム（起動後のみ） ---
		if (Checkpoint.isActive) {
			float pulse = (sinf(tutTimer * 0.2f) + 1.0f) * 0.5f;
			unsigned int bAlpha = (int)(20 + pulse * 30);
			// メインビーム
			Novice::DrawBox(centerX - 15, bottomY - 100, 30, 96, 0.0f, (mainColor & 0xFFFFFF00) | bAlpha, kFillModeSolid);
			// 芯の部分
			Novice::DrawBox(centerX - 2, bottomY - 100, 4, 96, 0.0f, (mainColor & 0xFFFFFF00) | (bAlpha + 40), kFillModeSolid);
		}
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

	///// ステージ4の横から流れてくるブロック /////
	for (const auto& block : Blocks) {
		// ここで画面内判定を直接行う（タイルのstartX/endXに頼らない）
		int drawX = (int)(block.pos.x - offset.x);
		int drawY = (int)(block.pos.y - offset.y);

		if (drawX > -kTileSize && drawX < kScreenWidth &&
			drawY > -kTileSize && drawY < kScreenHeight) {

			// ★ここから下のデザインは一切変えずにそのまま貼り付け★
			int size = kTileSize;
			unsigned int cFrame = 0x444455FF;      // 外枠
			unsigned int cInner = 0x222233FF;      // 内側パネル
			unsigned int cLine = 0x00AAAAFF;       // 発光ライン
			unsigned int cCore = block.isActive ? 0x00FFFFFF : 0x005555FF;

			Novice::DrawBox(drawX, drawY, size, size, 0.0f, cFrame, kFillModeSolid);
			int margin = 4;
			Novice::DrawBox(drawX + margin, drawY + margin, size - margin * 2, size - margin * 2, 0.0f, cInner, kFillModeSolid);
			int lineL = 8;
			Novice::DrawLine(drawX + 6, drawY + 6, drawX + 6 + lineL, drawY + 6, cLine);
			Novice::DrawLine(drawX + 6, drawY + 6, drawX + 6, drawY + 6 + lineL, cLine);
			Novice::DrawLine(drawX + size - 6, drawY + size - 6, drawX + size - 6 - lineL, drawY + size - 6, cLine);
			Novice::DrawLine(drawX + size - 6, drawY + size - 6, drawX + size - 6, drawY + size - 6 - lineL, cLine);

			static float corePulse = 0.0f;
			corePulse += 0.1f;
			int pulseSize = (int)(sinf(corePulse) * 2.0f);
			int coreX = drawX + size / 2;
			int coreY = drawY + size / 2;
			int coreR = 6 + (block.isActive ? pulseSize : 0);

			Novice::DrawEllipse(coreX, coreY, coreR + 4, coreR + 4, 0.0f, cLine & 0xFFFFFF66, kFillModeSolid);
			Novice::DrawEllipse(coreX, coreY, coreR, coreR, 0.0f, cCore, kFillModeSolid);
			Novice::DrawBox(drawX, drawY, size, size, 0.0f, BLACK, kFillModeWireFrame);
			// ★ここまで★
		}
	}

	// Map.cpp の Draw内に追加してデバッグ
	//
	// Novice::ScreenPrintf(0, 100, "MapData[10][10]: %d", mapData[10][10]);
}

	

/*
　 　 　 　 　 　 　 　 ┏━━━┳━━━┓
　 　 　 　 　 　 　 　 ┃ 　♎ 　┃ ♍ ┃
　 　 　 　 　 　 　 　 ┃天秤宮┃処女宮┃
　 　 　 　 ┏━━━╋━━━┻━━━╋━━━┓
　 　 　 　 ┃ 　♏ 　┃　 　 　 　 　 　  ┃ 　♌ ┃
　 　 　 　 ┃天蝎宮┃　 　 　 　 　 　 　 ┃獅子宮┃
	┏━━━╋━━━┛　 　 　 　 　 　    ┗━━━╋━━━┓
	┃ 　♐	 ┃　 　 　 　 　 　 　 　 　 　 　 　 ┃ 　♋ ┃
	┃人馬宮┃　 　 　 　 　 　 　 　 　 　  　 　 ┃巨蟹宮┃
	┣━━━┫　 　 　 　 　  　 　 　 　 　 　 　 ┣━━━┫
	┃ 　♑ ┃　 　 　 　 　  　 　 　 　 　 　 　 ┃ 　♊ ┃
	┃磨羯宮┃　 　 　  　 　 　 　 　 　 　 　 　 ┃双子宮┃
	┗━━━╋━━━┓　 　 　 　 　 　 　 ┏━━━╋━━━┛
　 　 　 　 ┃ 　♒ 　┃　 　 　 　 　 　 　┃ 　♉ ┃
　 　 　 　 ┃宝瓶宮┃　 　 　 　 　 　     ┃金牛宮┃
　 　 　 　 ┗━━━╋━━━┳━━━╋━━━┛
　 　 　 　 　 　 　┃ 　♓ ┃ 　♈ 
　 　 　 　 　 　 　┃双魚宮┃白羊宮┃
　 　 　 　 　 　 　┗━━━┻━━━┛

黄道十二宮配置図

*/
  