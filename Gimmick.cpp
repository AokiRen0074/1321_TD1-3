#include "Gimmick.h"
#include <Novice.h>
#include "Collision.h"
#include "Player.h"
#include <cmath> // std::abs のために必要

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
		Vector2 targetPos = {startPos_.x + moveLimit_.x, startPos_.y + moveLimit_.y};

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
	//Novice::ScreenPrintf(0, 800, "lift isActive_: %d", isActive_);
	//Novice::ScreenPrintf(0, 820, "lift isRunning_: %d", isRunning_);
	//Novice::ScreenPrintf(0, 840, "lift isReturning_: %d", isReturning_);
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
	// PlayerのAABB
	float ax1 = player.status_.pos.x;
	float ay1 = player.status_.pos.y;
	float ax2 = player.status_.pos.x + player.status_.width;
	float ay2 = player.status_.pos.y + player.status_.height;

	// リフトのAABB
	float bx1 = pos_.x;
	float by1 = pos_.y;
	float bx2 = pos_.x + size_.x;
	float by2 = pos_.y + size_.y;

	// 矩形判定
	if (Collision::RectRect(ax1, ay2, ax2, ay1, bx1, by2, bx2, by1)) {

		// めり込み量を計算
		float overlapTop    = ay2 - by1;
		float overlapBottom = by2 - ay1;
		float overlapLeft   = ax2 - bx1;
		float overlapRight  = bx2 - ax1;

		float minOverlap = overlapTop;
		if (overlapBottom < minOverlap) minOverlap = overlapBottom;
		if (overlapLeft < minOverlap) minOverlap = overlapLeft;
		if (overlapRight < minOverlap) minOverlap = overlapRight;

		// 1. 上から乗っている判定（落下中または接地中のみ有効）
		if (minOverlap == overlapTop && player.status_.Velocity.y >= 0) {
			player.status_.pos.y = pos_.y - player.status_.height;
			player.status_.isLift = true;
			player.status_.Velocity.y = 0.0f;
			player.status_.isJumop = false;
			player.isOnGround = true;

			if (isRunning_) {
				float moveX = 0, moveY = 0;
				if (!isReturning_) {
					if (moveLimit_.x != 0) moveX = (moveLimit_.x > 0) ? speed_ : -speed_;
					if (moveLimit_.y != 0) moveY = (moveLimit_.y > 0) ? speed_ : -speed_;
				} else {
					if (moveLimit_.x != 0) moveX = (moveLimit_.x > 0) ? -speed_ : speed_;
					if (moveLimit_.y != 0) moveY = (moveLimit_.y > 0) ? -speed_ : speed_;
				}
				player.status_.pos.x += moveX;
				player.status_.pos.y += moveY;
			}
		}
		// 2. 下から頭をぶつけた判定（ここが吸い付き防止の鍵）
		else if (minOverlap == overlapBottom) {
			player.status_.pos.y = pos_.y + size_.y;
			if (player.status_.Velocity.y < 0) {
				player.status_.Velocity.y = 0; // 上昇を止める
			}
		}
		// 3. 横からの押し戻し
		else if (minOverlap == overlapLeft) {
			player.status_.pos.x = pos_.x - player.status_.width;
		}
		else if (minOverlap == overlapRight) {
			player.status_.pos.x = pos_.x + size_.x;
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

// =========================================================
// リフト用ボタンの描画
// =========================================================
void LiftGimmickButton::Draw(Vector2 offset) {
	int drawX = (int)(pos_.x - offset.x);
	int drawY = (int)(pos_.y - offset.y);
	int w = (int)size_.x; // 通常32
	int h = (int)size_.y; // 通常32

	// 中心座標と底辺座標を基準に描画する
	int cx = drawX + w / 2;
	int bottomY = drawY + h;

	// ★サイズ設定（判定より大きくする！）
	int visualW = w + 20; // 横幅を +20px 大きく (52px)
	int baseH = 10;       // 台座を高く (6 -> 10)

	// --- カラーパレット ---
	unsigned int cBase = 0x444455FF; // 台座
	unsigned int cOff = 0xFF2222FF; // OFF（赤）
	unsigned int cOn = 0x00FF00FF; // ON（緑）
	unsigned int cShadow = 0x222233FF; // 影
	unsigned int cLight = 0xFFFFFFFF; // ハイライト

	unsigned int btnColor = isPressed_ ? cOn : cOff;

	// 1. 台座（地面に固定されている部分）
	// 中心(cx)から左右に広げる
	int baseX = cx - visualW / 2;
	int baseY = bottomY - baseH;

	// 台座本体
	Novice::DrawBox(baseX, baseY, visualW, baseH, 0.0f, cBase, kFillModeSolid);
	// 台座の装飾（斜めストライプ）
	Novice::DrawLine(baseX, baseY, baseX + visualW, baseY, 0x888899FF); // 上線
	Novice::DrawBox(baseX, baseY, visualW, baseH, 0.0f, cShadow, kFillModeWireFrame); // 枠

	// 2. ボタン本体（可動部）
	int margin = 6; // 台座より少し内側
	int btnW = visualW - margin * 2;

	// 押されている時は低くする
	int btnHeight = isPressed_ ? 6 : 16; // 高さアップ (12 -> 16)
	int btnY = baseY - btnHeight;
	int btnX = cx - btnW / 2;

	// 本体描画
	Novice::DrawBox(btnX, btnY, btnW, btnHeight, 0.0f, btnColor, kFillModeSolid);

	// 本体の枠と立体感
	Novice::DrawBox(btnX, btnY, btnW, btnHeight, 0.0f, cShadow, kFillModeWireFrame);
	// 上面のハイライト
	if (!isPressed_) {
		Novice::DrawLine(btnX, btnY, btnX + btnW, btnY, cLight);
	}

	// 3. 発光エフェクト（ONのとき）
	if (isPressed_) {
		Novice::SetBlendMode(kBlendModeAdd);
		// 本体が強く光る
		Novice::DrawBox(btnX, btnY, btnW, btnHeight, 0.0f, 0x00FF00AA, kFillModeSolid);
		Novice::SetBlendMode(kBlendModeNormal);
	} else {
		// OFFのときは赤いランプが点滅（サイズアップ）
		static int blink = 0;
		blink++;
		if ((blink / 30) % 2 == 0) {
			Novice::DrawBox(cx - 6, btnY + 4, 12, 4, 0.0f, 0xFF8888FF, kFillModeSolid);
		}
	}
}
void LiftGimmickButton::CheckCollision(Player& player) {
	// CollisionクラスのCheckRectを使用して判定
	if (Collision::CheckRect(
		pos_, size_.x, size_.y,
		player.status_.pos, player.status_.width, player.status_.height)) {
		isPressed_ = true;
	} else {
		isPressed_ = false;
	}
}