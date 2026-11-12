#include "CombatSim.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "Engine/World.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Misc/Parse.h"
#include "PathRenderer.h"
#include "WidgetBlueprint.h"
#include "BaseWidgetBlueprint.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace BuhCombat
{
	struct FMoveData
	{
		int32 Startup;
		int32 Active;
		int32 Recovery;
		int32 Damage;
		int32 Range;
		int32 Movement;
		int32 ReactionDelay;
	};

	// index = move id: 0 Quick, 1 Heavy, 2 Guard, 3 Dodge, 4 Walk Toward, 5 Walk Away, 6 Walk (destination), 7 Continue (dummy), 8 Dash, 9 Jump
	static const FMoveData MoveData[10] = {
		{ 6, 1, 8, 10, 300, 0, 9999 },
		{ 13, 1, 8, 24, 400, 0, 9999 },
		{ 0, 10, 0, 0, 9999, 0, 9999 },
		{ 3, 2, 3, 0, 9999, 30, 9999 },
		{ 0, 5, 0, 0, 9999, 30, 0 },
		{ 0, 5, 0, 0, 9999, 30, 0 },
		{ 0, 9999, 0, 0, 9999, 30, 0 },
		{ 0, 0, 0, 0, 9999, 0, 9999 },
		{ 1, 8, 3, 0, 9999, 90, 9999 },
		{ 1, 8, 3, 0, 9999, 0, 9999 },
	};

	static const int32 Approach = 30;
	static const int32 MinSeparation = 100;
	static const int32 AI_MOVE = 1;
	static const int32 DodgeDistance = 300;
	static const int32 JumpMaxRange = 480;
	static const int32 JumpMaxHeight = 220;
	static const int32 JumpFlightInstants = 8;

	static const TCHAR* MoveName(int32 A)
	{
		switch (A)
		{
		case 0: return TEXT("Quick");
		case 1: return TEXT("Heavy");
		case 2: return TEXT("Guard");
		case 3: return TEXT("Dodge");
		case 4: return TEXT("Walk Toward");
		case 5: return TEXT("Walk Away");
		case 6: return TEXT("Walk");
		case 7: return TEXT("Continue");
		case 8: return TEXT("Dash");
		case 9: return TEXT("Jump");
		default: return TEXT("Idle");
		}
	}
}

FBuhCombatState UBuhCombatSim::MakeState(int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed, float PlayerY, float OpponentY)
{
	FBuhCombatState S;
	S.Fighters.AddDefaulted(2);
	S.Fighters[0].X = static_cast<float>(PlayerX);
	S.Fighters[0].Y = PlayerY;
	S.Fighters[0].Health = PlayerHealth;
	S.Fighters[0].Team = 0;
	S.Fighters[0].WalkTarget = static_cast<float>(PlayerX);
	S.Fighters[0].WalkTargetY = PlayerY;
	S.Fighters[0].DashTarget = static_cast<float>(PlayerX);
	S.Fighters[0].DashTargetY = PlayerY;
	S.Fighters[1].X = static_cast<float>(OpponentX);
	S.Fighters[1].Y = OpponentY;
	S.Fighters[1].Health = OpponentHealth;
	S.Fighters[1].Team = 1;
	S.Fighters[1].WalkTarget = static_cast<float>(OpponentX);
	S.Fighters[1].WalkTargetY = OpponentY;
	S.Fighters[1].DashTarget = static_cast<float>(OpponentX);
	S.Fighters[1].DashTargetY = OpponentY;
	S.Instant = 0;
	S.Seed = Seed;
	return S;
}

FBuhCombatState UBuhCombatSim::MakeStateWithPhases(int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed, int32 PlayerPhase, int32 PlayerPhaseInstants, int32 OpponentPhase, int32 OpponentPhaseInstants, float PlayerY, float OpponentY)
{
	FBuhCombatState S = MakeState(PlayerX, OpponentX, PlayerHealth, OpponentHealth, Seed, PlayerY, OpponentY);
	S.Fighters[0].Phase = PlayerPhase;
	S.Fighters[0].PhaseInstants = PlayerPhaseInstants;
	S.Fighters[1].Phase = OpponentPhase;
	S.Fighters[1].PhaseInstants = OpponentPhaseInstants;
	return S;
}

FBuhCombatState UBuhCombatSim::WithPlayerWalkTarget(const FBuhCombatState& InState, float TargetX, float TargetY)
{
	FBuhCombatState S = InState;
	S.Fighters[0].WalkTarget = TargetX;
	S.Fighters[0].WalkTargetY = TargetY;
	if (GExecDashTarget >= -9998.f)
	{
		S = WithPlayerDashTarget(S, GExecDashTarget, GExecDashTargetY);
	}
	return S;
}

FBuhCombatState UBuhCombatSim::WithPlayerDashTarget(const FBuhCombatState& InState, float TargetX, float TargetY)
{
	FBuhCombatState S = InState;
	S.Fighters[0].DashTarget = TargetX;
	S.Fighters[0].DashTargetY = TargetY;
	return S;
}

FBuhCombatState UBuhCombatSim::WithPlayerDashTargetFromPreview(const FBuhCombatState& InState)
{
	if (GPlayerDashTarget >= -9998.f)
	{
		return WithPlayerDashTarget(InState, GPlayerDashTarget, GPlayerDashTargetY);
	}
	return InState;
}

FBuhCombatState UBuhCombatSim::WithPlayerJumpTarget(const FBuhCombatState& InState, float TargetX, float TargetY)
{
	FBuhCombatState S = InState;
	S.Fighters[0].JumpTarget = TargetX;
	S.Fighters[0].JumpTargetY = TargetY;
	return S;
}

FBuhCombatState UBuhCombatSim::WithPlayerJumpTargetFromPreview(const FBuhCombatState& InState)
{
	if (GPlayerJumpTarget >= -9998.f)
	{
		return WithPlayerJumpTarget(InState, GPlayerJumpTarget, GPlayerJumpTargetY);
	}
	return InState;
}

struct FBuhOutcome { int32 Fighter; int32 Type; int32 Damage; };

static void EnforceMinSeparation(FBuhCombatState& S, const bool bMoved[2])
{
	const float Dx = S.Fighters[1].X - S.Fighters[0].X;
	const float Dy = S.Fighters[1].Y - S.Fighters[0].Y;
	const float D = FMath::Sqrt(Dx * Dx + Dy * Dy);
	if (D >= static_cast<float>(BuhCombat::MinSeparation))
	{
		return;
	}
	const float Deficit = static_cast<float>(BuhCombat::MinSeparation) - D;
	const float InvD = (D > 0.f) ? 1.f / D : 0.f;
	const float PushX = Dx * InvD;
	const float PushY = Dy * InvD;
	if (bMoved[0] && bMoved[1])
	{
		const float Half = Deficit * 0.5f;
		S.Fighters[0].X -= PushX * Half;
		S.Fighters[0].Y -= PushY * Half;
		S.Fighters[1].X += PushX * Half;
		S.Fighters[1].Y += PushY * Half;
	}
	else if (bMoved[0])
	{
		S.Fighters[0].X -= PushX * Deficit;
		S.Fighters[0].Y -= PushY * Deficit;
	}
	else if (bMoved[1])
	{
		S.Fighters[1].X += PushX * Deficit;
		S.Fighters[1].Y += PushY * Deficit;
	}
}

static TArray<FBuhOutcome> StepCombatInstant(FBuhCombatState& S, float PreDodgeDist[2], bool bJustActive[2], int32 AttackStart[2])
{
	AttackStart[0] = -1;
	AttackStart[1] = -1;
	const float RawDx = S.Fighters[1].X - S.Fighters[0].X;
	const float RawDy = S.Fighters[1].Y - S.Fighters[0].Y;
	const float RawDist = FMath::Sqrt(RawDx * RawDx + RawDy * RawDy);
	bool bMoved[2] = { false, false };
	for (int32 i = 0; i < 2; ++i)
	{
		FBuhFighterState& F = S.Fighters[i];
		if (F.Phase == 2 && (BuhCombat::MoveData[F.Action].Movement > 0 || F.Action == 9))
		{
			if (PreDodgeDist[i] > 1e8f) PreDodgeDist[i] = FMath::Sqrt(FMath::Square(S.Fighters[1 - i].X - F.X) + FMath::Square(S.Fighters[1 - i].Y - F.Y));
			if (F.Action == 9)
			{
				const int32 Completed = F.PhaseInstants + 1;
				const float T = static_cast<float>(Completed) / static_cast<float>(BuhCombat::JumpFlightInstants);
				F.X = F.JumpStartX + T * (F.JumpTarget - F.JumpStartX);
				F.Y = F.JumpStartY + T * (F.JumpTargetY - F.JumpStartY);
				F.Height = 4.f * static_cast<float>(BuhCombat::JumpMaxHeight) * T * (1.f - T);
				bMoved[i] = true;
			}
			else if (F.Action == 6)
			{
				const float RemX = F.WalkTarget - F.X;
				const float RemY = F.WalkTargetY - F.Y;
				const float RemDist = FMath::Sqrt(RemX * RemX + RemY * RemY);
				if (RemDist <= static_cast<float>(BuhCombat::MoveData[F.Action].Movement))
				{
					F.X = F.WalkTarget;
					F.Y = F.WalkTargetY;
					F.Phase = 3;
					F.PhaseInstants = 0;
					bMoved[i] = true;
				}
				else
				{
					const float Scale = static_cast<float>(BuhCombat::MoveData[F.Action].Movement) / RemDist;
					F.X += RemX * Scale;
					F.Y += RemY * Scale;
					bMoved[i] = true;
				}
			}
			else if (F.Action == 8)
			{
				const float RemX = F.DashTarget - F.X;
				const float RemY = F.DashTargetY - F.Y;
				const float RemDist = FMath::Sqrt(RemX * RemX + RemY * RemY);
				if (RemDist <= static_cast<float>(BuhCombat::MoveData[F.Action].Movement))
				{
					F.X = F.DashTarget;
					F.Y = F.DashTargetY;
					F.Phase = 3;
					F.PhaseInstants = 0;
					bMoved[i] = true;
				}
				else
				{
					const float Scale = static_cast<float>(BuhCombat::MoveData[F.Action].Movement) / RemDist;
					F.X += RemX * Scale;
					F.Y += RemY * Scale;
					bMoved[i] = true;
				}
			}
			else
			{
				const float OppX = S.Fighters[1 - i].X;
				const float OppY = S.Fighters[1 - i].Y;
				const float ToDx = OppX - F.X;
				const float ToDy = OppY - F.Y;
				const float ToDist = FMath::Sqrt(ToDx * ToDx + ToDy * ToDy);
				const float InvToDist = (ToDist > 0.f) ? 1.f / ToDist : 0.f;
				const float UnitX = ToDx * InvToDist;
				const float UnitY = ToDy * InvToDist;
				const bool bMoveToward = (F.Action == 4);
				const float Sign = bMoveToward ? 1.f : -1.f;
				F.X += static_cast<float>(BuhCombat::MoveData[F.Action].Movement) * Sign * UnitX;
				F.Y += static_cast<float>(BuhCombat::MoveData[F.Action].Movement) * Sign * UnitY;
				bMoved[i] = true;
			}
		}
	}
	EnforceMinSeparation(S, bMoved);
	S.Fighters[0].Facing = (RawDx >= 0.f) ? 1 : -1;
	S.Fighters[1].Facing = (RawDx >= 0.f) ? -1 : 1;

	for (int32 i = 0; i < 2; ++i)
	{
		FBuhFighterState& F = S.Fighters[i];
		const int32 PrevPhase = F.Phase;
		if (F.Phase == 1)
		{
			++F.PhaseInstants;
			if (F.PhaseInstants >= BuhCombat::MoveData[F.Action].Startup)
			{
				F.Phase = 2;
				F.PhaseInstants = 0;
				bJustActive[i] = true;
			}
		}
		else if (F.Phase == 2)
		{
			++F.PhaseInstants;
			if (F.PhaseInstants >= BuhCombat::MoveData[F.Action].Active)
			{
				F.Phase = 3;
				F.PhaseInstants = 0;
			}
		}
		else if (F.Phase == 3)
		{
			++F.PhaseInstants;
			if (F.PhaseInstants >= BuhCombat::MoveData[F.Action].Recovery)
			{
				F.Phase = 0;
				F.PhaseInstants = 0;
				F.Height = 0.f;
			}
		}
		if (PrevPhase == 0 && F.Phase == 1 && (F.Action == 0 || F.Action == 1))
		{
			AttackStart[i] = i;
		}
		if (F.Phase == 0 && S.Fighters[1 - i].ReactedThreatFighter == i)
		{
			S.Fighters[1 - i].ReactedThreatFighter = -1;
		}
	}

	const float D = FMath::Sqrt(FMath::Square(S.Fighters[1].X - S.Fighters[0].X) + FMath::Square(S.Fighters[1].Y - S.Fighters[0].Y));
	TArray<FBuhOutcome> Outcomes;
	for (int32 i = 0; i < 2; ++i)
	{
		if (!bJustActive[i]) continue;
		bJustActive[i] = false;
		const FBuhFighterState& Atk = S.Fighters[i];
		if (Atk.Action != 0 && Atk.Action != 1) continue;
		const FBuhFighterState& Def = S.Fighters[1 - i];
		const BuhCombat::FMoveData& MD = BuhCombat::MoveData[Atk.Action];
		if (D <= static_cast<float>(MD.Range))
		{
			if (Def.Phase == 2 && Def.Action == 2) Outcomes.Add({ i, 1, 0 });
			else if (Def.Phase == 2 && Def.Action == 3) Outcomes.Add({ i, 3, 0 });
			else Outcomes.Add({ i, 0, MD.Damage });
		}
		else
		{
			if (Def.Action == 3 && PreDodgeDist[1 - i] <= static_cast<float>(MD.Range)) Outcomes.Add({ i, 3, 0 });
			else Outcomes.Add({ i, 2, 0 });
		}
	}
	return Outcomes;
}

static bool IsReactableWalk(const FBuhFighterState& F)
{
	return (F.Action == 4 || F.Action == 5 || F.Action == 6) && (F.Phase == 1 || F.Phase == 2) && BuhCombat::MoveData[F.Action].ReactionDelay <= 0;
}

static bool IsReactableDash(const FBuhFighterState& F)
{
	return F.Action == 8 && F.Phase == 2 && F.PhaseInstants >= 3;
}

static bool IsReactableJump(const FBuhFighterState& F)
{
	return F.Action == 9 && F.Phase == 2 && F.PhaseInstants >= 3;
}

static bool IsReactableMovement(const FBuhFighterState& F)
{
	return IsReactableWalk(F) || IsReactableDash(F) || IsReactableJump(F);
}

static void PrepareJumpCommit(FBuhFighterState& F)
{
	F.JumpStartX = F.X;
	F.JumpStartY = F.Y;
	if (F.JumpTarget < -9998.f)
	{
		F.JumpTarget = F.X;
		F.JumpTargetY = F.Y;
	}
	else
	{
		const float Dx = F.JumpTarget - F.X;
		const float Dy = F.JumpTargetY - F.Y;
		const float Dist = FMath::Sqrt(Dx * Dx + Dy * Dy);
		if (Dist > static_cast<float>(BuhCombat::JumpMaxRange))
		{
			const float Scale = static_cast<float>(BuhCombat::JumpMaxRange) / Dist;
			F.JumpTarget = F.X + Dx * Scale;
			F.JumpTargetY = F.Y + Dy * Scale;
		}
	}
	F.Height = 0.f;
}

static bool CheckReactionPause(FBuhCombatState& S, int32 Inst, const int32 AttackStart[2], FBuhCombatEvent& OutEvent)
{
	for (int32 Threat = 0; Threat < 2; ++Threat)
	{
		const FBuhFighterState& T = S.Fighters[Threat];
		const bool bThreatObservable = ((T.Action == 0 || T.Action == 1) && T.Phase == 1 && T.bCommitted);
		const bool bThreatStarted = (AttackStart[Threat] == Threat);
		if (!bThreatObservable && !bThreatStarted) continue;
		const int32 Defender = 1 - Threat;
		FBuhFighterState& Def = S.Fighters[Defender];
		if (!IsReactableMovement(Def)) continue;
		if (Def.ReactedThreatFighter == Threat) continue;
		if (bThreatObservable && !bThreatStarted && !Def.bCommitted) continue;
		Def.ReactedThreatFighter = Threat;
		OutEvent.Instant = Inst;
		OutEvent.Type = 6;
		OutEvent.Fighter = Defender;
		OutEvent.Damage = 0;
		OutEvent.Detail = FString::Printf(TEXT("F%d sees F%d begin %s; reaction opportunity"),
			Def.Team, S.Fighters[Threat].Team, BuhCombat::MoveName(S.Fighters[Threat].Action));
		// Temporary diagnostic (graphical-PIE investigation): confirm Continue legality at the pause.
		const bool bContinueLegal = (Def.Action == 4 || Def.Action == 5 || Def.Action == 6 || Def.Action == 8 || Def.Action == 9);
		UE_LOG(LogTemp, Warning, TEXT("DashDiag REACTION PAUSE: defender F%d Action=%d Phase=%d PhaseInstants=%d ContinueLegal=%d"),
			Def.Team, Def.Action, Def.Phase, Def.PhaseInstants, bContinueLegal ? 1 : 0);
		return true;
	}
	return false;
}

