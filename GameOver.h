#pragma once

class GameOver {
public:
	void Update(char keys[], char preKeys[]);
	void Draw();
	bool ShouldRetry() { return retryFlag; } // リトライが押されたらGAMEシーンに戻る

	void Reset() { 
		retryFlag = false; 
		internalTimer = 0;
	}

private:
	bool retryFlag = false;
	int internalTimer = 0; // 演出用のタイマー

};