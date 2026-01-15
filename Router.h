#pragma once
#include <vector>    
#include "Vector2.h"
#include "Map.h"
class Router
{

public:

	struct RouterStatus
	{
		Vector2 pos;
		float radius;//自由に動けるゾーン
		float bigRadius;//プレイヤーをコマンドで動かせるゾーン
	
	}router_;
	int id_;

	Router(int id, int mapData[kMapHeight][kMapWidth]);
	void InitRouter(int id, int mapData[kMapHeight][kMapWidth]);
	void UpdateRouter(int mapData[kMapHeight][kMapWidth]);
	void DrawRouter(Vector2 offset);
	

};

