#include "Player.h"
#include "Novice.h"
#include "Map.h"
#include "const.h"

Player::Player() {
	status_.pos.x = 300.0f;
	status_.pos.y = 704.0f;
	InitPlayer();
}

void Player::InitPlayer() {


	//加速度計
	status_.acceleration.x = 0.0f;
	status_.acceleration.y = 0.0f;
	status_.Velocity.x = 0.0f;
	status_.Velocity.y = +9.8f;
	status_.Speed = 5.0f;

	//スケール
	status_.scale.x = 64.0f;
	status_.scale.y = 64.0f;

	status_.isActive = true;
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

	cmdIndex = 0;

}

void Player::UpdatePlayer(char keys[256], char preKeys[256], int  mapData[kMapHeight][kMapWidth]) {

	if (status_.isActive) {
		MovePlayer(keys, preKeys, mapData);
	}
}

// コマンドで動かせるプレイヤー
void Player::UpdateByCommands(const std::vector<CommandType>& commands, int mapData[kMapHeight][kMapWidth]) {


	// まだ実行すべきコマンドが残っているか？
	if (cmdIndex < commands.size()) {

		CommandType currentCmd = commands[cmdIndex];

		switch (currentCmd) {

			/*--------------------------------------
			右へ進む
			-----------------------------*/
		case CommandType::MoveRight:
			status_.moveDir = 1.0f;
			cmdIndex++; // コマンド完了（動き続けるので即次へ）
			break;

			/*---------------------------
			左へ進む
			---------------------------------*/
		case CommandType::MoveLeft:
			status_.moveDir = -1.0f;
			cmdIndex++;
			break;

			// ---------------------------------------------------
			// 壁があったらジャンプ：
			// 「壁が来るまで待機」する。
			// ---------------------------------------------------
		case CommandType::CheckWallJump:
			status_.pos.x += status_.Speed;

			// 壁があったらジャンプ
			if (!status_.isJumop) {
				if (IsWallAhead(mapData)) {
					ActionTryJump();
					cmdIndex++;
				}
			}
			break;

			// ---------------------------------------------------
			// 崖があったらジャンプ：
			// 「崖が来るまで待機」する。
			// ---------------------------------------------------
		case CommandType::CheckCliffJump:
			status_.pos.x += status_.Speed;

			// ★重要修正：地面にいるときだけチェックする！
			if (!status_.isJumop) {
				if (IsCliffAhead(mapData)) {
					ActionTryJump();
					cmdIndex++; // ジャンプしたので役目終了、次へ
				}
			}
			break;
		}
	}
	else {
		// コマンドがなくなった後
		status_.pos.x += status_.Speed * status_.moveDir;
	}

	isRightWall(mapData, BLOCK);
	isLeftWall(mapData, BLOCK);


	Gravity();//重力処理

	//下のタイルの座標系さんと当たり判定
	isGrounded(mapData, BLOCK);//通常の地面判定
	isGrounded(mapData, HALF_FLOOR);  // ★ハーフ床の地面もチェック！

	isTopWall(mapData, BLOCK);
}






void Player::DrawPlayer(Vector2 offset) {
	Novice::DrawBox(
		static_cast<int>(status_.pos.x - offset.x), 
		static_cast<int>(status_.pos.y - offset.y),
		static_cast<int>(status_.scale.x), 
		static_cast<int>(status_.scale.y),
		0.0f, WHITE, kFillModeSolid
	);
}

//------------------------------------------------------------------------------------------------------
//プライベート関数など
//------------------------------------------------------------------------------------------------------


void Player::MovePlayer(char keys[256], char preKeys[256],
	int mapData[kMapHeight][kMapWidth]) {
	if (status_.isMoveFree) {
		// ジャンプ（押した瞬間）
		if (!status_.isJumop) {
			if (keys[DIK_SPACE] && !preKeys[DIK_SPACE]) {
				status_.isJumop = true;
				status_.Velocity.y = -status_.jumpPower;
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
	

	

	Gravity();

	//下のタイルの座標系さんと当たり判定

	isGrounded(mapData, BLOCK);
	isGrounded(mapData, HALF_FLOOR);  // ★ハーフ床の地面もチェック！


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
	if (!status_.isJumop) {
		status_.isJumop = true;
		status_.Velocity.y = -status_.jumpPower;
	}
}

// 前に壁があるかチェック
bool Player::IsWallAhead(int mapData[kMapHeight][kMapWidth]) {

	int checkX;

	// ★右を向いているか、左を向いているか
	// でチェック位置を変える
	if (status_.moveDir > 0) {
		// 右方向：自分の右端 + 5px
		checkX = (int)(status_.pos.x + status_.width + 5.0f) / kTileSize;
	} else {
		// 左方向：自分の左端 - 5px
		checkX = (int)(status_.pos.x - 5.0f) / kTileSize;
	}

	int checkY = (int)(status_.pos.y + status_.height / 2.0f) / kTileSize;

	if (checkX >= 0 && checkX < kMapWidth && checkY >= 0 && checkY < kMapHeight) {
		if (mapData[checkY][checkX] != 0) return true; // 壁あり
	}
	return false;
}


// 足元が崖かチェック
bool Player::IsCliffAhead(int mapData[kMapHeight][kMapWidth]) {
	int checkX;
	if (status_.moveDir > 0) {
		checkX = (int)(status_.pos.x + status_.width + 5.0f) / kTileSize;
	} else {
		checkX = (int)(status_.pos.x - 5.0f) / kTileSize;
	}

	int checkY = (int)(status_.pos.y + status_.height + 5.0f) / kTileSize;

	if (checkX >= 0 && checkX < kMapWidth && checkY >= 0 && checkY < kMapHeight) {
		if (mapData[checkY][checkX] == 0) return true; // 0なら崖
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
	int tileTopY = (int)((status_.pos.y+2.0f) / kTileSize);
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
	int tileTopY = (int)((status_.pos.y+2.0f) / kTileSize);
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
