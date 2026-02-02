#include "Player.h"
#include "Novice.h"
#include "Map.h"
#include "const.h"
#include "json.hpp" 


float EaseOutExpo(float x) {
	return x == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * x);
}


// HSVから0xRRGGBBAA形式に変換する関数
// これをEnemy.cppなどの上の方にコピー＆ペーストしてください
unsigned int HSVToRGBA(float h, float s, float v, unsigned char alpha) {
	float r, g, b;

	int i = (int)(h / 60.0f) % 6;
	float f = (h / 60.0f) - (int)(h / 60.0f);
	float p = v * (1.0f - s);
	float q = v * (1.0f - f * s);
	float t = v * (1.0f - (1.0f - f) * s);

	switch (i) {
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	default: r = 0; g = 0; b = 0; break;
	}

	return ((unsigned int)(r * 255) << 24) |
		((unsigned int)(g * 255) << 16) |
		((unsigned int)(b * 255) << 8) |
		alpha;
}


Player::Player() {
	status_.pos.x = 300.0f;
	status_.pos.y = 704.0f;
	soundJump = Novice::LoadAudio("./Sounds/jump.mp3");
	InitPlayer();
}

void Player::InitPlayer() {


	//加速度計
	status_.acceleration.x = 0.0f;
	status_.acceleration.y = 0.0f;
	status_.Velocity.x = 0.0f;
	status_.Velocity.y = +9.8f;
	status_.Speed = 5.5f;

	//スケール
	status_.scale.x = 64.0f;
	status_.scale.y = 64.0f;

	status_.isActive = true;
	status_.isAlive = true;
	status_.isJumop = false;
	status_.jumpPower = 20.0f;
	status_.radius = 64.0f;
	status_.moveDir = 1.0f;

	//幅高さ
	status_.height = 64.0f;
	status_.width = 64.0f;

	//自由に動けるかの確認
	status_.isMoveFree = true;
	status_.isCommandMove = true;
	status_.isBlet = false;
	status_.isBlack = false;
	status_.isWaitingForLanding = false;
	cmdIndex = 0;

	status_.waitTimer = 0;

}

void Player::UpdatePlayer(char keys[256], char preKeys[256], int  mapData[kMapHeight][kMapWidth], std::vector<Block>& blocks) {

	if (status_.isActive) {
		MovePlayer(keys, preKeys, mapData);
		CheckBlockWall(blocks);
		CheckBlockGround(blocks);
		CheckBlockCeiling(blocks);
	}
}

// ---------------------------------------------------------
// 復活演出の開始（粒子をばら撒く準備）
// ---------------------------------------------------------
void Player::StartRespawnAnim(Vector2 centerPos) {
	isRespawning = true;
	isDying = false;
	respawnTimer = 0;
	deathTimer = 0;
	status_.isActive = false;
	status_.pos = centerPos;

	// 死亡音を停止
	if (audioManager != nullptr) {
		audioManager->Stop(SE_DEATH);
	}

	particles.clear();

	float pSize = 8.0f;
	int cols = (int)(status_.width / pSize);
	int rows = (int)(status_.height / pSize);

	// --- オレンジ系のカラーパレット ---
	unsigned int color1 = 0xE6B422FF; // メインの重機イエロー/オレンジ
	unsigned int color2 = 0xFF8C00FF; // ダークオレンジ
	unsigned int color3 = 0xFFFFFFFF; // ハイライト（白）

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			RespawnParticle p;

			p.targetPos.x = centerPos.x + (x * pSize);
			p.targetPos.y = centerPos.y + (y * pSize);

			float angle = (float)(rand() % 360) * (3.14159f / 180.0f);
			float dist = 300.0f + (float)(rand() % 200);

			p.startPos.x = p.targetPos.x + cosf(angle) * dist;
			p.startPos.y = p.targetPos.y + sinf(angle) * dist;

			p.currentPos = p.startPos;
			p.size = pSize;

			// --- 色の決定（オレンジ系でランダムに散らす） ---
			int r = rand() % 10;
			if (r < 6)      p.color = color1; // 60% はメインのオレンジ
			else if (r < 9) p.color = color2; // 30% は濃いオレンジ
			else            p.color = color3; // 10% は輝き（白）

			particles.push_back(p);
		}
	}
}

// ---------------------------------------------------------
// 復活演出の更新（粒子を集める）
// ---------------------------------------------------------
void Player::UpdateRespawnAnim() {
	if (!isRespawning) return;

	// 演出中は物理挙動（重力）を完全に停止
	status_.Velocity.y = 0.0f;

	respawnTimer++;
	float t = (float)respawnTimer / (float)kRespawnTimeMax;
	if (t > 1.0f) t = 1.0f;

	float easeT = EaseOutExpo(t);

	for (auto& p : particles) {
		p.currentPos.x = p.startPos.x + (p.targetPos.x - p.startPos.x) * easeT;
		p.currentPos.y = p.startPos.y + (p.targetPos.y - p.startPos.y) * easeT;
	}

	// --- 演出終了時：めり込みを完全に防ぐ ---
	if (respawnTimer >= kRespawnTimeMax) {
		isRespawning = false;
		status_.isActive = true;

		// 速度をゼロにする
		status_.Velocity.y = 0.0f;

		// 【重要】座標をタイル単位でスナップ（吸着）させる
		// 1ピクセルでも地面に潜っていると重力でめり込むため、
		// 座標を整数のタイル位置ピッタリ（-0.1f余裕を持たせる）に配置します
		status_.pos.y = floorf(status_.pos.y / kTileSize) * kTileSize;

		particles.clear();

		//InitPlayer();
	}
}

