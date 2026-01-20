#include "Player.h"
#include "Novice.h"
#include "Map.h"
#include "const.h"
#include "json.hpp" 
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
	status_.Speed = 5.5f;

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
	status_.isBlet = false;
	status_.isWaitingForLanding = false;
	cmdIndex = 0;

}

void Player::UpdatePlayer(char keys[256], char preKeys[256], int  mapData[kMapHeight][kMapWidth]) {

	if (status_.isActive) {
		MovePlayer(keys, preKeys, mapData);
	}
}

// コマンドで動かせるプレイヤー
void Player::UpdateByCommands(const std::vector<CommandType>& commands, int mapData[kMapHeight][kMapWidth], std::vector<Beltconveyor>& Beltconveyors) {
	if (status_.isActive) {

		// ★重要：コマンド処理の「前」に一旦フラグをリセットしてベルト判定を行う
		// これにより、コマンド移動前の正しい接地状態がセットされる
		CheckBeltCollision(Beltconveyors);

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
				status_.pos.x += status_.Speed * status_.moveDir;

				// 移動した「後」に再度ベルト判定（座標の吸着とフラグ更新）
				CheckBeltCollision(Beltconveyors);

				if (status_.isWaitingForLanding) {
					if (!status_.isJumop) {
						status_.isWaitingForLanding = false;
						cmdIndex++;
					}
				}
				else {
					if (!status_.isJumop && IsWallAhead(mapData)) {
						ActionTryJump();
						status_.isWaitingForLanding = true;
					}
				}
				break;

			case CommandType::CheckCliffJump:
				// 1. まず移動
				status_.pos.x += status_.Speed * status_.moveDir;

				// 2. 「今」ベルトに乗っているかを即座に確定させる
				CheckBeltCollision(Beltconveyors);

				// 3. ベルトに乗っているなら、崖なんて関係ない。ジャンプもせず次へ
				if (status_.isBlet) {
					status_.isWaitingForLanding = false;
					status_.isJumop = false;
					status_.Velocity.y = 0;
					cmdIndex++; // 「この場所の崖（ベルト）は攻略した」とみなして次のコマンドへ
					break;
				}

				// 4. ベルトに乗っていない場合のみ、通常の着地待ち or 崖チェック
				if (!status_.isBlet) {
					if (status_.isWaitingForLanding) {
						if (!status_.isJumop) {
							status_.isWaitingForLanding = false;
							cmdIndex++;
						}
					}
					else {
						// ここでも isBlet をチェック（念のため）
						if (!status_.isJumop && !status_.isBlet && IsCliffAhead(mapData, Beltconveyors)) {
							ActionTryJump(); // ジャンプ開始
							status_.isWaitingForLanding = true;
						}
					}
				}
				break;
			}
		}
		else {
			status_.pos.x += status_.Speed * status_.moveDir;
			CheckBeltCollision(Beltconveyors);
		}
	}
	// --- 物理処理 ---
	isRightWall(mapData, BLOCK);
	isLeftWall(mapData, BLOCK);

	// ★Gravity の前に isBlet が確定している必要がある
	Gravity();

	// ★接地判定（ベルトに乗っていない時だけ重力や接地を通常処理する）
	if (!status_.isBlet) {
		isGrounded(mapData, BLOCK);
		isGrounded(mapData, HALF_FLOOR);
		isGrounded(mapData, SCRAPMACHINE);
	}
	else {
		// ベルトに乗っているなら空中フラグを折る（ダメ押し）
		status_.isJumop = false;
		status_.Velocity.y = 0;
	}

	isTopWall(mapData, BLOCK);

	// --- UpdateByCommands の一番最後 ---

// 1. 全ての移動が終わった「最終的な座標」で、もう一度ベルト判定を上書きする
	CheckBeltCollision(Beltconveyors);

	// 2. もし最終的にベルトに乗っているなら、このフレームで発生したジャンプを「なかったこと」にする
	if (status_.isBlet) {
		status_.isJumop = false;
		status_.Velocity.y = 0;
		// もしジャンプ待ち状態に入ってしまっていたら、それも解除してコマンドを進める
		if (status_.isWaitingForLanding) {
			status_.isWaitingForLanding = false;
			cmdIndex++;
		}
	}

}





void Player::DrawPlayer(Vector2 offset) {
	if (status_.isActive) {

		Novice::DrawBox(
			static_cast<int>(status_.pos.x - offset.x),
			static_cast<int>(status_.pos.y - offset.y),
			static_cast<int>(status_.scale.x),
			static_cast<int>(status_.scale.y),
			0.0f, WHITE, kFillModeSolid
		);
	}	
	Novice::ScreenPrintf(0, 400, "isBlet%d", status_.isBlet);

}

//------------------------------------------------------------------------------------------------------
//プライベート関数など
//------------------------------------------------------------------------------------------------------


void Player::MovePlayer(char keys[256], char preKeys[256],
	int mapData[kMapHeight][kMapWidth]) {
	if (status_.isActive) {
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
	// ★鉄壁のガード
	// どんな理由があろうと、ベルトの上ならジャンプ入力を「無効」にする
	if (status_.isBlet) {
		status_.isJumop = false;
		status_.Velocity.y = 0;
		return;
	}

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
	}
	else {
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
// 引数に Beltconveyors を追加
bool Player::IsCliffAhead(int mapData[kMapHeight][kMapWidth], const std::vector<Beltconveyor>& Beltconveyors) {
	// 1. ベルトに乗っている最中なら、先がどうあれ今はジャンプしない
	if (status_.isBlet) return false;

	// 2. チェックする座標を計算
	float checkWorldX = status_.pos.x + (status_.moveDir > 0 ? status_.width + 5.0f : -5.0f);
	float checkWorldY = status_.pos.y + status_.height + 5.0f; // 足元より少し下

	int tileX = (int)(checkWorldX / kTileSize);
	int tileY = (int)(checkWorldY / kTileSize);

	// 3. マップチップが「空」であることを確認
	bool isMapEmpty = false;
	if (tileX >= 0 && tileX < kMapWidth && tileY >= 0 && tileY < kMapHeight) {
		if (mapData[tileY][tileX] == 0) {
			isMapEmpty = true;
		}
	}

	// 4. マップが空の時、そこに「ベルトのエミッター」がないか確認する
	if (isMapEmpty) {
		for (const auto& belt : Beltconveyors) {
			float beltWidth = (float)kTileSize * 16; // エミッターの幅
			float beltHeight = (float)kTileSize;

			// チェック地点がベルトの矩形内に入っているか？
			if (checkWorldX >= belt.pos.x && checkWorldX <= belt.pos.x + beltWidth &&
				checkWorldY >= belt.pos.y && checkWorldY <= belt.pos.y + beltHeight) {
				// ベルトがある！そこは崖ではない！
				return false;
			}
		}
		// マップも空で、ベルトもなかったら、そこは本物の崖！
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
		}
	}
}


void Player::CheckBeltCollision(std::vector<Beltconveyor>& Beltconveyors) {
	status_.isBlet = false;	

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
				if (belt.isReversed) status_.pos.x -= belt.speed;
				else status_.pos.x += belt.speed;

				return; // 上に乗ったらこのフレームの処理は完了
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


