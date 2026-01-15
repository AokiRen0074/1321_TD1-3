#include <Novice.h>
#define _USE_MATH_DEFINES
#include <assert.h>
#include "Map.h"
#include <math.h>
#include "Player.h"
#include "Game.h"
// (^▽^)/あ


#include "Vector2.h"
#include "SceneManager.h"
#include "Game.h"
#include "Router.h"
#include "ScrollCamera.h"

// (^▽^)/あ
// (^▽^)/あ

const char kWindowTitle[] = "TD1_3";

//////////
/// 構造体
//////////

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	//////////
	/// 変数の宣言
	//////////
	
	// 画面
	Vector2 screenSize;
	screenSize.x = 1980.0f;
	screenSize.y = 1080.0f;

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y));

	// キー入力結果を受け取る箱
	char keys[256] = {0};
	char preKeys[256] = {0};

	// スクリーンモード
	bool screenMode = false;

  
	// ゲームシーンマネージャーを生成
	SceneManager* sceneManager = new SceneManager();

	//int textureBlock = Novice::LoadTexture("./block.png");

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓更新処理ここから
		///

		// ゲームシーンの管理
		sceneManager->Run();

		// スクリーン
		if(keys[DIK_P]&& !preKeys[DIK_P]){
			screenMode = !screenMode;
		}

		if (screenMode) {
			Novice::SetWindowMode(kFullscreen);
		} else {
			Novice::SetWindowMode(kWindowed);
		}
		/*for (int i = 0; i < 250; i++) {
			router[i]->UpdateRouter(map->mapData);
		}*/
		//////////
		/// 自機
		//////////

	

		//////////
		/// 座標変換
		//////////

		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		//map->Draw();
		
		
		//////////
		/// デバッグ処理
		//////////

		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	// 解放エクササイズ
	delete sceneManager;

	// ライブラリの終了
	Novice::Finalize();
	return 0;
}