// ---------------------------------------------------------
// 復活演出の描画
// ---------------------------------------------------------
void Player::DrawRespawnAnim(Vector2 offset) {
	if (!isRespawning) return;

	// 現在の復活進捗 (0.0 ～ 1.0)
	float t = (float)respawnTimer / (float)kRespawnTimeMax;
	if (t > 1.0f) t = 1.0f;

	// ★ 元の豪華な pillarAlpha の計算式をそのまま使用
	float pillarAlpha = (t < 0.2f) ? (t * 5.0f) : (1.0f - t);

	// pillarAlpha が少しでもあれば柱を描画する
	if (pillarAlpha > 0.0f) {
		float playerHue = 35.0f;
		int centerX = (int)(status_.pos.x + status_.width / 2.0f - offset.x);
		int centerY = (int)(status_.pos.y + status_.height / 2.0f - offset.y);

		// --- 1. 光の柱（元のロジック維持） ---
		int auraWidth = (int)(status_.width * 1.5f * pillarAlpha);
		unsigned int auraColor = HSVToRGBA(playerHue, 0.8f, 1.0f, (unsigned char)(pillarAlpha * 120));
		unsigned int coreColor = HSVToRGBA(playerHue - 5.0f, 0.3f, 1.0f, (unsigned char)(pillarAlpha * 255));

		// 柱の描画（上方向に伸ばす）
		Novice::DrawBox(centerX - auraWidth / 2, centerY - 2000, auraWidth, 2000, 0.0f, auraColor, kFillModeSolid);
		Novice::DrawBox(centerX - 4, centerY - 2000, 8, 2000, 0.0f, coreColor, kFillModeSolid);

		// --- 2. 大量の環境パーティクル（元のロジック維持） ---
		const int kEnvParticleCount = 45;
		for (int i = 0; i < kEnvParticleCount; i++) {
			// ここで使っている t も復活タイマー由来なので同期するはずです
			float pOffsetT = fmodf(t * 1.5f + (float)i / kEnvParticleCount, 1.0f);
			float pX = (float)centerX + sinf((float)i * 0.8f + t * 5.0f) * (auraWidth * 0.6f);
			float pY = (float)centerY - (pOffsetT * 800.0f);
			float pSize = (1.0f - pOffsetT) * 5.0f + 1.0f;
			unsigned int pColor = HSVToRGBA(playerHue - (pOffsetT * 10.0f), 0.7f, 1.0f, (unsigned char)(pillarAlpha * (1.0f - pOffsetT) * 255));

			Novice::DrawBox((int)pX, (int)pY, (int)pSize, (int)pSize, pOffsetT * 6.28f, pColor, kFillModeSolid);
			if (i % 4 == 0) {
				Novice::DrawBox((int)pX + 1, (int)pY, 2, 2, 0.0f, 0xFFFFFFFF, kFillModeSolid);
			}
		}
	}

	// --- 3. プレイヤー本体を形作る粒子（元の描画ロジック維持） ---
	for (auto& p : particles) {
		Novice::DrawBox(
			(int)(p.currentPos.x - offset.x), (int)(p.currentPos.y - offset.y),
			(int)p.size, (int)p.size, 0.0f, p.color, kFillModeSolid
		);
	}
}


// コマンドで動かせるプレイヤー

void Player::UpdateByCommands(const std::vector<CommandType>& commands, int mapData[kMapHeight][kMapWidth],
	std::vector<Beltconveyor>& Beltconveyors, std::vector<Block>& blocks, std::vector<LiftGimmickBlock>& liftBlocks) {

	// 冒頭でのフラグ初期化
	status_.isBlet = false;
	status_.isBlack = false;
	status_.isLift = false;

	if (cmdIndex < commands.size()) {
		CommandType currentCmd = commands[cmdIndex];
		switch (currentCmd) {

		case CommandType::MoveRight:
			status_.moveDir = 1.0f;
			cmdIndex++;
			break;

		case CommandType::MoveLeft:
			status_.moveDir = -1.0f;
			cmdIndex++;
			break;

		case CommandType::CheckWallJump:
			// 1. まず移動させる
			status_.pos.x += status_.Speed * status_.moveDir;

			// 2. 移動した「後」に、壁や床の押し戻しを確定させる
			CheckBlockWall(blocks);
			CheckBeltCollision(Beltconveyors);
			CheckLiftCollision(liftBlocks);
			CheckBlockGround(blocks);
			CheckBlockCeiling(blocks);

			isRightWall(mapData, HALF_FLOOR);
			isLeftWall(mapData, HALF_FLOOR);

			// 3. 特殊床ならジャンプせず次へ（ break で switch を抜ける）
			if (status_.isBlet || status_.isBlack) {
				status_.isWaitingForLanding = false;
				status_.isJumop = false;
				status_.Velocity.y = 0;
				cmdIndex++;
				break;
			}

			// 4. 壁があるかチェック（押し戻された後の最終位置で判定）
			// wallFoundNow はこの直前で再取得する
			if (IsWallAhead(mapData, blocks)) {
				if (!status_.isJumop && !status_.isWaitingForLanding) {
					ActionTryJump();
					status_.isWaitingForLanding = true;
				}
			}

			// 5. 着地待ち処理
			if (status_.isWaitingForLanding && !status_.isJumop) {
				status_.isWaitingForLanding = false;
				cmdIndex++;
			}
			break;

		case CommandType::CheckCliffJump:
			status_.pos.x += status_.Speed * status_.moveDir;

			CheckBlockWall(blocks);
			CheckBeltCollision(Beltconveyors);
			CheckLiftCollision(liftBlocks);
			CheckBlockGround(blocks);
			CheckBlockCeiling(blocks);
			isRightWall(mapData, HALF_FLOOR);
			isLeftWall(mapData, HALF_FLOOR);
			// 特殊床ならジャンプをスキップ
			if (status_.isBlet || status_.isBlack || status_.isLift) {
				status_.isWaitingForLanding = false;
				status_.isJumop = false;
				status_.Velocity.y = 0;
				cmdIndex++;
				break;
			}

			if (IsCliffAhead(mapData, Beltconveyors, blocks, liftBlocks)) {
				if (!status_.isJumop && !status_.isWaitingForLanding) {
					ActionTryJump();
					status_.isWaitingForLanding = true;
				}
			}

			if (status_.isWaitingForLanding && !status_.isJumop) {
				status_.isWaitingForLanding = false;
				cmdIndex++;
			}
			break;
		}
	}
	else {
		// コマンド終了後の慣性移動
		status_.pos.x += status_.Speed * status_.moveDir;
		CheckBeltCollision(Beltconveyors);
		CheckLiftCollision(liftBlocks);
		CheckBlockGround(blocks);
	}

	// --- 物理処理（共通） ---
	// ここで mapData に対する押し戻しを行う
	isRightWall(mapData, BLOCK);
	isLeftWall(mapData, BLOCK);

	Gravity();

	if (!status_.isBlet && !status_.isBlack) {
		isGrounded(mapData, BLOCK);
		isGrounded(mapData, HALF_FLOOR);
		isGrounded(mapData, SCRAPMACHINE);
	}
	else {
		status_.isJumop = false;
		status_.Velocity.y = 0;
	}
	isTopWall(mapData, BLOCK);
	isTopWall(mapData, HALF_FLOOR);

}