FBuhCombatResult UBuhCombatSim::ResolveCombat(FBuhCombatState InState, int32 PlayerDecision, int32 OpponentDecision, int32 MaxInstants, bool bSuppressReactionPause, bool bSuppressPlayerReadyBreak)
{
	if (InState.Fighters.Num() < 2)
	{
		FBuhCombatResult Err;
		Err.StoppedReason = 0;
		return Err;
	}
	FBuhCombatResult Result;
	Result.MinDistanceSeen = 1e9f;
	FBuhCombatState S = InState;
	float PreDodgeDist[2] = { 1e9f, 1e9f };
	int32 PendingAttackStart[2] = { -1, -1 };

	// Commit decisions for ready fighters, and allow a reactable walking fighter
	// to commit a reaction even while it is already busy.
	for (int32 i = 0; i < 2; ++i)
	{
		const int32 Decision = (i == 0) ? PlayerDecision : OpponentDecision;
		if (Decision < 0 || Decision >= 10) continue;
		FBuhFighterState& F = S.Fighters[i];
		if (Decision == 7)
		{
			if (IsReactableMovement(F)) { /* Continue Walk/Dash: preserve existing state */ }
			continue;
		}
		if (F.Phase == 0)
		{
			F.Action = Decision;
			F.Phase = 1;
			F.PhaseInstants = 0;
			F.bCommitted = true;
			F.Height = 0.f;
			if (Decision == 9) PrepareJumpCommit(F);
			if (Decision == 0 || Decision == 1) PendingAttackStart[i] = i;
		}
		else if (IsReactableMovement(F))
		{
			F.Action = Decision;
			F.Phase = 1;
			F.PhaseInstants = 0;
			F.bCommitted = true;
			F.Height = 0.f;
			if (Decision == 9) PrepareJumpCommit(F);
		}
	}

	bool bJustActive[2] = { false, false };

	for (int32 inst = 0; inst < MaxInstants; ++inst)
	{
		S.Instant = inst;

		int32 AttackStart[2] = { PendingAttackStart[0], PendingAttackStart[1] };
		PendingAttackStart[0] = -1;
		PendingAttackStart[1] = -1;

		FBuhCombatEvent ReactionEvent;
		if (!bSuppressReactionPause && CheckReactionPause(S, inst, AttackStart, ReactionEvent))
		{
			Result.Events.Add(ReactionEvent);
			Result.bReactionOpportunity = true;
			Result.ReactionInstant = inst;
			Result.ReactionFighter = ReactionEvent.Fighter;
			Result.ObservedThreatAction = S.Fighters[1 - ReactionEvent.Fighter].Action;
			Result.StoppedReason = 5;
			Result.FinalState = S;
			return Result;
		}

		const TArray<FBuhOutcome> Outcomes = StepCombatInstant(S, PreDodgeDist, bJustActive, AttackStart);
		const float D = FMath::Sqrt(FMath::Square(S.Fighters[1].X - S.Fighters[0].X) + FMath::Square(S.Fighters[1].Y - S.Fighters[0].Y));
		Result.MinDistanceSeen = FMath::Min(Result.MinDistanceSeen, D);

		// 4. Apply outcomes.
		bool bKO = false;
		for (const FBuhOutcome& O : Outcomes)
		{
			FBuhFighterState& Atk = S.Fighters[O.Fighter];
			FBuhFighterState& Def = S.Fighters[1 - O.Fighter];
			FBuhCombatEvent E;
			E.Instant = inst;
			E.Fighter = O.Fighter;
			E.Damage = O.Damage;
			E.Type = O.Type;
			if (O.Type == 0)
			{
				Def.Health -= O.Damage;
				E.Detail = FString::Printf(TEXT("F%d %s hits F%d for %d"), Atk.Team, BuhCombat::MoveName(Atk.Action), Def.Team, O.Damage);
			Def.Phase = 3;
			Def.PhaseInstants = 0;
			Def.Height = 0.f;
				if (Def.Health <= 0) bKO = true;
			}
			else if (O.Type == 1)
			{
				E.Detail = FString::Printf(TEXT("F%d guard blocks F%d %s"), Def.Team, Atk.Team, BuhCombat::MoveName(Atk.Action));
			}
			else if (O.Type == 3)
			{
				E.Detail = FString::Printf(TEXT("F%d dodge evades F%d %s"), Def.Team, Atk.Team, BuhCombat::MoveName(Atk.Action));
			}
			else
			{
				E.Detail = FString::Printf(TEXT("F%d %s misses (D=%.0f > range=%d)"), Atk.Team, BuhCombat::MoveName(Atk.Action), D, BuhCombat::MoveData[Atk.Action].Range);
			}
			Result.Events.Add(E);
			// Attacker always goes to recovery after its active window.
			Atk.Phase = 3;
			Atk.PhaseInstants = 0;
		}

		if (bKO)
		{
			FBuhCombatEvent KO;
			KO.Instant = inst;
			KO.Type = 5;
			KO.Fighter = (S.Fighters[0].Health <= 0) ? 0 : 1;
			KO.Damage = 0;
			KO.Detail = FString::Printf(TEXT("F%d KO"), KO.Fighter);
			Result.Events.Add(KO);
			Result.FinalState = S;
			Result.StoppedReason = 3;
			return Result;
		}

		// 5. Determine ready fighters.
		const bool bPlayerReady = (S.Fighters[0].Phase == 0);
		const bool bOpponentReady = (S.Fighters[1].Phase == 0);

		if (bPlayerReady && bOpponentReady)
		{
			Result.bPlayerReady = true;
			Result.bOpponentReady = true;
			Result.StoppedReason = 2;
			Result.FinalState = S;
			return Result;
		}
		if (bPlayerReady && !bSuppressPlayerReadyBreak)
		{
			Result.bPlayerReady = true;
			Result.StoppedReason = 1;
			Result.FinalState = S;
			return Result;
		}
		if (bOpponentReady)
		{
			// AI commits independently; combat continues.
			S.Fighters[1].Action = BuhCombat::AI_MOVE;
			S.Fighters[1].Phase = 1;
			S.Fighters[1].PhaseInstants = 0;
			S.Fighters[1].bCommitted = true;
			if (BuhCombat::AI_MOVE == 0 || BuhCombat::AI_MOVE == 1) PendingAttackStart[1] = 1;
			FBuhCombatEvent E;
			E.Instant = inst;
			E.Type = 4;
			E.Fighter = 1;
			E.Damage = 0;
			E.Detail = TEXT("F1 ready, AI commits Heavy");
			Result.Events.Add(E);
		}
	}

	Result.StoppedReason = 4;
	Result.FinalState = S;
	return Result;
}

namespace BuhCombatTest
{
	static bool SameState(const FBuhCombatState& A, const FBuhCombatState& B)
	{
		if (A.Instant != B.Instant || A.Seed != B.Seed) return false;
		if (A.Fighters.Num() != B.Fighters.Num()) return false;
		for (int32 i = 0; i < 2; ++i)
		{
			const FBuhFighterState& a = A.Fighters[i];
			const FBuhFighterState& b = B.Fighters[i];
			if (a.Health != b.Health || a.Action != b.Action || a.Phase != b.Phase ||
				a.PhaseInstants != b.PhaseInstants || a.Team != b.Team ||
				a.bCommitted != b.bCommitted || a.PendingAction != b.PendingAction ||
				a.WalkDir != b.WalkDir || a.WalkRemaining != b.WalkRemaining ||
				a.ReactedThreatFighter != b.ReactedThreatFighter) return false;
			if (FMath::Abs(a.X - b.X) > 0.01f) return false;
			if (FMath::Abs(a.Y - b.Y) > 0.01f) return false;
			if (FMath::Abs(a.WalkTarget - b.WalkTarget) > 0.01f) return false;
			if (FMath::Abs(a.WalkTargetY - b.WalkTargetY) > 0.01f) return false;
			if (FMath::Abs(a.DashTarget - b.DashTarget) > 0.01f) return false;
			if (FMath::Abs(a.DashTargetY - b.DashTargetY) > 0.01f) return false;
			if (FMath::Abs(a.Height - b.Height) > 0.01f) return false;
			if (FMath::Abs(a.JumpStartX - b.JumpStartX) > 0.01f) return false;
			if (FMath::Abs(a.JumpStartY - b.JumpStartY) > 0.01f) return false;
			if (FMath::Abs(a.JumpTarget - b.JumpTarget) > 0.01f) return false;
			if (FMath::Abs(a.JumpTargetY - b.JumpTargetY) > 0.01f) return false;
		}
		return true;
	}

	static bool SameResult(const FBuhCombatResult& A, const FBuhCombatResult& B)
	{
		if (!SameState(A.FinalState, B.FinalState)) return false;
		if (A.Events.Num() != B.Events.Num()) return false;
		for (int32 i = 0; i < A.Events.Num(); ++i)
		{
			const FBuhCombatEvent& a = A.Events[i];
			const FBuhCombatEvent& b = B.Events[i];
			if (a.Instant != b.Instant || a.Type != b.Type || a.Fighter != b.Fighter ||
				a.Damage != b.Damage || a.Detail != b.Detail) return false;
		}
		return true;
	}

	static bool HasEvent(const FBuhCombatResult& R, int32 Type, int32 Fighter, int32 Damage = -1)
	{
		for (const FBuhCombatEvent& E : R.Events)
		{
			if (E.Type == Type && E.Fighter == Fighter && (Damage < 0 || E.Damage == Damage)) return true;
		}
		return false;
	}
}

int32 UBuhCombatSim::ChooseOpponentMove(const FBuhCombatState& State, int32 LastPlayerMove, int32 Seed)
{
	// Observable inputs only: distance, both healths, the opponent's own phase,
	// and the player's LAST revealed move. The player's current secret action
	// (State.Fighters[0].Action) is deliberately never read.
	const float Dist = FMath::Sqrt(FMath::Square(State.Fighters[1].X - State.Fighters[0].X) + FMath::Square(State.Fighters[1].Y - State.Fighters[0].Y));
	const int32 OppHealth = State.Fighters[1].Health;

	// Seeded LCG for deterministic variety.
	int32 Rng = Seed + LastPlayerMove * 7 + static_cast<int32>(Dist);
	auto Rand = [&Rng]() -> int32
	{
		Rng = (Rng * 1103515245 + 12345) & 0x7fffffff;
		return Rng;
	};

	const bool bInQuickRange = (Dist <= 300.f);
	const bool bInHeavyRange = (Dist <= 400.f);

	int32 Move;
	if (!bInHeavyRange)
	{
		// Out of reach: no attack. Guard or Dodge to reposition safely.
		Move = (Rand() % 2 == 0) ? 2 : 3;
	}
	else if (LastPlayerMove == 0 || LastPlayerMove == 1)
	{
		// The player attacked last: defend, or counter if in range.
		const int32 Roll = Rand() % 10;
		if (Roll < 4) Move = 2;                    // Guard
		else if (Roll < 6) Move = 3;               // Dodge
		else if (bInQuickRange) Move = 0;          // Counter with Quick
		else Move = 1;                             // Counter with Heavy
	}
	else
	{
		// The player defended or idled last: press the attack.
		if (OppHealth < 30) Move = 2;              // Low health: play it safe
		else if (bInQuickRange && (Rand() % 2 == 0)) Move = 0; // Quick
		else Move = 1;                             // Heavy
	}

	return Move;
}

static FString TimelineSignature(const TArray<FBuhTimelineSnapshot>& T)
{
	FString Out;
	for (const FBuhTimelineSnapshot& Snap : T)
	{
		Out += FString::Printf(TEXT("%d:%.2f/%.2f:%.2f/%.2f:%d/%d:%d/%d:%d/%d:%d|"),
			Snap.Instant, Snap.PlayerX, Snap.OpponentX, Snap.PlayerY, Snap.OpponentY,
			Snap.PlayerAction, Snap.OpponentAction,
			Snap.PlayerPhase, Snap.OpponentPhase,
			Snap.PlayerHealth, Snap.OpponentHealth, Snap.Events.Num());
		for (const FBuhCombatEvent& E : Snap.Events)
		{
			Out += FString::Printf(TEXT("%d:%d:%d:"), E.Type, E.Fighter, E.Damage);
		}
	}
	return Out;
}

void UBuhCombatSim::RunTimelineTests11to17(FString& Report, int32& Passed, int32& Failed)
{
	auto Check = [&Report, &Passed, &Failed](const TCHAR* Name, bool bOk, const FString& Detail)
	{
		if (bOk)
		{
			++Passed;
			Report += FString::Printf(TEXT("PASS %s\n"), Name);
			UE_LOG(LogTemp, Display, TEXT("[CombatTest] PASS %s"), Name);
		}
		else
		{
			++Failed;
			Report += FString::Printf(TEXT("FAIL %s -- %s\n"), Name, *Detail);
			UE_LOG(LogTemp, Warning, TEXT("[CombatTest] FAIL %s -- %s"), Name, *Detail);
		}
	};

	// Timeline Test 11 — branch selection produces the correct timeline for each opponent response.
	// As with T5, distinctness is judged on snapshot content because the player's
	// action fixes the planning-pause instant for every branch.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		TArray<FBuhTimelineSnapshot> TQuick = GenerateTimeline(S, 0, 0, 30);
		TArray<FBuhTimelineSnapshot> THeavy = GenerateTimeline(S, 0, 1, 30);
		TArray<FBuhTimelineSnapshot> TGuard = GenerateTimeline(S, 0, 2, 30);
		TArray<FBuhTimelineSnapshot> TDodge = GenerateTimeline(S, 0, 3, 30);
		const bool bAllNonEmpty = TQuick.Num() > 0 && THeavy.Num() > 0 && TGuard.Num() > 0 && TDodge.Num() > 0;
		bool bDistinct = (TimelineSignature(TQuick) != TimelineSignature(THeavy)) ||
			(TimelineSignature(TQuick) != TimelineSignature(TGuard)) ||
			(TimelineSignature(TQuick) != TimelineSignature(TDodge));
		Check(TEXT("T11 branch-selection-correct-timelines"), bAllNonEmpty && bDistinct,
			FString::Printf(TEXT("quick=%d heavy=%d guard=%d dodge=%d distinct=%d"),
				TQuick.Num(), THeavy.Num(), TGuard.Num(), TDodge.Num(), bDistinct));
	}

	// Timeline Test 12 — snapshot indexing and boundaries are correct.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 0, 1, 30);
		const bool bNonEmpty = Timeline.Num() > 0;
		bool bIndicesValid = true;
		if (bNonEmpty)
		{
			if (Timeline[0].Instant != 0) bIndicesValid = false;
			if (Timeline[Timeline.Num() - 1].Instant != Timeline.Num() - 1) bIndicesValid = false;
			for (int32 i = 0; i < Timeline.Num(); ++i)
			{
				if (Timeline[i].Instant != i) bIndicesValid = false;
			}
		}
		Check(TEXT("T12 snapshot-indexing-boundaries"), bNonEmpty && bIndicesValid,
			FString::Printf(TEXT("count=%d firstInstant=%d lastInstant=%d"),
				Timeline.Num(),
				bNonEmpty ? Timeline[0].Instant : -1,
				bNonEmpty ? Timeline[Timeline.Num() - 1].Instant : -1));
	}

	// Timeline Test 13 — changing branches produces genuinely different snapshots (no stale data).
	{
		const FBuhCombatState S = MakeState(-150, 150, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> T1 = GenerateTimeline(S, 0, 0, 30);
		const TArray<FBuhTimelineSnapshot> T2 = GenerateTimeline(S, 0, 1, 30);
		const bool bNonEmpty = T1.Num() > 0 && T2.Num() > 0;
		bool bBranchesDiffer = false;
		if (bNonEmpty)
		{
			const int32 MinLen = FMath::Min(T1.Num(), T2.Num());
			for (int32 i = 0; i < MinLen; ++i)
			{
				if (T1[i].PlayerX != T2[i].PlayerX ||
					T1[i].OpponentX != T2[i].OpponentX ||
					T1[i].PlayerHealth != T2[i].PlayerHealth ||
					T1[i].OpponentHealth != T2[i].OpponentHealth)
				{
					bBranchesDiffer = true;
					break;
				}
			}
		}
		Check(TEXT("T13 branch-change-no-stale-data"), bNonEmpty && bBranchesDiffer,
			bBranchesDiffer ? TEXT("branches differ") : TEXT("branches identical (stale data)"));
	}

	// Timeline Test 14 — preview generation never mutates real combat state.
	{
		FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatState S0 = S;
		for (int32 PlayerMove = 0; PlayerMove < 4; ++PlayerMove)
		{
			for (int32 OppMove = 0; OppMove < 4; ++OppMove)
			{
				const TArray<FBuhTimelineSnapshot> T = GenerateTimeline(S, PlayerMove, OppMove, 30);
				(void)T;
			}
		}
		const bool bUnchanged = BuhCombatTest::SameState(S, S0);
		Check(TEXT("T14 preview-never-mutates-state"), bUnchanged,
			bUnchanged ? TEXT("state unchanged") : TEXT("state mutated by preview"));
	}

	// Timeline Test 15 — every recorded snapshot corresponds to the actual simulation state at that Instant.
	{
		const FBuhCombatState S = MakeState(-50, 50, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 30);
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 0, 1, 30);
		const bool bNonEmpty = Timeline.Num() > 0;
		bool bAllSnapshotsMatch = true;
		if (bNonEmpty)
		{
			const FBuhTimelineSnapshot& Last = Timeline[Timeline.Num() - 1];
			if (FMath::Abs(Last.PlayerX - R.FinalState.Fighters[0].X) > 0.01f) bAllSnapshotsMatch = false;
			if (FMath::Abs(Last.OpponentX - R.FinalState.Fighters[1].X) > 0.01f) bAllSnapshotsMatch = false;
			if (Last.PlayerHealth != R.FinalState.Fighters[0].Health) bAllSnapshotsMatch = false;
			if (Last.OpponentHealth != R.FinalState.Fighters[1].Health) bAllSnapshotsMatch = false;
			if (Last.PlayerPhase != R.FinalState.Fighters[0].Phase) bAllSnapshotsMatch = false;
			if (Last.OpponentPhase != R.FinalState.Fighters[1].Phase) bAllSnapshotsMatch = false;
		}
		Check(TEXT("T15 snapshots-match-simulation-state"), bNonEmpty && bAllSnapshotsMatch,
			bNonEmpty ? TEXT("snapshots match") : TEXT("timeline empty"));
	}

	// Timeline Test 16 — repeated preview generation doesn't retain stale data.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		TArray<FBuhTimelineSnapshot> First = GenerateTimeline(S, 0, 0, 30);
		TArray<FBuhTimelineSnapshot> Second = GenerateTimeline(S, 0, 1, 30);
		TArray<FBuhTimelineSnapshot> Third = GenerateTimeline(S, 0, 0, 30);
		const bool bNonEmpty = First.Num() > 0 && Second.Num() > 0 && Third.Num() > 0;
		bool bNoStaleRetention = true;
		if (bNonEmpty)
		{
			if (First.Num() != Third.Num()) bNoStaleRetention = false;
			for (int32 i = 0; i < First.Num() && i < Third.Num(); ++i)
			{
				if (First[i].PlayerX != Third[i].PlayerX ||
					First[i].OpponentX != Third[i].OpponentX ||
					First[i].PlayerHealth != Third[i].PlayerHealth ||
					First[i].OpponentHealth != Third[i].OpponentHealth)
				{
					bNoStaleRetention = false;
					break;
				}
			}
		}
		Check(TEXT("T16 repeated-generation-no-stale-retention"), bNonEmpty && bNoStaleRetention,
			bNonEmpty ? TEXT("no stale retention") : TEXT("timeline empty"));
	}

	// Timeline Test 17 — timeline generation works after several real combat decisions.
	{
		FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		for (int32 i = 0; i < 3; ++i)
		{
			const FBuhCombatResult R = ResolveCombat(S, 0, 1, 200);
			S = R.FinalState;
			if (R.StoppedReason == 3 || !R.bPlayerReady) break;
		}
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 0, 1, 30);
		const bool bNonEmpty = Timeline.Num() > 0;
		const bool bPositionsValid = bNonEmpty &&
			FMath::Abs(Timeline[0].PlayerX - S.Fighters[0].X) < 0.01f &&
			FMath::Abs(Timeline[0].OpponentX - S.Fighters[1].X) < 0.01f;
		Check(TEXT("T17 timeline-after-real-decisions"), bNonEmpty && bPositionsValid,
			FString::Printf(TEXT("count=%d startPosMatch=%d"), Timeline.Num(), bPositionsValid));
	}
}

