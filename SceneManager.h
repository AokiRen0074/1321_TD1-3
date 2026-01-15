#pragma once
// 各シーンをインクルード
#include "Title.h"
#include "Game.h"

enum class Scene {
	TITLE,
	GAME,

	CREDIT,
	SETTINGS,
	TUTORIAL,
	STAGESELECT,
	CLEAR,
	GAMEOVER,
	EXIT_GAME,
};

class SceneManager {
public:
	// コンストラクタ
	SceneManager();

	// デストラクタ
	~SceneManager();

	void Run(); // mainのループ内で呼ぶメイン処理
	
private:
	// シーンの更新
	Scene currentScene; // 現在のシーン
	// 各シーンのポインタ
	Title* titleScene;
	Game* gameScene;

	// キー入力を受け取るための変数(Novice用)
	char keys[256] = {0};
	char preKeys[256] = {0};
};

