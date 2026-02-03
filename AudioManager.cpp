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

	// 復活
	handles[SE_RESPAWN] = Novice::LoadAudio("./Sounds/respawn.mp3");
	volumes[SE_RESPAWN] = 1.5f;

	// ベルトコンベア
	handles[SE_BELTCONVEYORS] = Novice::LoadAudio("./Sounds/beltconveyors.mp3");
	volumes[SE_BELTCONVEYORS] = 1.0f;

	///// BGM /////
	// スタート(TITLE)
	handles[BGM_TITLE] = Novice::LoadAudio("./Sounds/title.mp3");
	volumes[BGM_TITLE] = 1.0f;

	// ゲーム
	handles[BGM_GAME] = Novice::LoadAudio("./Sounds/mainGame.mp3");
	volumes[BGM_GAME] = 0.5f;

	// クリア
	handles[BGM_CLEAR] = Novice::LoadAudio("./Sounds/clearKakkoKari.mp3");
	volumes[BGM_CLEAR] = 1.2f;

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
	// 再生中のハンドルに対して音量を即座に反映させる
	if (Novice::IsPlayingAudio(playHandles[id])) {
		Novice::SetAudioVolume(playHandles[id], volumes[id]);
	}
}