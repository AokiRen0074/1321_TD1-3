#include "Game.h"
#include <Novice.h>

#include "Map.h"
#include "Player.h"
#include "ScrollCamera.h"
#include "Router.h"
#include <fstream> // ファイル操作に必要
#include "AudioManager.h"

Game::Game() {
	player = new Player();
	map = new Map();
	scrollCamera = new ScrollCamera(); // スクロールちゃん
	for (int i = 0; i < 250; i++) {
		router[i] = nullptr;
	}
	Initialize();

	audioManager = new AudioManager(); // 生成
	audioManager->LoadAll(); // 読み込み
	player->SetAudioManager(audioManager);
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

	Checkpoint = Novice::LoadAudio("./Sounds/checkpoint.mp3");

	// 音
	player->SetAudioManager(audioManager);

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
	if (!player->status_.isActive && 
		!player->IsRespawning() && 
		!player->IsDying() && 
		!isGameOver && 
		fadeState_ == FADE_NONE && // フェード中でない
		!isGameClear) // クリア中でもない
	{
		player->StartDeathAnim();
		player->status_.Velocity = {0.0f, 0.0f};
		isGameOver = true;
	}

	// 死亡演出
	if (player->IsDying()) {
		player->UpdateDeathAnim();     // ← ★ここで更新★

		if (player->deathTimer >= player->kDeathTimeMax) {
			// ここでは SceneManager に任せる
		}

		return; // 死亡中は通常更新しない
	}

	// リスポーン
	if (player->IsRespawning()) {
		player->UpdateRespawnAnim();
		return;
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
					// ==============================================
					// ゲームクリア時の完全リセット処理
					// ==============================================

					// 1. フラグとタイマーの完全リセット
					isGameClear = false;
					isRunning = false;
					cantStartCount = 0;
					gameClearTimer = 0; 

					// 2. リスポーン地点を初期位置に戻す
					respawnPos.x = 300.0f;
					respawnPos.y = 704.0f;

					// 3. プレイヤーのリセット
					player->InitPlayer();
					player->status_.pos = respawnPos;

					// 4. マップのリセット
					std::vector<std::string> layers = { "IntGrid", "HalfBlock" };
					map->LoadMapFromLDtk("./mapTest9999.ldtk", layers);

					// 5. ★重要★ カメラの状態を「最初の状態」に強制リセット
					// これがないと、カメラがステージ3のままになったりします
					scrollCamera->SetStageIndex(0);      // ステージ番号を0に戻す
					scrollCamera->SetIsScrollMode(false); // スクロール固定を解除する
					scrollCamera->Update(player->status_.pos); // プレイヤーの位置に瞬時に合わせる

					// 6. 復活演出の開始
					player->StartRespawnAnim(respawnPos);

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
				if (!player->IsRespawning()) {
					player->status_.isActive = true;
				}
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

	player->DrawDeathAnim(offset);

	// 警告メッセージの表示
	if (cantStartCount > 0) {
		Novice::ScreenPrintf(800, 400, "Router Area! Can't Start!");
	}

	// ==========================================
	//  UI (右側) の描画
	// ==========================================

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

	// (背景と区切り線のコードはそのまま...)
	Novice::DrawBox(1400, 0, 580, 1080, 0.0f, 0x11111633, kFillModeSolid);
	Novice::DrawLine(1400, 0, 1400, 1080, 0x00AAAAFF);

	// カラーパレット
	unsigned int cCyan = 0x00FFFFFF;
	unsigned int cGreen = 0x00FF00FF;
	unsigned int cRed = 0xFF0055FF;
	unsigned int cText = 0xFFFFFFFF;

	// 2. ボタンを描画する関数
	auto DrawCyberBtn = [&](Button& btn, const char* label, unsigned int color) {
		// ボタン背景・枠・アクセント (前回のコードと同じ)
		Novice::DrawBox((int)btn.x, (int)btn.y, (int)btn.w, (int)btn.h, 0.0f, 0x000000FF , kFillModeSolid);
		Novice::DrawBox((int)btn.x, (int)btn.y, (int)btn.w, (int)btn.h, 0.0f, color, kFillModeWireFrame);
		Novice::DrawBox((int)btn.x, (int)btn.y, 10, (int)btn.h, 0.0f, color, kFillModeSolid);

		int mx, my;
		Novice::GetMousePosition(&mx, &my);
		if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
			Novice::DrawBox((int)btn.x, (int)btn.y, (int)btn.w, (int)btn.h, 0.0f, (color & 0xFFFFFF44), kFillModeSolid);
		}

		// ★ここを変更！Printfをやめてドットで描画
		// size=3.0f くらいがちょうどいい大きさです
		DrawDotText(btn.x + 30, btn.y + 15, label, 3.0f, cText);
		};

	// 描画実行（英語・大文字で！）
	DrawCyberBtn(btnLeft, "MOVE LEFT", cCyan);
	DrawCyberBtn(btnRight, "MOVE RIGHT", cCyan);
	DrawCyberBtn(btnWallJump, "IF(WALL) JUMP", cGreen);
	DrawCyberBtn(btnCliffJump, "IF(AIR) JUMP", cGreen);

	DrawCyberBtn(btnStart, "START", cRed);
	DrawCyberBtn(btnReset, "STOP", cRed);


	// --- プログラムリスト（下部）の描画 ---

	// ヘッダー装飾
	Novice::DrawBox(1400, 400, 580, 30, 0.0f, 0x222233FF, kFillModeSolid);
	// ★ここもドット文字に
	DrawDotText(1420, 408, "--- MAIN FUNCTION", 2.0f, 0xAAAAAAFF);
	DrawDotText(1610, 408, " (CLICK TO DELETE)", 2.0f, RED);
	DrawDotText(1815, 408, " ---", 2.0f, 0xAAAAAAFF);

	float blockY = programArea.y + 50;

	int currentIndex = -1;
	if (isRunning) {
		currentIndex = player->GetCurrentCommandIndex();
	}

	if (isRunning) {
		// RUNNING... の文字もドット絵風に (ScreenPrintfをやめる場合)
		DrawDotText(10, 10, "RUNNING...", 10.0f, 0x00FFFF88);

		if (!player->IsRespawning()) {
			// ★ここを変更！実行中のフィルター（薄い青）
			// 0x00CCFF44 -> R=00, G=CC, B=FF (水色), A=44 (半透明)
			Novice::DrawBox(0, 0, 1400, 1080, 0.0f, 0x00CCFF22, kFillModeSolid);
		}
	} else {
		// EDIT MODE もドット絵風に
		DrawDotText(10, 10, "EDIT MODE", 10.0f, 0xAAAAAA88);
	}

	for (int i = 0; i < commandList.size(); i++) {

		unsigned int cmdColor = 0xFFFFFFFF;
		const char* cmdText = "UNKNOWN";

		// ここは大文字のみ（小文字は未実装のため）
		switch (commandList[i]) {
		case CommandType::MoveRight:
			cmdColor = cCyan;
			cmdText = "MOVE_RIGHT"; // 大文字
			break;
		case CommandType::MoveLeft:
			cmdColor = cCyan;
			cmdText = "MOVE_LEFT";
			break;
		case CommandType::CheckWallJump:
			cmdColor = cGreen;
			cmdText = "IF(WALL) JUMP";
			break;
		case CommandType::CheckCliffJump:
			cmdColor = cGreen;
			cmdText = "IF(AIR) JUMP";
			break;
		}

		if (i == currentIndex) {
			Novice::DrawBox(1410, (int)blockY, 500, 50, 0.0f, 0x550000FF, kFillModeSolid);
			// カーソル
			DrawDotText(1420, blockY + 15, ">", 3.0f, 0xFF0000FF);
		} else {
			Novice::DrawBox(1410, (int)blockY, 500, 50, 0.0f, 0x222222FF, kFillModeSolid);
		}

		Novice::DrawBox(1410, (int)blockY, 500, 50, 0.0f, cmdColor, kFillModeWireFrame);
		Novice::DrawBox(1410, (int)blockY, 5, 50, 0.0f, cmdColor, kFillModeSolid);

		// ★ここもドット文字に！
		DrawDotText(1450, blockY + 15, cmdText, 3.0f, cmdColor);


		if (i < commandList.size() - 1) {
			Novice::DrawLine(1660, (int)blockY + 50, 1660, (int)blockY + 60, 0x444455FF);
		}

		blockY += 60;
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


	// ===========================================================
	// ★チュートリアル演出（UIの上に重ねて描画！）
	// ===========================================================
	static int tutTimer = 0; // タイマー用
	tutTimer++;

	float letsX = 3200.0f;
	float letsY = 350.0f;
	float guideDx = letsX - offset.x;
	float guideDy = letsY - offset.y;

	// 看板が画面内にあるときだけ演出
	if (guideDx > -100 && guideDx < 1400) {

		// ■ スタート地点：看板の文字の右側
		float guideStartX = guideDx + 650.0f;
		float guideStartY = guideDy + 80.0f;

		// ■ ゴール地点：「MOVE RIGHT」ボタンの位置
		// Game.cpp内なので btnRight がそのまま使えます
		float guideEndX = btnRight.x + 30.0f;
		float guideEndY = btnRight.y + 25.0f;

		// --- アニメーション計算 ---
		float loopTime = fmodf((float)tutTimer, 120.0f);
		float t = loopTime / 120.0f;

		// イージング
		float easeT = 1.0f - powf(1.0f - t, 4.0f);

		// カーソル位置
		float ptrX = guideStartX + (guideEndX - guideStartX) * easeT;
		float ptrY = guideStartY + (guideEndY - guideStartY) * easeT;

		// クリック演出
		bool isClicking = (t > 0.85f && t < 0.95f);
		if (isClicking) {
			ptrY += 5.0f;
		}

		// 1. ガイド線（点線）
		float dist = sqrtf(powf(guideEndX - guideStartX, 2) + powf(guideEndY - guideStartY, 2));
		int dotCount = (int)(dist / 30.0f);

		for (int i = 0; i <= dotCount; i++) {
			float rate = (float)i / dotCount;
			float flowRate = fmodf(rate - (tutTimer * 0.02f), 1.0f);
			if (flowRate < 0) flowRate += 1.0f;

			float dotX = guideStartX + (guideEndX - guideStartX) * flowRate;
			float dotY = guideStartY + (guideEndY - guideStartY) * flowRate;

			// 点線を描画
			Novice::DrawBox((int)dotX, (int)dotY, 6, 6, 0.0f, 0x00AAAA88, kFillModeSolid);
		}

		// 2. マウスカーソルの描画（ラムダ式再定義）
		auto DrawCursor = [&](float px, float py, unsigned int color) {
			// カーソルの「影」や「枠」を考慮して、少し大きめに描画します

			// --- 本体（三角形） ---
			// 頂点(0,0) -> 左下(0, 28) -> 右(22, 22) の直角っぽい三角形
			Novice::DrawTriangle(
				(int)px, (int)py,            // 先端
				(int)px, (int)py + 28,       // 左下
				(int)px + 22, (int)py + 22,  // 右
				color, kFillModeSolid
			);

			// --- 持ち手（しっぽ） ---
			// 三角形の下辺の「内側」から線を伸ばして、自然に繋げる
			// Start(8, 20) -> End(16, 36)
			// 線を何本か重ねて太さを出す
			for (int w = 0; w < 7; w++) {
				Novice::DrawLine(
					(int)px + 8 + w, (int)py + 20, // 根元（本体の内側に隠す）
					(int)px + 16 + w, (int)py + 36, // 先端
					color
				);
			}
			};

		// 影
		DrawCursor(ptrX + 3, ptrY + 3, 0x00000088);

		// 本体
		unsigned int cursorCol = isClicking ? 0xFFFFFFFF : 0x00FFFFFF;
		DrawCursor(ptrX, ptrY, cursorCol);

		// 3. クリック波紋
		if (isClicking) {
			float rippleSize = (t - 0.85f) * 500.0f;
			unsigned int rippleAlpha = (unsigned int)((0.95f - t) * 10.0f * 255.0f);
			unsigned int rippleCol = (0x00FFFF00) | rippleAlpha;

			// 波紋
			Novice::DrawEllipse((int)ptrX, (int)ptrY, (int)rippleSize, (int)rippleSize, 0.0f, rippleCol, kFillModeWireFrame);

			// ボタン位置の枠線強調
			Novice::DrawBox((int)btnRight.x - 5, (int)btnRight.y - 5, (int)btnRight.w + 10, (int)btnRight.h + 10, 0.0f, rippleCol, kFillModeWireFrame);
		}
	}

}



void Game::CheckpointPlayer() {
	// map->Checkpoints の中身を直接書き換えるために「&」を付ける
	for (auto& cp : map->Checkpoints) {

		// プレイヤーとチェックポイントの当たり判定（矩形交差）
		// 判定を少し甘く（広めに）しておくとスムーズです
		if (player->status_.pos.x < cp.pos.x + kTileSize * 2.0f &&
			player->status_.pos.x + player->status_.width > cp.pos.x &&
			player->status_.pos.y < cp.pos.y + kTileSize &&
			player->status_.pos.y + player->status_.height > cp.pos.y) {

			// リスポーン地点を更新
			this->respawnPos.x = cp.pos.x;
			this->respawnPos.y = cp.pos.y - 32.0f; // 少し浮かせる

			// まだアクティブでない場合のみ処理
			if (!cp.isActive) {

				Novice::PlayAudio(Checkpoint, false, 1.0f);

				cp.isActive = true;  // ★ここが map 内のデータに反映される
				SaveProgress();      // 保存
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

	player->InitPlayer();
	player->status_.pos = respawnPos; // プレイヤーを復活地点へ

	// 復活演出を開始する
	player->StartRespawnAnim(player->status_.pos);

	// カメラも即座に復活地点へ移動させる
	scrollCamera->Update(player->status_.pos);

	// カメラも合わせる
	scrollCamera->Update(respawnPos);

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
		// ボタンで出現させていたなら、再び非表示にする
		block.isActive = false;

		// もし座標が変わっていたなら、初期位置（Map読み込み時の位置）に戻す
		block.pos = block.STpos;
	}

	for (auto& water : map->waters) {
		water.isActive = true;
	}

	for (auto& Beltconveyor : map->Beltconveyors) {
		Beltconveyor.isReversed = true;
	}
}