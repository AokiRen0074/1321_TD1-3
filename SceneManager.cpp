#include "SceneManager.h"
#include "GameOver.h"
#include <Novice.h>
#include "Game.h"

// コンストラクタ
SceneManager::SceneManager() {
	currentScene = Scene::TITLE; // 最初はタイトルから

	// シーンの実体を生成
	titleScene = new Title();
	gameScene = new Game();
	gameOverScene = new GameOver();

	// 起動時にタイトルのbgmを鳴らす
	gameScene->audioManager->Play(BGM_TITLE, true);
}

// デストラクタ
SceneManager::~SceneManager() {
	delete titleScene;
	delete gameScene;
	delete gameOverScene;
}

void SceneManager::Run() {
	// キー入力の更新
	memcpy(preKeys, keys, 256);
	Novice::GetHitKeyStateAll(keys);

	// シーンごとの更新処理
	switch (currentScene) {
		case Scene::TITLE:
			titleScene->Update();

			// エンターキーが押されたらGAMEシーンへ
			if (keys[DIK_RETURN] && !preKeys[DIK_RETURN]) {
				currentScene = Scene::GAME;
				gameScene->audioManager->Stop(BGM_TITLE);
				gameScene->audioManager->Play(BGM_GAME, true);
			}

			break;

		case Scene::GAME:
			gameScene->Update(keys, preKeys);

			if (gameScene->isGameOver) {
				currentScene = Scene::GAMEOVER;
				gameOverScene->Reset(); // タイマーやフラグをリセット
			}

			break;

		case Scene::GAMEOVER:
			gameOverScene->Update(keys, preKeys);
			gameScene->Update(keys, preKeys);

			if (gameOverScene->ShouldRetry()) {
				//gameOverScene->Reset();
				// ゲームシーンに戻す
				currentScene = Scene::GAME;

				gameScene->ResetGameOver();

				// ゲーム側のフラグをリセット
				//gameScene->isGameOver = false;
				//gameScene->SetIsRunning(false);

				//gameScene->Initialize();

				// プレイヤーの復活
				//Player* player = gameScene->GetPlayer();
				//Vector2 respawnPos = gameScene->GetRespawnPos();
				//player->InitPlayer();

				// 復活演出
				//player->StartRespawnAnim(respawnPos);

				// ゲームbgmを再度鳴らす
				if (gameScene->audioManager != nullptr) {
					gameScene->audioManager->Play(BGM_GAME, true); 
				}
				
			}

			break;
	}

	// シーンごとの描画処理
	switch (currentScene) {
		case Scene::TITLE:
			titleScene->Draw();
			break;

		case Scene::GAME:
			gameScene->Draw();
			break;

		case Scene::GAMEOVER:
			gameScene->Draw(); // 背景にゲーム画面を出したまま
			gameOverScene->Draw();
			break;
	}
}