void Player::DrawPlayer(Vector2 offset) {
	if (status_.isActive) {
		// --- 1. テクスチャの読み込み（通常はInitでやるのが理想ですが、一旦ここで） ---
		static int tex = Novice::LoadTexture("./Images/player.png");

		// --- 2. アニメーション変数の設定 ---
		const int kFrameCount = 5;      // 全フレーム数（320 / 64 = 5）
		const int kFrameWidth = 64;     // 1フレームの幅
		const int kFrameHeight = 64;    // 1フレームの高さ
		const int kAnimSpeed = 8;       // アニメーション速度（小さいほど速い）

		static int animTimer = 0;
		animTimer++;

		// --- 3. 現在のフレーム番号を計算 ---
		// 止まっている時は 0フレーム目で固定、動いている時だけループさせる
		int currentFrame = 0;

		// キー入力（A or D）があるか、またはコマンド移動中なら動かす
		// ※ status_.Velocity.x など、実際の移動判定に合わせて調整してください
		bool isMoving = (Novice::CheckHitKey(DIK_D) || Novice::CheckHitKey(DIK_A) || status_.isCommandMove);

		if (isMoving) {
			currentFrame = (animTimer / kAnimSpeed) % kFrameCount;
		}
		else {
			currentFrame = 0; // 待機状態
		}

		// --- 4. 切り抜き位置（画像上の左上座標）の計算 ---
		int srcX = currentFrame * kFrameWidth;
		int srcY = 0;

		// --- 5. 左右の向き反転処理 ---
		// moveDirがマイナス（左向き）ならスケールを反転させる
		float scaleX = (status_.moveDir > 0) ? 1.0f : -1.0f;
		// 反転した際、描画位置がずれるのを防ぐためのオフセット
		float drawPosX = status_.pos.x - offset.x;
		if (scaleX < 0) {
			drawPosX += kFrameWidth;
		}

		// --- 6. 描画実行 ---
		Novice::DrawQuad(
			(int)drawPosX, (int)(status_.pos.y - offset.y),                      // 左上
			(int)drawPosX + (int)(kFrameWidth * scaleX), (int)(status_.pos.y - offset.y), // 右上
			(int)drawPosX, (int)(status_.pos.y + kFrameHeight - offset.y),       // 左下
			(int)drawPosX + (int)(kFrameWidth * scaleX), (int)(status_.pos.y + kFrameHeight - offset.y), // 右下
			srcX, srcY,             // ソースの左上
			kFrameWidth, kFrameHeight, // ソースの幅・高さ
			tex,
			0xFFFFFFFF
		);
	}

	// デバッグ情報
	Novice::ScreenPrintf(0, 400, "isBlet:%d", status_.isBlet);
	Novice::ScreenPrintf(0, 440, "isBlack:%d", status_.isBlack);
}

//------------------------------------------------------------------------------------------------------
//プライベート関数など
//------------------------------------------------------------------------------------------------------


