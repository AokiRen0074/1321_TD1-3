#pragma once
#include "Vector2.h"

#include "Player.h"
#include "Command.h"
#include "const.h" 
#include <vector>
#include "Map.h"
#include "ScrollCamera.h"
#include "Router.h"

#include <iostream>
#include <fstream>

struct GameScreen {
	Vector2 position;
	Vector2 size;
};


struct CommandScreen {
	Vector2 position;
	Vector2 size;
};


// コマンドのボタン
struct Button {
	float x, y, w, h;
	const char* label;
	CommandType cmdType; // Noneならシステム用ボタン
	int color;
	int textureHandle;
};

class Game {
public:
	Game();  // コンストラクタの宣言
	~Game(); // デストラクタの宣言

	// 初期化
	void Initialize();

	// ゲームシーン
		// 更新
	void Update(char keys[256], char preKeys[256]);

	// 描画
	void Draw();



private:
	// screen
// ゲーム画面とコマンド設定画面を分ける
// ゲーム(1400*1080),コマンド(580*1080),計1980*1080
	GameScreen gameScreen;
	CommandScreen commandScreen;


	Player* player;
	Map* map;

	Router* router[250]; // ルーターの配列
	int routerCount = 0;   // 実際にマップにあったルーターの数

	ScrollCamera* scrollCamera;

	// ゲームシーン

	// エリア
	struct Area { float x, y, w, h; };
	Area gameArea;    // 左側：ゲーム画面
	Area paletteArea; // 右上：コマンド選択
	Area programArea; // 右下：プログラム置き場


	// プレイヤーとマップデータ

	int mapData[kMapHeight][kMapWidth]; // Gameクラスで持つか、外部から渡す想定

	// 実行モードフラグ (true:実行中, false:編集・停止中)
	bool isRunning;
	int cantStartCount = 0;

	// コマンドリスト
	std::vector<CommandType> commandList;


	// ボタン類
	Button btnRight;
	Button btnLeft;
	Button btnWallJump;
	Button btnCliffJump;
	Button btnStart;
	Button btnReset;
	Button btnNone;

	int texRight;
	int texLeft;
	int texWallJump;
	int texCliffJump;
	int texStart;
	int texStop;

	// 更新
	void Update();

	void CheckpointPlayer();

	void SaveProgress();
	
	// Game.h
	Vector2 respawnPos = { 300.0f, 704.0f }; // 初期値を設定しておく

	///// 画面の暗転処理(切り替えかも～おいしいかも～) /////
	enum FadeState { FADE_NONE, FADE_OUT, FADE_IN };
	FadeState fadeState_ = FADE_NONE;
	int fadeTimer_ = 0;
	const int kFadeMax = 60; // 演出にかけるフレーム数

	// 画面分割の設定
	const int kBlockSize = 120;
	const int kCols = (1920 / 120) + 1; // 32列
	const int kRows = (1080 / 120) + 1; // 18列
};

