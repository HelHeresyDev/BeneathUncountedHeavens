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