void Player::MovePlayer(char keys[256], char preKeys[256],
	int mapData[kMapHeight][kMapWidth]) {
	if (status_.isActive) {
		if (status_.isMoveFree) {

			status_.isBlet = false;
			status_.isBlack = false;

			// ジャンプ（押した瞬間）
			if (!status_.isJumop) {
				if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
					status_.isJumop = true;
					status_.Velocity.y = -status_.jumpPower;
					Novice::PlayAudio(soundJump, false, 0.6f);
				}
			}


			// --- 左右移動の処理 ---
			if (keys[DIK_D]) {

				status_.pos.x += status_.Speed;
				isRightWall(mapData, BLOCK);
				isRightWall(mapData, HALF_FLOOR);

			}
			if (keys[DIK_A]) {

				status_.pos.x -= status_.Speed;
				isLeftWall(mapData, BLOCK);
				isLeftWall(mapData, HALF_FLOOR);
			}
		}
	}



	Gravity();

	//下のタイルの座標系さんと当たり判定

	isGrounded(mapData, BLOCK);
	isGrounded(mapData, HALF_FLOOR);  // ★ハーフ床の地面もチェック！
	isGrounded(mapData, SCRAPMACHINE);//ウクラップによる処理


	isTopWall(mapData, BLOCK);
	isTopWall(mapData, HALF_FLOOR);//ハーフブロック判定

}

void Player::Gravity() {

	status_.Velocity.y += 0.98f;
	status_.pos.y += status_.Velocity.y;
}

/*--------------------
コマンドでうごかす処理
---------------------*/

void Player::ActionMoveRight() {
	status_.pos.x += status_.Speed;
}

void Player::ActionTryJump() {
	// どんな理由があろうと、特殊な床の上ならジャンプ処理そのものを「抹消」する
	if (status_.isBlet || status_.isBlack || status_.isLift) {
		status_.isJumop = false;
		status_.Velocity.y = 0.0f; // 速度を殺す
		return;
	}

	if (!status_.isJumop) {
		status_.isJumop = true;
		status_.Velocity.y = -status_.jumpPower;
		//Novice::PlayAudio(soundJump, false, 0.6f);
	}

	if (audioManager) {
		audioManager->Play(SE_JUMP, false);
	}
}

// 前に壁があるかチェック
bool Player::IsWallAhead(int mapData[kMapHeight][kMapWidth], std::vector<Block>& blocks) {
	// 1. まずマップチップ（静止壁）のチェック
	int checkX;
	if (status_.moveDir > 0) {
		checkX = (int)(status_.pos.x + status_.width + 5.0f) / kTileSize;
	}
	else {
		checkX = (int)(status_.pos.x - 5.0f) / kTileSize;
	}
	int checkY = (int)(status_.pos.y + status_.height / 2.0f) / kTileSize;

	if (checkX >= 0 && checkX < kMapWidth && checkY >= 0 && checkY < kMapHeight) {
		if (mapData[checkY][checkX] != 0) return true; // マップに壁あり
	}

	// 2. 次に「ブロック（動く壁）」があるかチェック
	// ★ここでは CheckMovingBlockCollision を呼ばない！
	for (auto& block : blocks) {
		float bWidth = (float)kTileSize;
		float bHeight = (float)kTileSize;

		// 進行方向にブロックの矩形があるか判定
		float nextX = (status_.moveDir > 0) ? (status_.pos.x + status_.width + 5.0f) : (status_.pos.x - 5.0f);
		float centerY = status_.pos.y + status_.height / 2.0f;

		if (nextX >= block.pos.x && nextX <= block.pos.x + bWidth &&
			centerY >= block.pos.y && centerY <= block.pos.y + bHeight) {
			return true; // ブロックを壁として認識
		}
	}

	return false;
}


// 足元が崖かチェック
// 引数に Beltconveyors を追加
bool Player::IsCliffAhead(int mapData[kMapHeight][kMapWidth], const std::vector<Beltconveyor>& Beltconveyors, const std::vector<Block>& blocks, const std::vector<LiftGimmickBlock>& liftBlocks) { // ★引数を追加

	// 1. 特殊な床に乗っている最中なら、先がどうあれ今はジャンプしない
	if (status_.isBlet || status_.isBlack) return false;

	// 2. チェックする足元の座標を計算
	float checkWorldX = status_.pos.x + (status_.moveDir > 0 ? status_.width + 5.0f : -5.0f);
	float checkWorldY = status_.pos.y + status_.height + 5.0f;

	int tileX = (int)(checkWorldX / kTileSize);
	int tileY = (int)(checkWorldY / kTileSize);

	// 3. マップチップ（静止壁）の確認
	bool isMapEmpty = true;
	if (tileX >= 0 && tileX < kMapWidth && tileY >= 0 && tileY < kMapHeight) {
		if (mapData[tileY][tileX] != 0) {
			isMapEmpty = false; // 何か床がある
		}
	}

	// マップチップが空なら、動体（ベルトやブロック）をチェックする
	if (isMapEmpty) {
		// 4. ベルトコンベアの確認
		for (const auto& belt : Beltconveyors) {
			float beltWidth = (float)kTileSize * 16;
			float beltHeight = (float)kTileSize;
			if (checkWorldX >= belt.pos.x && checkWorldX <= belt.pos.x + beltWidth &&
				checkWorldY >= belt.pos.y && checkWorldY <= belt.pos.y + beltHeight) {
				return false; // ベルトがあるから崖じゃない
			}
		}

		// 5. ★追加：動くブロックの確認
		for (const auto& block : blocks) {
			float blockWidth = (float)kTileSize;
			float blockHeight = (float)kTileSize;
			if (checkWorldX >= block.pos.x && checkWorldX <= block.pos.x + blockWidth &&
				checkWorldY >= block.pos.y && checkWorldY <= block.pos.y + blockHeight) {
				return false; // ブロックがあるから崖じゃない
			}
		}

		// リフトの確認
		for (const auto& lift : liftBlocks) {
			if (checkWorldX >= lift.pos_.x && checkWorldX <= lift.pos_.x + lift.size_.x &&
				checkWorldY >= lift.pos_.y && checkWorldY <= lift.pos_.y + lift.size_.y) {
				return false; // リフトがあるから崖じゃない
			}
		}

		// どこにも床がなかったら本物の崖
		return true;
	}

	return false;
}


