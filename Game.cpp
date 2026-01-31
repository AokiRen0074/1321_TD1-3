#include "Game.h"
#include <Novice.h>

#include "Map.h"
#include "Player.h"
#include "ScrollCamera.h"
#include "Router.h"
#include <fstream> // ファイル操作に必要

Game::Game() {
	player = new Player();
	map = new Map();
	scrollCamera = new ScrollCamera(); // スクロールちゃん
	for (int i = 0; i < 250; i++) {
		router[i] = nullptr;
	}
	Initialize();
}

Game::~Game() {
	delete player;
	delete map;
	delete scrollCamera;
	for (int i = 0; i < 250; i++) {
		delete router[i];
	}
}

void Game::Initialize() {
	// エリア定義
	gameArea = {0, 0, 1400, 1080};
	paletteArea = {1400, 0, 580, 400};     // 右上：パレット
	programArea = {1400, 400, 580, 680};   // 右下：プログラム置き場

	// 変数をリセット
	isRunning = false;
	player->InitPlayer();
	commandList.clear();

	isGameClear = false;
	gameClearTimer = 0;

	isGameOver = false;
	//gameOverTimer = 0;

	map->Initialize();

	if (respawnPos.x == 0 && respawnPos.y == 0) {
		respawnPos.x = 300.0f;
		respawnPos.y = 704.0f;
	}

	// プレイヤーの位置を最後に保存したチェックポイントへ移動させる
	player->status_.pos = respawnPos;

	fadeState_ = FADE_NONE;

	/*---------------------------------
	コマンドUIのリソース
	--------------------------------*/
	texRight = Novice::LoadTexture("./Images/moveRight.png");
	texLeft = Novice::LoadTexture("./Images/moveLeft.png");
	texWallJump = Novice::LoadTexture("./Images/wallJamp.png");
	texCliffJump = Novice::LoadTexture("./Images/airJamp.png");
	texStart = Novice::LoadTexture("./Images/start.png");
	texStop = Novice::LoadTexture("./Images/stop.png");





	float btnX = 1450;
	float btnW = 400;
	float btnH = 50;

	btnLeft = {btnX, 50, btnW, btnH, "Left", CommandType::MoveLeft, (int)WHITE, texLeft};
	btnRight = {btnX, 110, btnW, btnH, "Right", CommandType::MoveRight, (int)WHITE, texRight};
	btnWallJump = {btnX, 170, btnW, btnH, "Wall", CommandType::CheckWallJump, (int)WHITE, texWallJump};
	btnCliffJump = {btnX, 230, btnW, btnH, "Cliff", CommandType::CheckCliffJump, (int)WHITE, texCliffJump};
	// スタート・リセット
	btnStart = {1450, 300, 180, 80, "START", (CommandType)-1, (int)WHITE, texStart};
	btnReset = {1670, 300, 180, 80, "STOP", (CommandType)-1, (int)WHITE, texStop};

	/*----------------------------------
				その他画像
	-------------------------------------*/
	texRouter = Novice::LoadTexture("./Images/ruta.png");

	/*------------------------
				音
	------------------------*/
	soundClick = Novice::LoadAudio("./Sounds/komandoCorect.mp3");
	soundStart = Novice::LoadAudio("./Sounds/komandoStart.mp3");
	soundDelete = Novice::LoadAudio("./Sounds/modosu.mp3");

	soundButtonPress = Novice::LoadAudio("./Sounds/switchOn.mp3");
	soundSceneChange = Novice::LoadAudio("./Sounds/anten.mp3");

	/*------------------------------
	ここにレイヤー名をいれるんだ！！
	-----------------------------*/
	std::vector<std::string> layers = {"IntGrid", "HalfBlock"};

	map->LoadMapFromLDtk("./mapTest9999.ldtk", layers);

	//ルーターの生成
	routerCount = 0;

	for (int i = 0; i < 250; i++) {
		if (router[i] != nullptr) {
			delete router[i];
			router[i] = nullptr;
		}
	}

	for (int y = 0; y < kMapHeight; y++) {
		for (int x = 0; x < kMapWidth; x++) {
			if (map->mapData[y][x] == 3) {
				if (routerCount < 250) {
					router[routerCount] = new Router(routerCount, map->mapData);
					routerCount++;
				}
			}
		}
	}
}

