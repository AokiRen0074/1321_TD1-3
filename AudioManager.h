#pragma once

enum AudioID {
	// 音が増えたらここに追加しろや

	/// <summary>
	/// SE
	/// </summary>
	SE_JUMP,
	SE_DEATH,
	SE_RESPAWN,
	SE_BELTCONVEYORS,


	/// <summary>
	/// BGM
	/// </summary>
	BGM_TITLE,
	BGM_GAME,
	BGM_CLEAR,
	
};

class AudioManager {
public:
	void LoadAll();
	void Play(AudioID id, bool loop);
	void Stop(AudioID id);

	// 音量をセット
	void SetVolume(AudioID id, float volume);

private:
	int handles[100];
	int playHandles[100]; // 今鳴らしている音の番号
	float volumes[100]; // 各音源の音量を保持する配列
};