//マップチップの当たり判定関数
#pragma region マップの当たり判定関数


// --- 接地判定 ---
void Player::isGrounded(int mapData[kMapHeight][kMapWidth], int mapId) {
	if (status_.Velocity.y <= 0) return;

	float leftX = status_.pos.x;
	float rightX = status_.pos.x + status_.width;
	float bottomY = status_.pos.y + status_.height;

	int tileLeftX = (int)(leftX / kTileSize);
	int tileRightX = (int)((rightX - 0.1f) / kTileSize);
	int tileBottomY = (int)((bottomY - 0.1f) / kTileSize);

	if (tileBottomY < 0 || tileBottomY >= kMapHeight) return;

	if (mapId == BLOCK) {
		if ((tileLeftX >= 0 && tileLeftX < kMapWidth && mapData[tileBottomY][tileLeftX] == BLOCK) ||
			(tileRightX >= 0 && tileRightX < kMapWidth && mapData[tileBottomY][tileRightX] == BLOCK)) {
			status_.pos.y = (float)(tileBottomY * kTileSize) - status_.height;
			status_.Velocity.y = 0;
			status_.isJumop = false;
		}
	}
	else if (mapId == SCRAPMACHINE) {
		if ((tileLeftX >= 0 && tileLeftX < kMapWidth && mapData[tileBottomY][tileLeftX] == SCRAPMACHINE) ||
			(tileRightX >= 0 && tileRightX < kMapWidth && mapData[tileBottomY][tileRightX] == SCRAPMACHINE)) {
			//チェックポイントで管理する場合はここにチェックポイントの座標を入れて
			status_.isActive = false;
		}
	}
	else if (mapId == HALF_FLOOR) {
		int checkX[] = { tileLeftX, tileRightX };
		for (int tx : checkX) {
			if (tx >= 0 && tx < kMapWidth && mapData[tileBottomY][tx] == HALF_FLOOR) {
				float tLeft = (float)(tx * kTileSize);
				float tCenter = tLeft + (kTileSize / 2.0f);
				// プレイヤーの足がタイルの左半分（実体）に乗っているか
				if (rightX > tLeft && leftX < tCenter) {
					float floorY = (float)(tileBottomY * kTileSize);
					status_.pos.y = floorY - status_.height;
					status_.Velocity.y = 0;
					status_.isJumop = false;
					return;
				}
			}
		}
	}


}


// --- 右壁判定 ---
void Player::isRightWall(int mapData[kMapHeight][kMapWidth], int mapId) {
	float rightX = status_.pos.x + status_.width;
	int tileRightX = (int)((rightX - 0.05f) / kTileSize); // 判定精度を微調整
	int tileTopY = (int)((status_.pos.y + 2.0f) / kTileSize);
	int tileBottomY = (int)((status_.pos.y + status_.height - 2.0f) / kTileSize);

	if (tileRightX < 0 || tileRightX >= kMapWidth) return;

	for (int ty = tileTopY; ty <= tileBottomY; ty++) {
		if (ty < 0 || ty >= kMapHeight) continue;

		int tileType = mapData[ty][tileRightX];
		if (tileType == mapId) {
			if (mapId == BLOCK) {
				status_.pos.x = (float)(tileRightX * kTileSize) - status_.width;
				return;
			}
			else if (mapId == SCRAPMACHINE) {
				status_.isActive = false;

			}
			else if (mapId == HALF_FLOOR) {
				// ハーフブロック（左半分）の場合、右から「空洞部分」に入ることがある
				// しかし、実体（タイルの左端）に右端が触れたら止める必要がある
				float tileLeftEdge = (float)(tileRightX * kTileSize);
				if (rightX > tileLeftEdge) {
					status_.pos.x = tileLeftEdge - status_.width;
					return;
				}
			}
		}
	}
}



// --- 左壁判定 ---
void Player::isLeftWall(int mapData[kMapHeight][kMapWidth], int mapId) {
	float leftX = status_.pos.x;
	int tileLeftX = (int)(leftX / kTileSize);
	int tileTopY = (int)((status_.pos.y + 2.0f) / kTileSize);
	int tileBottomY = (int)((status_.pos.y + status_.height - 2.0f) / kTileSize);

	if (tileLeftX < 0 || tileLeftX >= kMapWidth) return;

	for (int ty = tileTopY; ty <= tileBottomY; ty++) {
		if (ty < 0 || ty >= kMapHeight) continue;

		int tileType = mapData[ty][tileLeftX];
		if (tileType == mapId) {
			if (mapId == BLOCK) {
				status_.pos.x = (float)((tileLeftX + 1) * kTileSize);
				return;
			}
			else if (mapId == HALF_FLOOR) {
				// ハーフブロック（左半分）の実体右端は「タイルの真ん中」
				float tileCenterX = (float)(tileLeftX * kTileSize) + (kTileSize / 2.0f);
				if (leftX < tileCenterX) {
					status_.pos.x = tileCenterX;
					return;
				}
			}
		}
	}

}