// プレイヤーとボタンの当たり判定を行う関数を追加
bool IsPlayerHit(Player* player, const ButtonA& button) {
	// プレイヤーの位置とボタンの位置を比較して当たり判定を行う
	float playerLeft = player->status_.pos.x;
	float playerRight = player->status_.pos.x + player->status_.width;
	float playerTop = player->status_.pos.y;
	float playerBottom = player->status_.pos.y + player->status_.height;

	float buttonLeft = button.pos.x;
	float buttonRight = button.pos.x + 32; // ボタンの幅を仮に32とする
	float buttonTop = button.pos.y;
	float buttonBottom = button.pos.y + 32; // ボタンの高さを仮に32とする


	return !(playerRight < buttonLeft || playerLeft > buttonRight || playerBottom < buttonTop || playerTop > buttonBottom);
};

void Game::Update(char keys[256], char preKeys[256]) {
	// Game::Update 内
	CheckpointPlayer();

	if (keys[DIK_R] && !preKeys[DIK_R]) {
		// 保存されているリスポーン地点（初期位置 or 最後に触れた旗）に戻す
		player->status_.pos = respawnPos;

		isRunning = false;
		cantStartCount = 0;
		player->InitPlayer(); // 速度などをゼロにする
	}

	////////////////////////////////////////////
	// ゲームオーバー判定
	if (!player->status_.isActive && !player->IsRespawning() && !isGameOver && fadeState_ == FADE_NONE) {
		isGameOver = true;
		// gameOverTimer = 0; // 削除
		player->status_.Velocity = { 0.0f, 0.0f };
	}

	// ゲームオーバーの演出
	if (player->IsRespawning()) {
		player->UpdateRespawnAnim();
		return; // 他の処理（移動や当たり判定）をスキップ
	}

	///////////////////////////////////////////////////////

// 全部のボタンをチェック
	for (auto& btn : map->buttons) {


		// プレイヤーがボタンを踏んだら
		if (IsPlayerHit(player, btn)) {
			if (btn.isPressed == false) {

				// 音を鳴らす（ループなし）
				Novice::PlayAudio(soundButtonPress, false, 1.0f);

				// 押された状態にする
				btn.isPressed = true;
			}

			// ★ここで連動！
			// 「このボタンと同じlinkIdを持つドア」をすべて探して開ける
			for (auto& door : map->doors) {
				if (door.linkId == btn.linkId) {
					door.isOpen = true; // ドアオープン！
				}
			}

			for (auto& water : map->waters) {
				if (water.linkId == btn.linkId) {
					water.isActive = false; // ドアオープン！
				}
			}

			for (auto& Beltconveyor : map->Beltconveyors) {
				if (Beltconveyor.linkId == btn.linkId) {
					Beltconveyor.isReversed = false; // ドアオープン！
				}
			}

			for (auto& floor : map->VanishingFloors) {
				if (floor.linkId == btn.linkId) {
					floor.isActive = false; // ドアオープン！
				}
			}

			for (auto& block : map->Blocks) {
				if (block.linkId == btn.linkId) {
					block.isActive = true; // ドアオープン！
				}
			}

		}
	}

	int mouseX, mouseY;
	Novice::GetMousePosition(&mouseX, &mouseY);
	bool isClick = Novice::IsTriggerMouse(0);

	for (int i = 0; i < routerCount; i++) {
		if (router[i] != nullptr) {
			router[i]->UpdateRouter(mapData); // Routerクラスにある描画関数を呼ぶ
		}
	}

	if (cantStartCount > 0) {
		cantStartCount--;
	}

	// ==========================================
	// モード分岐
	// ==========================================
	if (isRunning) {

		if (player->IsRespawning()) {
			player->UpdateRespawnAnim(); // 粒子の計算だけする
		} else {
			// --- 実行モード ---
			bool isArrived = player->CheckRouter(router, 250);
			//ドアと水とベルトの当たり判定とかの処理
			player->CheckDoorCollision(map->doors);
			player->CheckWaterCollision(map->waters);
			player->CheckBeltCollision(map->Beltconveyors);
			player->CheckFlooCollision(map->VanishingFloors);
			player->CheckBlockGround(map->Blocks);
			player->CheckBlockWall(map->Blocks);
			if (isArrived) {
				isRunning = false;
			} else {
				player->UpdateByCommands(commandList, map->mapData, map->Beltconveyors, map->Blocks, map->liftBlocks);
			}





		}


		// ストップボタン判定
		if (isClick) {
			if (mouseX >= btnReset.x && mouseX <= btnReset.x + btnReset.w &&
				mouseY >= btnReset.y && mouseY <= btnReset.y + btnReset.h) {
				isRunning = false;
				player->InitPlayer();
			}
		}
		player->CheckBlockGround(map->Blocks);
		player->CheckBlockWall(map->Blocks);
	} else {
		// --- 編集モード ---
		player->UpdatePlayer(keys, preKeys, map->mapData, map->Blocks);
		//player->CheckRouter(router, 250);
		bool isInsideRouter = player->CheckRouter(router, 250);

		//ドアと水とベルトの当たり判定とかの処理
		player->CheckDoorCollision(map->doors);
		player->CheckWaterCollision(map->waters);
		player->CheckBeltCollision(map->Beltconveyors);
		player->CheckFlooCollision(map->VanishingFloors);
		player->CheckBlockGround(map->Blocks);
		player->CheckBlockWall(map->Blocks);
		if (isClick) {
			bool isCommandAdded = false;
			// 1. パレットのボタンを押してコマンドを追加
			if (mouseX >= btnRight.x && mouseX <= btnRight.x + btnRight.w && mouseY >= btnRight.y && mouseY <= btnRight.y + btnRight.h) {
				commandList.push_back(btnRight.cmdType);
				isCommandAdded = true;
			}

			if (mouseX >= btnLeft.x && mouseX <= btnLeft.x + btnLeft.w && mouseY >= btnLeft.y && mouseY <= btnLeft.y + btnLeft.h) {
				commandList.push_back(btnLeft.cmdType);
				isCommandAdded = true;
			}

			if (mouseX >= btnWallJump.x && mouseX <= btnWallJump.x + btnWallJump.w && mouseY >= btnWallJump.y && mouseY <= btnWallJump.y + btnWallJump.h) {
				commandList.push_back(btnWallJump.cmdType);
				isCommandAdded = true;
			}
			if (mouseX >= btnCliffJump.x && mouseX <= btnCliffJump.x + btnCliffJump.w && mouseY >= btnCliffJump.y && mouseY <= btnCliffJump.y + btnCliffJump.h) {
				commandList.push_back(btnCliffJump.cmdType);
				isCommandAdded = true;
			}

			if (isCommandAdded) {

				if (Novice::IsPlayingAudio(soundClick) == 0 || soundClick != -1) {
					Novice::PlayAudio(soundClick, false, 0.4f);
				}
			}


			// 2. スタートボタン
			if (mouseX >= btnStart.x && mouseX <= btnStart.x + btnStart.w && mouseY >= btnStart.y && mouseY <= btnStart.y + btnStart.h) {



				if (!isInsideRouter) {
					isRunning = true;
					player->InitPlayer();
					cantStartCount = 0;

					Novice::PlayAudio(soundStart, false, 0.4f);
				} else {
					// (オプション)「ここではスタートできません」みたいなログを出してもいいかも
					cantStartCount = 60;
				}
			}
			// 下の方に表示されているブロックほど、リストの後ろの方にある
			float blockY = programArea.y + 50;
			for (int i = 0; i < commandList.size(); i++) {
				// ブロックの当たり判定
				if (mouseX >= 1450 && mouseX <= 1450 + 400 &&
					mouseY >= blockY && mouseY <= blockY + 50) {

					Novice::PlayAudio(soundDelete, false, 1.0f);
					// このコマンドを削除する
					commandList.erase(commandList.begin() + i);
					break;
				}
				blockY += 60; // 次のブロックの位置へ
			}
		}
	}

	if (!isGameClear) {
		// 座標を 10500, 3520 に合わせました
		float gx = 10500.0f;
		float gy = 3520.0f;   // 修正
		float range = 100.0f; // 判定の広さ

		// プレイヤーがゴールの範囲に入ったら
		if (player->status_.pos.x > gx - range && player->status_.pos.x < gx + range &&
			player->status_.pos.y > gy - range && player->status_.pos.y < gy + range) {

			isGameClear = true;
			isRunning = false; // プレイヤーを止める

			// 演出用タイマーをリセット
			gameClearTimer = 0;
		}
	}

	// --- クリア演出のタイマー更新 ---
	// これが isRunning の外にないと、クリア画面のアニメーションが動きません
	if (isGameClear) {
		gameClearTimer++;
		if (gameClearTimer > 300 && fadeState_ == FADE_NONE) {
			Novice::PlayAudio(soundSceneChange, false, 1.0f); // 暗転音
			fadeState_ = FADE_OUT;
			fadeTimer_ = 0;
		}

	}
	// --- フェード中の処理 ---
	if (fadeState_ != FADE_NONE) {
		fadeTimer_++;

		// プレイヤーをその場で停止させる
		player->status_.isActive = false;
		player->status_.Velocity = {0.0f, 0.0f};

		// --- 暗転が完了した瞬間の処理 ---
		if (fadeState_ == FADE_OUT) {
			if (fadeTimer_ >= kFadeMax) {
				// 音の停止処理などはそのまま
				if (voiceSceneChange != -1) {
					if (Novice::IsPlayingAudio(voiceSceneChange)) {
						Novice::StopAudio(voiceSceneChange);
					}
					voiceSceneChange = -1;
				}

				if (isGameOver) {
					scrollCamera->Update(player->status_.pos);

				} else if (isGameClear) {
					// 1. フラグのリセット
					isGameClear = false;
					isRunning = false; // 実行モード終了
					cantStartCount = 0;

					// 2. プレイヤーのリセット
					player->InitPlayer();
					// 初期スポーン位置へ戻す（もし固定座標なら {300.0f, 704.0f} 等を指定）
					player->status_.pos = respawnPos;

					// 3. マップ・ギミックの完全リセット
					std::vector<std::string> layers = { "IntGrid", "HalfBlock" };
					map->LoadMapFromLDtk("./mapTest9999.ldtk", layers);

					// 4. カメラ位置も即座に戻す
					scrollCamera->Update(player->status_.pos);

					// フェードインへ移行
					fadeState_ = FADE_IN;
					fadeTimer_ = 0;
				} else {
					// 【落下によるステージ進行の場合】
					int nextIdx = scrollCamera->GetStageIndex() + 1;
					if (nextIdx < 3) {
						scrollCamera->SetStageIndex(nextIdx);

						// カメラの座標を新しいステージ位置に反映させる
						scrollCamera->Update(player->status_.pos);

						// プレイヤーのY座標を新しいステージの「上空」へ
						float newY = scrollCamera->GetStageYPosition(nextIdx);
						player->status_.pos.y = newY - 200.0f;
					}
				}
				// --- 修正ここまで ---

				fadeState_ = FADE_IN;
				fadeTimer_ = 0;
			}
		}
		// --- 2. 画面が明るくなる処理 ---
		else if (fadeState_ == FADE_IN) {
			if (fadeTimer_ >= kFadeMax) {
				// 完全に明るくなったら移動再開
				fadeState_ = FADE_NONE;
				fadeTimer_ = 0;
				player->status_.isActive = true;
			}
		}

		// フェード中は通常のUpdateをスキップ
		return;
	}

	// --- 既存の切り替えトリガー（落下判定） ---
	Vector2 camOffset = scrollCamera->GetOffset();
	if (player->status_.pos.y > (camOffset.y + 1200.0f)) {
		if (scrollCamera->GetStageIndex() + 1 < 3) {
			voiceSceneChange = Novice::PlayAudio(soundSceneChange, false, 1.0f);
			fadeState_ = FADE_OUT; // 暗転開始
			fadeTimer_ = 0;
		}
	}

	if (player->status_.pos.y > 3000.0f) {
		scrollCamera->SetIsScrollMode(true);
		// player->status_.pos.y = 2800.0f;
	}

	// マップ更新(当たり判定など)
	map->Update(*player);

	// スクロールカメラ
	// プレイヤーの座標を渡してカメラを更新
	scrollCamera->Update(player->status_.pos);
	player->CheckBlockGround(map->Blocks);
	player->CheckBlockWall(map->Blocks);
}

