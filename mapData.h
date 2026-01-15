#pragma once

class Map {
	static const int kMapHeight_ = 28;
	static const int kMapWidth_ = 38;
	const int kTileSize_ = 32;

	// マップデータ
	int mapData[kMapHeight_][kMapWidth_] = { 0 };

	void LoadMapFromLdtk(const char* fileName);

};