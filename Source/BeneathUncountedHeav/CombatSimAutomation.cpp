#include "CombatSim.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhCombatSimTest, "BUH.CombatSimTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhCombatSimTest::RunTest(const FString& Parameters)
{
	const FString Report = UBuhCombatSim::RunAllTests();
	UE_LOG(LogTemp, Display, TEXT("=== BUH.CombatSimTests report ==="));
	UE_LOG(LogTemp, Display, TEXT("%s"), *Report);
	UE_LOG(LogTemp, Display, TEXT("=== end report ==="));

	const FString FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CombatTestReport.txt"));
	FFileHelper::SaveStringToFile(Report, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8);

	const bool bAllPassed = Report.Contains(TEXT("0 failed"));
	if (!bAllPassed)
	{
		AddError(TEXT("Combat simulation tests failed. See CombatTestReport.txt for details."));
	}
	return bAllPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhCombatFlowTest, "BUH.CombatFlowTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhCombatFlowTest::RunTest(const FString& Parameters)
{
	const int32 Seed = 12345;

	// Step A — initial state: two fighters at ±500, 100 HP, no move selected (sentinel -1).
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		TestTrue(TEXT("A: two fighters"), S.Fighters.Num() == 2);
		TestEqual(TEXT("A: player X"), S.Fighters[0].X, -500.f);
		TestEqual(TEXT("A: opponent X"), S.Fighters[1].X, 500.f);
		TestEqual(TEXT("A: player health"), S.Fighters[0].Health, 100);
		TestEqual(TEXT("A: opponent health"), S.Fighters[1].Health, 100);
		TestEqual(TEXT("A: player action sentinel"), S.Fighters[0].Action, -1);
		TestEqual(TEXT("A: player phase idle"), S.Fighters[0].Phase, 0);
	}

	// Step C — select Quick Strike (0) vs opponent Heavy Strike (1), then execute.
	// At distance 400 the Quick (range 300) misses and the Heavy (range 400) connects for 24.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-200, 200, 100, 100, Seed);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, -200, 200, 100, 100, Seed, /*PlayerMove=*/0, /*OpponentMove=*/1, /*WalkTarget=*/0.f);
		TestEqual(TEXT("C: player took Heavy damage"), R.FinalState.Fighters[0].Health, 76);
		TestEqual(TEXT("C: opponent unharmed"), R.FinalState.Fighters[1].Health, 100);
		TestTrue(TEXT("C: fight stopped (player or opponent ready)"), R.bPlayerReady || R.bOpponentReady);
		TestTrue(TEXT("C: valid stop reason"), R.StoppedReason >= 1 && R.StoppedReason <= 3);
	}

	// Step E — select Walk destination (6) toward -300, then execute. The opponent is busy
	// Guarding so the variable-length walk runs to completion and the fighter reaches the target.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, -500, 500, 100, 100, Seed, /*PlayerMove=*/6, /*OpponentMove=*/2, /*WalkTarget=*/-300.f);
		const float FinalX = R.FinalState.Fighters[0].X;
		TestTrue(TEXT("E: player walked toward target"), FinalX > -500.f);
		TestTrue(TEXT("E: player reached walk target"), FMath::Abs(FinalX - (-300.f)) < 1.f);
		TestTrue(TEXT("E: player ready after walk"), R.bPlayerReady);
	}

	// Step G — reaction opportunity: the player is Walking (Action 4, active) and the opponent
	// begins a Heavy Strike. The simulation pauses with a reaction opportunity for the player.
	{
		FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		const FBuhCombatResult R = UBuhCombatSim::ResolveCombat(S, /*PlayerMove=*/-1, /*OpponentMove=*/1, 200);
		TestEqual(TEXT("G: stopped on reaction"), R.StoppedReason, 5);
		TestTrue(TEXT("G: reaction opportunity"), R.bReactionOpportunity);
		TestEqual(TEXT("G: reacting fighter is player"), R.ReactionFighter, 0);
		TestEqual(TEXT("G: observed threat is Heavy"), R.ObservedThreatAction, 1);
	}

	// Step H — reset: a fresh MakeState restores the initial positions, health, and sentinel.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		TestEqual(TEXT("H: player X restored"), S.Fighters[0].X, -500.f);
		TestEqual(TEXT("H: opponent X restored"), S.Fighters[1].X, 500.f);
		TestEqual(TEXT("H: player health restored"), S.Fighters[0].Health, 100);
		TestEqual(TEXT("H: opponent health restored"), S.Fighters[1].Health, 100);
		TestEqual(TEXT("H: player action sentinel"), S.Fighters[0].Action, -1);
	}

	// Step I — idle player + opponent Dodge. With bSuppressPlayerReadyBreak the timeline
	// generates the full Dodge (3 startup + 2 active + 3 recovery = 9 instants) instead of
	// stopping at instant 0. The opponent moves 60 units away (30 per active instant), from
	// X=500 to X=560. Without the flag the idle player is always ready, so the timeline
	// degenerates to a single instant-0 snapshot (the old bug).
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		const TArray<FBuhTimelineSnapshot> Full = UBuhCombatSim::GenerateTimeline(
			S, /*PlayerMove=*/-1, /*OpponentMove=*/3, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/true);
		TestEqual(TEXT("I: full Dodge timeline has 8 instants"), Full.Num(), 8);
		TestEqual(TEXT("I: opponent dodged 60 units away"), Full.Last().OpponentX, 560.f);
		TestEqual(TEXT("I: player stayed idle at -500"), Full.Last().PlayerX, -500.f);

		const TArray<FBuhTimelineSnapshot> Degenerate = UBuhCombatSim::GenerateTimeline(
			S, /*PlayerMove=*/-1, /*OpponentMove=*/3, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/false);
		TestEqual(TEXT("I: without suppress the timeline stops at instant 0"), Degenerate.Num(), 1);
	}

	// Step J — Walk (6) vs hypothetical Dodge (3), the exact Walk-transition regression.
	// With bSuppressPlayerReadyBreak=false the player is ready the instant Walk commits, so the
	// timeline breaks at instant 0 (the reported "0/0" + ghost-not-separated symptom). With the
	// flag true the timeline runs to the end and the Dodge ghost is displaced.
	{
		FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		S = UBuhCombatSim::WithPlayerWalkTarget(S, -400.f);
		const TArray<FBuhTimelineSnapshot> WalkDodgeSuppress = UBuhCombatSim::GenerateTimeline(
			S, /*PlayerMove=*/6, /*OpponentMove=*/3, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/true);
		TestTrue(TEXT("J: Walk-vs-Dodge with suppress generates a multi-instant timeline"), WalkDodgeSuppress.Num() > 1);
		TestTrue(TEXT("J: Walk-vs-Dodge with suppress displaces the Dodge ghost"), WalkDodgeSuppress.Last().OpponentX > 500.f);

		const TArray<FBuhTimelineSnapshot> WalkDodgeNoSuppress = UBuhCombatSim::GenerateTimeline(
			S, /*PlayerMove=*/6, /*OpponentMove=*/3, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/false);
		TestTrue(TEXT("J: Walk-vs-Dodge without suppress still generates a multi-instant timeline"), WalkDodgeNoSuppress.Num() > 1);

		// Step K — documents WHY the widget must call WithPlayerWalkTarget: a plain MakeState
		// with NO walk target degenerates (Walk commits instantly, timeline breaks at instant 0,
		// ghosts never move). RegeneratePredictionPreview now applies WithPlayerWalkTarget for
		// move 6, so this degenerate path is no longer what the widget exercises.
		{
			const FBuhCombatState Plain = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
			const TArray<FBuhTimelineSnapshot> PlainTimeline = UBuhCombatSim::GenerateTimeline(
				Plain, /*PlayerMove=*/6, /*OpponentMove=*/3, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/false);
			UE_LOG(LogTemp, Display, TEXT("K: plain MakeState Walk-vs-Dodge timeline Num=%d lastOppX=%.1f lastPlayerX=%.1f"),
				PlainTimeline.Num(), PlainTimeline.Last().OpponentX, PlainTimeline.Last().PlayerX);
			TestEqual(TEXT("K: plain-state Walk-vs-Dodge degenerates to 2 instants (no walk target)"), PlainTimeline.Num(), 2);
			TestEqual(TEXT("K: plain-state opponent ghost stays put"), FMath::IsNearlyEqual(PlainTimeline.Last().OpponentX, 500.f), true);
			TestEqual(TEXT("K: plain-state player ghost stays put"), FMath::IsNearlyEqual(PlainTimeline.Last().PlayerX, -500.f), true);
		}
	}

	// Step L — Walk (6) vs hypothetical Quick (0), Heavy (1), and Guard (2), the exact
	// Walk-reaction audit. Same initial state, same walk target toward the opponent.
	// Verify four prediction scenarios with the same starting positions and Walk target.
	// Quick and Heavy begin at instant 0, so the reaction pause triggers immediately.
	// Guard is not an attack, so the player walks until the decision boundary.
	// Dodge is not an attack, so no reaction pause; the preview ends at the decision boundary.
	{
		FBuhCombatState Base = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		Base = UBuhCombatSim::WithPlayerWalkTarget(Base, 500.f);

		const TArray<FBuhTimelineSnapshot> TQuick = UBuhCombatSim::GenerateTimeline(
			Base, /*PlayerMove=*/6, /*OpponentMove=*/0, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/true, /*bStopAtDecisionBoundary=*/true);
		UE_LOG(LogTemp, Display, TEXT("L: Walk-vs-Quick Num=%d stop=%d playerX=%.1f oppX=%.1f reaction=%d threat=%d"),
			TQuick.Num(), TQuick.Last().StoppedReason, TQuick.Last().PlayerX, TQuick.Last().OpponentX, (int32)TQuick.Last().bReactionOpportunity, TQuick.Last().ObservedThreatAction);
		TestEqual(TEXT("L: Walk-vs-Quick reaction pause at instant 0"), TQuick.Num(), 1);
		TestEqual(TEXT("L: Walk-vs-Quick stop reason is reaction"), TQuick.Last().StoppedReason, 5);
		TestEqual(TEXT("L: Walk-vs-Quick player ghost stays at start"), TQuick.Last().PlayerX, -500.f);
		TestTrue(TEXT("L: Walk-vs-Quick reaction opportunity"), TQuick.Last().bReactionOpportunity);
		TestEqual(TEXT("L: Walk-vs-Quick observed threat is Quick"), TQuick.Last().ObservedThreatAction, 0);

		const TArray<FBuhTimelineSnapshot> THeavy = UBuhCombatSim::GenerateTimeline(
			Base, /*PlayerMove=*/6, /*OpponentMove=*/1, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/true, /*bStopAtDecisionBoundary=*/true);
		UE_LOG(LogTemp, Display, TEXT("L: Walk-vs-Heavy Num=%d stop=%d playerX=%.1f oppX=%.1f reaction=%d threat=%d"),
			THeavy.Num(), THeavy.Last().StoppedReason, THeavy.Last().PlayerX, THeavy.Last().OpponentX, (int32)THeavy.Last().bReactionOpportunity, THeavy.Last().ObservedThreatAction);
		TestEqual(TEXT("L: Walk-vs-Heavy reaction pause at instant 0"), THeavy.Num(), 1);
		TestEqual(TEXT("L: Walk-vs-Heavy stop reason is reaction"), THeavy.Last().StoppedReason, 5);
		TestEqual(TEXT("L: Walk-vs-Heavy player ghost stays at start"), THeavy.Last().PlayerX, -500.f);
		TestTrue(TEXT("L: Walk-vs-Heavy reaction opportunity"), THeavy.Last().bReactionOpportunity);
		TestEqual(TEXT("L: Walk-vs-Heavy observed threat is Heavy"), THeavy.Last().ObservedThreatAction, 1);

		const TArray<FBuhTimelineSnapshot> TGuard = UBuhCombatSim::GenerateTimeline(
			Base, /*PlayerMove=*/6, /*OpponentMove=*/2, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/true, /*bStopAtDecisionBoundary=*/true);
		UE_LOG(LogTemp, Display, TEXT("L: Walk-vs-Guard Num=%d stop=%d playerX=%.1f oppX=%.1f reaction=%d"),
			TGuard.Num(), TGuard.Last().StoppedReason, TGuard.Last().PlayerX, TGuard.Last().OpponentX, (int32)TGuard.Last().bReactionOpportunity);
		TestTrue(TEXT("L: Walk-vs-Guard player ghost moves"), TGuard.Last().PlayerX > -500.f);
		TestEqual(TEXT("L: Walk-vs-Guard ends at decision boundary (12 instants)"), TGuard.Num(), 12);
		TestEqual(TEXT("L: Walk-vs-Guard stop reason is decision boundary"), TGuard.Last().StoppedReason, 2);
		TestFalse(TEXT("L: Walk-vs-Guard no reaction"), TGuard.Last().bReactionOpportunity);

		const TArray<FBuhTimelineSnapshot> TDodge = UBuhCombatSim::GenerateTimeline(
			Base, /*PlayerMove=*/6, /*OpponentMove=*/3, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/true, /*bStopAtDecisionBoundary=*/true);
		UE_LOG(LogTemp, Display, TEXT("L: Walk-vs-Dodge Num=%d stop=%d playerX=%.1f oppX=%.1f reaction=%d"),
			TDodge.Num(), TDodge.Last().StoppedReason, TDodge.Last().PlayerX, TDodge.Last().OpponentX, (int32)TDodge.Last().bReactionOpportunity);
		TestTrue(TEXT("L: Walk-vs-Dodge player ghost moves"), TDodge.Last().PlayerX > -500.f);
		TestFalse(TEXT("L: Walk-vs-Dodge no reaction (Dodge is not an attack)"), TDodge.Last().bReactionOpportunity);
		TestEqual(TEXT("L: Walk-vs-Dodge stop reason is decision boundary"), TDodge.Last().StoppedReason, 2);
	}

	// Verify Continue Walk (decision 7) preserves the existing Walk state during a reaction pause.
	{
		FBuhCombatState Base = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		Base = UBuhCombatSim::WithPlayerWalkTarget(Base, 500.f);

		// First, simulate Walk vs Quick to get the reaction pause state.
		FBuhCombatResult R1 = UBuhCombatSim::ResolveCombat(Base, /*PlayerDecision=*/6, /*OpponentDecision=*/0, /*MaxInstants=*/200);
		TestTrue(TEXT("M: Walk-vs-Quick reaction opportunity"), R1.bReactionOpportunity);
		TestEqual(TEXT("M: Walk-vs-Quick reaction instant is 0"), R1.ReactionInstant, 0);

		// Now, from the reaction pause state, choose Continue Walk (decision 7).
		// The player's Walk action should be preserved (Action=6), and the simulation
		// continues with the existing Walk state (no restart, no new action).
		FBuhCombatResult R2 = UBuhCombatSim::ResolveCombat(R1.FinalState, /*PlayerDecision=*/7, /*OpponentDecision=*/-1, /*MaxInstants=*/200);
		UE_LOG(LogTemp, Display, TEXT("M: Continue-Walk playerAction=%d playerPhase=%d playerX=%.1f stop=%d"),
			R2.FinalState.Fighters[0].Action, R2.FinalState.Fighters[0].Phase, R2.FinalState.Fighters[0].X, R2.StoppedReason);
		TestEqual(TEXT("M: Continue Walk preserves player Walk action"), R2.FinalState.Fighters[0].Action, 6);
		TestTrue(TEXT("M: Continue Walk player walks (X > -500)"), R2.FinalState.Fighters[0].X > -500.f);
	}

	// Verify one uninterrupted encounter: Walk, opponent Quick, reaction pause, Continue Walk, walk resumes.
	{
		FBuhCombatState Base = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		Base = UBuhCombatSim::WithPlayerWalkTarget(Base, 500.f);

		// Step 1: Walk vs Quick, reaction pause at instant 0.
		FBuhCombatResult R1 = UBuhCombatSim::ResolveCombat(Base, /*PlayerDecision=*/6, /*OpponentDecision=*/0, /*MaxInstants=*/200);
		TestTrue(TEXT("N: Step 1 reaction opportunity"), R1.bReactionOpportunity);

		// Step 2: Continue Walk from the reaction pause state.
		FBuhCombatResult R2 = UBuhCombatSim::ResolveCombat(R1.FinalState, /*PlayerDecision=*/7, /*OpponentDecision=*/-1, /*MaxInstants=*/200);
		UE_LOG(LogTemp, Display, TEXT("N: Step 2 playerX=%.1f playerPhase=%d stop=%d"),
			R2.FinalState.Fighters[0].X, R2.FinalState.Fighters[0].Phase, R2.StoppedReason);
		TestTrue(TEXT("N: Step 2 player walks (X > -500)"), R2.FinalState.Fighters[0].X > -500.f);
		TestEqual(TEXT("N: Step 2 player still walking (Action=6)"), R2.FinalState.Fighters[0].Action, 6);

		// Step 3: Reset the encounter.
		FBuhCombatState Reset = UBuhCombatSim::MakeState(-500, 500, 100, 100, Seed);
		TestEqual(TEXT("N: Step 3 reset player X"), Reset.Fighters[0].X, -500.f);
		TestEqual(TEXT("N: Step 3 reset opponent X"), Reset.Fighters[1].X, 500.f);
	}

	// === 2D Movement Tests (Stage C) ===

	// O: Walk in positive Y. Player at (0,0), opponent at (1000,0). Walk to (0,200).
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 0.f, 0.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 1000, 100, 100, Seed, /*PlayerMove=*/6, /*OpponentMove=*/2, /*WalkTarget=*/0.f, /*WalkTargetY=*/200.f);
		TestTrue(TEXT("O: player walked in +Y"), R.FinalState.Fighters[0].Y > 0.f);
		TestTrue(TEXT("O: player reached +Y target"), FMath::Abs(R.FinalState.Fighters[0].Y - 200.f) < 1.f);
		TestEqual(TEXT("O: player X unchanged"), R.FinalState.Fighters[0].X, 0.f);
	}

	// P: Walk in negative Y. Player at (0,200), opponent at (1000,0). Walk to (0,0).
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 200.f, 0.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 1000, 100, 100, Seed, /*PlayerMove=*/6, /*OpponentMove=*/2, /*WalkTarget=*/0.f, /*WalkTargetY=*/0.f);
		TestTrue(TEXT("P: player walked in -Y"), R.FinalState.Fighters[0].Y < 200.f);
		TestTrue(TEXT("P: player reached -Y target"), FMath::Abs(R.FinalState.Fighters[0].Y) < 1.f);
	}

	// Q: Walk diagonally. Player at (0,0), opponent at (1000,1000). Walk to (200,200).
	// Distance = sqrt(200^2+200^2) = 283 units. At 30 units/instant, takes ~10 instants.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 0.f, 0.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 1000, 100, 100, Seed, /*PlayerMove=*/6, /*OpponentMove=*/2, /*WalkTarget=*/200.f, /*WalkTargetY=*/200.f);
		TestTrue(TEXT("Q: player walked in +X"), R.FinalState.Fighters[0].X > 0.f);
		TestTrue(TEXT("Q: player walked in +Y"), R.FinalState.Fighters[0].Y > 0.f);
		TestTrue(TEXT("Q: player reached diagonal X target"), FMath::Abs(R.FinalState.Fighters[0].X - 200.f) < 1.f);
		TestTrue(TEXT("Q: player reached diagonal Y target"), FMath::Abs(R.FinalState.Fighters[0].Y - 200.f) < 1.f);
	}

	// R: Walk a distance shorter than one instant. Player at (0,0), target (10,0).
	// Distance 10 < 30 (one instant of walking), so the walk completes in one instant.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 0.f, 0.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 1000, 100, 100, Seed, /*PlayerMove=*/6, /*OpponentMove=*/2, /*WalkTarget=*/10.f);
		TestTrue(TEXT("R: short walk completed"), R.bPlayerReady);
		TestTrue(TEXT("R: player reached short target"), FMath::Abs(R.FinalState.Fighters[0].X - 10.f) < 1.f);
	}

	// S: Walk away from opponent with Y separation. Player at (0,0), opponent at (300,0).
	// Walk Away (5) should move the player away from the opponent in the ground plane.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 300, 100, 100, Seed, 0.f, 0.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 300, 100, 100, Seed, /*PlayerMove=*/5, /*OpponentMove=*/2, /*WalkTarget=*/0.f);
		TestTrue(TEXT("S: walk away moved player in -X"), R.FinalState.Fighters[0].X < 0.f);
	}

	// === 2D Combat Range Tests ===

	// T: Quick Strike misses at distance 500 with Y separation.
	// Player at (0,0), opponent at (300,400). Distance = 500. Quick range = 300. Should miss.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 300, 100, 100, Seed, 0.f, 400.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 300, 100, 100, Seed, /*PlayerMove=*/0, /*OpponentMove=*/2, /*WalkTarget=*/0.f, /*WalkTargetY=*/0.f, /*PlayerY=*/0.f, /*OpponentY=*/400.f);
		TestEqual(TEXT("T: Quick misses at 2D distance 500"), R.FinalState.Fighters[1].Health, 100);
	}

	// U: Heavy Strike misses at distance 500 with Y separation.
	// Player at (0,0), opponent at (300,400). Distance = 500. Heavy range = 400. Should miss.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 300, 100, 100, Seed, 0.f, 400.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 300, 100, 100, Seed, /*PlayerMove=*/1, /*OpponentMove=*/2, /*WalkTarget=*/0.f, /*WalkTargetY=*/0.f, /*PlayerY=*/0.f, /*OpponentY=*/400.f);
		TestEqual(TEXT("U: Heavy misses at 2D distance 500"), R.FinalState.Fighters[1].Health, 100);
	}

	// V: Quick Strike connects at 2D distance 200 with Y separation.
	// Player at (0,0), opponent at (0,200). Distance = 200. Quick range = 300. Should connect.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 0, 100, 100, Seed, 0.f, 200.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 0, 100, 100, Seed, /*PlayerMove=*/0, /*OpponentMove=*/-1, /*WalkTarget=*/0.f, /*WalkTargetY=*/0.f, /*PlayerY=*/0.f, /*OpponentY=*/200.f);
		TestEqual(TEXT("V: Quick connects at 2D distance 200 (Y-sep)"), R.FinalState.Fighters[1].Health, 90);
	}
	// V2: Quick Strike connects at X distance 200 (control test).
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 200, 100, 100, Seed, 0.f, 0.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 200, 100, 100, Seed, /*PlayerMove=*/0, /*OpponentMove=*/-1, /*WalkTarget=*/0.f, /*WalkTargetY=*/0.f, /*PlayerY=*/0.f, /*OpponentY=*/0.f);
		TestEqual(TEXT("V2: Quick connects at X distance 200"), R.FinalState.Fighters[1].Health, 90);
	}

	// W: Dodge displaces in the ground plane. Player at (0,0), opponent at (300,0).
	// Dodge (3) moves the player 60 units away from the opponent.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(0, 300, 100, 100, Seed, 0.f, 0.f);
		const FBuhCombatResult R = UBuhCombatSim::ExecuteMoveWithVars(
			false, S, 0, 300, 100, 100, Seed, /*PlayerMove=*/3, /*OpponentMove=*/2, /*WalkTarget=*/0.f);
		TestTrue(TEXT("W: dodge displaced player in -X"), R.FinalState.Fighters[0].X < 0.f);
		TestTrue(TEXT("W: dodge moved 60 units"), FMath::Abs(R.FinalState.Fighters[0].X) > 50.f);
	}

	// === 2D Reaction Tests ===

	// X: Continue resumes a partially completed diagonal walk.
	// Player at (0,0), walks to (200,200). Opponent Quick causes reaction pause at instant 0.
	// Continue Walk should resume the diagonal walk from the paused position.
	{
		FBuhCombatState Base = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 0.f, 0.f);
		Base = UBuhCombatSim::WithPlayerWalkTarget(Base, 200.f, 200.f);
		FBuhCombatResult R1 = UBuhCombatSim::ResolveCombat(Base, /*PlayerDecision=*/6, /*OpponentDecision=*/0, /*MaxInstants=*/200);
		TestTrue(TEXT("X: diagonal walk reaction opportunity"), R1.bReactionOpportunity);
		FBuhCombatResult R2 = UBuhCombatSim::ResolveCombat(R1.FinalState, /*PlayerDecision=*/7, /*OpponentDecision=*/-1, /*MaxInstants=*/200);
		TestTrue(TEXT("X: continue diagonal walk moved in +X"), R2.FinalState.Fighters[0].X > 0.f);
		TestTrue(TEXT("X: continue diagonal walk moved in +Y"), R2.FinalState.Fighters[0].Y > 0.f);
	}

	// Y: Original XY target is preserved after reaction pause.
	{
		FBuhCombatState Base = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 0.f, 0.f);
		Base = UBuhCombatSim::WithPlayerWalkTarget(Base, 300.f, 400.f);
		FBuhCombatResult R1 = UBuhCombatSim::ResolveCombat(Base, /*PlayerDecision=*/6, /*OpponentDecision=*/0, /*MaxInstants=*/200);
		TestTrue(TEXT("Y: reaction opportunity with diagonal target"), R1.bReactionOpportunity);
		TestTrue(TEXT("Y: walk target X preserved"), FMath::Abs(R1.FinalState.Fighters[0].WalkTarget - 300.f) < 1.f);
		TestTrue(TEXT("Y: walk target Y preserved"), FMath::Abs(R1.FinalState.Fighters[0].WalkTargetY - 400.f) < 1.f);
	}

	// Z: Opponent's committed attack continues during reaction pause.
	{
		FBuhCombatState Base = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 0.f, 0.f);
		Base = UBuhCombatSim::WithPlayerWalkTarget(Base, 500.f, 0.f);
		FBuhCombatResult R1 = UBuhCombatSim::ResolveCombat(Base, /*PlayerDecision=*/6, /*OpponentDecision=*/1, /*MaxInstants=*/200);
		TestTrue(TEXT("Z: reaction opportunity vs Heavy"), R1.bReactionOpportunity);
		TestEqual(TEXT("Z: opponent action is Heavy"), R1.FinalState.Fighters[1].Action, 1);
		TestTrue(TEXT("Z: opponent phase progressed"), R1.FinalState.Fighters[1].Phase > 0);
	}

	// === 2D Prediction/Ghost Tests ===

	// AA: Timeline snapshots have correct Y values for diagonal walk.
	{
		FBuhCombatState Base = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 0.f, 0.f);
		Base = UBuhCombatSim::WithPlayerWalkTarget(Base, 300.f, 300.f);
		const TArray<FBuhTimelineSnapshot> T = UBuhCombatSim::GenerateTimeline(
			Base, /*PlayerMove=*/6, /*OpponentMove=*/2, /*MaxInstants=*/30, /*bSuppressPlayerReadyBreak=*/true);
		TestTrue(TEXT("AA: diagonal walk timeline has multiple instants"), T.Num() > 1);
		TestTrue(TEXT("AA: final snapshot player Y > 0"), T.Last().PlayerY > 0.f);
		TestTrue(TEXT("AA: final snapshot player Y approaches target"), T.Last().PlayerY > 100.f);
		TestTrue(TEXT("AA: final snapshot player X > 0"), T.Last().PlayerX > 0.f);
	}

	// BB: Ghost positions (PlayerY/OpponentY) in snapshots match expected values.
	{
		FBuhCombatState Base = UBuhCombatSim::MakeState(0, 500, 100, 100, Seed, 0.f, 100.f);
		const TArray<FBuhTimelineSnapshot> T = UBuhCombatSim::GenerateTimeline(
			Base, /*PlayerMove=*/-1, /*OpponentMove=*/-1, /*MaxInstants=*/5, /*bSuppressPlayerReadyBreak=*/true);
		TestTrue(TEXT("BB: snapshot has opponent Y"), T[0].OpponentY > 0.f);
		TestTrue(TEXT("BB: opponent Y preserved in snapshot"), FMath::Abs(T[0].OpponentY - 100.f) < 1.f);
	}

	// CC: Changing destination does not mutate authoritative state.
	{
		FBuhCombatState S = UBuhCombatSim::MakeState(0, 1000, 100, 100, Seed, 0.f, 0.f);
		const float OrigX = S.Fighters[0].X;
		const float OrigY = S.Fighters[0].Y;
		S = UBuhCombatSim::WithPlayerWalkTarget(S, 500.f, 300.f);
		TestEqual(TEXT("CC: walk target does not mutate player X"), S.Fighters[0].X, OrigX);
		TestEqual(TEXT("CC: walk target does not mutate player Y"), S.Fighters[0].Y, OrigY);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhWalkPreviewTest, "BUH.WalkPreviewTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhWalkPreviewTest::RunTest(const FString& Parameters)
{
	// GetBranchSummaryText must apply the preview walk target for move 6 (Walk).
	// With a target set, the player walks toward it and the summary differs from the
	// no-target case (where the walk degenerates to standing still).
	// Clear any stale timeline so the summary uses the default state, not a snapshot.
	UBuhCombatSim::SetCurrentTimeline(TArray<FBuhTimelineSnapshot>());

	UBuhCombatSim::SetPreviewWalkTarget(-9999.f);
	const FString NoTarget = UBuhCombatSim::GetBranchSummaryText(/*PlayerMove=*/6, /*OpponentMove=*/1, /*CurrentInstant=*/-1);

	UBuhCombatSim::SetPreviewWalkTarget(-100.f);
	const FString WithTarget = UBuhCombatSim::GetBranchSummaryText(/*PlayerMove=*/6, /*OpponentMove=*/1, /*CurrentInstant=*/-1);

	TestFalse(TEXT("Walk preview: target changes the summary"), NoTarget == WithTarget);
	TestTrue(TEXT("Walk preview: with-target summary reports the walk"), WithTarget.Contains(TEXT("Walk")));

	// A non-walk move must ignore the stored target (no behavior change for Quick/Heavy/Guard/Dodge).
	UBuhCombatSim::SetPreviewWalkTarget(-100.f);
	const FString QuickWithTarget = UBuhCombatSim::GetBranchSummaryText(/*PlayerMove=*/0, /*OpponentMove=*/1, /*CurrentInstant=*/-1);
	UBuhCombatSim::SetPreviewWalkTarget(-9999.f);
	const FString QuickNoTarget = UBuhCombatSim::GetBranchSummaryText(/*PlayerMove=*/0, /*OpponentMove=*/1, /*CurrentInstant=*/-1);
	TestEqual(TEXT("Walk preview: non-walk move ignores stored target"), QuickWithTarget, QuickNoTarget);

	// Restore the sentinel so later tests start clean.
	UBuhCombatSim::SetPreviewWalkTarget(-9999.f);
	return true;
}