void Game::Draw() {
	// カメラのオフセットを取得(スクロールで必要)
	Vector2 offset = scrollCamera->GetOffset();

	

	// --- ゲーム画面 ---

	// 背景
	Novice::DrawBox((int)gameArea.x, (int)gameArea.y, (int)gameArea.w, (int)gameArea.h, 0.0f, 0x000000FF, kFillModeSolid);



	// パレットエリア背景
	Novice::DrawBox((int)paletteArea.x, (int)paletteArea.y, (int)paletteArea.w, (int)paletteArea.h, 0.0f, 0x333333FF, kFillModeSolid);

	// プログラムエリア背景
	Novice::DrawBox((int)programArea.x, (int)programArea.y, (int)programArea.w, (int)programArea.h, 0.0f, 0x222222FF, kFillModeSolid);

	// 区切り線
	Novice::DrawBox(1400, 398, 580, 4, 0.0f, WHITE, kFillModeSolid);

	Novice::DrawSprite(
		(int)(3450.0f - offset.x), // カメラの動きに合わせてズレるようにする
		(int)(330.0f - offset.y),
		texRight,
		1.0f, 1.0f, 0.0f,
		0xFFFFFFFF
	);

	// ルーター描画
	for (int i = 0; i < routerCount; i++) {
		if (router[i] != nullptr) {
			router[i]->DrawRouter(offset);// Routerクラスにある描画関数を呼ぶ
		}
	}

	// マップ描画
	map->Draw(offset);

	// プレイヤー描画
	player->DrawPlayer(offset);

	player->DrawRespawnAnim(offset);

	// 警告メッセージの表示
	if (cantStartCount > 0) {
		Novice::ScreenPrintf(800, 400, "Router Area! Can't Start!");
	}

	// --- UIボタン描画 ---
	auto DrawBtn = [](Button& b) {
		Novice::DrawSprite((int)b.x, (int)b.y, b.textureHandle, 1.0f, 1.0f, 0.0f, b.color);
		};

	DrawBtn(btnRight);
	DrawBtn(btnLeft);
	DrawBtn(btnWallJump);
	DrawBtn(btnCliffJump);
	DrawBtn(btnStart);
	DrawBtn(btnReset);

	// ... (ボタン描画などの後) ...

	// --- プログラムリストのブロック描画 ---
	Novice::ScreenPrintf(1420, 410, "--- YOUR PROGRAM (Click to Delete) ---");

	float blockY = programArea.y + 50;

	int currentIndex = -1;
	if (isRunning) {
		currentIndex = player->GetCurrentCommandIndex();
	}
	for (int i = 0; i < commandList.size(); i++) {

		// ここで描画する画像を決める
		int currentTex = 0;

		switch (commandList[i]) {

			case CommandType::MoveRight:
				currentTex = texRight;
				break;
			case CommandType::MoveLeft:
				currentTex = texLeft;
				break;
			case CommandType::CheckWallJump:
				currentTex = texWallJump;
				break;
			case CommandType::CheckCliffJump:
				currentTex = texCliffJump;
				break;

		}

		// 実行中の強調表示（色を変えるなど）
		unsigned int color = 0xFFFFFFFF;
		if (i == currentIndex) {
			color = 0xFFAAAAFF; // 実行中は少し赤っぽくする例
		}

		if (currentTex != 0) {
			Novice::DrawSprite(1450, (int)blockY, currentTex, 1.0f, 1.0f, 0.0f, color);

		}


		// 矢印や強調枠の処理
		if (i == currentIndex) {
			Novice::ScreenPrintf(1420, (int)blockY + 15, "->");
			Novice::DrawBox(1445, (int)blockY - 5, 410, 60, 0.0f, RED, kFillModeWireFrame);
		}

		if (i < commandList.size() - 1) {
			Novice::DrawTriangle(1650, (int)blockY + 50, 1630, (int)blockY + 60, 1670, (int)blockY + 60, WHITE, kFillModeSolid);
		}

		blockY += 60; // 次の表示位置へ
	}

	// 実行中などのステータス表示
	if (isRunning) {
		Novice::ScreenPrintf(10, 10, "RUNNING...");
		if (!player->IsRespawning()) {
			// 実行中の赤いフィルター
			Novice::DrawBox(0, 0, 1400, 1080, 0.0f, 0xFF000044, kFillModeSolid);
		}
	} else {
		Novice::ScreenPrintf(10, 10, "EDIT MODE");
	}

	// 描画
	if (player->IsRespawning()) {
		// 復活演出中のみ描画（この中には Novice::Draw系が書かれているはずです）
		player->DrawRespawnAnim(offset);
	} else {
		// 通常時はプレイヤーを描画
		player->DrawPlayer(offset);
	}

	// --- 暗転ブロックの描画 ---
	if (fadeState_ != FADE_NONE) {
		int maxDiagonal = kCols + kRows;
		float progress = 0.0f;

		if (fadeState_ == FADE_OUT) {
			// FADE_OUT：タイマー 0→Max に向かってブロックが増える
			progress = (float)fadeTimer_ / kFadeMax;
		} else {
			// FADE_IN：タイマー 0→Max に向かってブロックが減る
			progress = 1.0f - ((float)fadeTimer_ / kFadeMax);
		}

		int currentThreshold = (int)(maxDiagonal * progress);

		for (int y = 0; y < kRows; y++) {
			for (int x = 0; x < kCols; x++) {
				int diagonalPos = (kCols - 1 - x) + y;
				if (diagonalPos <= currentThreshold) {
					Novice::DrawBox(
						x * kBlockSize, y * kBlockSize,
						kBlockSize + 1, kBlockSize + 1,
						0.0f, BLACK, kFillModeSolid
					);
				}
			}
		}
	}

	

	int mx, my;
	Novice::GetMousePosition(&mx, &my);
	Novice::ScreenPrintf(0, 0, "Mouse: %d, %d", mx, my);
	Novice::ScreenPrintf(0, 20, "BtnRight X: %d", (int)btnRight.x);
	Novice::ScreenPrintf(0, 80, "player Pos X: %.2f", player->status_.pos.x);
	Novice::ScreenPrintf(0, 110, "player Pos Y: %.2f", player->status_.pos.y);

	Novice::ScreenPrintf(0, 150, "player Pos Y: %.2f", respawnPos.y);


	for (const auto& btn : map->buttons) {
		// ボタンの場所にIDを表示
		Novice::ScreenPrintf(
			(int)(btn.pos.x - offset.x),
			(int)(btn.pos.y - offset.y - 20),
			"ID:%d", btn.linkId
		);
	}

	for (const auto& door : map->doors) {
		// ドアの場所にIDを表示
		Novice::ScreenPrintf(
			(int)(door.pos.x - offset.x),
			(int)(door.pos.y - offset.y - 20),
			"ID:%d", door.linkId
		);
	}


	// ==========================================
	// ゲームクリア画面の描画（全画面・リセット案内付き）
	// ==========================================
	if (isGameClear) {
		// 1. 背景：画面全体（プログラムエリア含む 1980px）を覆う
		int alpha = gameClearTimer * 3;
		if (alpha > 220) alpha = 220;
		unsigned int bgCol = (0x00001100) | alpha;

		// ★変更：幅を1400から1980に変更して画面全体を隠す
		Novice::DrawBox(0, 0, 1980, 1080, 0.0f, bgCol, kFillModeSolid);

		// 2. 背景演出：流れるグリッド
		if (gameClearTimer > 20) {
			unsigned int gridCol = 0x00FFCC44;
			static float scroll = 0.0f;
			scroll += 3.0f;

			// 縦線（右端まで描画）
			for (int i = 0; i < 1980; i += 100) {
				Novice::DrawLine(i, 0, i, 1080, gridCol);
			}
			// 横線
			for (int i = 0; i < 1080; i += 80) {
				float y = fmodf(i + scroll, 1080.0f);
				Novice::DrawLine(0, (int)y, 1980, (int)y, gridCol);
			}
		}

		// 3. テキスト描画
		if (gameClearTimer > 60) {
			// ★変更：中心座標を画面全体（1980）の真ん中へ
			float cx = 1980.0f / 2.0f;
			float cy = 1080.0f / 2.0f;

			// アニメーション
			float scale = (gameClearTimer - 60) / 20.0f;
			if (scale > 1.0f) scale = 1.0f;
			float bounce = scale + sinf(scale * 3.14f) * 0.15f;
			if (gameClearTimer > 90) bounce = 1.0f;

			float blockSize = 12.0f * bounce;
			unsigned int mainColor = 0x00FF00FF;
			
			unsigned int shadowColor = 0x004400FF;

			// --- 図形で文字を描く関数（パターン追加版） ---
			auto DrawBlockChar = [&](char c, float x, float y, float size, unsigned int color) {
				int pattern[5][5] = {0};


				switch (c) {
					// --- 既存の文字 ---
					case 'M': pattern[0][0] = 1; pattern[0][4] = 1; pattern[1][0] = 1; pattern[1][1] = 1; pattern[1][3] = 1; pattern[1][4] = 1; pattern[2][0] = 1; pattern[2][2] = 1; pattern[2][4] = 1; pattern[3][0] = 1; pattern[3][4] = 1; pattern[4][0] = 1; pattern[4][4] = 1; break;
					case 'I': pattern[0][1] = 1; pattern[0][2] = 1; pattern[0][3] = 1; pattern[1][2] = 1; pattern[2][2] = 1; pattern[3][2] = 1; pattern[4][1] = 1; pattern[4][2] = 1; pattern[4][3] = 1; break;
					case 'S': pattern[0][1] = 1; pattern[0][2] = 1; pattern[0][3] = 1; pattern[1][0] = 1; pattern[2][1] = 1; pattern[2][2] = 1; pattern[3][4] = 1; pattern[4][0] = 1; pattern[4][1] = 1; pattern[4][2] = 1; pattern[4][3] = 1; break;
					case 'O': pattern[0][1] = 1; pattern[0][2] = 1; pattern[0][3] = 1; pattern[1][0] = 1; pattern[1][4] = 1; pattern[2][0] = 1; pattern[2][4] = 1; pattern[3][0] = 1; pattern[3][4] = 1; pattern[4][1] = 1; pattern[4][2] = 1; pattern[4][3] = 1; break;
					case 'N': pattern[0][0] = 1; pattern[0][4] = 1; pattern[1][0] = 1; pattern[1][1] = 1; pattern[1][4] = 1; pattern[2][0] = 1; pattern[2][2] = 1; pattern[2][4] = 1; pattern[3][0] = 1; pattern[3][3] = 1; pattern[3][4] = 1; pattern[4][0] = 1; pattern[4][4] = 1; break;
					case 'C': pattern[0][1] = 1; pattern[0][2] = 1; pattern[0][3] = 1; pattern[1][0] = 1; pattern[1][4] = 1; pattern[2][0] = 1; pattern[3][0] = 1; pattern[3][4] = 1; pattern[4][1] = 1; pattern[4][2] = 1; pattern[4][3] = 1; break;
					case 'L': pattern[0][0] = 1; pattern[1][0] = 1; pattern[2][0] = 1; pattern[3][0] = 1; pattern[4][0] = 1; pattern[4][1] = 1; pattern[4][2] = 1; pattern[4][3] = 1; break;
					case 'E': pattern[0][0] = 1; pattern[0][1] = 1; pattern[0][2] = 1; pattern[0][3] = 1; pattern[1][0] = 1; pattern[2][0] = 1; pattern[2][1] = 1; pattern[2][2] = 1; pattern[3][0] = 1; pattern[4][0] = 1; pattern[4][1] = 1; pattern[4][2] = 1; pattern[4][3] = 1; break;
					case 'P': pattern[0][0] = 1; pattern[0][1] = 1; pattern[0][2] = 1; pattern[1][0] = 1; pattern[1][3] = 1; pattern[2][0] = 1; pattern[2][1] = 1; pattern[2][2] = 1; pattern[3][0] = 1; pattern[4][0] = 1; break;
					case 'T': pattern[0][0] = 1; pattern[0][1] = 1; pattern[0][2] = 1; pattern[0][3] = 1; pattern[0][4] = 1; pattern[1][2] = 1; pattern[2][2] = 1; pattern[3][2] = 1; pattern[4][2] = 1; break;

						// --- ★追加した文字（R TO RETURN用）---
					case 'R':
						pattern[0][0] = 1; pattern[0][1] = 1; pattern[0][2] = 1;
						pattern[1][0] = 1; pattern[1][3] = 1;
						pattern[2][0] = 1; pattern[2][1] = 1; pattern[2][2] = 1;
						pattern[3][0] = 1; pattern[3][2] = 1;
						pattern[4][0] = 1; pattern[4][3] = 1;
						break;
					case 'U':
						pattern[0][0] = 1; pattern[0][4] = 1;
						pattern[1][0] = 1; pattern[1][4] = 1;
						pattern[2][0] = 1; pattern[2][4] = 1;
						pattern[3][0] = 1; pattern[3][4] = 1;
						pattern[4][1] = 1; pattern[4][2] = 1; pattern[4][3] = 1;
						break;
					case ' ': // スペース（何もしない）
						break;
				}

				for (int i = 0; i < 5; i++) {
					for (int j = 0; j < 5; j++) {
						if (pattern[i][j] == 1) {
							// 影と本体
							Novice::DrawBox((int)(x + j * size + 4), (int)(y + i * size + 4), (int)size, (int)size, 0.0f, shadowColor, kFillModeSolid);
							Novice::DrawBox((int)(x + j * size), (int)(y + i * size), (int)size, (int)size, 0.0f, color, kFillModeSolid);
						}
					}
				}
				};

			// --- メインメッセージの描画 ---
			const char* str1 = "MISSION";
			const char* str2 = "COMPLETE";
			float startX1 = cx - (7 * 6 * blockSize) / 2.0f;
			float startX2 = cx - (8 * 6 * blockSize) / 2.0f;

			for (int i = 0; i < 7; i++) DrawBlockChar(str1[i], startX1 + i * 6 * blockSize, cy - blockSize * 6, blockSize, mainColor);
			for (int i = 0; i < 8; i++) DrawBlockChar(str2[i], startX2 + i * 6 * blockSize, cy + blockSize * 1, blockSize, mainColor);

			
			

			// 装飾枠
			if ((gameClearTimer / 30) % 2 == 0) {
				float borderW = 800.0f * bounce;
				float borderH = 300.0f * bounce;
				Novice::DrawBox((int)(cx - borderW / 2), (int)(cy - borderH / 2), (int)borderW, (int)borderH, 0.0f, mainColor, kFillModeWireFrame);
			}
		}
	}


}



