#pragma once
#include "Vector2.h"
#include "const.h"
#include "Command.h" 
#include <vector>
#include "Router.h"

enum TileType {
	NONE = 0,
	BLOCK = 1,
	HALF_FLOOR = 2,
	ROUTER = 3
};

class Player
{

public:
	// --- プレイヤーステータス ---
	struct PlayerStatus
	{
		//プレイヤーの位置
		Vector2 pos;

		//プレイヤーの加速度
		Vector2 acceleration;
		Vector2 Velocity;//重力系
		float Speed;

		Vector2 scale;//大きさ
		float moveDir;//プレイヤーの向き

		float height;
		float width;

		bool isActive;//生存フラグ
		float jumpPower;//ジャンプ力
		bool isJumop;//ジャンプフラグ
		float radius;


		bool isMoveFree;//自由に動けるかのフラグ
		bool isCommandMove;//コマンドで動かす範囲
	}status_;//status＿で宣言


	Player();
	void InitPlayer();
	void UpdatePlayer(char keys[256], char preKeys[256], int  mapData[kMapHeight][kMapWidth]);
	// コマンドで動かせる
	void UpdateByCommands(const std::vector<CommandType>& commands, int mapData[kMapHeight][kMapWidth]);
	void DrawPlayer(Vector2 offset);

	// 今どのコマンドを実行中か
	int GetCurrentCommandIndex() const { return cmdIndex; }

	//ルーターの通信範囲内でさらにその中でも自由に行動できるのかの関数
	// 引数でルーターの配列を受け取るようにする
	bool CheckRouter(Router* router[], int count);

private:

	void MovePlayer(char keys[256], char preKeys[256], int  mapData[kMapHeight][kMapWidth]);
	void Gravity();

	// コマンドの処理関数
	// アクション関数
	void ActionMoveRight();
	void ActionTryJump();

	// センサー関数
	bool IsWallAhead(int mapData[kMapHeight][kMapWidth]);
	bool IsCliffAhead(int mapData[kMapHeight][kMapWidth]);

	// マップの当たり判定関数
	void isGrounded(int mapData[kMapHeight][kMapWidth],int mapId);
	void isRightWall(int mapData[kMapHeight][kMapWidth], int mapId);
	void isLeftWall(int mapData[kMapHeight][kMapWidth], int mapId);
	void isTopWall(int mapData[kMapHeight][kMapWidth], int mapId);

	// コマンド実行時のインデックス
	int cmdIndex;

};