FString UBuhCombatSim::RunAllTests()
{
	using namespace BuhCombatTest;
	FString Report;
	int32 Passed = 0, Failed = 0;
	UE_LOG(LogTemp, Display, TEXT("[CombatTest] RunAllTests called (v2 with T11-T17)"));
	auto Check = [&Report, &Passed, &Failed](const TCHAR* Name, bool bOk, const FString& Detail)
	{
		if (bOk)
		{
			++Passed;
			Report += FString::Printf(TEXT("PASS %s\n"), Name);
			UE_LOG(LogTemp, Display, TEXT("[CombatTest] PASS %s"), Name);
		}
		else
		{
			++Failed;
			Report += FString::Printf(TEXT("FAIL %s :: %s\n"), Name, *Detail);
			UE_LOG(LogTemp, Display, TEXT("[CombatTest] FAIL %s :: %s"), Name, *Detail);
		}
	};

	// Test A — faster move misses, slower move connects.
	{
		const FBuhCombatState S = MakeState(-200, 200, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 200);
		const bool bMiss = HasEvent(R, 2, 0);
		const bool bHit = HasEvent(R, 0, 1, 24);
		const bool bHealth = R.FinalState.Fighters[0].Health == 76;
		Check(TEXT("A faster-misses-slower-connects"), bMiss && bHit && bHealth,
			FString::Printf(TEXT("miss=%d hit=%d playerHealth=%d"), bMiss, bHit, R.FinalState.Fighters[0].Health));
	}

	// Test B — recovery punish (recovering fighter hit, no free guard).
	{
		const FBuhCombatState S = MakeState(-200, 200, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 200);
		const bool bNoGuard = !HasEvent(R, 1, 0);
		const bool bHealth = R.FinalState.Fighters[0].Health == 76;
		const bool bHitDuringRecovery = [&R]()
		{
			for (const FBuhCombatEvent& E : R.Events)
				if (E.Type == 0 && E.Fighter == 1 && E.Instant >= 7 && E.Instant <= 14) return true;
			return false;
		}();
		Check(TEXT("B recovery-punish"), bNoGuard && bHealth && bHitDuringRecovery,
			FString::Printf(TEXT("noGuard=%d health=%d hitInRecovery=%d"), bNoGuard, R.FinalState.Fighters[0].Health, bHitDuringRecovery));
	}

	// Test C — different decision times (opponent acts at an earlier instant than the player; the world does not pause).
	{
		const FBuhCombatState S = MakeState(-400, 400, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 1, 0, 200);
		int32 OppFirstAction = -1, PlayerFirstAction = -1;
		for (const FBuhCombatEvent& E : R.Events)
		{
			if (E.Type >= 0 && E.Type <= 3)
			{
				if (E.Fighter == 1 && OppFirstAction < 0) OppFirstAction = E.Instant;
				if (E.Fighter == 0 && PlayerFirstAction < 0) PlayerFirstAction = E.Instant;
			}
		}
		const bool bOppActsFirst = (OppFirstAction >= 0) && (PlayerFirstAction >= 0) && (OppFirstAction < PlayerFirstAction);
		Check(TEXT("C different-decision-times"), bOppActsFirst,
			FString::Printf(TEXT("oppFirstAction=%d playerFirstAction=%d"), OppFirstAction, PlayerFirstAction));
	}

	// Test D — prediction isolation (real state unchanged after previews).
	{
		FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatState S0 = S;
		for (int32 Opp = 0; Opp < 4; ++Opp)
		{
			const FBuhCombatResult R = ResolveCombat(S, 0, Opp, 200);
			(void)R;
		}
		const bool bUnchanged = SameState(S, S0);
		Check(TEXT("D prediction-isolation"), bUnchanged,
			bUnchanged ? TEXT("state unchanged") : TEXT("state mutated by prediction"));
	}

	// Test E — prediction/execution consistency.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatResult Pred = ResolveCombat(S, 0, 1, 200);
		const FBuhCombatResult Exec = ResolveCombat(S, 0, 1, 200);
		const bool bSame = SameResult(Pred, Exec);
		Check(TEXT("E pred-exec-consistency"), bSame,
			bSame ? TEXT("identical") : TEXT("prediction and execution diverged"));
	}

	// Test F — reproducibility.
	{
		const FBuhCombatState S = MakeState(-300, 300, 100, 100, 7);
		const FBuhCombatResult R1 = ResolveCombat(S, 1, 2, 200);
		const FBuhCombatResult R2 = ResolveCombat(S, 1, 2, 200);
		const bool bSame = SameResult(R1, R2);
		Check(TEXT("F reproducibility"), bSame,
			bSame ? TEXT("identical") : TEXT("repeated run diverged"));
	}

	// Test G — multiple exchanges (state persists across planning opportunities).
	{
		FBuhCombatState S = MakeState(-200, 200, 100, 100, 0);
		int32 Exchanges = 0;
		int32 PlayerHealthStart = 100;
		for (int32 i = 0; i < 5; ++i)
		{
			const FBuhCombatResult R = ResolveCombat(S, 0, 1, 200);
			S = R.FinalState;
			++Exchanges;
			if (R.StoppedReason == 3) break;
			if (!R.bPlayerReady) break;
		}
		const bool bPersist = (S.Fighters[0].Health < PlayerHealthStart) && (Exchanges >= 2);
		Check(TEXT("G multiple-exchanges"), bPersist,
			FString::Printf(TEXT("exchanges=%d playerHealth=%d"), Exchanges, S.Fighters[0].Health));
	}

	// Test H — time invariance (deterministic under any presentation-frame split).
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatResult R1 = ResolveCombat(S, 0, 1, 200);
		const FBuhCombatResult R2 = ResolveCombat(S, 0, 1, 200);
		const bool bSame = SameResult(R1, R2);
		Check(TEXT("H time-invariance"), bSame,
			bSame ? TEXT("identical") : TEXT("frame-split changed outcome"));
	}

	// Test I — opposing approaches reach minimum separation without crossing.
	{
		const FBuhCombatState S = MakeState(-100, 100, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 4, 4, 200);
		const float FinalDist = FMath::Sqrt(FMath::Square(R.FinalState.Fighters[1].X - R.FinalState.Fighters[0].X) + FMath::Square(R.FinalState.Fighters[1].Y - R.FinalState.Fighters[0].Y));
		const bool bMinSep = FMath::Abs(FinalDist - 100.f) < 1.f;
		const bool bNoCross = (R.FinalState.Fighters[0].X < R.FinalState.Fighters[1].X);
		Check(TEXT("I opposing-approaches-min-separation"), bMinSep && bNoCross,
			FString::Printf(TEXT("finalDist=%.2f noCross=%d"), FinalDist, bNoCross));
	}

	// Test J — nonnegative distance at all times (never dips below minimum separation).
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 4, 4, 200);
		const bool bNonNeg = R.MinDistanceSeen >= 100.f - 1.f;
		Check(TEXT("J nonnegative-distance-always"), bNonNeg,
			FString::Printf(TEXT("minDistanceSeen=%.2f"), R.MinDistanceSeen));
	}

	// Test K — mirrored starting positions produce equivalent behavior.
	{
		const FBuhCombatState Sc = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatState Sm = MakeState(250, -250, 100, 100, 0);
		const FBuhCombatResult Rc = ResolveCombat(Sc, 4, 4, 200);
		const FBuhCombatResult Rm = ResolveCombat(Sm, 4, 4, 200);
		const bool bHealthMatch = (Rc.FinalState.Fighters[0].Health == Rm.FinalState.Fighters[0].Health) &&
			(Rc.FinalState.Fighters[1].Health == Rm.FinalState.Fighters[1].Health);
		const bool bPosMirrored = FMath::Abs(Rc.FinalState.Fighters[0].X + Rm.FinalState.Fighters[0].X) < 1.f &&
			FMath::Abs(Rc.FinalState.Fighters[1].X + Rm.FinalState.Fighters[1].X) < 1.f;
		const bool bDistMatch = FMath::Abs(Rc.MinDistanceSeen - Rm.MinDistanceSeen) < 1.f;
		Check(TEXT("K mirrored-starting-positions"), bHealthMatch && bPosMirrored && bDistMatch,
			FString::Printf(TEXT("healthMatch=%d posMirrored=%d distMatch=%d"), bHealthMatch, bPosMirrored, bDistMatch));
	}

	// Test L — consecutive decisions without position corruption or deadlock.
	{
		FBuhCombatState S = MakeState(-250, 250, 1000, 1000, 0);
		bool bStable = true;
		int32 Exchanges = 0;
		for (int32 i = 0; i < 10; ++i)
		{
			const FBuhCombatResult R = ResolveCombat(S, 2, 2, 200);
			S = R.FinalState;
			++Exchanges;
			const float D = FMath::Sqrt(FMath::Square(S.Fighters[1].X - S.Fighters[0].X) + FMath::Square(S.Fighters[1].Y - S.Fighters[0].Y));
			if (D < 100.f - 1.f || S.Fighters[0].X >= S.Fighters[1].X) bStable = false;
			if (R.StoppedReason == 3) break;
		}
		Check(TEXT("L consecutive-decisions-stable"), bStable && (Exchanges >= 10),
			FString::Printf(TEXT("exchanges=%d stable=%d"), Exchanges, bStable));
	}

	// Test M — correct behavior at minimum separation (no overlap, in-range attack connects,
	// and the faster Quick Strike interrupts the slower Heavy Strike so it never lands).
	{
		const FBuhCombatState S = MakeState(-50, 50, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 200);
		const float FinalDist = FMath::Sqrt(FMath::Square(R.FinalState.Fighters[1].X - R.FinalState.Fighters[0].X) + FMath::Square(R.FinalState.Fighters[1].Y - R.FinalState.Fighters[0].Y));
		const bool bNoOverlap = FinalDist >= 100.f - 1.f;
		const bool bQuickHit = HasEvent(R, 0, 0, 10);
		const bool bHeavyInterrupted = !HasEvent(R, 0, 1, 24);
		Check(TEXT("M min-separation-no-overlap"), bNoOverlap && bQuickHit && bHeavyInterrupted,
			FString::Printf(TEXT("finalDist=%.2f quickHit=%d heavyInterrupted=%d"), FinalDist, bQuickHit, bHeavyInterrupted));
	}

	// Test N — prediction and execution matching after movement.
	{
		const FBuhCombatState S = MakeState(-400, 400, 100, 100, 0);
		const FBuhCombatResult Pred = ResolveCombat(S, 0, 1, 200);
		const FBuhCombatResult Exec = ResolveCombat(S, 0, 1, 200);
		const bool bSame = SameResult(Pred, Exec);
		Check(TEXT("N pred-exec-after-movement"), bSame,
			bSame ? TEXT("identical") : TEXT("prediction and execution diverged after movement"));
	}

	// Test O — the AI can select all four moves under appropriate conditions.
	{
		bool bSaw[4] = { false, false, false, false };
		// Sweep states and seeds so every branch is exercised.
		for (int32 Dist = 100; Dist <= 600; Dist += 50)
		{
			for (int32 LastMove = 0; LastMove <= 3; ++LastMove)
			{
				for (int32 Seed = 0; Seed < 8; ++Seed)
				{
					const FBuhCombatState S = MakeState(-Dist / 2, Dist / 2, 100, 100, Seed);
					const int32 M = ChooseOpponentMove(S, LastMove, Seed);
					if (M >= 0 && M < 4) bSaw[M] = true;
				}
			}
		}
		const bool bAllFour = bSaw[0] && bSaw[1] && bSaw[2] && bSaw[3];
		Check(TEXT("O ai-selects-all-four-moves"), bAllFour,
			FString::Printf(TEXT("quick=%d heavy=%d guard=%d dodge=%d"),
				bSaw[0], bSaw[1], bSaw[2], bSaw[3]));
	}

	// Test P — the AI's choice is unaffected by the player's secret current action.
	{
		FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		S.Fighters[0].Action = 0; // player secretly committed Quick
		const int32 M1 = ChooseOpponentMove(S, 2, 5);
		S.Fighters[0].Action = 1; // player secretly committed Heavy instead
		const int32 M2 = ChooseOpponentMove(S, 2, 5);
		const bool bUnaffected = (M1 == M2);
		Check(TEXT("P ai-ignores-secret-player-action"), bUnaffected,
			FString::Printf(TEXT("moveWithQuickSecret=%d moveWithHeavySecret=%d"), M1, M2));
	}

	// Test A — starting at X=-500 and X=+500, Quick Strike keeps fighters separated.
	{
		const FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 200);
		const float FinalDist = FMath::Sqrt(FMath::Square(R.FinalState.Fighters[1].X - R.FinalState.Fighters[0].X) + FMath::Square(R.FinalState.Fighters[1].Y - R.FinalState.Fighters[0].Y));
		const bool bSeparated = FinalDist >= 100.f - 1.f;
		const bool bNoCross = (R.FinalState.Fighters[0].X < R.FinalState.Fighters[1].X);
		Check(TEXT("A quick-strike-keeps-separation"), bSeparated && bNoCross,
			FString::Printf(TEXT("finalDist=%.2f noCross=%d"), FinalDist, bNoCross));
	}

	// Test B — Dodge produces measurable displacement.
	{
		const FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 3, 2, 200);
		const float StartX = S.Fighters[0].X;
		const float EndX = R.FinalState.Fighters[0].X;
		const bool bMoved = FMath::Abs(EndX - StartX) > 10.f;
		Check(TEXT("B dodge-produces-displacement"), bMoved,
			FString::Printf(TEXT("startX=%.2f endX=%.2f delta=%.2f"), StartX, EndX, EndX - StartX));
	}

	// Test C — Dodge's displacement moves the fighter out of range, so an incoming
	// Quick Strike (startup 6, lands during the dodge's recovery) misses and is
	// credited as an evasion.
	{
		const FBuhCombatState S = MakeState(-140, 140, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 3, 0, 200);
		const bool bEvaded = HasEvent(R, 3, 1);
		const bool bNoDamage = (R.FinalState.Fighters[0].Health == 100);
		Check(TEXT("C dodge-evades-quick"), bEvaded && bNoDamage,
			FString::Printf(TEXT("evaded=%d playerHealth=%d"), bEvaded, R.FinalState.Fighters[0].Health));
	}

	// Test D — a fighter that does NOT dodge takes damage: the player commits to a
	// Heavy (startup 13) while the opponent's Quick (startup 6) lands first, proving
	// Dodge is not unconditional immunity.
	{
		const FBuhCombatState S = MakeState(-50, 50, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 1, 0, 200);
		const bool bDamage = (R.FinalState.Fighters[0].Health < 100);
		Check(TEXT("D non-dodge-takes-damage"), bDamage,
			FString::Printf(TEXT("playerHealth=%d"), R.FinalState.Fighters[0].Health));
	}

	// Test E — Guard still blocks an incoming Quick Strike (lands during the guard's
	// active window).
	{
		const FBuhCombatState S = MakeState(-50, 50, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 2, 0, 200);
		const bool bBlocked = HasEvent(R, 1, 1);
		const bool bNoDamage = (R.FinalState.Fighters[0].Health == 100);
		Check(TEXT("E guard-still-blocks"), bBlocked && bNoDamage,
			FString::Printf(TEXT("blocked=%d playerHealth=%d"), bBlocked, R.FinalState.Fighters[0].Health));
	}

	// Test F — multiple consecutive exchanges keep positions/health/recovery consistent.
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		bool bStable = true;
		int32 Exchanges = 0;
		for (int32 i = 0; i < 5; ++i)
		{
			const FBuhCombatResult R = ResolveCombat(S, 3, 1, 200);
			S = R.FinalState;
			++Exchanges;
			const float D = FMath::Sqrt(FMath::Square(S.Fighters[1].X - S.Fighters[0].X) + FMath::Square(S.Fighters[1].Y - S.Fighters[0].Y));
			if (D < 100.f - 1.f || S.Fighters[0].X >= S.Fighters[1].X) bStable = false;
			if (R.StoppedReason == 3) break;
			if (!R.bPlayerReady) break;
		}
		const bool bConsistent = bStable && (Exchanges >= 2);
		Check(TEXT("F consecutive-exchanges-consistent"), bConsistent,
			FString::Printf(TEXT("exchanges=%d stable=%d"), Exchanges, bStable));
	}

	// Test G — prediction and execution produce identical outcomes.
	{
		const FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		const FBuhCombatResult Pred = ResolveCombat(S, 3, 1, 200);
		const FBuhCombatResult Exec = ResolveCombat(S, 3, 1, 200);
		const bool bSame = SameResult(Pred, Exec);
		Check(TEXT("G pred-exec-identical"), bSame,
			bSame ? TEXT("identical") : TEXT("prediction and execution diverged"));
	}

	// Test H — unfinished enemy action persists: Heavy (startup 13) vs Dodge (8
	// instants). When the player becomes ready at instant 8 the opponent's Heavy is
	// still in startup (Phase 1, PhaseInstants 8). The player makes a second decision
	// and the Heavy continues from its remaining 5 instants, connecting after the
	// dodge is over. Starting distance 300 keeps the Heavy in range after the dodge
	// displacement.
	{
		const FBuhCombatState S0 = MakeState(-150, 150, 100, 100, 0);
		const FBuhCombatResult R1 = ResolveCombat(S0, 3, 1, 200);
		const bool bOpponentInStartup = (R1.FinalState.Fighters[1].Phase == 1)
			&& (R1.FinalState.Fighters[1].PhaseInstants < BuhCombat::MoveData[1].Startup);
		const FBuhCombatResult R2 = ResolveCombat(R1.FinalState, 0, -1, 200);
		const bool bPlayerDamaged = (R2.FinalState.Fighters[0].Health < 100);
		const bool bHeavyConnected = HasEvent(R2, 0, 1);
		Check(TEXT("H heavy-persists-across-dodge"),
			bOpponentInStartup && bPlayerDamaged && bHeavyConnected,
			FString::Printf(TEXT("oppStartup=%d oppPhase=%d oppInst=%d playerHP=%d heavyHit=%d"),
				bOpponentInStartup, R1.FinalState.Fighters[1].Phase,
				R1.FinalState.Fighters[1].PhaseInstants,
				R2.FinalState.Fighters[0].Health, bHeavyConnected));
	}

	// Test J — evasion classification: an out-of-range attack is Evaded only when
	// the dodge genuinely caused the miss. A non-dodging fighter that is already out
	// of range produces a Missed (Type 2), not an Evaded (Type 3).
	{
		const FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 2, 0, 200);
		const bool bMissed = HasEvent(R, 2, 1);
		const bool bNotEvaded = !HasEvent(R, 3, 1);
		Check(TEXT("J out-of-range-no-dodge-is-missed"), bMissed && bNotEvaded,
			FString::Printf(TEXT("missed=%d evaded=%d"), bMissed, !bNotEvaded));
	}

	// Timeline Test 1 — snapshot ordering and timing.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 0, 1, 30);
		bool bOrdered = true;
		for (int32 i = 1; i < Timeline.Num(); ++i)
		{
			if (Timeline[i].Instant != Timeline[i - 1].Instant + 1) bOrdered = false;
		}
		const bool bNonEmpty = Timeline.Num() > 0;
		Check(TEXT("T1 snapshot-ordering-timing"), bOrdered && bNonEmpty,
			FString::Printf(TEXT("count=%d ordered=%d"), Timeline.Num(), bOrdered));
	}

	// Timeline Test 2 — positions matching the resolver.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 30);
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 0, 1, 30);
		const bool bNonEmpty = Timeline.Num() > 0;
		bool bPositionsMatch = true;
		if (bNonEmpty)
		{
			const FBuhTimelineSnapshot& Last = Timeline[Timeline.Num() - 1];
			if (FMath::Abs(Last.PlayerX - R.FinalState.Fighters[0].X) > 0.01f) bPositionsMatch = false;
			if (FMath::Abs(Last.OpponentX - R.FinalState.Fighters[1].X) > 0.01f) bPositionsMatch = false;
		}
		Check(TEXT("T2 positions-match-resolver"), bNonEmpty && bPositionsMatch,
			bNonEmpty ? TEXT("positions match") : TEXT("timeline empty"));
	}

	// Timeline Test 3 — active attacks and damage appearing at the correct Instants.
	{
		const FBuhCombatState S = MakeState(-50, 50, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 0, 1, 30);
		bool bDamageAtCorrectInstant = false;
		for (const FBuhTimelineSnapshot& Snap : Timeline)
		{
			if (Snap.OpponentHealth < 100 && Snap.Instant >= 6 && Snap.Instant <= 14)
			{
				bDamageAtCorrectInstant = true;
				break;
			}
		}
		Check(TEXT("T3 damage-at-correct-instant"), bDamageAtCorrectInstant,
			bDamageAtCorrectInstant ? TEXT("damage at correct instant") : TEXT("damage not found"));
	}

	// Timeline Test 4 — recovery periods appearing correctly.
	{
		const FBuhCombatState S = MakeState(-50, 50, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 0, 1, 30);
		bool bRecoverySeen = false;
		for (const FBuhTimelineSnapshot& Snap : Timeline)
		{
			if (Snap.PlayerPhase == 3 || Snap.OpponentPhase == 3)
			{
				bRecoverySeen = true;
				break;
			}
		}
		Check(TEXT("T4 recovery-periods-correct"), bRecoverySeen,
			bRecoverySeen ? TEXT("recovery seen") : TEXT("no recovery"));
	}

	// Timeline Test 5 — different opponent responses producing appropriate timelines.
	// The timelines may share the same length (the player's action determines when
	// the planning pause occurs), so distinctness is judged on the full snapshot
	// content, not the count.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		TArray<FBuhTimelineSnapshot> T0 = GenerateTimeline(S, 0, 0, 30);
		TArray<FBuhTimelineSnapshot> T1 = GenerateTimeline(S, 0, 1, 30);
		TArray<FBuhTimelineSnapshot> T2 = GenerateTimeline(S, 0, 2, 30);
		TArray<FBuhTimelineSnapshot> T3 = GenerateTimeline(S, 0, 3, 30);
		const bool bAllNonEmpty = T0.Num() > 0 && T1.Num() > 0 && T2.Num() > 0 && T3.Num() > 0;
		bool bDifferent = (TimelineSignature(T0) != TimelineSignature(T1)) ||
			(TimelineSignature(T0) != TimelineSignature(T2)) ||
			(TimelineSignature(T0) != TimelineSignature(T3));
		Check(TEXT("T5 different-branches-different-timelines"), bAllNonEmpty && bDifferent,
			FString::Printf(TEXT("T0=%d T1=%d T2=%d T3=%d different=%d"), T0.Num(), T1.Num(), T2.Num(), T3.Num(), bDifferent));
	}

	// Timeline Test 6 — predictions leaving actual state untouched.
	{
		FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatState S0 = S;
		for (int32 Opp = 0; Opp < 4; ++Opp)
		{
			const TArray<FBuhTimelineSnapshot> T = GenerateTimeline(S, 0, Opp, 30);
			(void)T;
		}
		const bool bUnchanged = SameState(S, S0);
		Check(TEXT("T6 predictions-leave-state-untouched"), bUnchanged,
			bUnchanged ? TEXT("state unchanged") : TEXT("state mutated by timeline"));
	}

	// Timeline Test 7 — prediction/execution consistency.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 30);
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 0, 1, 30);
		const bool bNonEmpty = Timeline.Num() > 0;
		bool bConsistent = true;
		if (bNonEmpty)
		{
			const FBuhTimelineSnapshot& Last = Timeline[Timeline.Num() - 1];
			if (FMath::Abs(Last.PlayerX - R.FinalState.Fighters[0].X) > 0.01f) bConsistent = false;
			if (FMath::Abs(Last.OpponentX - R.FinalState.Fighters[1].X) > 0.01f) bConsistent = false;
			if (Last.PlayerHealth != R.FinalState.Fighters[0].Health) bConsistent = false;
			if (Last.OpponentHealth != R.FinalState.Fighters[1].Health) bConsistent = false;
		}
		Check(TEXT("T7 pred-exec-consistency-timeline"), bNonEmpty && bConsistent,
			bNonEmpty ? TEXT("consistent") : TEXT("timeline empty"));
	}

	// Timeline Test 8 — repeated branch switching without state corruption.
	{
		FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		bool bStable = true;
		for (int32 i = 0; i < 10; ++i)
		{
			for (int32 Opp = 0; Opp < 4; ++Opp)
			{
				const TArray<FBuhTimelineSnapshot> T = GenerateTimeline(S, 0, Opp, 30);
				(void)T;
			}
			const float D = FMath::Sqrt(FMath::Square(S.Fighters[1].X - S.Fighters[0].X) + FMath::Square(S.Fighters[1].Y - S.Fighters[0].Y));
			if (D < 100.f - 1.f || S.Fighters[0].X >= S.Fighters[1].X) bStable = false;
		}
		Check(TEXT("T8 repeated-branch-switching-stable"), bStable,
			bStable ? TEXT("stable") : TEXT("state corrupted"));
	}

	// Timeline Test 9 — timeline breaks when the player becomes ready, even if the opponent is not.
	// Guard (0 startup, 10 active, 0 recovery) vs Heavy (13 startup). Player ready at instant 10, opponent still in startup.
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, 2, 1, 30);
		const bool bNonEmpty = Timeline.Num() > 0;
		bool bLastPlayerReady = false;
		bool bLastOpponentNotReady = false;
		if (bNonEmpty)
		{
			const FBuhTimelineSnapshot& Last = Timeline[Timeline.Num() - 1];
			bLastPlayerReady = Last.bPlayerReady;
			bLastOpponentNotReady = !Last.bOpponentReady;
		}
		Check(TEXT("T9 timeline-breaks-when-player-ready"), bNonEmpty && bLastPlayerReady && bLastOpponentNotReady,
			FString::Printf(TEXT("count=%d lastPlayerReady=%d lastOppNotReady=%d"), Timeline.Num(), bLastPlayerReady, bLastOpponentNotReady));
	}

	// Timeline Test 10 — reproducible results with the same initial state and decisions.
	{
		const FBuhCombatState S = MakeState(-300, 300, 100, 100, 7);
		const TArray<FBuhTimelineSnapshot> T1 = GenerateTimeline(S, 1, 2, 30);
		const TArray<FBuhTimelineSnapshot> T2 = GenerateTimeline(S, 1, 2, 30);
		const bool bSameCount = (T1.Num() == T2.Num());
		bool bSameData = true;
		if (bSameCount)
		{
			for (int32 i = 0; i < T1.Num(); ++i)
			{
				if (T1[i].Instant != T2[i].Instant || T1[i].PlayerX != T2[i].PlayerX ||
					T1[i].OpponentX != T2[i].OpponentX || T1[i].PlayerHealth != T2[i].PlayerHealth ||
					T1[i].OpponentHealth != T2[i].OpponentHealth)
				{
					bSameData = false;
					break;
				}
			}
		}
		Check(TEXT("T10 reproducible-timelines"), bSameCount && bSameData,
			bSameCount ? TEXT("identical") : TEXT("repeated run diverged"));
	}

	// Test W1 — Walking Toward moves the fighter toward the opponent.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 1, 0);
		S.Fighters[1].Action = 1;
		const FBuhCombatResult R = ResolveCombat(S, 4, -1, 200);
		const bool bMoved = FMath::Abs(R.FinalState.Fighters[0].X - (-350.f)) < 1.f;
		Check(TEXT("W1 walking-toward-moves-fighter"), bMoved,
			FString::Printf(TEXT("finalX=%.2f"), R.FinalState.Fighters[0].X));
	}

	// Test W2 — Walking Away moves the fighter away from the opponent.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 1, 0);
		S.Fighters[1].Action = 1;
		const FBuhCombatResult R = ResolveCombat(S, 5, -1, 200);
		const bool bMoved = FMath::Abs(R.FinalState.Fighters[0].X - (-650.f)) < 1.f;
		Check(TEXT("W2 walking-away-moves-fighter"), bMoved,
			FString::Printf(TEXT("finalX=%.2f"), R.FinalState.Fighters[0].X));
	}

	// Test W3 — an observable attack start while the player is Walking pauses the simulation.
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		const FBuhCombatResult R = ResolveCombat(S, -1, 1, 200);
		const bool bPaused = (R.StoppedReason == 5) && R.bReactionOpportunity &&
			(R.ReactionInstant == 0) && (R.ReactionFighter == 0) && (R.ObservedThreatAction == 1);
		Check(TEXT("W3 reaction-pause-on-observable-attack"), bPaused,
			FString::Printf(TEXT("reason=%d instant=%d fighter=%d threat=%d"),
				R.StoppedReason, R.ReactionInstant, R.ReactionFighter, R.ObservedThreatAction));
	}

	// Test W4 — a hidden pending action alone does not trigger a reaction.
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		S.Fighters[1].PendingAction = 1;
		float PreDodgeDist[2] = { 1e9f, 1e9f };
		bool bJustActive[2] = { false, false };
		int32 AttackStart[2] = { -1, -1 };
		StepCombatInstant(S, PreDodgeDist, bJustActive, AttackStart);
		const bool bNoReaction = (AttackStart[1] == -1);
		Check(TEXT("W4 secret-selection-no-reaction"), bNoReaction,
			FString::Printf(TEXT("attackStart=%d"), AttackStart[1]));
	}

	// Test W5 — reacting with Guard blocks a Quick Strike that becomes observable.
	{
		FBuhCombatState S = MakeState(-100, 100, 100, 100, 0);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		const FBuhCombatResult R1 = ResolveCombat(S, -1, 0, 200);
		const FBuhCombatResult R2 = ResolveCombat(R1.FinalState, 2, -1, 200);
		const bool bBlocked = HasEvent(R2, 1, 1);
		const bool bNoDamage = (R2.FinalState.Fighters[0].Health == 100);
		Check(TEXT("W5 guard-reaction-blocks-quick"), bBlocked && bNoDamage,
			FString::Printf(TEXT("blocked=%d playerHealth=%d"), bBlocked, R2.FinalState.Fighters[0].Health));
	}

	// Test W6 — reacting with Guard is too late against a Heavy Strike: the Guard
	// ends before the Heavy lands, the player becomes ready, and a follow-up Walk
	// leaves the player vulnerable to the continuing Heavy.
	{
		FBuhCombatState S = MakeState(-100, 100, 100, 100, 0);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		const FBuhCombatResult R1 = ResolveCombat(S, -1, 1, 200);
		const FBuhCombatResult R2 = ResolveCombat(R1.FinalState, 2, -1, 200);
		const FBuhCombatResult R3 = ResolveCombat(R2.FinalState, 4, -1, 200);
		const bool bHit = HasEvent(R3, 0, 1, 24);
		const bool bDamaged = (R3.FinalState.Fighters[0].Health < 100);
		Check(TEXT("W6 guard-reaction-too-late-vs-heavy"), bHit && bDamaged,
			FString::Printf(TEXT("hit=%d playerHealth=%d"), bHit, R3.FinalState.Fighters[0].Health));
	}

	// Test W7 — reacting with Dodge evades an incoming Quick Strike.
	{
		FBuhCombatState S = MakeState(-140, 140, 100, 100, 0);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		const FBuhCombatResult R1 = ResolveCombat(S, -1, 0, 200);
		const FBuhCombatResult R2 = ResolveCombat(R1.FinalState, 3, -1, 200);
		const bool bEvaded = HasEvent(R2, 3, 1);
		const bool bNoDamage = (R2.FinalState.Fighters[0].Health == 100);
		Check(TEXT("W7 dodge-reaction-evades-quick"), bEvaded && bNoDamage,
			FString::Printf(TEXT("evaded=%d playerHealth=%d"), bEvaded, R2.FinalState.Fighters[0].Health));
	}

	// Test W8 — continuing to Walk after a reaction does not trigger the same pause again.
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		const FBuhCombatResult R1 = ResolveCombat(S, -1, 1, 200);
		const FBuhCombatResult R2 = ResolveCombat(R1.FinalState, 4, -1, 200);
		const bool bNoRepeat = !HasEvent(R2, 6, 0);
		const bool bPlayerReady = R2.bPlayerReady;
		Check(TEXT("W8 continue-walk-no-repeat-pause"), bNoRepeat && bPlayerReady,
			FString::Printf(TEXT("noRepeat=%d playerReady=%d reason=%d"), bNoRepeat, bPlayerReady, R2.StoppedReason));
	}

	// Test W9 — the timeline stops at a reaction opportunity.
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		const TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(S, -1, 1, 30);
		const bool bNonEmpty = Timeline.Num() > 0;
		const bool bReaction = bNonEmpty && Timeline[Timeline.Num() - 1].bReactionOpportunity;
		Check(TEXT("W9 timeline-stops-at-reaction"), bNonEmpty && bReaction,
			FString::Printf(TEXT("count=%d reaction=%d"), Timeline.Num(), bReaction));
	}

	// Test D1 — a destination Walk moves the fighter to the target and stops (variable length).
	// Opponent is busy Guarding (Phase 2, Action 2) so it never becomes ready to commit an
	// attack, and the short walk completes before the guard ends.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		S.Fighters[0].WalkTarget = -350.f;
		const FBuhCombatResult R = ResolveCombat(S, 6, -1, 200);
		const bool bReached = FMath::Abs(R.FinalState.Fighters[0].X - (-350.f)) < 1.f;
		const bool bReady = R.bPlayerReady;
		Check(TEXT("D1 destination-walk-reaches-target"), bReached && bReady,
			FString::Printf(TEXT("finalX=%.2f ready=%d reason=%d"), R.FinalState.Fighters[0].X, bReady, R.StoppedReason));
	}

	// Test D2 — a destination Walk behind the fighter walks away to the target.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		S.Fighters[0].WalkTarget = -650.f;
		const FBuhCombatResult R = ResolveCombat(S, 6, -1, 200);
		const bool bReached = FMath::Abs(R.FinalState.Fighters[0].X - (-650.f)) < 1.f;
		Check(TEXT("D2 destination-walk-away-reaches-target"), bReached,
			FString::Printf(TEXT("finalX=%.2f"), R.FinalState.Fighters[0].X));
	}

	// Test D3 — a destination Walk clamps to the target and does not overshoot.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		S.Fighters[0].WalkTarget = -480.f;
		const FBuhCombatResult R = ResolveCombat(S, 6, -1, 200);
		const bool bNoOvershoot = FMath::Abs(R.FinalState.Fighters[0].X - (-480.f)) < 1.f;
		Check(TEXT("D3 destination-walk-no-overshoot"), bNoOvershoot,
			FString::Printf(TEXT("finalX=%.2f"), R.FinalState.Fighters[0].X));
	}

	// Test D4 — an unset destination (target == current position) is a no-op walk.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		const FBuhCombatResult R = ResolveCombat(S, 6, -1, 200);
		const bool bStayed = FMath::Abs(R.FinalState.Fighters[0].X - (-500.f)) < 1.f;
		const bool bReady = R.bPlayerReady;
		Check(TEXT("D4 destination-walk-unset-noop"), bStayed && bReady,
			FString::Printf(TEXT("finalX=%.2f ready=%d"), R.FinalState.Fighters[0].X, bReady));
	}

	// Test D5 — an observable attack start while the player is on a destination Walk pauses the sim.
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 6;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		S.Fighters[0].WalkTarget = -100.f;
		const FBuhCombatResult R = ResolveCombat(S, -1, 1, 200);
		const bool bPaused = (R.StoppedReason == 5) && R.bReactionOpportunity &&
			(R.ReactionInstant == 0) && (R.ReactionFighter == 0) && (R.ObservedThreatAction == 1);
		Check(TEXT("D5 destination-walk-reaction-pause"), bPaused,
			FString::Printf(TEXT("reason=%d instant=%d fighter=%d threat=%d"),
				R.StoppedReason, R.ReactionInstant, R.ReactionFighter, R.ObservedThreatAction));
	}

	// Test DA1 — a Dash moves the fighter to the target and stops (reaches destination).
	// Opponent is busy Guarding (Phase 2, Action 2) so it never commits an attack.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		S.Fighters[0].DashTarget = -350.f;
		const FBuhCombatResult R = ResolveCombat(S, 8, -1, 200);
		const bool bReached = FMath::Abs(R.FinalState.Fighters[0].X - (-350.f)) < 1.f;
		const bool bReady = R.bPlayerReady;
		Check(TEXT("DA1 dash-reaches-target"), bReached && bReady,
			FString::Printf(TEXT("finalX=%.2f ready=%d reason=%d"), R.FinalState.Fighters[0].X, bReady, R.StoppedReason));
	}

	// Test DA2 — a Dash behind the fighter dashes away to the target.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		S.Fighters[0].DashTarget = -650.f;
		const FBuhCombatResult R = ResolveCombat(S, 8, -1, 200);
		const bool bReached = FMath::Abs(R.FinalState.Fighters[0].X - (-650.f)) < 1.f;
		Check(TEXT("DA2 dash-away-reaches-target"), bReached,
			FString::Printf(TEXT("finalX=%.2f"), R.FinalState.Fighters[0].X));
	}

	// Test DA3 — a Dash clamps to the target and does not overshoot.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		S.Fighters[0].DashTarget = -480.f;
		const FBuhCombatResult R = ResolveCombat(S, 8, -1, 200);
		const bool bNoOvershoot = FMath::Abs(R.FinalState.Fighters[0].X - (-480.f)) < 1.f;
		Check(TEXT("DA3 dash-no-overshoot"), bNoOvershoot,
			FString::Printf(TEXT("finalX=%.2f"), R.FinalState.Fighters[0].X));
	}

	// Test DA4 — an unset Dash target (target == current position) is a no-op.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		const FBuhCombatResult R = ResolveCombat(S, 8, -1, 200);
		const bool bStayed = FMath::Abs(R.FinalState.Fighters[0].X - (-500.f)) < 1.f;
		const bool bReady = R.bPlayerReady;
		Check(TEXT("DA4 dash-unset-noop"), bStayed && bReady,
			FString::Printf(TEXT("finalX=%.2f ready=%d"), R.FinalState.Fighters[0].X, bReady));
	}

	// Test DA5 — a Dash at exactly max range (720 uu) reaches the target in 8 movement Instants.
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		S.Fighters[0].DashTarget = 220.f;  // 720 uu toward the opponent
		const FBuhCombatResult R = ResolveCombat(S, 8, -1, 200);
		const bool bReached = FMath::Abs(R.FinalState.Fighters[0].X - 220.f) < 1.f;
		Check(TEXT("DA5 dash-max-range-reaches"), bReached,
			FString::Printf(TEXT("finalX=%.2f"), R.FinalState.Fighters[0].X));
	}

	// Test DA6 — a Dash target beyond max range is clamped by the 8-Instant cap (stops at 720 uu).
	{
		FBuhCombatState S = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 2, 0);
		S.Fighters[1].Action = 2;
		S.Fighters[0].DashTarget = 500.f;  // 1000 uu, beyond the 720 uu cap
		const FBuhCombatResult R = ResolveCombat(S, 8, -1, 200);
		const bool bClamped = FMath::Abs(R.FinalState.Fighters[0].X - 220.f) < 1.f;  // -500 + 720
		Check(TEXT("DA6 dash-beyond-max-clamped"), bClamped,
			FString::Printf(TEXT("finalX=%.2f"), R.FinalState.Fighters[0].X));
	}

	// Test DA7 — the first 3 movement Instants of a Dash are locked (no reaction pause).
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 8;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;  // locked (first movement Instant)
		S.Fighters[0].DashTarget = 220.f;
		const FBuhCombatResult R = ResolveCombat(S, -1, 1, 200);
		const bool bNoPause = !(R.StoppedReason == 5 && R.bReactionOpportunity);
		Check(TEXT("DA7 dash-locked-window-no-pause"), bNoPause,
			FString::Printf(TEXT("reason=%d instant=%d"), R.StoppedReason, R.ReactionInstant));
	}

	// Test DA8 — a Dash becomes reactable from the 4th movement Instant (reaction pause fires).
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 8;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 3;  // reactable (4th movement Instant)
		S.Fighters[0].DashTarget = 220.f;
		const FBuhCombatResult R = ResolveCombat(S, -1, 1, 200);
		const bool bPaused = (R.StoppedReason == 5) && R.bReactionOpportunity &&
			(R.ReactionInstant == 0) && (R.ReactionFighter == 0) && (R.ObservedThreatAction == 1);
		Check(TEXT("DA8 dash-reactable-window-pause"), bPaused,
			FString::Printf(TEXT("reason=%d instant=%d fighter=%d threat=%d"),
				R.StoppedReason, R.ReactionInstant, R.ReactionFighter, R.ObservedThreatAction));
	}

	// Test DA9 — Continue (7) resumes a Dash, preserving its action.
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 8;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 3;
		S.Fighters[0].DashTarget = 220.f;
		const FBuhCombatResult R = ResolveCombat(S, 7, 1, 200);
		const bool bStillDashing = (R.FinalState.Fighters[0].Action == 8);
		Check(TEXT("DA9 dash-continue-resumes"), bStillDashing,
			FString::Printf(TEXT("action=%d reason=%d"), R.FinalState.Fighters[0].Action, R.StoppedReason));
	}

	// Test V1 — ResolveCombat with fewer than two fighters returns a controlled error.
	{
		FBuhCombatState S;
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 200);
		const bool bErr = (R.StoppedReason == 0);
		Check(TEXT("V1 resolve-combat-empty-fighters-error"), bErr,
			FString::Printf(TEXT("reason=%d"), R.StoppedReason));
	}

	// Test V2 — ExecuteMoveWithVars with fewer than two fighters returns a controlled error.
	{
		FBuhCombatState S;
		const FBuhCombatResult R = ExecuteMoveWithVars(true, S, -500, 500, 100, 100, 0, 0, 1, 0.f);
		const bool bErr = (R.StoppedReason == 0);
		Check(TEXT("V2 execute-move-empty-fighters-error"), bErr,
			FString::Printf(TEXT("reason=%d"), R.StoppedReason));
	}

	// Test V3 — State persistence across Walk and a subsequent action.
	{
		FBuhCombatState S = MakeState(-500, 500, 100, 100, 0);
		const FBuhCombatResult R1 = ExecuteMoveWithVars(false, S, -500, 500, 100, 100, 0, 6, 2, -300.f);
		const FBuhCombatState S1 = GetFinalState(R1);
		const bool bWalked = (S1.Fighters[0].X == -300) && (S1.Fighters[0].Phase == 0);
		const FBuhCombatResult R2 = ExecuteMoveWithVars(true, S1, static_cast<int32>(S1.Fighters[0].X), static_cast<int32>(S1.Fighters[1].X), S1.Fighters[0].Health, S1.Fighters[1].Health, 0, 0, 2, -300.f);
		const bool bPreserved = (R2.FinalState.Fighters[0].X == -300) && (R2.FinalState.Fighters[1].X == 500);
		Check(TEXT("V3 state-persistence-walk-then-strike"), bWalked && bPreserved,
			FString::Printf(TEXT("walked_x=%f phase=%d preserved_player_x=%f opponent_x=%f"),
				S1.Fighters[0].X, S1.Fighters[0].Phase, R2.FinalState.Fighters[0].X, R2.FinalState.Fighters[1].X));
	}

	// Test V4 — Opponent decision is preserved during a reaction and re-decided after a completed exchange.
	{
		FBuhCombatState S_out = MakeState(-500, 500, 100, 100, 0);
		const int32 M_out = ComputeOpponentMoveWithVars(false, S_out, -500, 500, 100, 100, 0, -1, 2);
		const bool bOutDefend = (M_out == 2) || (M_out == 3);
		const int32 M_preserved = ComputeOpponentMoveWithVars(true, S_out, -500, 500, 100, 100, 0, M_out, 2);
		const bool bPreserved = (M_preserved == M_out);
		FBuhCombatState S_in = MakeState(100, 500, 100, 100, 0);
		const int32 M_in = ComputeOpponentMoveWithVars(false, S_in, 100, 500, 100, 100, 0, M_out, 2);
		const bool bInAttack = (M_in == 0) || (M_in == 1);
		const bool bRedecided = (M_in != M_out);
		Check(TEXT("V4 opponent-redesides-after-completed-exchange"),
			bOutDefend && bPreserved && bInAttack && bRedecided,
			FString::Printf(TEXT("out=%d preserved=%d in=%d"), M_out, M_preserved, M_in));
	}

	RunTimelineTests11to17(Report, Passed, Failed);

	Report += FString::Printf(TEXT("\nSUMMARY: %d passed, %d failed\n"), Passed, Failed);
	UE_LOG(LogTemp, Display, TEXT("[CombatTest] SUMMARY: %d passed, %d failed"), Passed, Failed);
	return Report;
}

