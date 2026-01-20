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
	gameArea = { 0, 0, 1400, 1080 };
	paletteArea = { 1400, 0, 580, 400 };     // 右上：パレット
	programArea = { 1400, 400, 580, 680 };   // 右下：プログラム置き場

	isRunning = false;
	player->InitPlayer();
	commandList.clear();

	map->Initialize();
	if (respawnPos.x == 0 && respawnPos.y == 0) {
		respawnPos.x = 300.0f;
		respawnPos.y = 704.0f;
	}

	/*------------------------------
	ここにレイヤー名をいれるんだ！！
	-----------------------------*/
	std::vector<std::string> layers = { "IntGrid","HalfBlock" };

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

	// ★パレットエリアにボタンを配置
	float btnX = 1450;
	float btnW = 400;
	float btnH = 50;

	// 左・右・壁・崖 の順に並べる
	btnLeft = { btnX, 50,  btnW, btnH, "<< Move Left",      CommandType::MoveLeft,      0x44AAFFFF };
	btnRight = { btnX, 110, btnW, btnH, ">> Move Right",     CommandType::MoveRight,     0x44AAFFFF };
	btnWallJump = { btnX, 170, btnW, btnH, "If Wall -> Jump",  CommandType::CheckWallJump, (int)0xFFAA44FF };
	btnCliffJump = { btnX, 230, btnW, btnH, "If Air -> Jump",   CommandType::CheckCliffJump,(int)0xFFAA44FF };

	btnStart = { 1450, 300, 180, 80, "START >", (CommandType)-1, (int)0xFF4444FF };
	btnReset = { 1670, 300, 180, 80, "STOP []", (CommandType)-1, 0x44FF44FF };
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

	if (!player->status_.isActive) {
		// 保存されているリスポーン地点（初期位置 or 最後に触れた旗）に戻す
		player->status_.pos = respawnPos;

		isRunning = false;
		cantStartCount = 0;
		player->InitPlayer(); // 速度などをゼロにする
	}

	// 例えば Map::Update() や GameScene::Update() などで

