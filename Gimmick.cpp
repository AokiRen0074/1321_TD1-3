#include "Gimmick.h"
#include <Novice.h>
#include "Collision.h"
#include "Player.h"

LiftGimmickBlock::LiftGimmickBlock() {
}

LiftGimmickBlock::~LiftGimmickBlock() {
}

void LiftGimmickBlock::Initialize(Vector2 pos, int linkId, Vector2 moveLimit, float speed) {
    pos_ = pos; // 引数で受け取った座標をセット
    linkId_ = linkId; // 引数で受け取ったIDをセット
    moveLimit_ = moveLimit; // LDtkのMoveX,MoveYをセット
    speed_ = speed; // LDtkのSpeedをセット
    isActive_ = false; // 最初は止まっている
    isRunning_ = false;
    isReturning_ = false;
	startPos_ = pos; // 元の位置を覚えておく
}

void LiftGimmickBlock::Update() {
    if (isActive_) {
        isRunning_ = true;
    }

    if (isRunning_) {
        // 目標地点の計算 (開始地点 + 移動量)
        Vector2 targetPos = { startPos_.x + moveLimit_.x, startPos_.y + moveLimit_.y };

        if (!isReturning_) {
            // --- 【往路】 ---
            // X移動
            if (moveLimit_.x > 0) pos_.x += speed_;
            else if (moveLimit_.x < 0) pos_.x -= speed_;

            // Y移動 (LDtkは下方向がプラスなので、マイナス移動なら上昇)
            if (moveLimit_.y > 0) pos_.y += speed_;
            else if (moveLimit_.y < 0) pos_.y -= speed_;

            // 目標地点に達したか判定 (誤差考慮のため abs で判定)
            if (std::abs(pos_.x - targetPos.x) < speed_ && std::abs(pos_.y - targetPos.y) < speed_) {
                pos_ = targetPos; // 座標をピッタリ合わせる
                isReturning_ = true;
            }
        } else {
            // --- 【復路】 ---
            if (moveLimit_.x > 0) pos_.x -= speed_;
            else if (moveLimit_.x < 0) pos_.x += speed_;

            if (moveLimit_.y > 0) pos_.y -= speed_;
            else if (moveLimit_.y < 0) pos_.y += speed_;

            // 元の位置に戻ったか判定
            if (std::abs(pos_.x - startPos_.x) < speed_ && std::abs(pos_.y - startPos_.y) < speed_) {
                pos_ = startPos_;
                isRunning_ = false;
                isActive_ = false;
                isReturning_ = false;
            }
        }
    }

    // デバッグ用
	Novice::ScreenPrintf(0, 800, "lift isActive_: %d", isActive_);
    Novice::ScreenPrintf(0, 820, "lift isRunning_: %d", isRunning_);
	Novice::ScreenPrintf(0, 840, "lift isReturning_: %d", isReturning_);
}

