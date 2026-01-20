#include "Gimmick.h"
#include <Novice.h>
#include "Collision.h"
#include "Player.h"

// うわ

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
	Novice::DrawBox(
		(int)(pos_.x - offset.x),
		(int)(pos_.y - offset.y),
		(int)size_.x,
		(int)size_.y,
		0.0f,
		0xFFFF00FF,
		kFillModeSolid
	);
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