// --- 天井判定 ---
void Player::isTopWall(int mapData[kMapHeight][kMapWidth], int mapId) {
	if (status_.Velocity.y >= 0) return;

	float leftX = status_.pos.x;
	float rightX = status_.pos.x + status_.width;
	float topY = status_.pos.y;

	int tileLeftX = (int)(leftX / kTileSize);
	int tileRightX = (int)((rightX - 0.1f) / kTileSize);
	int tileTopY = (int)(topY / kTileSize);

	if (tileTopY < 0 || tileTopY >= kMapHeight) return;

	if (mapId == BLOCK) {
		if ((tileLeftX >= 0 && tileLeftX < kMapWidth && mapData[tileTopY][tileLeftX] == BLOCK) ||
			(tileRightX >= 0 && tileRightX < kMapWidth && mapData[tileTopY][tileRightX] == BLOCK)) {
			status_.pos.y = (float)((tileTopY + 1) * kTileSize);
			status_.Velocity.y = 0;
		}
	}
	else if (mapId == HALF_FLOOR) {
		int checkX[] = { tileLeftX, tileRightX };
		for (int tx : checkX) {
			if (tx >= 0 && tx < kMapWidth && mapData[tileTopY][tx] == HALF_FLOOR) {
				float tLeft = (float)(tx * kTileSize);
				float tCenter = tLeft + (kTileSize / 2.0f);
				if (rightX > tLeft && leftX < tCenter) {
					status_.pos.y = (float)((tileTopY + 1) * kTileSize);
					status_.Velocity.y = 0;
					return;
				}
			}
		}
	}
}

#pragma endregion


bool Player::CheckRouter(Router* router[], int count) {
	// ★1. まずは「圏外」のデフォルト状態にする
	// 全てのルーターから外れているときは、動けないように設定

	bool isArrived = false;

	status_.isMoveFree = false;
	status_.isCommandMove = false;

	for (int i = 0; i < count; i++) {
		// 未生成のルーターを飛ばすガード句
		if (router[i] == nullptr) continue;
		if (router[i]->router_.pos.x < -5000.0f) continue;

		// ★2. 座標の計算（プレイヤーの中心 vs ルーターの中心）
		// 矩形(左上)と円の判定だとズレるので、プレイヤーの中心座標を作る
		float playerCenterX = status_.pos.x + status_.width / 2.0f;
		float playerCenterY = status_.pos.y + status_.height / 2.0f;

		float dx = router[i]->router_.pos.x - playerCenterX;
		float dy = router[i]->router_.pos.y - playerCenterY;
		float distSq = dx * dx + dy * dy;

		float r = router[i]->router_.radius;
		float br = router[i]->router_.bigRadius;

		// ★3. 判定（内側の円）
		if (distSq <= r * r) {
			status_.isMoveFree = true;
			status_.isCommandMove = true;

			// ここで「到着フラグ」を立てる
			isArrived = true;

			break;
		}
		// ★外側の円（まだ到着してないけど、電波はある）
		else if (distSq <= br * br) {
			status_.isMoveFree = false;
			status_.isCommandMove = true;
			// 到着はしていないので isArrived は false のまま
		}
	}

	// ★結果を返す
	return isArrived;
}

//=================================================================
//プレイヤーのエミッター周りの判定処理
//=================================================================


void Player::CheckDoorCollision(std::vector<Door>& doors) {
	for (auto& door : doors) {
		// ドアが開いている（isOpen == true）なら当たり判定を無視する
		if (door.isOpen) continue;

		// --- 矩形判定（AABB）の直接計算 ---
		// プレイヤーの四角形とドアの四角形が重なっているか
		if (status_.pos.x < door.pos.x + kTileSize &&
			status_.pos.x + status_.width > door.pos.x &&
			status_.pos.y < door.pos.y + kTileSize * 2 &&
			status_.pos.y + status_.height > door.pos.y) {

			// --- 押し戻し計算 ---
			// 現在の移動方向（moveDir）に基づいて、めり込まない位置に座標を固定する
			if (status_.moveDir > 0) {
				// 右に移動中なら、ドアの左端で止める
				status_.pos.x = door.pos.x - status_.width;
			}
			else if (status_.moveDir < 0) {
				// 左に移動中なら、ドアの右端（door.pos.x + kTileSize）で止める
				status_.pos.x = door.pos.x + (float)kTileSize;
			}
		}
	}
}


void Player::CheckWaterCollision(std::vector<Water>& waters) {
	for (auto& water : waters) {
		// ドアが開いている（isOpen == true）なら当たり判定を無視する
		if (!water.isActive) continue;

		// --- 矩形判定（AABB）の直接計算 ---
		// プレイヤーの四角形とドアの四角形が重なっているか
		if (status_.pos.x < water.pos.x + kTileSize * 8 &&
			status_.pos.x + status_.width > water.pos.x &&
			status_.pos.y < water.pos.y + kTileSize &&
			status_.pos.y + status_.height > water.pos.y) {

			//ここに数位没処理をかく
			//ここもチャックポイントの変数を入れて戻せる要確認
			status_.isActive = false;
			status_.isAlive = false;
		}
	}
}


