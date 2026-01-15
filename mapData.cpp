#include <fstream>
#include "json.hpp"
#include <Novice.h>
#include "mapData.h"
#include "const.h"

using json = nlohmann::json;

// マップ読み込み関数
void LoadMapFromLDtk(const char* fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        Novice::ScreenPrintf(0, 0, "Error: File not found.");
        return;
    }
    json data;
    file >> data;

    auto& level = data["levels"][0];
    auto& layers = level["layerInstances"];

    for (auto& layer : layers) {
        // レイヤー名は自分の環境に合わせて変更してください（例: "IntGrid", "Block", "Tiles"など）
        if (layer["__identifier"] == "IntGrid") {
            auto& csvData = layer["intGridCsv"];
            for (int i = 0; i < csvData.size(); i++) {
                int x = i % kMapWidth_;
                int y = i / kMapWidth_;
                if (y < kMapHeight_ && x < kMapWidth_) {
                    mapData[y][x] = csvData[i];
                }
            }
            break;
        }
    }
}
