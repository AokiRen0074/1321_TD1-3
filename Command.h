#pragma once


// プレイヤーのコマンドの種類
enum class CommandType {
    None,
    MoveRight,      // 左があるまで右
	MoveLeft,       // 右があるまで左
    CheckWallJump,  // 前に壁があったらジャンプ
    CheckCliffJump  // 下がブロックじゃなかったらジャンプ
};