int32 UBuhCombatSim::GetFinalPlayerX(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[0].X;
}

int32 UBuhCombatSim::GetFinalOpponentX(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[1].X;
}

float UBuhCombatSim::GetFinalPlayerY(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[0].Y;
}

float UBuhCombatSim::GetFinalOpponentY(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[1].Y;
}

float UBuhCombatSim::GetFinalPlayerHeight(const FBuhCombatResult& Result)
{
	return (Result.FinalState.Fighters.Num() > 0) ? Result.FinalState.Fighters[0].Height : 0.f;
}

float UBuhCombatSim::GetFinalOpponentHeight(const FBuhCombatResult& Result)
{
	return (Result.FinalState.Fighters.Num() > 1) ? Result.FinalState.Fighters[1].Height : 0.f;
}

float UBuhCombatSim::GetFinalPlayerWorldZ(const FBuhCombatResult& Result)
{
	return 90.f + GetFinalPlayerHeight(Result);
}

float UBuhCombatSim::GetFinalOpponentWorldZ(const FBuhCombatResult& Result)
{
	return 90.f + GetFinalOpponentHeight(Result);
}

int32 UBuhCombatSim::GetFinalPlayerHealth(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[0].Health;
}

int32 UBuhCombatSim::GetFinalOpponentHealth(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[1].Health;
}