void LiftGimmickBlock::Draw(Vector2 offset) {
    int drawX = (int)(pos_.x - offset.x);
    int drawY = (int)(pos_.y - offset.y);
    int w = (int)size_.x;
    int h = (int)size_.y;


    unsigned int cFrame = 0x444455FF; // 外枠・鉄骨
    unsigned int cDarkBg = 0x111115FF; // 鉄骨の隙間（奥の暗がり）
    unsigned int cStripe = 0xDDCC00FF; // 警告色
    unsigned int cShadow = 0x222233FF; // 影・ディテール
    unsigned int cHighlight = 0x777788FF; // 上面の光沢

    // =================================================
    // ベース
    // =================================================
    // 中が詰まった箱ではなく「枠組み」に見せるため、ベースを暗くする
    Novice::DrawBox(drawX, drawY, w, h, 0.0f, cDarkBg, kFillModeSolid);

    // =================================================
    // 内部のトラス構造
    // =================================================



    int beamThick = 4; // 梁の太さ

    // 左上から右下への斜め材
    Novice::DrawLine(drawX, drawY, drawX + w, drawY + h, cFrame);
    Novice::DrawLine(drawX + beamThick, drawY, drawX + w, drawY + h - beamThick, cFrame); // 太らせる
    Novice::DrawLine(drawX, drawY + beamThick, drawX + w - beamThick, drawY + h, cFrame); // 太らせる

    // 右上から左下への斜め材
    Novice::DrawLine(drawX + w, drawY, drawX, drawY + h, cFrame);
    Novice::DrawLine(drawX + w - beamThick, drawY, drawX, drawY + h - beamThick, cFrame);
    Novice::DrawLine(drawX + w, drawY + beamThick, drawX + beamThick, drawY + h, cFrame);

    // =================================================
    // 外枠
    // =================================================
    int frameW = 4; // 枠の太さ

    // 上下の枠
    Novice::DrawBox(drawX, drawY, w, frameW, 0.0f, cFrame, kFillModeSolid); // 上
    Novice::DrawBox(drawX, drawY + h - frameW, w, frameW, 0.0f, cFrame, kFillModeSolid); // 下

    // 左右の枠（支柱）
    Novice::DrawBox(drawX, drawY, frameW, h, 0.0f, cFrame, kFillModeSolid); // 左
    Novice::DrawBox(drawX + w - frameW, drawY, frameW, h, 0.0f, cFrame, kFillModeSolid); // 右

    // 枠の内側に影を入れて立体感を出す
    Novice::DrawBox(drawX + frameW, drawY + frameW, w - frameW * 2, h - frameW * 2, 0.0f, cShadow, kFillModeWireFrame);


    int stripeY = drawY + h - frameW;

    Novice::DrawBox(drawX, stripeY, w, frameW, 0.0f, cStripe, kFillModeSolid);
    for (int i = 0; i < w; i += 8) {
        Novice::DrawLine(drawX + i, stripeY, drawX + i, stripeY + frameW, cShadow);
    }
    // ストライプの枠
    Novice::DrawBox(drawX, stripeY, w, frameW, 0.0f, cShadow, kFillModeWireFrame);

    // =================================================
    // 上面の踏み板
    // =================================================
    // 上のフレーム部分に滑り止めのディテールを入れる
    int topY = drawY;
    for (int i = 4; i < w; i += 12) {
        Novice::DrawBox(drawX + i, topY + 1, 2, 2, 0.0f, cHighlight, kFillModeSolid);
    }

    // 一番上のハイライト線（エッジ）
    Novice::DrawLine(drawX, drawY, drawX + w, drawY, 0xFFFFFFFF);


    // 角を太くして頑丈に見せる
    int cornerSize = 8;
    Novice::DrawBox(drawX, drawY, cornerSize, cornerSize, 0.0f, cFrame, kFillModeSolid);
    Novice::DrawBox(drawX + w - cornerSize, drawY, cornerSize, cornerSize, 0.0f, cFrame, kFillModeSolid);
    Novice::DrawBox(drawX, drawY + h - cornerSize, cornerSize, cornerSize, 0.0f, cFrame, kFillModeSolid);
    Novice::DrawBox(drawX + w - cornerSize, drawY + h - cornerSize, cornerSize, cornerSize, 0.0f, cFrame, kFillModeSolid);

    // 角のリベット
    Novice::DrawBox(drawX + 2, drawY + 2, 2, 2, 0.0f, cShadow, kFillModeSolid);
    Novice::DrawBox(drawX + w - 4, drawY + 2, 2, 2, 0.0f, cShadow, kFillModeSolid);
    Novice::DrawBox(drawX + 2, drawY + h - 4, 2, 2, 0.0f, cShadow, kFillModeSolid);
    Novice::DrawBox(drawX + w - 4, drawY + h - 4, 2, 2, 0.0f, cShadow, kFillModeSolid);
}

