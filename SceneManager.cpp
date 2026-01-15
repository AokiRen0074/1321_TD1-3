#include "SceneManager.h"
#include <Novice.h>

// コンストラクタ
SceneManager::SceneManager() {
    currentScene = Scene::TITLE; // 最初はタイトルから
    
    // シーンの実体を生成
    titleScene = new Title();
    gameScene = new Game();

}

// デストラクタ
SceneManager::~SceneManager() {
    delete titleScene;
    delete gameScene;
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
            }
            break;

        case Scene::GAME:
            gameScene->Update(keys, preKeys);
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
    }
}