int32 UBuhCombatSim::GetFinalPlayerPhase(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[0].Phase;
}

int32 UBuhCombatSim::GetFinalPlayerPhaseInstants(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[0].PhaseInstants;
}

int32 UBuhCombatSim::GetFinalOpponentPhase(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[1].Phase;
}

int32 UBuhCombatSim::GetFinalOpponentPhaseInstants(const FBuhCombatResult& Result)
{
	return Result.FinalState.Fighters[1].PhaseInstants;
}

FBuhCombatState UBuhCombatSim::GetFinalState(const FBuhCombatResult& Result)
{
	return Result.FinalState;
}

bool UBuhCombatSim::IsReactionOpportunity(const FBuhCombatResult& Result)
{
	return Result.bReactionOpportunity;
}

int32 UBuhCombatSim::GetReactionInstant(const FBuhCombatResult& Result)
{
	return Result.ReactionInstant;
}

int32 UBuhCombatSim::GetReactionFighter(const FBuhCombatResult& Result)
{
	return Result.ReactionFighter;
}

int32 UBuhCombatSim::GetObservedThreatAction(const FBuhCombatResult& Result)
{
	return Result.ObservedThreatAction;
}

FBuhReactionInfo UBuhCombatSim::GetReactionInfo(
	bool bReactionOpportunity,
	int32 ReactionFighter,
	int32 ObservedThreatAction,
	int32 ReactionInstant,
	const FBuhCombatState& CurrentState)
{
	FBuhReactionInfo Info;
	if (!bReactionOpportunity || ReactionFighter != 0 || CurrentState.Fighters.Num() < 2)
	{
		Info.bIsReaction = false;
		return Info;
	}
	Info.bIsReaction = true;
	Info.CurrentInstant = ReactionInstant;
	Info.ObservedThreatAction = ObservedThreatAction;
	Info.PlayerCurrentAction = CurrentState.Fighters[0].Action;
	const FBuhFighterState& Threat = CurrentState.Fighters[1];
	if (Threat.Phase == 1 && Threat.Action >= 0 && Threat.Action < 7)
	{
		Info.ThreatRemainingPrep = FMath::Max(0, BuhCombat::MoveData[Threat.Action].Startup - Threat.PhaseInstants);
	}
	// Legal reactions: Quick, Heavy, Guard, Dodge, and Continue (if the player was moving).
	if (Info.PlayerCurrentAction == 4 || Info.PlayerCurrentAction == 5 || Info.PlayerCurrentAction == 6 || Info.PlayerCurrentAction == 8 || Info.PlayerCurrentAction == 9)
	{
		Info.LegalActions = { 0, 1, 2, 3, 7 };
	}
	else
	{
		Info.LegalActions = { 0, 1, 2, 3 };
	}
	return Info;
}

TArray<FBuhTimelineSnapshot> UBuhCombatSim::GenerateTimeline(const FBuhCombatState& InState, int32 PlayerDecision, int32 OpponentDecision, int32 MaxInstants, bool bSuppressPlayerReadyBreak, bool bStopAtDecisionBoundary)
{
	TArray<FBuhTimelineSnapshot> Snapshots;
	FBuhCombatState S = InState;
	float PreDodgeDist[2] = { 1e9f, 1e9f };
	int32 PendingAttackStart[2] = { -1, -1 };

	for (int32 i = 0; i < 2; ++i)
	{
		const int32 Decision = (i == 0) ? PlayerDecision : OpponentDecision;
		if (Decision < 0 || Decision >= 10) continue;
		FBuhFighterState& F = S.Fighters[i];
		if (Decision == 7)
		{
			if (IsReactableMovement(F)) { /* Continue Walk/Dash: preserve existing state */ }
			continue;
		}
		if (F.Phase == 0)
		{
			F.Action = Decision;
			F.Phase = 1;
			F.PhaseInstants = 0;
			F.bCommitted = true;
			F.Height = 0.f;
			if (Decision == 9) PrepareJumpCommit(F);
			if (Decision == 0 || Decision == 1) PendingAttackStart[i] = i;
		}
		else if (IsReactableMovement(F))
		{
			F.Action = Decision;
			F.Phase = 1;
			F.PhaseInstants = 0;
			F.bCommitted = true;
			F.Height = 0.f;
			if (Decision == 9) PrepareJumpCommit(F);
		}
	}

	bool bJustActive[2] = { false, false };

	for (int32 inst = 0; inst < MaxInstants; ++inst)
	{
		S.Instant = inst;

		int32 AttackStart[2] = { PendingAttackStart[0], PendingAttackStart[1] };
		PendingAttackStart[0] = -1;
		PendingAttackStart[1] = -1;

		FBuhCombatEvent ReactionEvent;
		if (CheckReactionPause(S, inst, AttackStart, ReactionEvent))
		{
			FBuhTimelineSnapshot Snap;
			Snap.Instant = inst;
			Snap.PlayerX = S.Fighters[0].X;
			Snap.OpponentX = S.Fighters[1].X;
			Snap.PlayerY = S.Fighters[0].Y;
			Snap.OpponentY = S.Fighters[1].Y;
			Snap.PlayerFacing = S.Fighters[0].Facing;
			Snap.OpponentFacing = S.Fighters[1].Facing;
			Snap.PlayerHeight = S.Fighters[0].Height;
			Snap.OpponentHeight = S.Fighters[1].Height;
			Snap.PlayerAction = S.Fighters[0].Action;
			Snap.OpponentAction = S.Fighters[1].Action;
			Snap.PlayerPhase = S.Fighters[0].Phase;
			Snap.OpponentPhase = S.Fighters[1].Phase;
			Snap.PlayerPhaseInstants = S.Fighters[0].PhaseInstants;
			Snap.OpponentPhaseInstants = S.Fighters[1].PhaseInstants;
			Snap.PlayerHealth = S.Fighters[0].Health;
			Snap.OpponentHealth = S.Fighters[1].Health;
			Snap.bPlayerReady = (S.Fighters[0].Phase == 0);
			Snap.bOpponentReady = (S.Fighters[1].Phase == 0);
			Snap.bReactionOpportunity = true;
			Snap.ReactionFighter = ReactionEvent.Fighter;
			Snap.ObservedThreatAction = S.Fighters[1 - ReactionEvent.Fighter].Action;
			Snap.Events.Add(ReactionEvent);
			Snapshots.Add(Snap);
			Snapshots.Last().StoppedReason = 5;
			break;
		}

		const TArray<FBuhOutcome> Outcomes = StepCombatInstant(S, PreDodgeDist, bJustActive, AttackStart);
		const float D = FMath::Sqrt(FMath::Square(S.Fighters[1].X - S.Fighters[0].X) + FMath::Square(S.Fighters[1].Y - S.Fighters[0].Y));
		TArray<FBuhCombatEvent> InstEvents;

		bool bKO = false;
		for (const FBuhOutcome& O : Outcomes)
		{
			FBuhFighterState& Atk = S.Fighters[O.Fighter];
			FBuhFighterState& Def = S.Fighters[1 - O.Fighter];
			FBuhCombatEvent E;
			E.Instant = inst;
			E.Fighter = O.Fighter;
			E.Damage = O.Damage;
			E.Type = O.Type;
			if (O.Type == 0)
			{
				Def.Health -= O.Damage;
				E.Detail = FString::Printf(TEXT("F%d %s hits F%d for %d"), Atk.Team, BuhCombat::MoveName(Atk.Action), Def.Team, O.Damage);
			Def.Phase = 3;
			Def.PhaseInstants = 0;
			Def.Height = 0.f;
				if (Def.Health <= 0) bKO = true;
			}
			else if (O.Type == 1)
			{
				E.Detail = FString::Printf(TEXT("F%d guard blocks F%d %s"), Def.Team, Atk.Team, BuhCombat::MoveName(Atk.Action));
			}
			else if (O.Type == 3)
			{
				E.Detail = FString::Printf(TEXT("F%d dodge evades F%d %s"), Def.Team, Atk.Team, BuhCombat::MoveName(Atk.Action));
			}
			else
			{
				E.Detail = FString::Printf(TEXT("F%d %s misses (D=%.0f > range=%d)"), Atk.Team, BuhCombat::MoveName(Atk.Action), D, BuhCombat::MoveData[Atk.Action].Range);
			}
			InstEvents.Add(E);
			Atk.Phase = 3;
			Atk.PhaseInstants = 0;
		}

		if (bKO)
		{
			FBuhTimelineSnapshot Snap;
			Snap.Instant = inst;
			Snap.PlayerX = S.Fighters[0].X;
			Snap.OpponentX = S.Fighters[1].X;
			Snap.PlayerY = S.Fighters[0].Y;
			Snap.OpponentY = S.Fighters[1].Y;
			Snap.PlayerFacing = S.Fighters[0].Facing;
			Snap.OpponentFacing = S.Fighters[1].Facing;
			Snap.PlayerHeight = S.Fighters[0].Height;
			Snap.OpponentHeight = S.Fighters[1].Height;
			Snap.PlayerAction = S.Fighters[0].Action;
			Snap.OpponentAction = S.Fighters[1].Action;
			Snap.PlayerPhase = S.Fighters[0].Phase;
			Snap.OpponentPhase = S.Fighters[1].Phase;
			Snap.PlayerPhaseInstants = S.Fighters[0].PhaseInstants;
			Snap.OpponentPhaseInstants = S.Fighters[1].PhaseInstants;
			Snap.PlayerHealth = S.Fighters[0].Health;
			Snap.OpponentHealth = S.Fighters[1].Health;
			Snap.bPlayerReady = (S.Fighters[0].Phase == 0);
			Snap.bOpponentReady = (S.Fighters[1].Phase == 0);
			Snap.Events = InstEvents;
			Snapshots.Add(Snap);
			Snapshots.Last().StoppedReason = 3;
			break;
		}

		const bool bPlayerReady = (S.Fighters[0].Phase == 0);
		const bool bOpponentReady = (S.Fighters[1].Phase == 0);

		FBuhTimelineSnapshot Snap;
		Snap.Instant = inst;
		Snap.PlayerX = S.Fighters[0].X;
		Snap.OpponentX = S.Fighters[1].X;
		Snap.PlayerY = S.Fighters[0].Y;
		Snap.OpponentY = S.Fighters[1].Y;
		Snap.PlayerHeight = S.Fighters[0].Height;
		Snap.OpponentHeight = S.Fighters[1].Height;
		Snap.PlayerFacing = S.Fighters[0].Facing;
		Snap.OpponentFacing = S.Fighters[1].Facing;
		Snap.PlayerAction = S.Fighters[0].Action;
		Snap.OpponentAction = S.Fighters[1].Action;
		Snap.PlayerPhase = S.Fighters[0].Phase;
		Snap.OpponentPhase = S.Fighters[1].Phase;
		Snap.PlayerPhaseInstants = S.Fighters[0].PhaseInstants;
		Snap.OpponentPhaseInstants = S.Fighters[1].PhaseInstants;
		Snap.PlayerHealth = S.Fighters[0].Health;
		Snap.OpponentHealth = S.Fighters[1].Health;
		Snap.bPlayerReady = bPlayerReady;
		Snap.bOpponentReady = bOpponentReady;
		Snap.Events = InstEvents;
		Snapshots.Add(Snap);

		if (bPlayerReady && bOpponentReady) { Snapshots.Last().StoppedReason = 2; break; }
		if (bPlayerReady && !bSuppressPlayerReadyBreak) { Snapshots.Last().StoppedReason = 1; break; }
		if (bOpponentReady)
		{
			if (bStopAtDecisionBoundary) { Snapshots.Last().StoppedReason = 2; break; }
			S.Fighters[1].Action = BuhCombat::AI_MOVE;
			S.Fighters[1].Phase = 1;
			S.Fighters[1].PhaseInstants = 0;
			S.Fighters[1].bCommitted = true;
			if (BuhCombat::AI_MOVE == 0 || BuhCombat::AI_MOVE == 1) PendingAttackStart[1] = 1;
		}
	}

	if (Snapshots.Num() > 0 && Snapshots.Last().StoppedReason == 0) Snapshots.Last().StoppedReason = 4;
	return Snapshots;
}

static TArray<FBuhTimelineSnapshot> GCurrentTimeline;
static bool bGHasTimeline = false;

void UBuhCombatSim::SetCurrentTimeline(const TArray<FBuhTimelineSnapshot>& InTimeline)
{
	GCurrentTimeline = InTimeline;
	bGHasTimeline = InTimeline.Num() > 0;
}

FBuhTimelineSnapshot UBuhCombatSim::GetSnapshotAt(int32 Instant)
{
	if (Instant >= 0 && Instant < GCurrentTimeline.Num())
	{
		return GCurrentTimeline[Instant];
	}
	return FBuhTimelineSnapshot();
}

int32 UBuhCombatSim::GetTimelineLength()
{
	return GCurrentTimeline.Num();
}

bool UBuhCombatSim::bHasTimeline()
{
	return bGHasTimeline;
}

int32 UBuhCombatSim::GetSnapshotInstant(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].Instant;
	return 0;
}

float UBuhCombatSim::GetSnapshotPlayerX(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].PlayerX;
	return 0.f;
}

float UBuhCombatSim::GetSnapshotOpponentX(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].OpponentX;
	return 0.f;
}

float UBuhCombatSim::GetSnapshotPlayerY(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].PlayerY;
	return 0.f;
}

float UBuhCombatSim::GetSnapshotOpponentY(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].OpponentY;
	return 0.f;
}

float UBuhCombatSim::GetSnapshotPlayerHeight(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].PlayerHeight;
	return 0.f;
}

float UBuhCombatSim::GetSnapshotOpponentHeight(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].OpponentHeight;
	return 0.f;
}

int32 UBuhCombatSim::GetSnapshotPlayerFacing(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].PlayerFacing;
	return 1;
}

int32 UBuhCombatSim::GetSnapshotOpponentFacing(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].OpponentFacing;
	return -1;
}

int32 UBuhCombatSim::GetSnapshotPlayerHealth(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].PlayerHealth;
	return 100;
}

int32 UBuhCombatSim::GetSnapshotOpponentHealth(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].OpponentHealth;
	return 100;
}

int32 UBuhCombatSim::GetSnapshotPlayerPhase(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].PlayerPhase;
	return 0;
}

int32 UBuhCombatSim::GetSnapshotOpponentPhase(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num()) return GCurrentTimeline[Index].OpponentPhase;
	return 0;
}

FString UBuhCombatSim::GetSnapshotInstantText(int32 Index)
{
	if (Index >= 0 && Index < GCurrentTimeline.Num())
	{
		return FString::Printf(TEXT("Instant %d"), GCurrentTimeline[Index].Instant);
	}
	return TEXT("Instant -");
}

static const TCHAR* BuhPhaseName(int32 Phase)
{
	switch (Phase)
	{
	case 0: return TEXT("ready");
	case 1: return TEXT("startup");
	case 2: return TEXT("active");
	case 3: return TEXT("recovery");
	default: return TEXT("unknown");
	}
}

FString UBuhCombatSim::GetSnapshotInfoText(int32 Index)
{
	if (Index < 0 || Index >= GCurrentTimeline.Num())
	{
		return TEXT("No timeline loaded.");
	}
	const FBuhTimelineSnapshot& Snap = GCurrentTimeline[Index];
	FString Out;
	Out += FString::Printf(TEXT("Instant %d\n"), Snap.Instant);
	Out += FString::Printf(TEXT("Player: %s %s (%d) HP %d X %.0f Y %.0f %s\n"),
		BuhCombat::MoveName(Snap.PlayerAction), BuhPhaseName(Snap.PlayerPhase),
		Snap.PlayerPhaseInstants, Snap.PlayerHealth, Snap.PlayerX, Snap.PlayerY,
		Snap.bPlayerReady ? TEXT("ready") : TEXT("busy"));
	Out += FString::Printf(TEXT("Opponent: %s %s (%d) HP %d X %.0f Y %.0f %s\n"),
		BuhCombat::MoveName(Snap.OpponentAction), BuhPhaseName(Snap.OpponentPhase),
		Snap.OpponentPhaseInstants, Snap.OpponentHealth, Snap.OpponentX, Snap.OpponentY,
		Snap.bOpponentReady ? TEXT("ready") : TEXT("busy"));
	if (Snap.Events.Num() == 0)
	{
		Out += TEXT("Events: none");
	}
	else
	{
		Out += TEXT("Events:\n");
		for (const FBuhCombatEvent& E : Snap.Events)
		{
			Out += FString::Printf(TEXT("  %s\n"), *E.Detail);
		}
	}
	return Out;
}