// 全部のボタンをチェック
	for (auto& btn : map->buttons) {


		// プレイヤーがボタンを踏んだら
		if (IsPlayerHit(player, btn)) {
			btn.isPressed = true;

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
		// --- 実行モード ---
		bool isArrived = player->CheckRouter(router, 250);
		//ドアと水とベルトの当たり判定とかの処理
		player->CheckDoorCollision(map->doors);
		player->CheckWaterCollision(map->waters);
		player->CheckBeltCollision(map->Beltconveyors);

		if (isArrived) {
			isRunning = false;
		}
		else {
			player->UpdateByCommands(commandList, map->mapData, map->Beltconveyors);
		}

		// ストップボタン判定
		if (isClick) {
			if (mouseX >= btnReset.x && mouseX <= btnReset.x + btnReset.w &&
				mouseY >= btnReset.y && mouseY <= btnReset.y + btnReset.h) {
				isRunning = false;
				player->InitPlayer();
			}
		}

	}
	else {
		// --- 編集モード ---
		player->UpdatePlayer(keys, preKeys, map->mapData);
		//player->CheckRouter(router, 250);
		bool isInsideRouter = player->CheckRouter(router, 250);

		//ドアと水とベルトの当たり判定とかの処理
		player->CheckDoorCollision(map->doors);
		player->CheckWaterCollision(map->waters);
		player->CheckBeltCollision(map->Beltconveyors);


		if (isClick) {
			// 1. パレットのボタンを押してコマンドを追加
			if (mouseX >= btnRight.x && mouseX <= btnRight.x + btnRight.w && mouseY >= btnRight.y && mouseY <= btnRight.y + btnRight.h) {
				commandList.push_back(btnRight.cmdType);
			}

			if (mouseX >= btnLeft.x && mouseX <= btnLeft.x + btnLeft.w && mouseY >= btnLeft.y && mouseY <= btnLeft.y + btnLeft.h) {
				commandList.push_back(btnLeft.cmdType);
			}

			if (mouseX >= btnWallJump.x && mouseX <= btnWallJump.x + btnWallJump.w && mouseY >= btnWallJump.y && mouseY <= btnWallJump.y + btnWallJump.h) {
				commandList.push_back(btnWallJump.cmdType);
			}
			if (mouseX >= btnCliffJump.x && mouseX <= btnCliffJump.x + btnCliffJump.w && mouseY >= btnCliffJump.y && mouseY <= btnCliffJump.y + btnCliffJump.h) {
				commandList.push_back(btnCliffJump.cmdType);
			}

		

			// 2. スタートボタン
			if (mouseX >= btnStart.x && mouseX <= btnStart.x + btnStart.w && mouseY >= btnStart.y && mouseY <= btnStart.y + btnStart.h) {



				if (!isInsideRouter) {
					isRunning = true;
					player->InitPlayer();
					cantStartCount = 0;
				}
				else {
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

					// このコマンドを削除する
					commandList.erase(commandList.begin() + i);
					break;
				}
				blockY += 60; // 次のブロックの位置へ
			}
		}
	}

	// マップ更新(当たり判定など)
	map->Update(*player);

	// スクロールカメラ
	// プレイヤーの座標を渡してカメラを更新
	scrollCamera->Update(player->status_.pos);

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
			router[i]->DrawRouter(offset); // Routerクラスにある描画関数を呼ぶ
		}
	}

	// マップ描画
	map->Draw(offset);

	// プレイヤー描画
	player->DrawPlayer(offset);



	// 警告メッセージの表示
	if (cantStartCount > 0) {
		Novice::ScreenPrintf(800, 400, "Router Area! Can't Start!");
	}

	// --- UIボタン描画 ---
	auto DrawBtn = [](Button& b) {
		Novice::DrawBox((int)b.x, (int)b.y, (int)b.w, (int)b.h, 0.0f, b.color, kFillModeSolid);
		Novice::ScreenPrintf((int)b.x + 20, (int)b.y + 20, b.label);
		};

	DrawBtn(btnRight);
	DrawBtn(btnLeft);
	DrawBtn(btnWallJump);
	DrawBtn(btnCliffJump);
	DrawBtn(btnStart);
	DrawBtn(btnReset);


	// --- プログラムリストのブロック描画 ---
	Novice::ScreenPrintf(1420, 410, "--- YOUR PROGRAM (Click to Delete) ---");

	float blockY = programArea.y + 50;

	int currentIndex = -1;
	if (isRunning) {
		currentIndex = player->GetCurrentCommandIndex();
	}

	for (int i = 0; i < commandList.size(); i++) {
		unsigned int color = 0xFFFFFFFF;
		const char* text = "";

		// コマンドの種類によって色と文字を変える
		switch (commandList[i]) {
		case CommandType::MoveRight:
			color = 0x44AAFFFF; // 青
			text = "Move Right";
			break;
		case CommandType::MoveLeft:
			color = 0x44AAFFFF; // 青
			text = "Move Left";
			break;
		case CommandType::CheckWallJump:
			color = 0xFFAA44FF; // オレンジ
			text = "If Wall -> Jump";
			break;
		case CommandType::CheckCliffJump:
			color = 0xFFAA44FF; // オレンジ
			text = "If Air -> Jump";
			break;

		

		}
		// もし今実行しているコマンドなら、色を「赤」に変えて目立たせる！
		if (i == currentIndex) {
			color = 0xFF0000FF; // 赤色
		}

		// ブロック描画
		Novice::DrawBox(1450, (int)blockY, 400, 50, 0.0f, color, kFillModeSolid);
		Novice::ScreenPrintf(1470, (int)blockY + 15, "%d: %s", i + 1, text);

		// さらに左側に矢印「▶」を出して強調する
		if (i == currentIndex) {
			Novice::ScreenPrintf(1420, (int)blockY + 15, "->");
			// 少しずらして枠っぽく描画する演出などを入れてもOK
			Novice::DrawBox(1445, (int)blockY - 5, 410, 60, 0.0f, RED, kFillModeWireFrame);
		}

		// つながりの矢印
		if (i < commandList.size() - 1) {
			Novice::DrawTriangle(1650, (int)blockY + 50, 1630, (int)blockY + 60, 1670, (int)blockY + 60, WHITE, kFillModeSolid);
		}

		blockY += 60; // 次の表示位置へ
	}

	// 実行中などのステータス表示
	if (isRunning) {
		Novice::ScreenPrintf(10, 10, "RUNNING...");
		// 画面全体に枠を表示して実行中っぽくする
		Novice::DrawBox(0, 0, 1400, 1080, 0.0f, 0xFF000044, kFillModeSolid);
	}
	else {
		Novice::ScreenPrintf(10, 10, "EDIT MODE");
	}

	int mx, my;
	Novice::GetMousePosition(&mx, &my);
	Novice::ScreenPrintf(0, 0, "Mouse: %d, %d", mx, my);
	Novice::ScreenPrintf(0, 20, "BtnRight X: %d", (int)btnRight.x);
	Novice::ScreenPrintf(0, 80, "player Pos X: %.2f", player->status_.pos.x);
	Novice::ScreenPrintf(0, 110, "player Pos Y: %.2f", player->status_.pos.y);
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
}



void Game::CheckpointPlayer() {
	for (size_t i = 0; i < map->Checkpoints.size(); i++) {
		auto& cp = map->Checkpoints[i];
		if (player->status_.pos.x < cp.pos.x + kTileSize &&
			player->status_.pos.x + player->status_.width > cp.pos.x &&
			player->status_.pos.y < cp.pos.y + kTileSize &&
			player->status_.pos.y + player->status_.height > cp.pos.y) {

			// 触れたらリスポーン地点を更新
			this->respawnPos = cp.pos;

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