void Player::CheckBeltCollision(std::vector<Beltconveyor>& Beltconveyors) {

	for (auto& belt : Beltconveyors) {
		float beltWidth = (float)kTileSize * 16;
		float beltHeight = (float)kTileSize;

		// AABB 矩形判定
		if (status_.pos.x < belt.pos.x + beltWidth &&
			status_.pos.x + status_.width > belt.pos.x &&
			status_.pos.y < belt.pos.y + beltHeight &&
			status_.pos.y + status_.height > belt.pos.y) {

			// めり込み量を計算
			float overlapLeft = (status_.pos.x + status_.width) - belt.pos.x;
			float overlapRight = (belt.pos.x + beltWidth) - status_.pos.x;
			float overlapTop = (status_.pos.y + status_.height) - belt.pos.y;
			float overlapBottom = (belt.pos.y + beltHeight) - status_.pos.y;

			// ★ どの方向のめり込みが一番小さいかを探す（一番小さい方向が「正解」の押し出し方向）
			float minOverlap = overlapTop;
			if (overlapLeft < minOverlap) minOverlap = overlapLeft;
			if (overlapRight < minOverlap) minOverlap = overlapRight;
			if (overlapBottom < minOverlap) minOverlap = overlapBottom;

			// 1. 上から乗っている判定 (一番浅いのが Top だった場合)
			if (minOverlap == overlapTop) {
				status_.pos.y = belt.pos.y - status_.height;
				status_.isBlet = true;
				status_.Velocity.y = 0.0f;
				status_.isJumop = false;

				// ベルトの移動効果


				if (belt.linkId == 100) {
					status_.pos.x -= 3.0;
				}
				else {
					if (belt.isReversed) status_.pos.x -= belt.speed;
					else status_.pos.x += belt.speed;
				}

			}
			// 2. 横からぶつかっている判定
			else if (minOverlap == overlapLeft) {
				status_.pos.x = belt.pos.x - status_.width;
			}
			else if (minOverlap == overlapRight) {
				status_.pos.x = belt.pos.x + beltWidth;
			}
			else if (minOverlap == overlapBottom) {
				status_.pos.y = belt.pos.y + beltHeight;
				status_.Velocity.y = 0.0f;
			}
		}
		
	}
}

//とぅんとぅンサフール
void Player::CheckFlooCollision(std::vector<VanishingFloor>& VanishingFloors) {
	for (auto& floor : VanishingFloors) {
		// 床がすでに消えている（isActive == false）なら当たり判定を無視する
		if (floor.isActive) {
			float floorWidth = (float)kTileSize * 4;
			float beltHeight = (float)kTileSize;
			// --- 矩形判定（AABB） ---
			// プレイヤーの四角形と床の四角形が重なっているか
			if (status_.pos.x < floor.pos.x + floorWidth &&
				status_.pos.x + status_.width > floor.pos.x &&
				status_.pos.y < floor.pos.y + beltHeight &&
				status_.pos.y + status_.height > floor.pos.y) {

				// めり込み量を計算（ベルトの時と同じロジック）
				float overlapTop = (status_.pos.y + status_.height) - floor.pos.y;
				float overlapBottom = (floor.pos.y + beltHeight) - status_.pos.y;
				float overlapLeft = (status_.pos.x + status_.width) - floor.pos.x;
				float overlapRight = (floor.pos.x + floorWidth) - status_.pos.x;

				// 一番めり込みが浅い方向に押し出す
				float minOverlap = overlapTop;
				if (overlapBottom < minOverlap) minOverlap = overlapBottom;
				if (overlapLeft < minOverlap) minOverlap = overlapLeft;
				if (overlapRight < minOverlap) minOverlap = overlapRight;

				// 1. 上から乗った場合（接地）
				if (minOverlap == overlapTop) {
					status_.pos.y = floor.pos.y - status_.height;
					status_.Velocity.y = 0.0f;
					status_.isJumop = false;

					// ★ ここで床の「消え始めるタイマー」などを起動させる処理を入れることが多いです
					// floor.isTouched = true; 
				}
				// 2. 下からぶつかった場合（天井）
				else if (minOverlap == overlapBottom) {
					status_.pos.y = floor.pos.y + beltHeight;
					status_.Velocity.y = 0.0f;
				}
				// 3. 横からぶつかった場合（壁）
				else if (minOverlap == overlapLeft) {
					status_.pos.x = floor.pos.x - status_.width;
				}
				else if (minOverlap == overlapRight) {
					status_.pos.x = floor.pos.x + floorWidth;
				}
			}
		}
	}
}

void Player::CheckBlockWall(std::vector<Block>& blocks) {
	for (auto& block : blocks) {
		float bW = (float)kTileSize;
		float bH = (float)kTileSize;

		// 1. そもそも重なっているか（AABB判定）
		if (status_.pos.x < block.pos.x + bW &&
			status_.pos.x + status_.width > block.pos.x &&
			status_.pos.y < block.pos.y + bH &&
			status_.pos.y + status_.height > block.pos.y) {

			// 2. 左右のめり込み量を計算
			float overlapLeft = (status_.pos.x + status_.width) - block.pos.x; // プレイヤーが左から右へ
			float overlapRight = (block.pos.x + bW) - status_.pos.x;            // プレイヤーが右から左へ

			// 3. 上下のめり込みも計算（どの面が一番近いかを判断するためだけに使う）
			float overlapTop = (status_.pos.y + status_.height) - block.pos.y;
			float overlapBottom = (block.pos.y + bH) - status_.pos.y;

			// 4. 一番「浅い」方向を探す
			float minOverlap = overlapLeft;
			int direction = 0; // 0:左, 1:右, 2:上, 3:下

			if (overlapRight < minOverlap) { minOverlap = overlapRight;  direction = 1; }
			if (overlapTop < minOverlap) { minOverlap = overlapTop;    direction = 2; }
			if (overlapBottom < minOverlap) { minOverlap = overlapBottom; direction = 3; }

			// 5. 【横方向だけ】押し戻す
			// case 2(上) と case 3(下) は何もしないことで、縦方向の判定をスルーさせる
			switch (direction) {
			case 0: // ブロックの左側に押し戻す
				status_.pos.x = block.pos.x - status_.width;
				break;
			case 1: // ブロックの右側に押し戻す
				status_.pos.x = block.pos.x + bW;
				break;
			}
		}
	}
}