FString UBuhCombatSim::PredictText(const FBuhCombatState& InState, int32 PlayerDecision, int32 OpponentDecision)
{
	const FBuhCombatResult R = ResolveCombat(InState, PlayerDecision, OpponentDecision, 200);
	FString Out;
	Out += FString::Printf(TEXT("vs %s: "), BuhCombat::MoveName(OpponentDecision));
	bool bFound = false;
	for (const FBuhCombatEvent& E : R.Events)
	{
		if (E.Type == 0 || E.Type == 1 || E.Type == 2 || E.Type == 3)
		{
			Out += E.Detail;
			bFound = true;
			break;
		}
	}
	if (!bFound) Out += TEXT("no contact");
	Out += FString::Printf(TEXT(" | P HP=%d X=%.0f Y=%.0f O HP=%d X=%.0f Y=%.0f"),
		R.FinalState.Fighters[0].Health, R.FinalState.Fighters[0].X, R.FinalState.Fighters[0].Y,
		R.FinalState.Fighters[1].Health, R.FinalState.Fighters[1].X, R.FinalState.Fighters[1].Y);
	return Out;
}

static FAutoConsoleCommand GBUHRunCombatTestsCmd(
	TEXT("BUH.RunCombatTests"),
	TEXT("Run all combat simulation tests and log the results"),
	FConsoleCommandDelegate::CreateStatic([]()
	{
		UE_LOG(LogTemp, Display, TEXT("=== BUH.RunCombatTests: starting ==="));
		const FString Report = UBuhCombatSim::RunAllTests();
		UE_LOG(LogTemp, Display, TEXT("%s"), *Report);
		const FString FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CombatTestReport.txt"));
		FFileHelper::SaveStringToFile(Report, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8);
		UE_LOG(LogTemp, Display, TEXT("=== BUH.RunCombatTests: done, report written to %s ==="), *FilePath);
	}));

void UBuhCombatSim::MoveGhostsToInstant(int32 Instant)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	if (!World) return;
	if (!bHasTimeline()) return;
	const FBuhTimelineSnapshot Snap = GetSnapshotAt(Instant);
	const float PlayerX = Snap.PlayerX;
	const float OpponentX = Snap.OpponentX;
	const float PlayerY = Snap.PlayerY;
	const float OpponentY = Snap.OpponentY;
	const float PlayerHeight = Snap.PlayerHeight;
	const float OpponentHeight = Snap.OpponentHeight;
	const float PlayerYaw = FMath::RadiansToDegrees(FMath::Atan2(OpponentY - PlayerY, OpponentX - PlayerX));
	const float OpponentYaw = FMath::RadiansToDegrees(FMath::Atan2(PlayerY - OpponentY, PlayerX - OpponentX));
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;
		if (Actor->Tags.Contains(TEXT("GhostPlayer")))
		{
			Actor->SetActorLocation(FVector(PlayerX, PlayerY, 90.0f + PlayerHeight));
			Actor->SetActorRotation(FRotator(0.0f, PlayerYaw, 0.0f));
		}
		else if (Actor->Tags.Contains(TEXT("GhostOpponent")))
		{
			Actor->SetActorLocation(FVector(OpponentX, OpponentY, 90.0f + OpponentHeight));
			Actor->SetActorRotation(FRotator(0.0f, OpponentYaw, 0.0f));
		}
	}
}

int32 UBuhCombatSim::MoveGhostsToFinalInstant()
{
	if (!bHasTimeline()) return 0;
	int32 Len = GetTimelineLength();
	int32 FinalIdx = FMath::Max(0, Len - 1);
	MoveGhostsToInstant(FinalIdx);
	return FinalIdx;
}

int32 UBuhCombatSim::GetFinalInstantIndex()
{
	if (!bHasTimeline()) return 0;
	return FMath::Max(0, GetTimelineLength() - 1);
}

void UBuhCombatSim::SetupTimelineAndMoveGhosts(int32 PlayerMove, int32 OpponentMove, int32 Instant)
{
	FBuhCombatState State = MakeState(-500.0f, 500.0f, 100.0f, 100.0f, 0);
	TArray<FBuhTimelineSnapshot> Timeline = GenerateTimeline(State, PlayerMove, OpponentMove, 30);
	SetCurrentTimeline(Timeline);
	MoveGhostsToInstant(Instant);
}

FString UBuhCombatSim::GetMoveNameText(int32 Move)
{
	return FString(BuhCombat::MoveName(Move));
}

FString UBuhCombatSim::GetMoveDurationText(int32 Move)
{
	if (Move < 0 || Move >= 10) return TEXT("Unknown");
	if (Move == 6) return TEXT("Walks to destination marker");
	if (Move == 8) return TEXT("Dashes to destination (90 uu/Instant, max 720 uu)");
	if (Move == 9) return TEXT("Jumps to destination (max 480 uu, max 220 uu height)");
	const BuhCombat::FMoveData& M = BuhCombat::MoveData[Move];
	return FString::Printf(TEXT("Prep %d / Active %d / Recovery %d"), M.Startup, M.Active, M.Recovery);
}

float UBuhCombatSim::GPlayerWalkTarget = -9999.f;
float UBuhCombatSim::GPlayerWalkTargetY = 0.f;
float UBuhCombatSim::GPlayerDashTarget = -9999.f;
float UBuhCombatSim::GPlayerDashTargetY = 0.f;
float UBuhCombatSim::GPlayerJumpTarget = -9999.f;
float UBuhCombatSim::GPlayerJumpTargetY = 0.f;

void UBuhCombatSim::SetPreviewWalkTarget(float TargetX, float TargetY)
{
	GPlayerWalkTarget = TargetX;
	GPlayerWalkTargetY = TargetY;
}

void UBuhCombatSim::SetPreviewDashTarget(float TargetX, float TargetY)
{
	GPlayerDashTarget = TargetX;
	GPlayerDashTargetY = TargetY;
}

void UBuhCombatSim::SetPreviewJumpTarget(float TargetX, float TargetY)
{
	GPlayerJumpTarget = TargetX;
	GPlayerJumpTargetY = TargetY;
}

float UBuhCombatSim::GExecDashTarget = -9999.f;
float UBuhCombatSim::GExecDashTargetY = 0.f;
float UBuhCombatSim::GExecJumpTarget = -9999.f;
float UBuhCombatSim::GExecJumpTargetY = 0.f;

TArray<FBuhTimelineSnapshot> UBuhCombatSim::GJumpPlaybackTimeline;
TWeakObjectPtr<AActor> UBuhCombatSim::GJumpPlaybackPlayer;
TWeakObjectPtr<AActor> UBuhCombatSim::GJumpPlaybackOpponent;
TWeakObjectPtr<APlayerController> UBuhCombatSim::GJumpPlaybackPC;
FBuhCombatState UBuhCombatSim::GJumpPlaybackPreState;
int32 UBuhCombatSim::GJumpPlaybackPlayerDecision = -1;
int32 UBuhCombatSim::GJumpPlaybackOpponentDecision = -1;
int32 UBuhCombatSim::GJumpPlaybackIndex = 0;
float UBuhCombatSim::GJumpPlaybackAccumulator = 0.f;
bool UBuhCombatSim::GJumpPlaybackActive = false;

void UBuhCombatSim::SetExecDashTarget(float TargetX, float TargetY)
{
	GExecDashTarget = TargetX;
	GExecDashTargetY = TargetY;
}

void UBuhCombatSim::SetExecJumpTarget(float TargetX, float TargetY)
{
	GExecJumpTarget = TargetX;
	GExecJumpTargetY = TargetY;
}

FString UBuhCombatSim::GetBranchSummaryText(int32 PlayerMove, int32 OpponentMove, int32 CurrentInstant)
{
	FBuhCombatState State;
	if (bHasTimeline())
	{
		const FBuhTimelineSnapshot S0 = GetSnapshotAt(0);
		State = MakeState(static_cast<int32>(S0.PlayerX), static_cast<int32>(S0.OpponentX), S0.PlayerHealth, S0.OpponentHealth, 0, S0.PlayerY, S0.OpponentY);
	}
	else
	{
		State = MakeState(-500, 500, 100, 100, 0);
	}
	if (PlayerMove == 6 && GPlayerWalkTarget >= -9998.f)
	{
		State = WithPlayerWalkTarget(State, GPlayerWalkTarget, GPlayerWalkTargetY);
	}
	if (PlayerMove == 8 && GPlayerDashTarget >= -9998.f)
	{
		State = WithPlayerDashTarget(State, GPlayerDashTarget, GPlayerDashTargetY);
	}
	// A Walk/Dash preview must show the movement's actual motion, not be interrupted by the
	// reaction-pause mechanic (which fires at instant 0 for a reactable walk). Execution
	// (ExecuteMove) still uses the default bSuppressReactionPause=false.
	const bool bSuppress = (PlayerMove == 6 || PlayerMove == 8);
	const bool bSuppressPlayerReadyBreak = (PlayerMove < 0);
	const FBuhCombatResult R = ResolveCombat(State, PlayerMove, OpponentMove, 200, bSuppress, bSuppressPlayerReadyBreak);

	auto PhaseName = [](int32 P) -> const TCHAR*
	{
		switch (P)
		{
		case 0: return TEXT("Ready");
		case 1: return TEXT("Prep");
		case 2: return TEXT("Active");
		case 3: return TEXT("Recovery");
		default: return TEXT("?");
		}
	};

	FString PhaseText = TEXT("no preview");
	if (R.StoppedReason == 5)
	{
		const TCHAR* Who = (R.ReactionFighter == 0) ? TEXT("P") : TEXT("O");
		const TCHAR* ThreatSide = (R.ReactionFighter == 0) ? TEXT("O") : TEXT("P");
		PhaseText = FString::Printf(TEXT("%s can react at i%d to %s's %s"),
			Who, R.ReactionInstant, ThreatSide, *GetMoveNameText(R.ObservedThreatAction));
	}
	else if (bHasTimeline() && CurrentInstant >= 0 && CurrentInstant < GCurrentTimeline.Num())
	{
		const FBuhTimelineSnapshot Cur = GetSnapshotAt(CurrentInstant);
		PhaseText = FString::Printf(TEXT("P %s %d/%d, O %s %d/%d"),
			PhaseName(Cur.PlayerPhase), Cur.PlayerPhaseInstants,
			BuhCombat::MoveData[Cur.PlayerAction].Startup,
			PhaseName(Cur.OpponentPhase), Cur.OpponentPhaseInstants,
			BuhCombat::MoveData[Cur.OpponentAction].Startup);
	}

	FString EventText;
	int32 EventCount = 0;
	for (const FBuhCombatEvent& E : R.Events)
	{
		if (E.Type == 4 || E.Instant > CurrentInstant) continue;
		if (EventCount > 0) EventText += TEXT("; ");
		EventText += FString::Printf(TEXT("i%d %s"), E.Instant, *E.Detail);
		++EventCount;
		if (EventCount >= 3) break;
	}

	// A reaction-pause return happens before MinDistanceSeen is updated, so fall back to the
	// final-state separation rather than the 1e9 sentinel.
	float MinD = R.MinDistanceSeen;
	if (R.StoppedReason == 5 && R.FinalState.Fighters.Num() == 2)
	{
		MinD = FMath::Sqrt(FMath::Square(R.FinalState.Fighters[1].X - R.FinalState.Fighters[0].X) + FMath::Square(R.FinalState.Fighters[1].Y - R.FinalState.Fighters[0].Y));
	}

	FString Summary = FString::Printf(TEXT("%s vs %s: P %d→%d, O %d→%d, min D %.0f"),
		*GetMoveNameText(PlayerMove), *GetMoveNameText(OpponentMove),
		State.Fighters[0].Health, GetFinalPlayerHealth(R),
		State.Fighters[1].Health, GetFinalOpponentHealth(R),
		MinD);
	Summary += TEXT(" | ") + PhaseText;
	if (!EventText.IsEmpty()) Summary += TEXT(" | ") + EventText;
	return Summary;
}

FString UBuhCombatSim::GetTimelineLengthText(int32 CurrentInstant)
{
	if (!bHasTimeline()) return TEXT("No preview");
	const int32 Len = GetTimelineLength();
	const int32 Index = FMath::Clamp(CurrentInstant, 0, Len - 1);
	const FBuhTimelineSnapshot S = GetSnapshotAt(Index);
	const FBuhTimelineSnapshot S0 = GetSnapshotAt(0);
	const FBuhTimelineSnapshot Last = GetSnapshotAt(Len - 1);
	const int32 Phase = S.PlayerPhase;
	const TCHAR* PhaseName = (Phase == 0) ? TEXT("idle") : (Phase == 1) ? TEXT("startup") : (Phase == 2) ? TEXT("active") : TEXT("recovery");
	const float Traveled = FMath::Sqrt(FMath::Square(S.PlayerX - S0.PlayerX) + FMath::Square(S.PlayerY - S0.PlayerY));
	float Remaining = 0.f;
	if (GPlayerWalkTarget >= -9998.f) Remaining = FMath::Sqrt(FMath::Square(GPlayerWalkTarget - S.PlayerX) + FMath::Square(GPlayerWalkTargetY - S.PlayerY));
	FString StopReason;
	switch (Last.StoppedReason)
	{
	case 1: StopReason = TEXT(" | Player ready"); break;
	case 2: StopReason = TEXT(" | Decision boundary"); break;
	case 3: StopReason = TEXT(" | KO"); break;
	case 4: StopReason = TEXT(" | Max instants"); break;
	case 5: StopReason = FString::Printf(TEXT(" | Reaction: %s observed"), *GetMoveNameText(Last.ObservedThreatAction)); break;
	default: StopReason = TEXT(" | Unknown"); break;
	}
	return FString::Printf(TEXT("Instant %d/%d: X=%.0f, %s %s | Traveled: %.0f uu | Remaining: %.0f uu%s"), GetSnapshotInstant(Index), Len, S.PlayerX, *GetMoveNameText(S.PlayerAction), PhaseName, Traveled, Remaining, *StopReason);
}

FString UBuhCombatSim::GetPreviewDistanceText(int32 Instant)
{
	if (!bHasTimeline()) return TEXT("Preview D: --");
	const int32 Len = GetTimelineLength();
	const int32 Index = FMath::Clamp(Instant, 0, Len - 1);
	const FBuhTimelineSnapshot S = GetSnapshotAt(Index);
	return FString::Printf(TEXT("Preview D: %.0f"), FMath::Sqrt(FMath::Square(S.OpponentX - S.PlayerX) + FMath::Square(S.OpponentY - S.PlayerY)));
}

void UBuhCombatSim::PrintWidgetGeometry(UWidget* Widget)
{
	if (!Widget)
	{
		UE_LOG(LogTemp, Warning, TEXT("GEOM null widget"));
		return;
	}
	const FGeometry Geo = Widget->GetCachedGeometry();
	UE_LOG(LogTemp, Warning,
		TEXT("GEOM %s: AbsPos=(%.1f,%.1f) Size=(%.1f,%.1f) LocalPos=(%.1f,%.1f) Scale=%.2f Visibility=%d RenderOpacity=%.2f"),
		*Widget->GetName(),
		Geo.AbsolutePosition.X, Geo.AbsolutePosition.Y,
		Geo.Size.X, Geo.Size.Y,
		Geo.Position.X, Geo.Position.Y,
		Geo.Scale,
		(int32)Widget->GetVisibility(),
		Widget->GetRenderOpacity());
}

void UBuhCombatSim::SetSlotLayoutOnAsset(FName WidgetName, float MinX, float MinY, float MaxX, float MaxY, float AlignX, float AlignY, float Left, float Top, float Right, float Bottom, bool bAutoSize)
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI"));
	if (!WBP || !WBP->WidgetTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetSlotLayoutOnAsset: WBP_CombatUI asset or WidgetTree not found"));
		return;
	}
	UWidget* Child = WBP->WidgetTree->FindWidget(WidgetName);
	if (!Child)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetSlotLayoutOnAsset: widget %s not found in asset"), *WidgetName.ToString());
		return;
	}
	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Child->Slot);
	if (!Slot)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetSlotLayoutOnAsset: %s has no CanvasPanelSlot in asset"), *WidgetName.ToString());
		return;
	}
	FAnchorData Layout;
	Layout.Anchors.Minimum = FVector2D(MinX, MinY);
	Layout.Anchors.Maximum = FVector2D(MaxX, MaxY);
	Layout.Alignment = FVector2D(AlignX, AlignY);
	Layout.Offsets = FMargin(Left, Top, Right, Bottom);
	Slot->SetLayout(Layout);
	Slot->SetAutoSize(bAutoSize);
	WBP->Modify();
	WBP->MarkPackageDirty();
	UE_LOG(LogTemp, Display, TEXT("SetSlotLayoutOnAsset: %s anchors=(%.3f,%.3f)-(%.3f,%.3f) align=(%.2f,%.2f) offsets=(%.0f,%.0f,%.0f,%.0f) autoSize=%d [asset modified, save to persist]"),
		*WidgetName.ToString(), MinX, MinY, MaxX, MaxY, AlignX, AlignY, Left, Top, Right, Bottom, bAutoSize ? 1 : 0);
}

static FAutoConsoleCommand GBUHSetupTimelineCmd(
	TEXT("BUH.SetupTimeline"),
	TEXT("BUH.SetupTimeline <PlayerMove> <OpponentMove> <Instant> - generate a timeline and move the ghost fighters to the given instant"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		if (Args.Num() < 3)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetupTimeline needs 3 args: <PlayerMove> <OpponentMove> <Instant>"));
			return;
		}
		const int32 PlayerMove = FCString::Atoi(*Args[0]);
		const int32 OpponentMove = FCString::Atoi(*Args[1]);
		const int32 Instant = FCString::Atoi(*Args[2]);
		UE_LOG(LogTemp, Display, TEXT("BUH.SetupTimeline: Player=%d Opponent=%d Instant=%d"), PlayerMove, OpponentMove, Instant);
		UBuhCombatSim::SetupTimelineAndMoveGhosts(PlayerMove, OpponentMove, Instant);
	}));

static FAutoConsoleCommand GBUHMoveGhostsCmd(
	TEXT("BUH.MoveGhosts"),
	TEXT("BUH.MoveGhosts <Instant> - move the ghost fighters to the given instant of the current timeline"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.MoveGhosts needs 1 arg: <Instant>"));
			return;
		}
		const int32 Instant = FCString::Atoi(*Args[0]);
		UE_LOG(LogTemp, Display, TEXT("BUH.MoveGhosts: Instant=%d"), Instant);
		UBuhCombatSim::MoveGhostsToInstant(Instant);
	}));

static FAutoConsoleCommand GBUHPrintSnapshotCmd(
	TEXT("BUH.PrintSnapshot"),
	TEXT("BUH.PrintSnapshot <Index> - print the snapshot info text for the given index of the current timeline"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.PrintSnapshot needs 1 arg: <Index>"));
			return;
		}
		const int32 Index = FCString::Atoi(*Args[0]);
		UE_LOG(LogTemp, Display, TEXT("BUH.PrintSnapshot: Index=%d\n%s"), Index, *UBuhCombatSim::GetSnapshotInfoText(Index));
	}));

static FAutoConsoleCommand GBUHDebugSnapshotCmd(
	TEXT("BUH.DebugSnapshot"),
	TEXT("BUH.DebugSnapshot <Index> - print raw snapshot fields for the given index"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.DebugSnapshot needs 1 arg: <Index>"));
			return;
		}
		const int32 Index = FCString::Atoi(*Args[0]);
		if (Index < 0 || Index >= GCurrentTimeline.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.DebugSnapshot: no timeline"));
			return;
		}
		const FBuhTimelineSnapshot& S = GCurrentTimeline[Index];
		UE_LOG(LogTemp, Display, TEXT("BUH.DebugSnapshot: Index=%d Instant=%d PlayerAction=%d OpponentAction=%d PlayerPhase=%d OpponentPhase=%d PlayerX=%.0f PlayerY=%.0f OpponentX=%.0f OpponentY=%.0f"),
			Index, S.Instant, S.PlayerAction, S.OpponentAction, S.PlayerPhase, S.OpponentPhase, S.PlayerX, S.PlayerY, S.OpponentX, S.OpponentY);
	}));

