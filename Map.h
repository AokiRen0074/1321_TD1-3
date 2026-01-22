#pragma once
#include "Const.h"
#include <vector>
#include <string>
#include "Vector2.h"
#include "Gimmick.h"

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

struct Water {
	Vector2 pos;  // 座標
	int linkId;   // LDtkで設定したIntegerの値
	bool isActive;  // 開いているか
};

//ベルトコンベアー
struct Beltconveyor {
	Vector2 pos;  // 座標
	int linkId;   // LDtkで設定したIntegerの値
	float speed;
	bool isReversed;  // 開いているか
};

//チェックポイント
struct Checkpoint {
	Vector2 pos;  // 座標
	int linkId;   // LDtkで設定したIntegerの値
	bool isActive;  // 開いているか
};

struct VanishingFloor {
	Vector2 pos;  // 座標
	int linkId;   // LDtkで設定したIntegerの値
	bool isActive;  // 開いているか
};

struct Block {
	Vector2 pos;  // 座標
	int linkId;   // LDtkで設定したIntegerの値
	bool isActive;  // 開いているか
};

class Map {
public:
	// マップデータ
	int mapData[kMapHeight][kMapWidth];

	// ブロックごとの画像を保存
	int blockTextures[kMaxBlocksType];

	int buttonOnTexture;
	int buttonOffTexture;

	// コンストラクタ
	Map();

	std::vector<Door> doors;
	std::vector<ButtonA> buttons;
	std::vector<LiftGimmickBlock> liftBlocks;
	std::vector<Water> waters;

	std::vector<LiftGimmickButton> liftButtons;
	std::vector<Beltconveyor> Beltconveyors;
	std::vector< Checkpoint>Checkpoints;//チェックポイント
	std::vector< VanishingFloor>VanishingFloors;//チェックポイント
	std::vector< Block>Blocks;//チェックポイント

	// 初期化
	void Initialize();

	// LDtk読み込み
	void LoadMapFromLDtk(const char* fileName,const std::vector<std::string>& layerName);

	// 更新
	void Update(Player& player);

	// 描画
	void Draw(Vector2 offset);
};