void LiftGimmickBlock::CheckCollision(Player& player) {
    // プレイヤーの4辺
    float ax1 = player.status_.pos.x;
    float ay1 = player.status_.pos.y;
    float ax2 = player.status_.pos.x + player.status_.width;
    float ay2 = player.status_.pos.y + player.status_.height;

    // リフトの4辺
    float bx1 = pos_.x;
    float by1 = pos_.y;
    float bx2 = pos_.x + size_.x;
    float by2 = pos_.y + size_.y;

    // 1. まずは RectRect で当たっているか確認
    if (Collision::RectRect(ax1, ay2, ax2, ay1, bx1, by2, bx2, by1)) {

        // 2. 四方のめり込み量を計算
        float pushRight = bx2 - ax1; // リフトの右側でプレイヤーを右に押す量
        float pushLeft  = ax2 - bx1; // リフトの左側でプレイヤーを左に押す量
        float pushBottom = by2 - ay1; // リフトの下側でプレイヤーを下に押す量
        float pushTop    = ay2 - by1; // リフトの上側でプレイヤーを上に押す量

        // 3. 一番めり込みが小さい方向（最短距離）を探して押し戻す
        float minOverlap = pushRight;
        int direction = 0; // 0:右, 1:左, 2:下, 3:上

        if (pushLeft < minOverlap)   { minOverlap = pushLeft;   direction = 1; }
        if (pushBottom < minOverlap) { minOverlap = pushBottom; direction = 2; }
        if (pushTop < minOverlap)    { minOverlap = pushTop;    direction = 3; }

        switch (direction) {
            case 0: // 右へ押し戻す
                player.status_.pos.x += pushRight;
                player.status_.Velocity.x = 0;
                break;
            case 1: // 左へ押し戻す
                player.status_.pos.x -= pushLeft;
                player.status_.Velocity.x = 0;
                break;
            case 2: // 下へ押し戻す（頭ぶつけ）
                player.status_.pos.y += pushBottom;
                player.status_.Velocity.y = 0;
                break;
            case 3: // 上へ押し戻す（着地）
                player.status_.pos.y -= pushTop;
                player.status_.Velocity.y = 0;
                player.status_.isJumop = false;

                if (isRunning_) {
                    float moveX = 0, moveY = 0;

                    // 現在の移動方向を割り出す
                    if (!isReturning_) {
                        if (moveLimit_.x != 0) moveX = (moveLimit_.x > 0) ? speed_ : -speed_;
                        if (moveLimit_.y != 0) moveY = (moveLimit_.y > 0) ? speed_ : -speed_;
                    } else {
                        if (moveLimit_.x != 0) moveX = (moveLimit_.x > 0) ? -speed_ : speed_;
                        if (moveLimit_.y != 0) moveY = (moveLimit_.y > 0) ? -speed_ : speed_;
                    }

                    // プレイヤーをリフトの移動分だけ動かす
                    player.status_.pos.x += moveX;
                    player.status_.pos.y += moveY;
                }

                break;
        }
    }
}

LiftGimmickButton::LiftGimmickButton() {
}

LiftGimmickButton::~LiftGimmickButton() {
}

void LiftGimmickButton::Initialize(Vector2 pos, Vector2 size, int linkId) {
    pos_ = pos;
    size_ = size;
    linkId_ = linkId;
    isPressed_ = false;
}

void LiftGimmickButton::Update() {
    // 特に更新処理はない
}

void LiftGimmickButton::Draw(Vector2 offset) {
    // 押されているときは緑、そうでないときは赤
    unsigned int color = isPressed_ ? 0xFF00FFFF : 0xFFFF00FF;

    Novice::DrawBox(
        (int)(pos_.x - offset.x),
        (int)(pos_.y - offset.y),
        (int)size_.x,
        (int)size_.y,
        0.0f,
        color,
        kFillModeSolid
    );
}

void LiftGimmickButton::CheckCollision(Player& player) {
    // CollisionクラスのCheckRectを使用して判定
    if (Collision::CheckRect(
        pos_, size_.x, size_.y, 
        player.status_.pos, player.status_.width, player.status_.height)) 
    {
        isPressed_ = true;
    } else {
        isPressed_ = false;
    }
}