void Player::CheckBlockGround(std::vector<Block>& blocks) {
	if (status_.Velocity.y < 0) return;

	for (auto& block : blocks) {
		if (status_.pos.x + status_.width > block.pos.x + 5.0f &&
			status_.pos.x < block.pos.x + kTileSize - 5.0f) {

			float hitThreshold = block.isActive ? 40.0f : 20.0f;

			if (status_.pos.y + status_.height > block.pos.y &&
				status_.pos.y + status_.height < block.pos.y + hitThreshold) {

				status_.pos.y = block.pos.y - status_.height;
				status_.Velocity.y = 0.0f;
				status_.isJumop = false;
				status_.isBlack = true; // 当たった時だけ true
				return; // 1つ当たれば十分なので抜ける
			}
		}
	}
	// ここまで来たら当たっていないということだが、
	// 呼び出し元の UpdateByCommands の冒頭で false にしているので、
	// ここで敢えて false に書かなくても大丈夫なはずです。
}


void Player::CheckBlockCeiling(std::vector<Block>& blocks) {
	if (status_.Velocity.y >= 0) return; // 落下中は無視

	for (auto& block : blocks) {
		// X軸の範囲内か
		if (status_.pos.x + status_.width > block.pos.x && status_.pos.x < block.pos.x + kTileSize) {
			// 頭が「底面」を貫通した瞬間を捕まえる
			if (status_.pos.y < block.pos.y + kTileSize && status_.pos.y > block.pos.y + kTileSize - 25.0f) {
				status_.pos.y = block.pos.y + kTileSize;
				status_.Velocity.y = 0.0f;

			}
			
		}
	}
}

//私はGitを許さない　<-!?!!?!?

// 俺が革命を起こす

// リフトとの当たり判定の実装
void Player::CheckLiftCollision(std::vector<LiftGimmickBlock>& liftBlocks) {
	for (auto& lift : liftBlocks) {
		lift.CheckCollision(*this); // Gimmick.cpp内の判定を呼ぶ
	}
}

// 革命を起こせなくても、その時が訪れるまでは...

// 死亡演出
void Player::StartDeathAnim() {
	// すでに死んでるなら何もしない
	if (isDying) return;

	isDying = true;
	deathTimer = 0;
	status_.isActive = false; // プレイヤーの当たり判定や描画をオフにする

	particles.clear();

	float pSize = 8.0f;
	int cols = (int)(status_.width / pSize);
	int rows = (int)(status_.height / pSize);

	// オレンジ系のカラー
	unsigned int color1 = 0xE6B422FF;
	unsigned int color2 = 0xFF8C00FF;
	unsigned int color3 = 0xFFFFFFFF;

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			RespawnParticle p;

			// 【逆の動き】現在地がスタート地点
			p.startPos.x = status_.pos.x + (x * pSize);
			p.startPos.y = status_.pos.y + (y * pSize);

			// 飛び散る先（ターゲット）をランダムに決定
			float angle = (float)(rand() % 360) * (3.14159f / 180.0f);
			float dist = 300.0f + (float)(rand() % 200);

			p.targetPos.x = p.startPos.x + cosf(angle) * dist;
			p.targetPos.y = p.startPos.y + sinf(angle) * dist;

			p.currentPos = p.startPos;
			p.size = pSize;

			// 色の決定（復活演出と同じ比率）
			int r = rand() % 10;
			if (r < 6)      p.color = color1;
			else if (r < 9) p.color = color2;
			else            p.color = color3;

			particles.push_back(p);
		}
	}
}

// 死亡演出の更新
void Player::UpdateDeathAnim() {
	if (!isDying) return;

	deathTimer++;
	float t = (float)deathTimer / (float)kDeathTimeMax;
	if (t > 1.0f) t = 1.0f;

	// 徐々に加速して散る感じにするために EaseOutExpo を使用
	float easeT = EaseOutExpo(t);

	for (auto& p : particles) {
		p.currentPos.x = p.startPos.x + (p.targetPos.x - p.startPos.x) * easeT;
		p.currentPos.y = p.startPos.y + (p.targetPos.y - p.startPos.y) * easeT;
	}

	// 演出終了
	if (deathTimer >= kDeathTimeMax) {
		isDying = false;
		particles.clear();
		// この後にゲームオーバー画面への遷移やリスポーン処理を呼ぶ
	}
}

// 死亡演出の描画
void Player::DrawDeathAnim(Vector2 offset) {
	if (!isDying) return;

	// 徐々に透明にする演出
	float t = (float)deathTimer / (float)kDeathTimeMax;
	unsigned char alpha = (unsigned char)((1.0f - t) * 255);

	for (auto& p : particles) {
		// パーティクルの色に透明度を適用
		unsigned int drawColor = (p.color & 0xFFFFFF00) | alpha;

		Novice::DrawBox(
			(int)(p.currentPos.x - offset.x),
			(int)(p.currentPos.y - offset.y),
			(int)p.size, (int)p.size,
			0.0f, drawColor, kFillModeSolid
		);
	}
}