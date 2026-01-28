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
	ROUTER = 3,
	SCRAPMACHINE = 4,
	WATER = 5
};

struct RespawnParticle {
	Vector2 startPos;  // スタート地
	Vector2 targetPos; // ゴール地点
	Vector2 currentPos;// 現在地
	float size;        // 粒子の大きさ
	unsigned int color;// 色
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
		bool isAlive;
		float jumpPower;//ジャンプ力
		bool isJumop;//ジャンプフラグ
		float radius;

		int waitTimer;

		bool isMoveFree;//自由に動けるかのフラグ
		bool isCommandMove;//コマンドで動かす範囲
		bool isWaitingForLanding;//着地待ちフラグ
		bool isBlet;
		bool isBlack;
		bool isLift;
	}status_;//status＿で宣言

	// 音
	int soundJump = -1;
	bool isRespawning = false;       // 演出中フラグ
	int respawnTimer = 0;            // タイマー
	const int kRespawnTimeMax = 60;  // 演出にかかる時間（1秒）
	std::vector<RespawnParticle> particles;

	// ★追加: 復活演出を開始する関数
	void StartRespawnAnim(Vector2 centerPos);

	// ★追加: 復活演出の更新と描画
	void UpdateRespawnAnim();
	void DrawRespawnAnim(Vector2 offset);

	// ★追加: 演出中かどうか
	bool IsRespawning() { return isRespawning; }

	Player();
	void InitPlayer();
	void UpdatePlayer(char keys[256], char preKeys[256], int  mapData[kMapHeight][kMapWidth], std::vector<Block>& blocks);
	// コマンドで動かせる
	void UpdateByCommands(const std::vector<CommandType>& commands, int mapData[kMapHeight][kMapWidth],
		std::vector<Beltconveyor>& Beltconveyors, std::vector<Block>& blocks, std::vector<LiftGimmickBlock>& liftBlocks);
	void DrawPlayer(Vector2 offset);

	// 今どのコマンドを実行中か
	int GetCurrentCommandIndex() const { return cmdIndex; }

	//ルーターの通信範囲内でさらにその中でも自由に行動できるのかの関数
	// 引数でルーターの配列を受け取るようにする
	bool CheckRouter(Router* router[], int count);
	void CheckDoorCollision(std::vector<Door>& doors);
	void CheckWaterCollision(std::vector<Water>& waters);
	void CheckBeltCollision(std::vector<Beltconveyor>& Beltconveyors);
	void CheckFlooCollision(std::vector< VanishingFloor>& VanishingFloors);
	void CheckBlockWall(std::vector<Block>& blocks);
	void CheckBlockGround(std::vector<Block>& blocks);
	void CheckBlockCeiling(std::vector<Block>& blocks);
	void CheckLiftCollision(std::vector<LiftGimmickBlock>& liftBlocks); // リフトとの当たり判定

	void MovePlayer(char keys[256], char preKeys[256], int  mapData[kMapHeight][kMapWidth]);
	void Gravity();

	// コマンドの処理関数
	// アクション関数
	void ActionMoveRight();
	void ActionTryJump();

	// センサー関数
	bool IsWallAhead(int mapData[kMapHeight][kMapWidth], std::vector<Block>& blocks);
	bool IsCliffAhead(int mapData[kMapHeight][kMapWidth], const std::vector<Beltconveyor>& Beltconveyors, const std::vector<Block>& blocks, const std::vector<LiftGimmickBlock>& liftBlocks);

	// マップの当たり判定関数
	void isGrounded(int mapData[kMapHeight][kMapWidth], int mapId);
	void isRightWall(int mapData[kMapHeight][kMapWidth], int mapId);
	void isLeftWall(int mapData[kMapHeight][kMapWidth], int mapId);
	void isTopWall(int mapData[kMapHeight][kMapWidth], int mapId);



	// コマンド実行時のインデックス
	int cmdIndex;

};

