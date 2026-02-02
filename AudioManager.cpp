#include "AudioManager.h"
#include <Novice.h>

// AudioManager.cpp の読み込み部分
void AudioManager::LoadAll() {
	// 音源を差し入れる、音の種類と音量
	
	///// SE /////
	// ジャンプ
	handles[SE_JUMP] = Novice::LoadAudio("./Sounds/jump.mp3");
	volumes[SE_JUMP] = 1.0f;

	// 死亡
	handles[SE_DEATH] = Novice::LoadAudio("./Sounds/death.mp3");
	volumes[SE_DEATH] = 0.5f;

	///// BGM /////
	// スタート(TITLE)
	handles[BGM_TITLE] = Novice::LoadAudio("./Sounds/title.mp3");
	volumes[BGM_TITLE] = 1.0f;

	// ゲーム
	handles[BGM_GAME] = Novice::LoadAudio("./Sounds/mainGame.mp3");
	volumes[BGM_GAME] = 0.5f;

	// handles[BGM_MAIN] = Novice::LoadAudio("./Resources/Sounds/test_bgm.wav");
}

void AudioManager::Play(AudioID id, bool loop) {
	// 指定されたIDのハンドルを使って再生
	playHandles[id] = Novice::PlayAudio(handles[id], loop, volumes[id]);
}

void AudioManager::Stop(AudioID id) {
	Novice::StopAudio(playHandles[id]);
}

void AudioManager::SetVolume(AudioID id, float volume) {
	volumes[id] = volume;
}