void Game::CheckpointPlayer() {
	for (size_t i = 0; i < map->Checkpoints.size(); i++) {
		auto& cp = map->Checkpoints[i];
		if (player->status_.pos.x < cp.pos.x + kTileSize &&
			player->status_.pos.x + player->status_.width > cp.pos.x &&
			player->status_.pos.y < cp.pos.y + kTileSize &&
			player->status_.pos.y + player->status_.height > cp.pos.y) {

			// 触れたらリスポーン地点を更新
// 座標を保存するとき、あえて 1px 浮かせて「めり込み」を物理的に不可能にする
			this->respawnPos.x = cp.pos.x;
			this->respawnPos.y = cp.pos.y - 30.0f; // ★ここ！

			if (!cp.isActive) {
				cp.isActive = true;
				SaveProgress(); // ここでファイル書き込み！
			}
		}
	}
}


void Game::SaveProgress() {
	// "save.txt" という名前で書き込みモードで開く
	std::ofstream ofs("./save.txt");
	if (ofs.is_open()) {
		// 座標を保存
		ofs << respawnPos.x << " " << respawnPos.y << std::endl;

		// 他に保存したいもの（ドアの開閉状況など）があれば続けて書く
		// ofs << someFlag << std::endl;

		ofs.close();
	}
}

void Game::ResetGameOver() {
	isGameOver = false;
	isRunning = false;  
	fadeState_ = FADE_NONE;

	player->InitPlayer();
	player->status_.pos = respawnPos; // プレイヤーを復活地点へ

	// ★追加: カメラも即座に復活地点へ移動させる
	// これにより、復活演出（StartRespawnAnim）が最初から画面内で見えるようになります
	scrollCamera->Update(player->status_.pos);

	player->status_.isActive = true; 
	player->StartRespawnAnim(respawnPos);

	// ボタンをすべて未入力に
	for (auto& button : map->buttons) {
		button.isPressed = false;
	}

	// ドアをすべて閉める
	for (auto& door : map->doors) {
		door.isOpen = false;
		door.openRatio = 0.0f;
	}

	// 消える床をすべて復活させる
	for (auto& floor : map->VanishingFloors) {
		floor.isActive = true;
	}

	// ギミックブロックを復活させる
	for (auto& block : map->Blocks) {
		block.isActive = true;
	}
}