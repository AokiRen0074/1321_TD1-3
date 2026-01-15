#pragma once
#include "Const.h"
#include <vector>
#include <string>
#include "Vector2.h"

// 一応最大ブロック種類の定義
const int kMaxBlocksType = 100;

struct Door {
	Vector2 pos;  // 座標
	int linkId;   // LDtkで設定したIntegerの値
	bool isOpen;  // 開いているか
};

struct ButtonA {
	Vector2 pos;  // 座標
	int linkId;   // LDtkで設定したIntegerの値
	bool isPressed; // 押されたか
};

class Map {
public:
	// マップデータ
	int mapData[kMapHeight][kMapWidth];

	// ブロックごとの画像を保存
	int blockTextures[kMaxBlocksType];

	// コンストラクタ
	Map();

	std::vector<Door> doors;
	std::vector<ButtonA> buttons;

	// 初期化
	void Initialize();

	// LDtk読み込み
	void LoadMapFromLDtk(const char* fileName,const std::vector<std::string>& layerName);

	// 描画
	void Draw(Vector2 offset);
};