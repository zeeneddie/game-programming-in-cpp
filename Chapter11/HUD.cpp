// ----------------------------------------------------------------
// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
// 
// Released under the BSD License
// See LICENSE in root directory for full details.
// ----------------------------------------------------------------

#include "HUD.h"
#include "Texture.h"
#include "Shader.h"
#include "Game.h"
#include "Renderer.h"
#include "PhysWorld.h"
#include "FPSActor.h"
#include <algorithm>
#include "TargetComponent.h"
#include "ArrowTarget.h"

HUD::HUD(Game* game)
	:UIScreen(game)
	,mRadarRange(2000.0f)
	,mRadarRadius(92.0f)
	,mTargetEnemy(false)
{
	Renderer* r = mGame->GetRenderer();
	mHealthBar = r->GetTexture("Assets/HealthBar.png");
	mRadar = r->GetTexture("Assets/Radar.png");
	mCrosshair = r->GetTexture("Assets/Crosshair.png");
	mCrosshairEnemy = r->GetTexture("Assets/CrosshairRed.png");
	mBlipTex = r->GetTexture("Assets/Blip.png");
	mBlipDownTex = r->GetTexture("Assets/BlipDown.png");
	mBlipUpTex = r->GetTexture("Assets/BlipUp.png");
	mRadarArrow = r->GetTexture("Assets/RadarArrow.png");
	mTargetArrow = r->GetTexture("Assets/Arrow.png");
}

HUD::~HUD()
{
}

void HUD::Update(float deltaTime)
{
	UIScreen::Update(deltaTime);
	
	UpdateCrosshair(deltaTime);
	UpdateRadar(deltaTime);
}

void HUD::Draw(Shader* shader)
{
	// Crosshair
	Texture* cross = mTargetEnemy ? mCrosshairEnemy : mCrosshair;
	DrawTexture(shader, cross, Vector2::Zero, 2.0f);
	
	// Radar
	const Vector2 cRadarPos(-390.0f, 275.0f);
	DrawTexture(shader, mRadar, cRadarPos, 1.0f);
	// Blips
	for (Vector2& blip : mBlips)
	{
		DrawTexture(shader, mBlipTex, cRadarPos + blip, 1.0f);
	}
	for (Vector2& blip : mBlipDowns)
	{
		DrawTexture(shader, mBlipDownTex, cRadarPos + blip, 1.0f);
	}
	for (Vector2& blip : mBlipUps)
	{
		DrawTexture(shader, mBlipUpTex, cRadarPos + blip, 1.0f);
	}
	// Radar arrow
	DrawTexture(shader, mRadarArrow, cRadarPos);
	
	//// Health bar
	//DrawTexture(shader, mHealthBar, Vector2(-350.0f, -350.0f));

	// Target arrow
	const Vector2 cArrowPos(0.0f, 275.0f);
	DrawTexture(shader, mTargetArrow, cArrowPos, 1.0f, mAngle);
}

void HUD::AddTargetComponent(TargetComponent* tc)
{
	mTargetComps.emplace_back(tc);
}

void HUD::RemoveTargetComponent(TargetComponent* tc)
{
	auto iter = std::find(mTargetComps.begin(), mTargetComps.end(),
		tc);
	mTargetComps.erase(iter);
}

void HUD::UpdateCrosshair(float deltaTime)
{
	// Reset to regular cursor
	mTargetEnemy = false;
	// Make a line segment
	const float cAimDist = 5000.0f;
	Vector3 start, dir;
	mGame->GetRenderer()->GetScreenDirection(start, dir);
	LineSegment l(start, start + dir * cAimDist);
	// Segment cast
	PhysWorld::CollisionInfo info;
	if (mGame->GetPhysWorld()->SegmentCast(l, info))
	{
		// Is this a target?
		for (auto tc : mTargetComps)
		{
			if (tc->GetOwner() == info.mActor)
			{
				mTargetEnemy = true;
				break;
			}
		}
	}
}

void HUD::UpdateRadar(float deltaTime)
{
	// Clear blip positions from last frame
	mBlips.clear();
	mBlipDowns.clear();
	mBlipUps.clear();
	
	// Convert player position to radar coordinates (x forward, z up)
	Vector3 playerPos = mGame->GetPlayer()->GetPosition();
	Vector2 playerPos2D(playerPos.y, playerPos.x);
	float playerHeight = playerPos.z;
	// Ditto for player forward
	Vector3 playerForward = mGame->GetPlayer()->GetForward();
	Vector2 playerForward2D(playerForward.x, playerForward.y);
	
	// Use atan2 to get rotation of radar
	float angle = Math::Atan2(playerForward2D.y, playerForward2D.x);
	// Make a 2D rotation matrix
	Matrix3 rotMat = Matrix3::CreateRotation(angle);
	
	// Get positions of blips
	for (auto tc : mTargetComps)
	{
		Vector3 targetPos = tc->GetOwner()->GetPosition();
		Vector2 actorPos2D(targetPos.y, targetPos.x);
		float targetHeight = targetPos.z;
		
		// Calculate vector between player and target
		Vector2 playerToTarget = actorPos2D - playerPos2D;
		
		// See if within range
		if (playerToTarget.LengthSq() <= (mRadarRange * mRadarRange))
		{
			// Convert playerToTarget into an offset from
			// the center of the on-screen radar
			Vector2 blipPos = playerToTarget;
			blipPos *= mRadarRadius/mRadarRange;
			
			// Rotate blipPos
			blipPos = Vector2::Transform(blipPos, rotMat);
			if (targetHeight > playerHeight)
			{
				mBlipUps.emplace_back(blipPos);
			}
			else if(targetHeight < playerHeight)
			{
				mBlipDowns.emplace_back(blipPos);
			}
			else
			{
				mBlips.emplace_back(blipPos);
			}
		}
	}

	// Get position of arrow target
	auto at = mGame->GetArrowTarget();
	{
		Vector3 targetPos = at->GetPosition();
		Vector2 targetPos2D(targetPos.x, targetPos.y);

		playerPos2D.Set(playerPos.x, playerPos.y);
		Vector2 playerToTarget = targetPos2D - playerPos2D;
		playerToTarget.Normalize();

		mAngle = Math::Atan2(playerToTarget.x, playerToTarget.y)
			- Math::Atan2(playerForward2D.x, playerForward2D.y);
	}
}