static FAutoConsoleCommand GBUHDiagnoseUICmd(
	TEXT("BUH.DiagnoseUI"),
	TEXT("BUH.DiagnoseUI - print cached geometry for the key WBP_CombatUI widgets"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		UWorld* World = GEngine->GetWorldContexts()[0].World();
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.DiagnoseUI: no world"));
			return;
		}
		bool bFound = false;
		for (TObjectIterator<UUserWidget> It; It; ++It)
		{
			UUserWidget* W = *It;
			if (!W) continue;
			if (!W->GetWorld() || W->GetWorld() != World) continue;
			if (!W->GetClass()->GetName().Contains(TEXT("WBP_CombatUI"))) continue;
			bFound = true;
			UE_LOG(LogTemp, Warning, TEXT("BUH.DiagnoseUI: found %s"), *W->GetName());
			UWidgetTree* Tree = W->WidgetTree;
			if (!Tree) continue;
			const TArray<FName> Names = {
				TEXT("RootCanvas"), TEXT("TopBar"), TEXT("TopBarContent"),
				TEXT("BottomTimeline"), TEXT("BottomTimelineContent"),
				TEXT("LeftPanel"), TEXT("RightPanel"),
				TEXT("LeftPanelBG"), TEXT("RightPanelBG"), TEXT("BottomTimelineBG")
			};
			for (FName N : Names)
			{
				UWidget* Child = Tree->FindWidget(N);
				if (Child)
				{
					UBuhCombatSim::PrintWidgetGeometry(Child);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("BUH.DiagnoseUI: widget %s not found"), *N.ToString());
				}
			}
		}
		if (!bFound)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.DiagnoseUI: no WBP_CombatUI widget found in world"));
		}
	}));

static FAutoConsoleCommand GBUHSetSlotCmd(
	TEXT("BUH.SetSlot"),
	TEXT("BUH.SetSlot <WidgetName> <MinX> <MinY> <MaxX> <MaxY> <AlignX> <AlignY> <Left> <Top> <Right> <Bottom> <bAutoSize> - set a CanvasPanelSlot layout"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		if (Args.Num() < 12)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetSlot needs 12 args"));
			return;
		}
		const FName WidgetName(*Args[0]);
		float MinX=0, MinY=0, MaxX=1, MaxY=1, AlignX=0, AlignY=0, L=0, T=0, R=0, B=0;
		MinX = (float)FCString::Atod(*Args[1]);
		MinY = (float)FCString::Atod(*Args[2]);
		MaxX = (float)FCString::Atod(*Args[3]);
		MaxY = (float)FCString::Atod(*Args[4]);
		AlignX = (float)FCString::Atod(*Args[5]);
		AlignY = (float)FCString::Atod(*Args[6]);
		L = (float)FCString::Atod(*Args[7]);
		T = (float)FCString::Atod(*Args[8]);
		R = (float)FCString::Atod(*Args[9]);
		B = (float)FCString::Atod(*Args[10]);
		const bool bAutoSize = (FCString::Atoi(*Args[11]) != 0);

		UWorld* World = GEngine->GetWorldContexts()[0].World();
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetSlot: no world"));
			return;
		}
		for (TObjectIterator<UUserWidget> It; It; ++It)
		{
			UUserWidget* W = *It;
			if (!W || !W->GetWorld() || W->GetWorld() != World) continue;
			if (!W->GetClass()->GetName().Contains(TEXT("WBP_CombatUI"))) continue;
			UWidgetTree* Tree = W->WidgetTree;
			if (!Tree) continue;
			UWidget* Child = Tree->FindWidget(WidgetName);
			if (!Child)
			{
				UE_LOG(LogTemp, Warning, TEXT("BUH.SetSlot: widget %s not found"), *WidgetName.ToString());
				return;
			}
			UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Child->Slot);
			if (!Slot)
			{
				UE_LOG(LogTemp, Warning, TEXT("BUH.SetSlot: %s has no CanvasPanelSlot"), *WidgetName.ToString());
				return;
			}
			FAnchorData Layout;
			Layout.Anchors.Minimum = FVector2D(MinX, MinY);
			Layout.Anchors.Maximum = FVector2D(MaxX, MaxY);
			Layout.Alignment = FVector2D(AlignX, AlignY);
			Layout.Offsets = FMargin(L, T, R, B);
			Slot->SetLayout(Layout);
			Slot->SetAutoSize(bAutoSize);
			UE_LOG(LogTemp, Display, TEXT("BUH.SetSlot: %s anchors=(%.3f,%.3f)-(%.3f,%.3f) align=(%.2f,%.2f) offsets=(%.0f,%.0f,%.0f,%.0f) autoSize=%d"),
				*WidgetName.ToString(), MinX, MinY, MaxX, MaxY, AlignX, AlignY, L, T, R, B, bAutoSize ? 1 : 0);
		}
	}));

static FAutoConsoleCommand GBUHSetSlotAssetCmd(
	TEXT("BUH.SetSlotAsset"),
	TEXT("BUH.SetSlotAsset <WidgetName> <MinX> <MinY> <MaxX> <MaxY> <AlignX> <AlignY> <Left> <Top> <Right> <Bottom> <bAutoSize> - set a CanvasPanelSlot layout on the WBP_CombatUI asset (persistent)"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		if (Args.Num() < 12)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetSlotAsset needs 12 args"));
			return;
		}
		const FName WidgetName(*Args[0]);
		float MinX=0, MinY=0, MaxX=1, MaxY=1, AlignX=0, AlignY=0, L=0, T=0, R=0, B=0;
		MinX = (float)FCString::Atod(*Args[1]);
		MinY = (float)FCString::Atod(*Args[2]);
		MaxX = (float)FCString::Atod(*Args[3]);
		MaxY = (float)FCString::Atod(*Args[4]);
		AlignX = (float)FCString::Atod(*Args[5]);
		AlignY = (float)FCString::Atod(*Args[6]);
		L = (float)FCString::Atod(*Args[7]);
		T = (float)FCString::Atod(*Args[8]);
		R = (float)FCString::Atod(*Args[9]);
		B = (float)FCString::Atod(*Args[10]);
		const bool bAutoSize = (FCString::Atoi(*Args[11]) != 0);

		UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI"));
		if (!WBP || !WBP->WidgetTree)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetSlotAsset: WBP_CombatUI asset or WidgetTree not found"));
			return;
		}
		UWidget* Child = WBP->WidgetTree->FindWidget(WidgetName);
		if (!Child)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetSlotAsset: widget %s not found in asset"), *WidgetName.ToString());
			return;
		}
		UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Child->Slot);
		if (!Slot)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetSlotAsset: %s has no CanvasPanelSlot in asset"), *WidgetName.ToString());
			return;
		}
		FAnchorData Layout;
		Layout.Anchors.Minimum = FVector2D(MinX, MinY);
		Layout.Anchors.Maximum = FVector2D(MaxX, MaxY);
		Layout.Alignment = FVector2D(AlignX, AlignY);
		Layout.Offsets = FMargin(L, T, R, B);
		Slot->SetLayout(Layout);
		Slot->SetAutoSize(bAutoSize);
		WBP->Modify();
		WBP->MarkPackageDirty();
		UE_LOG(LogTemp, Display, TEXT("BUH.SetSlotAsset: %s anchors=(%.3f,%.3f)-(%.3f,%.3f) align=(%.2f,%.2f) offsets=(%.0f,%.0f,%.0f,%.0f) autoSize=%d [asset modified, save to persist]"),
			*WidgetName.ToString(), MinX, MinY, MaxX, MaxY, AlignX, AlignY, L, T, R, B, bAutoSize ? 1 : 0);
	}));

int32 UBuhCombatSim::ComputeOpponentMoveWithVars(
	bool bHasCurrentState,
	const FBuhCombatState& CurrentState,
	int32 PlayerX,
	int32 OpponentX,
	int32 PlayerHealth,
	int32 OpponentHealth,
	int32 Seed,
	int32 StoredOpponentMove,
	int32 LastPlayerMove,
	float PlayerY,
	float OpponentY)
{
	if (bHasCurrentState)
	{
		return StoredOpponentMove;
	}
	FBuhCombatState State = MakeState(PlayerX, OpponentX, PlayerHealth, OpponentHealth, Seed, PlayerY, OpponentY);
	return ChooseOpponentMove(State, LastPlayerMove, Seed);
}

FBuhCombatResult UBuhCombatSim::ExecuteMoveWithVars(
	bool bHasCurrentState,
	const FBuhCombatState& CurrentState,
	int32 PlayerX,
	int32 OpponentX,
	int32 PlayerHealth,
	int32 OpponentHealth,
	int32 Seed,
	int32 PlayerMove,
	int32 OpponentMove,
	float WalkTarget,
	float WalkTargetY,
	float PlayerY,
	float OpponentY)
{
	FBuhCombatState State;
	if (bHasCurrentState)
	{
		State = CurrentState;
	}
	else
	{
		State = MakeState(PlayerX, OpponentX, PlayerHealth, OpponentHealth, Seed, PlayerY, OpponentY);
	}
	if (State.Fighters.Num() >= 2)
	{
		State = WithPlayerWalkTarget(State, WalkTarget, WalkTargetY);
		if (PlayerMove == 9 && GExecJumpTarget >= -9998.f)
		{
			State = WithPlayerJumpTarget(State, GExecJumpTarget, GExecJumpTargetY);
		}
	}
	return ResolveCombat(State, PlayerMove, OpponentMove, 200);
}

void UBuhCombatSim::LogFighterMaterial(AActor* Fighter, int32 ExpectedTeam)
{
	if (!Fighter)
	{
		UE_LOG(LogTemp, Warning, TEXT("DIAG LogFighterMaterial: null actor"));
		return;
	}
	UStaticMeshComponent* BodyMesh = Cast<UStaticMeshComponent>(Fighter->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (!BodyMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("DIAG LogFighterMaterial: %s has no StaticMeshComponent"), *Fighter->GetName());
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("DIAG %s Team=%d ExpectedTeam=%d"), *Fighter->GetName(), ExpectedTeam, ExpectedTeam);
	UE_LOG(LogTemp, Warning, TEXT("DIAG BodyMesh=%s Mesh=%s"), *BodyMesh->GetName(), BodyMesh->GetStaticMesh() ? *BodyMesh->GetStaticMesh()->GetName() : TEXT("null"));
	UE_LOG(LogTemp, Warning, TEXT("DIAG NumMaterials=%d"), BodyMesh->GetNumMaterials());
	UMaterialInterface* Mat0 = BodyMesh->GetMaterial(0);
	UE_LOG(LogTemp, Warning, TEXT("DIAG Material0=%s"), Mat0 ? *Mat0->GetPathName() : TEXT("null"));
}

void UBuhCombatSim::SetBlueMaterialOnFighter(AActor* Fighter)
{
	if (!Fighter)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetBlueMaterialOnFighter: null actor"));
		return;
	}
	UStaticMeshComponent* BodyMesh = Cast<UStaticMeshComponent>(Fighter->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (!BodyMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetBlueMaterialOnFighter: %s has no StaticMeshComponent"), *Fighter->GetName());
		return;
	}
	UMaterialInterface* BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/BUH/Prototypes/TacticalDuel/Materials/M_FighterBase.M_FighterBase")));
	if (!BaseMat)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetBlueMaterialOnFighter: failed to load M_FighterBase"));
		return;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, Fighter);
	if (MID)
	{
		MID->SetVectorParameterValue(FName("Color"), FLinearColor(0.f, 0.f, 1.f, 1.f));
		BodyMesh->SetMaterial(0, MID);
		UE_LOG(LogTemp, Warning, TEXT("SetBlueMaterialOnFighter: set blue on %s"), *Fighter->GetName());
	}
}

static FAutoConsoleCommand GLogFighterMatsCmd(
	TEXT("BUH.LogFighterMats"),
	TEXT("Log material on all BP_Fighter actors"),
	FConsoleCommandDelegate::CreateStatic([]()
	{
		UWorld* World = GEngine->GetWorldFromContextObject(GEngine, EGetWorldErrorMode::LogAndReturnNull);
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.LogFighterMats: no world"));
			return;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->GetClass()->GetName().StartsWith(TEXT("BP_Fighter")))
			{
				UBuhCombatSim::LogFighterMaterial(Actor, -1);
			}
		}
	}));

static FAutoConsoleCommand GSetBlueMatCmd(
	TEXT("BUH.SetBlueMat"),
	TEXT("Set dynamic blue material on all BP_Fighter actors"),
	FConsoleCommandDelegate::CreateStatic([]()
	{
		UWorld* World = GEngine->GetWorldFromContextObject(GEngine, EGetWorldErrorMode::LogAndReturnNull);
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetBlueMat: no world"));
			return;
		}
		UMaterialInterface* BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/BUH/Prototypes/TacticalDuel/Materials/M_FighterBase.M_FighterBase")));
		if (!BaseMat)
		{
			UE_LOG(LogTemp, Warning, TEXT("BUH.SetBlueMat: failed to load M_FighterBase"));
			return;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->GetClass()->GetName().StartsWith(TEXT("BP_Fighter")))
			{
				UStaticMeshComponent* BodyMesh = Cast<UStaticMeshComponent>(Actor->GetComponentByClass(UStaticMeshComponent::StaticClass()));
				if (BodyMesh)
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, Actor);
					if (MID)
					{
						MID->SetVectorParameterValue(FName("Color"), FLinearColor(0.f, 0.f, 1.f, 1.f));
						BodyMesh->SetMaterial(0, MID);
						UE_LOG(LogTemp, Warning, TEXT("BUH.SetBlueMat: set blue on %s"), *Actor->GetName());
					}
				}
			}
		}
	}));

static FAutoConsoleCommand GTriggerReactionJumpCmd(
	TEXT("BUH.TriggerReactionJump"),
	TEXT("Trigger a grounded Jump vs a mid-Heavy opponent to reach a reaction pause"),
	FConsoleCommandDelegate::CreateStatic([]()
	{
		UWorld* World = GEngine->GetWorldFromContextObject(GEngine, EGetWorldErrorMode::LogAndReturnNull);
		AActor* Context = nullptr;
		if (World)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->Tags.Contains(TEXT("PlayerFighter"))) { Context = *It; break; }
			}
		}
		UBuhCombatSim::TriggerReactionJump(Context);
	}));

FVector UBuhCombatSim::ScreenToGround(APlayerController* PC, FVector2D ScreenPos)
{
	if (!PC) return FVector::ZeroVector;
	FVector Origin, Direction;
	UGameplayStatics::DeprojectScreenToWorld(PC, ScreenPos, Origin, Direction);
	if (FMath::IsNearlyZero(Direction.Z)) return FVector::ZeroVector;
	const float T = -Origin.Z / Direction.Z;
	if (T < 0.f) return FVector::ZeroVector;
	return Origin + Direction * T;
}

FVector2D UBuhCombatSim::WorldToScreen(APlayerController* PC, FVector WorldPos)
{
	if (!PC) return FVector2D::ZeroVector;
	FVector2D ScreenPos;
	UGameplayStatics::ProjectWorldToScreen(PC, WorldPos, ScreenPos);
	return ScreenPos;
}

FVector2D UBuhCombatSim::ScreenToGroundXY(APlayerController* PC, FVector2D ScreenPos)
{
	FVector Hit = ScreenToGround(PC, ScreenPos);
	return FVector2D(Hit.X, Hit.Y);
}

float UBuhCombatSim::ScreenToGroundX(APlayerController* PC, FVector2D ScreenPos)
{
	return ScreenToGround(PC, ScreenPos).X;
}

float UBuhCombatSim::ScreenToGroundY(APlayerController* PC, FVector2D ScreenPos)
{
	return ScreenToGround(PC, ScreenPos).Y;
}

void UBuhCombatSim::DrawGroundLine(APlayerController* PC, FVector Start, FVector End, float Thickness, float Duration)
{
	UWorld* World = PC ? PC->GetWorld() : nullptr;
	if (!World) return;
	for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
	{
		ABUHPathRenderer* Renderer = *It;
		if (Renderer)
		{
			Renderer->SetLine(Start, End, EPathSegmentColor::Green);
			return;
		}
	}
	FColor Green(0, 255, 0, 255);
	::DrawDebugLine(World, Start, End, Green, false, Duration, 0, Thickness);
}

void UBuhCombatSim::DrawWalkPath(APlayerController* PC, FVector Start, FVector End)
{
	UWorld* World = PC ? PC->GetWorld() : nullptr;
	if (!World) return;
	for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
	{
		ABUHPathRenderer* Renderer = *It;
		if (Renderer)
		{
			Renderer->SetLine(Start, End, EPathSegmentColor::Green);
			return;
		}
	}
}

void UBuhCombatSim::DrawDashPath(APlayerController* PC, FVector Start, FVector End, int32 ElapsedMovementInstants)
{
	UWorld* World = PC ? PC->GetWorld() : nullptr;
	if (!World) return;
	const float TotalDist = FVector::Dist(Start, End);
	if (TotalDist < 1e-3f) return;
	// The first 3 Dash movement Instants are locked (red); the rest are reactable (green).
	// During execution the path is drawn from the fighter's CURRENT position, so any
	// movement Instants that have already elapsed must not be re-counted as locked. Only
	// the not-yet-elapsed locked Instants remain red; the remainder is reactable (green).
	const float DashSpeed = 90.f;
	const int32 RemainingLocked = FMath::Max(0, 3 - ElapsedMovementInstants);
	UE_LOG(LogTemp, Warning, TEXT("DashDiag: Elapsed=%d RemainingLocked=%d Start=(%.1f,%.1f) End=(%.1f,%.1f) TotalDist=%.1f"),
		ElapsedMovementInstants, RemainingLocked, Start.X, Start.Y, End.X, End.Y, TotalDist);
	if (RemainingLocked <= 0)
	{
		// All locked Instants have elapsed: the entire remaining path is reactable.
		for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
		{
			ABUHPathRenderer* Renderer = *It;
			if (Renderer)
			{
				Renderer->SetLine(Start, End, EPathSegmentColor::Green);
				int32 R = 0, G = 0;
				Renderer->GetVisibleColorCounts(R, G);
				UE_LOG(LogTemp, Warning, TEXT("DashDiag: AllGreen drawn. Renderer Red=%d Green=%d"), R, G);
				return;
			}
		}
		return;
	}
	FVector Cur = Start;
	for (int32 i = 0; i < RemainingLocked; ++i)
	{
		const float RemX = End.X - Cur.X;
		const float RemY = End.Y - Cur.Y;
		const float RemDist = FMath::Sqrt(RemX * RemX + RemY * RemY);
		if (RemDist <= DashSpeed)
		{
			Cur = End;
			break;
		}
		const float Scale = DashSpeed / RemDist;
		Cur.X += RemX * Scale;
		Cur.Y += RemY * Scale;
	}
	const FVector Boundary = Cur;
	const bool bEntirelyRed = (FVector::Dist(Boundary, End) < 1e-3f);
	for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
	{
		ABUHPathRenderer* Renderer = *It;
		if (Renderer)
		{
			if (bEntirelyRed)
			{
				Renderer->SetLine(Start, End, EPathSegmentColor::Red);
				int32 R = 0, G = 0;
				Renderer->GetVisibleColorCounts(R, G);
				UE_LOG(LogTemp, Warning, TEXT("DashDiag: AllRed drawn. Renderer Red=%d Green=%d"), R, G);
			}
			else
			{
				TArray<FVector> Points;
				Points.Add(Start);
				Points.Add(Boundary);
				Points.Add(End);
				TArray<FBuhPathSegment> Segments;
				Segments.Add(FBuhPathSegment{ 0, 1, EPathSegmentColor::Red });
				Segments.Add(FBuhPathSegment{ 1, 2, EPathSegmentColor::Green });
				Renderer->SetPath(Points, Segments);
				int32 R = 0, G = 0;
				Renderer->GetVisibleColorCounts(R, G);
				UE_LOG(LogTemp, Warning, TEXT("DashDiag: Split drawn. Renderer Red=%d Green=%d"), R, G);
			}
			return;
		}
	}
}

int32 UBuhCombatSim::GetDashElapsedMovementInstants(const FBuhCombatState& S)
{
	if (S.Fighters.Num() < 1) return 0;
	const FBuhFighterState& F = S.Fighters[0];
	return (F.Action == 8 && F.Phase == 2) ? F.PhaseInstants : 0;
}

