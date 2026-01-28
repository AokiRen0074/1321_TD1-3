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
	blockTextures[3] = Novice::LoadTexture("./Images./ruta.png");// <-だれかルーター書いて；；


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
					nweCheckpoint.isActive = true;
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

				if (block.linkId == 50) {
					// ベルトの向きに合わせて移動
					if (blet.isReversed) {
						block.pos.x -= blet.speed * 0.04f;
					} else {
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


	// ドアの描画
	for (const auto& door : doors) {
		int drawX = (int)(door.pos.x - offset.x);
		int drawY = (int)(door.pos.y - offset.y);
		int w = kTileSize;
		int h = kTileSize * 2;

		// カラーパレット（壁と同化するような重い色）
		unsigned int cPassage = 0x050510FF;   // 開いた奥の暗闇
		unsigned int cWall = 0x444455FF;   // 壁（ドア本体）の色
		unsigned int cShadow = 0x222233FF;   // 影・溝
		unsigned int cStripe = 0xDDCC00FF;   // 警告色
		unsigned int cLampRed = 0xFF0000FF;   // ロック中ランプ
		unsigned int cLampGreen = 0x00FF00FF; // 解除ランプ

		// ==========================================
		// 1. 背景
		// ==========================================
		// ドアの後ろにある暗い空間。ドアが持ち上がるとこれが見える。
		Novice::DrawBox(drawX, drawY, w, h, 0.0f, cPassage, kFillModeSolid);

		// 奥へ続く床のガイド線（遠近感）
		Novice::DrawLine(drawX + 10, drawY + h, drawX + 20, drawY + h - 20, 0x004400FF);
		Novice::DrawLine(drawX + w - 10, drawY + h, drawX + w - 20, drawY + h - 20, 0x004400FF);


		// ==========================================
		//  「動く壁」の描画
		// ==========================================

		float currentHeight = h * (1.0f - door.openRatio);

		if (door.openRatio > 0.0f && currentHeight < 10.0f) currentHeight = 10.0f;

		if (currentHeight > 0) {
			// 壁
			Novice::DrawBox(drawX, drawY, w, (int)currentHeight, 0.0f, cWall, kFillModeSolid);

			// 枠線
			Novice::DrawBox(drawX, drawY, w, (int)currentHeight, 0.0f, cShadow, kFillModeWireFrame);

			// --- ディテール：重厚感を出すための横溝 ---
			for (int i = 20; i < h; i += 20) {
				if (i < currentHeight) {
					Novice::DrawLine(drawX + 5, drawY + i, drawX + w - 5, drawY + i, cShadow);
				}
			}

	
			int stripeH = 10;
			int bottomY = drawY + (int)currentHeight - stripeH;

			if (currentHeight > stripeH) {
				// 黄色い帯
				Novice::DrawBox(drawX, bottomY, w, stripeH, 0.0f, cStripe, kFillModeSolid);

				// 黒い縞模様を入れる
				for (int i = 0; i < w; i += 8) {
					Novice::DrawLine(drawX + i, bottomY, drawX + i, bottomY + stripeH, cShadow);
				}
				// 帯の上のライン
				Novice::DrawLine(drawX, bottomY, drawX + w, bottomY, cShadow);
			}
		}

		unsigned int lampColor = (door.openRatio > 0.5f) ? cLampGreen : cLampRed;

		// ランプの土台
		Novice::DrawBox(drawX + w / 2 - 8, drawY, 16, 6, 0.0f, cShadow, kFillModeSolid);
		// 光る部分
		Novice::DrawBox(drawX + w / 2 - 6, drawY + 1, 12, 4, 0.0f, lampColor, kFillModeSolid);
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
		unsigned int color = Block.isActive ? 0xFFFF00FF : 0xFFFFFFFF;

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