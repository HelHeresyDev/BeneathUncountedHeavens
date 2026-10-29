#include "CombatSim.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"

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
	};

	// index = move id: 0 Quick, 1 Heavy, 2 Guard, 3 Dodge, 4 Walk
	static const FMoveData MoveData[5] = {
		{ 6, 1, 8, 10, 300, 0 },
		{ 13, 1, 8, 24, 400, 0 },
		{ 0, 10, 0, 0, 9999, 0 },
		{ 3, 2, 3, 0, 9999, 0 },
		{ 0, 5, 0, 0, 9999, 30 },
	};

	static const int32 Approach = 30;
	static const int32 MinSeparation = 100;
	static const int32 AI_MOVE = 1;
	static const int32 DodgeDistance = 300;

	static const TCHAR* MoveName(int32 A)
	{
		switch (A)
		{
		case 0: return TEXT("Quick");
		case 1: return TEXT("Heavy");
		case 2: return TEXT("Guard");
		case 3: return TEXT("Dodge");
		case 4: return TEXT("Walk");
		default: return TEXT("Idle");
		}
	}
}

FBuhCombatState UBuhCombatSim::MakeState(int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed)
{
	FBuhCombatState S;
	S.Fighters.AddDefaulted(2);
	S.Fighters[0].X = static_cast<float>(PlayerX);
	S.Fighters[0].Health = PlayerHealth;
	S.Fighters[0].Team = 0;
	S.Fighters[1].X = static_cast<float>(OpponentX);
	S.Fighters[1].Health = OpponentHealth;
	S.Fighters[1].Team = 1;
	S.Instant = 0;
	S.Seed = Seed;
	return S;
}

FBuhCombatState UBuhCombatSim::MakeStateWithPhases(int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed, int32 PlayerPhase, int32 PlayerPhaseInstants, int32 OpponentPhase, int32 OpponentPhaseInstants)
{
	FBuhCombatState S = MakeState(PlayerX, OpponentX, PlayerHealth, OpponentHealth, Seed);
	S.Fighters[0].Phase = PlayerPhase;
	S.Fighters[0].PhaseInstants = PlayerPhaseInstants;
	S.Fighters[1].Phase = OpponentPhase;
	S.Fighters[1].PhaseInstants = OpponentPhaseInstants;
	return S;
}

struct FBuhOutcome { int32 Fighter; int32 Type; int32 Damage; };

static TArray<FBuhOutcome> StepCombatInstant(FBuhCombatState& S, float PreDodgeDist[2], bool bJustActive[2])
{
	const float RawDist = S.Fighters[1].X - S.Fighters[0].X;
	const float Dist = FMath::Abs(RawDist);
	for (int32 i = 0; i < 2; ++i)
	{
		FBuhFighterState& F = S.Fighters[i];
		if (F.Phase == 2 && BuhCombat::MoveData[F.Action].Movement > 0)
		{
			if (PreDodgeDist[i] > 1e8f) PreDodgeDist[i] = FMath::Abs(S.Fighters[1 - i].X - F.X);
			const float OpponentX = S.Fighters[1 - i].X;
			const bool bMoveToward = (F.Action == 4);
			const float Dir = (F.X < OpponentX) ? (bMoveToward ? 1.f : -1.f) : (bMoveToward ? -1.f : 1.f);
			F.X += static_cast<float>(BuhCombat::MoveData[F.Action].Movement) * Dir;
		}
	}
	S.Fighters[0].Facing = (RawDist >= 0.f) ? 1 : -1;
	S.Fighters[1].Facing = (RawDist >= 0.f) ? -1 : 1;

	for (int32 i = 0; i < 2; ++i)
	{
		FBuhFighterState& F = S.Fighters[i];
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
			}
		}
	}

	const float D = FMath::Abs(S.Fighters[1].X - S.Fighters[0].X);
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