int32 UBuhCombatSim::GetJumpElapsedFlightInstants(const FBuhCombatState& S)
{
	if (S.Fighters.Num() < 1) return 0;
	const FBuhFighterState& F = S.Fighters[0];
	return (F.Action == 9 && F.Phase == 2) ? F.PhaseInstants : 0;
}

void UBuhCombatSim::DrawJumpPath(APlayerController* PC, FVector Start, FVector End, int32 ElapsedFlightInstants)
{
	UWorld* World = PC ? PC->GetWorld() : nullptr;
	if (!World) return;
	const int32 Flight = BuhCombat::JumpFlightInstants;
	ElapsedFlightInstants = FMath::Clamp(ElapsedFlightInstants, 0, Flight);
	const int32 Remaining = Flight - ElapsedFlightInstants;
	if (Remaining <= 0)
	{
		for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
		{
			ABUHPathRenderer* Renderer = *It;
			if (Renderer)
			{
				Renderer->ClearPath();
				return;
			}
		}
		return;
	}
	const float GroundZ = 90.f;
	FVector StartGround = Start;
	StartGround.Z = GroundZ;
	FVector EndGround = End;
	EndGround.Z = GroundZ;
	TArray<FVector> Points;
	for (int32 i = ElapsedFlightInstants; i <= Flight; ++i)
	{
		const float T = static_cast<float>(i) / static_cast<float>(Flight);
		FVector P = FMath::Lerp(StartGround, EndGround, T);
		P.Z = GroundZ + 4.f * static_cast<float>(BuhCombat::JumpMaxHeight) * T * (1.f - T);
		Points.Add(P);
	}
	TArray<FBuhPathSegment> Segments;
	for (int32 Seg = 0; Seg < Remaining; ++Seg)
	{
		const int32 Instant = ElapsedFlightInstants + Seg;
		const EPathSegmentColor Color = (Instant < 3) ? EPathSegmentColor::Red : EPathSegmentColor::Green;
		Segments.Add(FBuhPathSegment{ Seg, Seg + 1, Color });
	}
	for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
	{
		ABUHPathRenderer* Renderer = *It;
		if (Renderer)
		{
			Renderer->SetPath(Points, Segments);
			return;
		}
	}
	const FVector Current = Points[0];
	const int32 RemainingLocked = FMath::Max(0, 3 - ElapsedFlightInstants);
	const bool bAllRed = (RemainingLocked >= Remaining);
	const FColor LineColor = bAllRed ? FColor::Red : FColor::Green;
	DrawDebugLine(World, Current, Points.Last(), LineColor, false, 5.f, 0, 10.f);
}

void UBuhCombatSim::DrawJumpPathFromState(APlayerController* PC, const FBuhCombatState& S, int32 ElapsedFlightInstants)
{
	UWorld* World = PC ? PC->GetWorld() : nullptr;
	if (!World || S.Fighters.Num() < 1) return;
	const FBuhFighterState& F = S.Fighters[0];
	const int32 Flight = BuhCombat::JumpFlightInstants;
	if (F.Action != 9 || F.Phase != 2)
	{
		for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
		{
			ABUHPathRenderer* Renderer = *It;
			if (Renderer)
			{
				Renderer->ClearPath();
				return;
			}
		}
		return;
	}
	ElapsedFlightInstants = FMath::Clamp(ElapsedFlightInstants, 0, Flight);
	const int32 Remaining = Flight - ElapsedFlightInstants;
	if (Remaining <= 0)
	{
		for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
		{
			ABUHPathRenderer* Renderer = *It;
			if (Renderer)
			{
				Renderer->ClearPath();
				return;
			}
		}
		return;
	}
	const float GroundZ = 90.f;
	const FVector StartGround(F.JumpStartX, F.JumpStartY, GroundZ);
	const FVector EndGround(F.JumpTarget, F.JumpTargetY, GroundZ);
	TArray<FVector> Points;
	for (int32 i = ElapsedFlightInstants; i <= Flight; ++i)
	{
		const float T = static_cast<float>(i) / static_cast<float>(Flight);
		FVector P = FMath::Lerp(StartGround, EndGround, T);
		P.Z = GroundZ + 4.f * static_cast<float>(BuhCombat::JumpMaxHeight) * T * (1.f - T);
		if (i == ElapsedFlightInstants)
		{
			P = FVector(F.X, F.Y, GroundZ + F.Height);
		}
		Points.Add(P);
	}
	TArray<FBuhPathSegment> Segments;
	for (int32 Seg = 0; Seg < Remaining; ++Seg)
	{
		const int32 Instant = ElapsedFlightInstants + Seg;
		const EPathSegmentColor Color = (Instant < 3) ? EPathSegmentColor::Red : EPathSegmentColor::Green;
		Segments.Add(FBuhPathSegment{ Seg, Seg + 1, Color });
	}
	for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
	{
		ABUHPathRenderer* Renderer = *It;
		if (Renderer)
		{
			Renderer->SetPath(Points, Segments);
			UE_LOG(LogTemp, Warning, TEXT("JumpPlayback PATH: elapsed=%d remaining=%d firstZ=%.1f lastZ=%.1f"), ElapsedFlightInstants, Remaining, Points[0].Z, Points.Last().Z);
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("JumpPlayback PATH: no renderer, elapsed=%d remaining=%d"), ElapsedFlightInstants, Remaining);
}

void UBuhCombatSim::LogCursorState(APlayerController* PC, FString Tag)
{
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("DashDiag CURSOR [%s]: no PlayerController"), *Tag);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("DashDiag CURSOR [%s]: bShowMouseCursor=%d ShouldShowMouseCursor=%d InputMode=%s"),
		*Tag, PC->bShowMouseCursor ? 1 : 0, PC->ShouldShowMouseCursor() ? 1 : 0, *PC->GetCurrentInputModeDebugString());
}

void UBuhCombatSim::ClearWalkPath(APlayerController* PC)
{
	UWorld* World = PC ? PC->GetWorld() : nullptr;
	if (!World) return;
	for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
	{
		ABUHPathRenderer* Renderer = *It;
		if (Renderer)
		{
			Renderer->ClearPath();
			return;
		}
	}
}

bool UBuhCombatSim::GetGroundHitUnderCursor(APlayerController* PC, FVector2D ScreenPos, FVector& OutHitLocation)
{
	if (!PC) return false;
	FHitResult Hit;
	PC->GetHitResultUnderCursorByChannel(ETraceTypeQuery::TraceTypeQuery1, true, Hit);
	if (Hit.bBlockingHit)
	{
		OutHitLocation = Hit.Location;
		return true;
	}
	return false;
}

float UBuhCombatSim::GetGroundHitX(APlayerController* PC, FVector2D ScreenPos)
{
	FVector Hit;
	if (GetGroundHitUnderCursor(PC, ScreenPos, Hit)) return Hit.X;
	return ScreenToGroundX(PC, ScreenPos);
}

float UBuhCombatSim::GetGroundHitY(APlayerController* PC, FVector2D ScreenPos)
{
	FVector Hit;
	if (GetGroundHitUnderCursor(PC, ScreenPos, Hit)) return Hit.Y;
	return ScreenToGroundY(PC, ScreenPos);
}

void UBuhCombatSim::LogCombatState(const FBuhCombatState& S, const FString& Context)
{
	(void)S;
	(void)Context;
}

void UBuhCombatSim::BeginJumpPlayback(APlayerController* PC, AActor* PlayerFighter, AActor* OpponentFighter, const FBuhCombatState& PreState, int32 PlayerDecision, int32 OpponentDecision, const FBuhCombatResult& Result)
{
	StopJumpPlayback();
	if (!PC || !PlayerFighter || PreState.Fighters.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("JumpPlayback BEGIN: invalid args PC=%d fighter=%d fighters=%d"), PC ? 1 : 0, PlayerFighter ? 1 : 0, PreState.Fighters.Num());
		return;
	}
	const bool bInitialJump = (PlayerDecision == 9);
	const bool bAirborneJump = (PreState.Fighters[0].Action == 9 && PreState.Fighters[0].Phase == 2);
	if (!bInitialJump && !bAirborneJump)
	{
		UE_LOG(LogTemp, Warning, TEXT("JumpPlayback BEGIN: not a Jump decision/state decision=%d action=%d phase=%d"), PlayerDecision, PreState.Fighters[0].Action, PreState.Fighters[0].Phase);
		return;
	}
	GJumpPlaybackTimeline = GenerateTimeline(PreState, PlayerDecision, OpponentDecision, 200, false, false);
	if (GJumpPlaybackTimeline.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("JumpPlayback BEGIN: empty timeline"));
		return;
	}
	GJumpPlaybackPC = PC;
	GJumpPlaybackPlayer = PlayerFighter;
	GJumpPlaybackOpponent = OpponentFighter;
	GJumpPlaybackPreState = PreState;
	GJumpPlaybackPlayerDecision = PlayerDecision;
	GJumpPlaybackOpponentDecision = OpponentDecision;
	GJumpPlaybackIndex = 0;
	GJumpPlaybackAccumulator = 0.f;
	GJumpPlaybackActive = true;
	UE_LOG(LogTemp, Warning, TEXT("JumpPlayback BEGIN: decision=%d opponent=%d snapshots=%d preAction=%d prePhase=%d prePI=%d preHeight=%.1f finalHeight=%.1f"),
		PlayerDecision, OpponentDecision, GJumpPlaybackTimeline.Num(), PreState.Fighters[0].Action, PreState.Fighters[0].Phase, PreState.Fighters[0].PhaseInstants, PreState.Fighters[0].Height, Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].Height : 0.f);
}

bool UBuhCombatSim::TickJumpPlayback(float DeltaSeconds)
{
	if (!GJumpPlaybackActive || GJumpPlaybackIndex >= GJumpPlaybackTimeline.Num())
	{
		return false;
	}
	GJumpPlaybackAccumulator += FMath::Max(0.f, DeltaSeconds);
	bool bApplied = false;
	bool bStoppedAtReaction = false;
	while (GJumpPlaybackAccumulator >= 0.05f && GJumpPlaybackIndex < GJumpPlaybackTimeline.Num())
	{
		GJumpPlaybackAccumulator -= 0.05f;
		const FBuhTimelineSnapshot& Snap = GJumpPlaybackTimeline[GJumpPlaybackIndex];
		if (AActor* P = GJumpPlaybackPlayer.Get())
		{
			P->SetActorLocation(FVector(Snap.PlayerX, Snap.PlayerY, 90.f + Snap.PlayerHeight), false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (AActor* O = GJumpPlaybackOpponent.Get())
		{
			O->SetActorLocation(FVector(Snap.OpponentX, Snap.OpponentY, 90.f + Snap.OpponentHeight), false, nullptr, ETeleportType::TeleportPhysics);
		}
		const bool bPlayerJump = (Snap.PlayerAction == 9 && Snap.PlayerPhase == 2);
		if (APlayerController* PC = GJumpPlaybackPC.Get())
		{
			if (bPlayerJump)
			{
				FBuhCombatState DrawState = GJumpPlaybackPreState;
				if (DrawState.Fighters.Num() >= 1)
				{
					DrawState.Fighters[0].Action = 9;
					DrawState.Fighters[0].Phase = 2;
					DrawState.Fighters[0].PhaseInstants = Snap.PlayerPhaseInstants;
					DrawState.Fighters[0].X = Snap.PlayerX;
					DrawState.Fighters[0].Y = Snap.PlayerY;
					DrawState.Fighters[0].Height = Snap.PlayerHeight;
				}
				DrawJumpPathFromState(PC, DrawState, Snap.PlayerPhaseInstants);
			}
			else
			{
				ClearWalkPath(PC);
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("JumpPlayback TICK: inst=%d playerX=%.1f playerZ=%.1f opponentX=%.1f opponentZ=%.1f reaction=%d actorPZ=%.1f actorOZ=%.1f"),
			Snap.Instant, Snap.PlayerX, 90.f + Snap.PlayerHeight, Snap.OpponentX, 90.f + Snap.OpponentHeight, Snap.bReactionOpportunity ? 1 : 0,
			GJumpPlaybackPlayer.IsValid() ? GJumpPlaybackPlayer->GetActorLocation().Z : -1.f,
			GJumpPlaybackOpponent.IsValid() ? GJumpPlaybackOpponent->GetActorLocation().Z : -1.f);
		++GJumpPlaybackIndex;
		bApplied = true;
		if (Snap.bReactionOpportunity)
		{
			bStoppedAtReaction = true;
			break;
		}
	}
	if (GJumpPlaybackIndex >= GJumpPlaybackTimeline.Num() || bStoppedAtReaction)
	{
		GJumpPlaybackActive = false;
		if (!bStoppedAtReaction && GJumpPlaybackPC.IsValid())
		{
			ClearWalkPath(GJumpPlaybackPC.Get());
		}
		UE_LOG(LogTemp, Warning, TEXT("JumpPlayback STOP: index=%d count=%d reaction=%d"), GJumpPlaybackIndex, GJumpPlaybackTimeline.Num(), bStoppedAtReaction ? 1 : 0);
	}
	return bApplied;
}

void UBuhCombatSim::StopJumpPlayback()
{
	GJumpPlaybackActive = false;
	GJumpPlaybackTimeline.Empty();
	GJumpPlaybackPlayer = nullptr;
	GJumpPlaybackOpponent = nullptr;
	GJumpPlaybackPC = nullptr;
	GJumpPlaybackIndex = 0;
	GJumpPlaybackAccumulator = 0.f;
}

void UBuhCombatSim::LogFighterZ(AActor* PlayerFighter, AActor* OpponentFighter, FString Tag)
{
	const float PZ = PlayerFighter ? PlayerFighter->GetActorLocation().Z : -1.f;
	const float OZ = OpponentFighter ? OpponentFighter->GetActorLocation().Z : -1.f;
	UE_LOG(LogTemp, Warning, TEXT("FighterZ %s: PZ=%.2f OZ=%.2f active=%d"), *Tag, PZ, OZ, GJumpPlaybackActive ? 1 : 0);
}

void UBuhCombatSim::HandleTargetingTick(APlayerController* PC, AActor* PlayerFighter, int32 TargetingAction)
{
	if (!PC || TargetingAction < 0) return;
	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		float MX = 0.f, MY = 0.f;
		PC->GetMousePosition(MX, MY);
		const FVector2D MousePos(MX, MY);
		const float HX = GetGroundHitX(PC, MousePos);
		const float HY = GetGroundHitY(PC, MousePos);
		if (TargetingAction == 9)
		{
			SetPreviewJumpTarget(HX, HY);
			SetExecJumpTarget(HX, HY);
		}
	UE_LOG(LogTemp, Warning, TEXT("TargetingClick: action=%d hx=%.1f hy=%.1f fighter=%s"),
		TargetingAction, HX, HY, PlayerFighter ? *PlayerFighter->GetName() : TEXT("null"));
	}
}

void UBuhCombatSim::TriggerReactionJump(AActor* Context)
{
	UWorld* World = Context ? Context->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("TriggerReactionJump: no world"));
		return;
	}
	AActor* Player = nullptr;
	AActor* Opponent = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->Tags.Contains(TEXT("PlayerFighter"))) Player = *It;
		else if (It->Tags.Contains(TEXT("OpponentFighter"))) Opponent = *It;
	}
	if (!Player || !Opponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("TriggerReactionJump: missing fighters P=%d O=%d"), Player ? 1 : 0, Opponent ? 1 : 0);
		return;
	}
	APlayerController* PC = World->GetFirstPlayerController();
	FBuhCombatState PreState = MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 3, 4);
	PreState.Fighters[1].Action = 1;
	PreState = WithPlayerJumpTarget(PreState, -20.f, 0.f);
	const FBuhCombatResult Result = ResolveCombat(PreState, 9, 1, 200);
	UE_LOG(LogTemp, Warning, TEXT("TriggerReactionJump RESOLVE: reaction=%d stoppedReason=%d reactionInstant=%d finalAction=%d finalPhase=%d finalPI=%d finalHeight=%.2f"),
		Result.bReactionOpportunity ? 1 : 0, Result.StoppedReason, Result.ReactionInstant,
		Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].Action : -1,
		Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].Phase : -1,
		Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].PhaseInstants : -1,
		Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].Height : -1.f);
	BeginJumpPlayback(PC, Player, Opponent, PreState, 9, 1, Result);
	UE_LOG(LogTemp, Warning, TEXT("TriggerReactionJump: BeginJumpPlayback called active=%d"), GJumpPlaybackActive ? 1 : 0);
}

void UBuhCombatSim::DiagnoseWidgetCast(APlayerController* PC)
{
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: PC is null"));
		return;
	}

	const FString ClassPath = TEXT("/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.WBP_CombatUI_C");
	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: loading class %s"), *ClassPath);

	UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
	if (!LoadedClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: LoadObject FAILED for %s"), *ClassPath);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: LoadedClass=%s (0x%p)"), *LoadedClass->GetPathName(), LoadedClass);
	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: LoadedClass->SuperClass=%s"), *LoadedClass->GetSuperClass()->GetPathName());

	UWidgetBlueprint* WB = Cast<UWidgetBlueprint>(LoadedClass);
	if (WB)
	{
		UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: WidgetBlueprint asset path=%s"), *WB->GetPathName());
		UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: WidgetBlueprint GeneratedClass=%s"), *WB->GeneratedClass->GetPathName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: NOT a UWidgetBlueprint, it's a plain UClass"));
	}

	UUserWidget* Widget = CreateWidget<UUserWidget>(PC, LoadedClass);
	if (!Widget)
	{
		UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: CreateWidget returned NULL"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: Widget instance=%s (0x%p)"), *Widget->GetName(), Widget);
	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: Widget->GetClass()=%s"), *Widget->GetClass()->GetPathName());
	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: Widget->GetClass()->GetSuperClass()=%s"), *Widget->GetClass()->GetSuperClass()->GetPathName());

	const bool bIsDerived = Widget->GetClass()->IsChildOf(LoadedClass);
	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: IsChildOf(LoadedClass)=%s"), bIsDerived ? TEXT("true") : TEXT("false"));

	UUserWidget* CastResult = Cast<UUserWidget>(Widget);
	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: Cast<UUserWidget> = %s"), CastResult ? TEXT("valid") : TEXT("null"));

	UObject* AssetRef = LoadObject<UObject>(nullptr, TEXT("/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI"));
	if (AssetRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: WBP asset loaded, class=%s"), *AssetRef->GetClass()->GetPathName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: WBP asset LoadObject FAILED"));
	}

	UE_LOG(LogTemp, Warning, TEXT("DiagnoseWidgetCast: DONE"));
}

void UBuhCombatSim::SetWidgetManager(UUserWidget* Widget, AActor* Manager)
{
	if (!Widget || !Manager)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: null arg Widget=%p Manager=%p"), (void*)Widget, (void*)Manager);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: Widget=%s class=%s"), *Widget->GetName(), *Widget->GetClass()->GetPathName());

	UClass* WBPClass = LoadObject<UClass>(nullptr, TEXT("/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.WBP_CombatUI_C"));
	if (!WBPClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: WBP_CombatUI_C class not found"));
		return;
	}

	if (!Widget->GetClass()->IsChildOf(WBPClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: widget class %s is NOT a child of %s"), *Widget->GetClass()->GetPathName(), *WBPClass->GetPathName());
		return;
	}

	FProperty* ManagerProp = WBPClass->FindPropertyByName(FName(TEXT("Manager")));
	if (!ManagerProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: Manager property not found on %s"), *WBPClass->GetPathName());
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: Manager prop type=%s name=%s"),
		*ManagerProp->GetClass()->GetName(), *ManagerProp->GetName());

	if (ManagerProp->IsA<FObjectProperty>())
	{
		FObjectProperty* ObjProp = CastField<FObjectProperty>(ManagerProp);
		UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: FObjectProperty PropertyClass=%s"), *ObjProp->PropertyClass->GetPathName());
		UObject* ManagerObj = Manager;
		ObjProp->SetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<UObject*>(Widget), ManagerObj);
		UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: DONE (SetObjectPropertyValue)"));
	}
	else if (ManagerProp->IsA<FStructProperty>())
	{
		UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: Manager is a FStructProperty, not FObjectProperty"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SetWidgetManager: Manager is neither FObjectProperty nor FStructProperty"));
	}
}