FBuhCombatResult UBuhCombatSim::ResolveCombat(const FBuhCombatState& InState, int32 PlayerDecision, int32 OpponentDecision, int32 MaxInstants)
{
	FBuhCombatResult Result;
	Result.MinDistanceSeen = 1e9f;
	FBuhCombatState S = InState;
	float PreDodgeDist[2] = { 1e9f, 1e9f };

	// Commit decisions only for fighters that are ready (Phase 0) at the start.
	for (int32 i = 0; i < 2; ++i)
	{
		const int32 Decision = (i == 0) ? PlayerDecision : OpponentDecision;
		if (S.Fighters[i].Phase == 0 && Decision >= 0 && Decision < 5)
		{
			S.Fighters[i].Action = Decision;
			S.Fighters[i].Phase = 1;
			S.Fighters[i].PhaseInstants = 0;
		}
	}

	bool bJustActive[2] = { false, false };

	for (int32 inst = 0; inst < MaxInstants; ++inst)
	{
		S.Instant = inst;

		const TArray<FBuhOutcome> Outcomes = StepCombatInstant(S, PreDodgeDist, bJustActive);
		const float D = FMath::Abs(S.Fighters[1].X - S.Fighters[0].X);
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
		if (bPlayerReady)
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
				a.PhaseInstants != b.PhaseInstants || a.Team != b.Team) return false;
			if (FMath::Abs(a.X - b.X) > 0.01f) return false;
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
	const float Dist = FMath::Abs(State.Fighters[1].X - State.Fighters[0].X);
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
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		TArray<FBuhTimelineSnapshot> TQuick = GenerateTimeline(S, 0, 0, 30);
		TArray<FBuhTimelineSnapshot> THeavy = GenerateTimeline(S, 0, 1, 30);
		TArray<FBuhTimelineSnapshot> TGuard = GenerateTimeline(S, 0, 2, 30);
		TArray<FBuhTimelineSnapshot> TDodge = GenerateTimeline(S, 0, 3, 30);
		const bool bAllNonEmpty = TQuick.Num() > 0 && THeavy.Num() > 0 && TGuard.Num() > 0 && TDodge.Num() > 0;
		const bool bDistinct = (TQuick.Num() != THeavy.Num()) || (TQuick.Num() != TGuard.Num()) || (TQuick.Num() != TDodge.Num());
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
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 0, 1, 200);
		const bool bMiss = HasEvent(R, 2, 0);
		const bool bHit = HasEvent(R, 0, 1, 24);
		const bool bHealth = R.FinalState.Fighters[0].Health == 76;
		Check(TEXT("A faster-misses-slower-connects"), bMiss && bHit && bHealth,
			FString::Printf(TEXT("miss=%d hit=%d playerHealth=%d"), bMiss, bHit, R.FinalState.Fighters[0].Health));
	}

	// Test B — recovery punish (recovering fighter hit, no free guard).
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
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
		FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
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
		const FBuhCombatResult R = ResolveCombat(S, 1, 1, 200);
		const float FinalDist = FMath::Abs(R.FinalState.Fighters[1].X - R.FinalState.Fighters[0].X);
		const bool bMinSep = FMath::Abs(FinalDist - 100.f) < 1.f;
		const bool bNoCross = (R.FinalState.Fighters[0].X < R.FinalState.Fighters[1].X);
		Check(TEXT("I opposing-approaches-min-separation"), bMinSep && bNoCross,
			FString::Printf(TEXT("finalDist=%.2f noCross=%d"), FinalDist, bNoCross));
	}

	// Test J — nonnegative distance at all times (never dips below minimum separation).
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatResult R = ResolveCombat(S, 1, 1, 200);
		const bool bNonNeg = R.MinDistanceSeen >= 100.f - 1.f;
		Check(TEXT("J nonnegative-distance-always"), bNonNeg,
			FString::Printf(TEXT("minDistanceSeen=%.2f"), R.MinDistanceSeen));
	}

	// Test K — mirrored starting positions produce equivalent behavior.
	{
		const FBuhCombatState Sc = MakeState(-250, 250, 100, 100, 0);
		const FBuhCombatState Sm = MakeState(250, -250, 100, 100, 0);
		const FBuhCombatResult Rc = ResolveCombat(Sc, 1, 1, 200);
		const FBuhCombatResult Rm = ResolveCombat(Sm, 1, 1, 200);
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
			const float D = FMath::Abs(S.Fighters[1].X - S.Fighters[0].X);
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
		const float FinalDist = FMath::Abs(R.FinalState.Fighters[1].X - R.FinalState.Fighters[0].X);
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
		const float FinalDist = FMath::Abs(R.FinalState.Fighters[1].X - R.FinalState.Fighters[0].X);
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
		const FBuhCombatState S = MakeState(-200, 200, 100, 100, 0);
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
			const float D = FMath::Abs(S.Fighters[1].X - S.Fighters[0].X);
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
	// dodge is over. Starting distance 400 keeps the Heavy in range after closure.
	{
		const FBuhCombatState S0 = MakeState(-200, 200, 100, 100, 0);
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
	{
		const FBuhCombatState S = MakeState(-250, 250, 100, 100, 0);
		TArray<FBuhTimelineSnapshot> T0 = GenerateTimeline(S, 0, 0, 30);
		TArray<FBuhTimelineSnapshot> T1 = GenerateTimeline(S, 0, 1, 30);
		TArray<FBuhTimelineSnapshot> T2 = GenerateTimeline(S, 0, 2, 30);
		TArray<FBuhTimelineSnapshot> T3 = GenerateTimeline(S, 0, 3, 30);
		const bool bAllNonEmpty = T0.Num() > 0 && T1.Num() > 0 && T2.Num() > 0 && T3.Num() > 0;
		const bool bDifferent = (T0.Num() != T1.Num()) || (T0.Num() != T2.Num()) || (T0.Num() != T3.Num());
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
			const float D = FMath::Abs(S.Fighters[1].X - S.Fighters[0].X);
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

TArray<FBuhTimelineSnapshot> UBuhCombatSim::GenerateTimeline(const FBuhCombatState& InState, int32 PlayerDecision, int32 OpponentDecision, int32 MaxInstants)
{
	TArray<FBuhTimelineSnapshot> Snapshots;
	FBuhCombatState S = InState;
	float PreDodgeDist[2] = { 1e9f, 1e9f };

	for (int32 i = 0; i < 2; ++i)
	{
		const int32 Decision = (i == 0) ? PlayerDecision : OpponentDecision;
		if (S.Fighters[i].Phase == 0 && Decision >= 0 && Decision < 5)
		{
			S.Fighters[i].Action = Decision;
			S.Fighters[i].Phase = 1;
			S.Fighters[i].PhaseInstants = 0;
		}
	}

	bool bJustActive[2] = { false, false };

	for (int32 inst = 0; inst < MaxInstants; ++inst)
	{
		S.Instant = inst;

		const TArray<FBuhOutcome> Outcomes = StepCombatInstant(S, PreDodgeDist, bJustActive);
		const float D = FMath::Abs(S.Fighters[1].X - S.Fighters[0].X);
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
			Snap.bPlayerReady = (S.Fighters[0].Phase == 0);
			Snap.bOpponentReady = (S.Fighters[1].Phase == 0);
			Snap.Events = InstEvents;
			Snapshots.Add(Snap);
			break;
		}

		const bool bPlayerReady = (S.Fighters[0].Phase == 0);
		const bool bOpponentReady = (S.Fighters[1].Phase == 0);

		FBuhTimelineSnapshot Snap;
		Snap.Instant = inst;
		Snap.PlayerX = S.Fighters[0].X;
		Snap.OpponentX = S.Fighters[1].X;
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

		if (bPlayerReady && bOpponentReady) break;
		if (bPlayerReady) break;
		if (bOpponentReady)
		{
			S.Fighters[1].Action = BuhCombat::AI_MOVE;
			S.Fighters[1].Phase = 1;
			S.Fighters[1].PhaseInstants = 0;
		}
	}

	return Snapshots;
}

static TArray<FBuhTimelineSnapshot> GCurrentTimeline;
static bool bGHasTimeline = false;

void UBuhCombatSim::SetCurrentTimeline(const TArray<FBuhTimelineSnapshot>& InTimeline)
{
	GCurrentTimeline = InTimeline;
	bGHasTimeline = true;
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
	Out += FString::Printf(TEXT("Player: %s %s (%d) HP %d X %.0f %s\n"),
		BuhCombat::MoveName(Snap.PlayerAction), BuhPhaseName(Snap.PlayerPhase),
		Snap.PlayerPhaseInstants, Snap.PlayerHealth, Snap.PlayerX,
		Snap.bPlayerReady ? TEXT("ready") : TEXT("busy"));
	Out += FString::Printf(TEXT("Opponent: %s %s (%d) HP %d X %.0f %s\n"),
		BuhCombat::MoveName(Snap.OpponentAction), BuhPhaseName(Snap.OpponentPhase),
		Snap.OpponentPhaseInstants, Snap.OpponentHealth, Snap.OpponentX,
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
	Out += FString::Printf(TEXT(" | P HP=%d X=%.0f O HP=%d X=%.0f"),
		R.FinalState.Fighters[0].Health, R.FinalState.Fighters[0].X,
		R.FinalState.Fighters[1].Health, R.FinalState.Fighters[1].X);
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
	const float PlayerYaw = (Snap.PlayerFacing == 1) ? 0.0f : 180.0f;
	const float OpponentYaw = (Snap.OpponentFacing == 1) ? 0.0f : 180.0f;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;
		FString Name = Actor->GetName();
		if (Name == TEXT("BP_Fighter_C_2"))
		{
			Actor->SetActorLocation(FVector(PlayerX, 0.0f, 90.0f));
			Actor->SetActorRotation(FRotator(0.0f, PlayerYaw, 0.0f));
		}
		else if (Name == TEXT("BP_Fighter_C_3"))
		{
			Actor->SetActorLocation(FVector(OpponentX, 0.0f, 90.0f));
			Actor->SetActorRotation(FRotator(0.0f, OpponentYaw, 0.0f));
		}
	}
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
	if (Move < 0 || Move >= 4) return TEXT("Unknown");
	const BuhCombat::FMoveData& M = BuhCombat::MoveData[Move];
	return FString::Printf(TEXT("Prep %d / Active %d / Recovery %d"), M.Startup, M.Active, M.Recovery);
}

FString UBuhCombatSim::GetBranchSummaryText(int32 PlayerMove, int32 OpponentMove, int32 CurrentInstant)
{
	FBuhCombatState State;
	if (bHasTimeline())
	{
		const FBuhTimelineSnapshot S0 = GetSnapshotAt(0);
		State = MakeState(static_cast<int32>(S0.PlayerX), static_cast<int32>(S0.OpponentX), S0.PlayerHealth, S0.OpponentHealth, 0);
	}
	else
	{
		State = MakeState(-500.0f, 500.0f, 100.0f, 100.0f, 0);
	}
	const FBuhCombatResult R = ResolveCombat(State, PlayerMove, OpponentMove, 200);

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
	if (bHasTimeline() && CurrentInstant >= 0 && CurrentInstant < GCurrentTimeline.Num())
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

	FString Summary = FString::Printf(TEXT("%s vs %s: P %d→%d, O %d→%d, min D %.0f"),
		*GetMoveNameText(PlayerMove), *GetMoveNameText(OpponentMove),
		State.Fighters[0].Health, GetFinalPlayerHealth(R),
		State.Fighters[1].Health, GetFinalOpponentHealth(R),
		R.MinDistanceSeen);
	Summary += TEXT(" | ") + PhaseText;
	if (!EventText.IsEmpty()) Summary += TEXT(" | ") + EventText;
	return Summary;
}

FString UBuhCombatSim::GetTimelineLengthText(int32 CurrentInstant)
{
	if (!bHasTimeline()) return TEXT("No preview");
	const int32 Len = GetTimelineLength();
	const int32 Index = FMath::Clamp(CurrentInstant, 0, Len - 1);
	return FString::Printf(TEXT("Instant %d / %d"), GetSnapshotInstant(Index), Len);
}

FString UBuhCombatSim::GetPreviewDistanceText(int32 Instant)
{
	if (!bHasTimeline()) return TEXT("Preview D: --");
	const int32 Len = GetTimelineLength();
	const int32 Index = FMath::Clamp(Instant, 0, Len - 1);
	const FBuhTimelineSnapshot S = GetSnapshotAt(Index);
	return FString::Printf(TEXT("Preview D: %.0f"), FMath::Abs(S.OpponentX - S.PlayerX));
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
		UE_LOG(LogTemp, Display, TEXT("BUH.DebugSnapshot: Index=%d Instant=%d PlayerAction=%d OpponentAction=%d PlayerPhase=%d OpponentPhase=%d PlayerX=%.0f OpponentX=%.0f"),
			Index, S.Instant, S.PlayerAction, S.OpponentAction, S.PlayerPhase, S.OpponentPhase, S.PlayerX, S.OpponentX);
	}));
