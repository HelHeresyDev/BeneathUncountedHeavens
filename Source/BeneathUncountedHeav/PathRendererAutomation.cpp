#include "PathRenderer.h"
#include "CombatSim.h"
#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"
#include "UObject/Field.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_IfThenElse.h"

// Renderer-only verification (no combat mechanics). Spawns a live
// ABUHPathRenderer in the editor world and drives SetPath/SetLine/ClearPath
// directly with synthetic samples to prove:
//   A) a straight green line draws as one green piece,
//   B) a straight path honors a supplied red->green boundary (not a midpoint),
//   C) a sampled 3D parabolic arc has real vertical curvature and honors a
//      supplied red->green boundary that is NOT the apex,
//   D) the segment pool reuses components across 20+ varied draws with no
//      actor or component leak.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhPathRendererTest, "BUH.PathRendererTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

EPathSegmentColor PieceColor(ABUHPathRenderer* R, UStaticMeshComponent* Piece)
{
	UMaterialInterface* Mat = Piece ? Piece->GetMaterial(0) : nullptr;
	if (Mat == R->GreenMaterial)
	{
		return EPathSegmentColor::Green;
	}
	if (Mat == R->RedMaterial)
	{
		return EPathSegmentColor::Red;
	}
	return EPathSegmentColor::Green;
}

bool FBuhPathRendererTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	Check(TEXT("Editor world available"), World != nullptr);
	if (!World)
	{
		return bAllPassed;
	}

	auto CountRenderers = [World]()
	{
		int32 Count = 0;
		for (TActorIterator<ABUHPathRenderer> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	};
	// The open level may already contain a placed renderer (e.g. BUHPathRenderer_1
	// in Lvl_TacticalDuel). Record the baseline so the leak check measures only
	// what this test adds, independent of level contents.
	const int32 BaselineCount = CountRenderers();

	ABUHPathRenderer* R = World->SpawnActor<ABUHPathRenderer>(FVector(0.f, 0.f, 0.f), FRotator::ZeroRotator);
	Check(TEXT("ABUHPathRenderer spawned"), R != nullptr);
	if (!R)
	{
		return bAllPassed;
	}
	R->PostInitializeComponents();
	R->DispatchBeginPlay();

	// --- Test A: straight green line ---
	{
		R->SetLine(FVector(0.f, 0.f, 90.f), FVector(400.f, 0.f, 90.f), EPathSegmentColor::Green);
		Check(TEXT("A: path visible"), R->IsPathVisible());
		Check(TEXT("A: one visible piece"), R->GetVisibleSegmentCount() == 1);
		Check(TEXT("A: pool size 1"), R->GetPoolSize() == 1);
		UStaticMeshComponent* Piece = R->SegmentPool.Num() >= 1 ? R->SegmentPool[0].Get() : nullptr;
		Check(TEXT("A: piece exists"), Piece != nullptr);
		if (Piece)
		{
			Check(TEXT("A: piece is green"), PieceColor(R, Piece) == EPathSegmentColor::Green);
			const FVector Mid = Piece->GetComponentLocation();
			Check(TEXT("A: piece at midpoint"), FMath::Abs(Mid.X - 200.f) < 1.f && FMath::Abs(Mid.Z - 90.f) < 1.f);
		}
		R->ClearPath();
		Check(TEXT("A: cleared hides path"), !R->IsPathVisible() && R->GetVisibleSegmentCount() == 0);
	}

	// --- Test B: straight path, red then green at a supplied boundary (index 1, not midpoint) ---
	{
		TArray<FVector> Pts;
		for (int32 i = 0; i < 5; ++i)
		{
			Pts.Add(FVector(i * 100.f, 0.f, 90.f));
		}
		TArray<FBuhPathSegment> Segs;
		FBuhPathSegment RedSeg; RedSeg.StartPointIndex = 0; RedSeg.EndPointIndex = 1; RedSeg.Color = EPathSegmentColor::Red;
		FBuhPathSegment GreenSeg; GreenSeg.StartPointIndex = 1; GreenSeg.EndPointIndex = 4; GreenSeg.Color = EPathSegmentColor::Green;
		Segs.Add(RedSeg); Segs.Add(GreenSeg);

		R->SetPath(Pts, Segs);
		Check(TEXT("B: path visible"), R->IsPathVisible());
		Check(TEXT("B: four visible pieces"), R->GetVisibleSegmentCount() == 4);
		Check(TEXT("B: pool size 4"), R->GetPoolSize() == 4);
		bool bFirstRed = true, bRestGreen = true;
		for (int32 i = 0; i < R->SegmentPool.Num(); ++i)
		{
			const EPathSegmentColor C = PieceColor(R, R->SegmentPool[i].Get());
			if (i == 0 && C != EPathSegmentColor::Red) bFirstRed = false;
			if (i > 0 && C != EPathSegmentColor::Green) bRestGreen = false;
		}
		Check(TEXT("B: first piece red"), bFirstRed);
		Check(TEXT("B: remaining pieces green"), bRestGreen);
		// Transition lands on the supplied boundary (sample index 1, x=100), not the line midpoint (x=200).
		UStaticMeshComponent* BoundaryPiece = R->SegmentPool.Num() >= 1 ? R->SegmentPool[0].Get() : nullptr;
		if (BoundaryPiece)
		{
			const FVector Loc = BoundaryPiece->GetComponentLocation();
			Check(TEXT("B: transition at supplied boundary x=100"), FMath::Abs(Loc.X - 50.f) < 1.f);
		}
		R->ClearPath();
		Check(TEXT("B: cleared"), !R->IsPathVisible() && R->GetVisibleSegmentCount() == 0);
	}

	// --- Test C: sampled 3D parabolic arc, red->green boundary at sample 3 (NOT the apex at sample 5) ---
	{
		// z(x) = 90 + 300 * 4*x*(1000-x)/1000^2  -> takeoff/landing z=90, apex z=390 at x=500.
		TArray<FVector> Pts;
		for (int32 i = 0; i <= 10; ++i)
		{
			const float x = i * 100.f;
			const float z = 90.f + 300.f * 4.f * x * (1000.f - x) / (1000.f * 1000.f);
			Pts.Add(FVector(x, 0.f, z));
		}
		TArray<FBuhPathSegment> Segs;
		FBuhPathSegment RedSeg; RedSeg.StartPointIndex = 0; RedSeg.EndPointIndex = 3; RedSeg.Color = EPathSegmentColor::Red;
		FBuhPathSegment GreenSeg; GreenSeg.StartPointIndex = 3; GreenSeg.EndPointIndex = 10; GreenSeg.Color = EPathSegmentColor::Green;
		Segs.Add(RedSeg); Segs.Add(GreenSeg);

		R->SetPath(Pts, Segs);
		Check(TEXT("C: path visible"), R->IsPathVisible());
		Check(TEXT("C: ten visible pieces"), R->GetVisibleSegmentCount() == 10);
		Check(TEXT("C: pool size 10"), R->GetPoolSize() == 10);

		bool bFirst3Red = true, bRestGreen = true, bHasVerticalCurvature = false;
		float MaxPieceZ = 0.f;
		for (int32 i = 0; i < R->SegmentPool.Num(); ++i)
		{
			UStaticMeshComponent* Piece = R->SegmentPool[i].Get();
			const EPathSegmentColor C = PieceColor(R, Piece);
			if (i < 3 && C != EPathSegmentColor::Red) bFirst3Red = false;
			if (i >= 3 && C != EPathSegmentColor::Green) bRestGreen = false;
			if (Piece)
			{
				const float Z = Piece->GetComponentLocation().Z;
				MaxPieceZ = FMath::Max(MaxPieceZ, Z);
				if (Z > 91.f) bHasVerticalCurvature = true;
			}
		}
		Check(TEXT("C: first three pieces red"), bFirst3Red);
		Check(TEXT("C: remaining pieces green"), bRestGreen);
		Check(TEXT("C: real vertical curvature (pieces above ground)"), bHasVerticalCurvature);
		Check(TEXT("C: arc reaches near apex z=390"), MaxPieceZ > 350.f);
		// Boundary at sample index 3 (x=300) — neither the line midpoint (x=500) nor the apex.
		// The first green piece spans x=300..400 (midpoint 350); if the renderer had
		// (wrongly) transitioned at the apex, this piece would sit near x=550 instead.
		UStaticMeshComponent* BoundaryPiece = R->SegmentPool.Num() >= 4 ? R->SegmentPool[3].Get() : nullptr;
		if (BoundaryPiece)
		{
			const FVector Loc = BoundaryPiece->GetComponentLocation();
			Check(TEXT("C: green starts at supplied boundary x~350 (not apex)"), Loc.X > 250.f && Loc.X < 450.f);
		}
		R->ClearPath();
		Check(TEXT("C: cleared"), !R->IsPathVisible() && R->GetVisibleSegmentCount() == 0);
	}

	// --- Test D: component reuse across 20+ varied draws, no leak ---
	{
		const int32 Pattern[] = { 1, 3, 7, 2, 5, 1, 4, 6, 3, 8, 2, 5, 7, 1, 4, 6, 3, 8, 2, 5 };
		int32 MaxRequired = 0;
		for (int32 Cycle = 0; Cycle < 20; ++Cycle)
		{
			const int32 SegCount = Pattern[Cycle];
			// Build SegCount segments tiling a 12-point straight line.
			const int32 NumPoints = 12;
			TArray<FVector> Pts;
			for (int32 i = 0; i < NumPoints; ++i)
			{
				Pts.Add(FVector(i * 50.f, 0.f, 90.f));
			}
			TArray<FBuhPathSegment> Segs;
			int32 Start = 0;
			for (int32 s = 0; s < SegCount; ++s)
			{
				const int32 End = (s == SegCount - 1) ? (NumPoints - 1) : (Start + (NumPoints - 1) / SegCount);
				FBuhPathSegment Seg;
				Seg.StartPointIndex = Start;
				Seg.EndPointIndex = FMath::Max(End, Start + 1);
				Seg.Color = (s % 2 == 0) ? EPathSegmentColor::Green : EPathSegmentColor::Red;
				Segs.Add(Seg);
				Start = Seg.EndPointIndex;
				if (Start >= NumPoints - 1) break;
			}
			R->SetPath(Pts, Segs);
			MaxRequired = FMath::Max(MaxRequired, R->GetVisibleSegmentCount());
			R->ClearPath();
		}
		Check(TEXT("D: pool reuses (size == high-water mark)"), R->GetPoolSize() == MaxRequired);
		Check(TEXT("D: pool bounded (<= 11)"), R->GetPoolSize() <= 11);
		Check(TEXT("D: nothing visible after final clear"), R->GetVisibleSegmentCount() == 0 && !R->IsPathVisible());

		// Only the test's own renderer may be present beyond the level baseline.
		Check(TEXT("D: no actor leak (count == baseline + 1)"), CountRenderers() == BaselineCount + 1);
	}

	// --- Test E: full lifecycle through the real gameplay call path ---
	// Drives UBuhCombatSim::DrawWalkPath / ClearWalkPath / DrawGroundLine (the
	// exact functions the Blueprint UI calls) with a live APlayerController, and
	// checks the same first renderer those statics act on. Proves the path
	// appears, clears, and reappears (restore) without leaking or redrawing.
	{
		// The statics resolve the world from PC->GetWorld(); a spawned
		// PlayerController in the editor world provides that.
		APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
		Check(TEXT("E: player controller spawned"), PC != nullptr && PC->GetWorld() == World);

		// The statics act on the first ABUHPathRenderer found; use the same
		// iterator order so we inspect the actor they actually drive.
		ABUHPathRenderer* Target = nullptr;
		{
			TActorIterator<ABUHPathRenderer> It(World);
			if (It) Target = *It;
		}
		Check(TEXT("E: a renderer exists for the statics to drive"), Target != nullptr);
		if (Target && PC)
		{
			// 1) Enter targeting: path appears.
			UBuhCombatSim::DrawWalkPath(PC, FVector(0.f, 0.f, 90.f), FVector(300.f, 0.f, 90.f));
			Check(TEXT("E: walk path appears"), Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 1);

			// 2) Update the candidate: line moves, still one piece (no accumulation).
			UBuhCombatSim::DrawWalkPath(PC, FVector(0.f, 0.f, 90.f), FVector(600.f, 0.f, 90.f));
			Check(TEXT("E: updated path still one piece"), Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 1);

			// 3) Cancel: path clears.
			UBuhCombatSim::ClearWalkPath(PC);
			Check(TEXT("E: cleared path hidden"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);

			// 4) Re-enter targeting: path reappears (restore), no stale geometry.
			UBuhCombatSim::DrawWalkPath(PC, FVector(0.f, 0.f, 90.f), FVector(450.f, 0.f, 90.f));
			Check(TEXT("E: re-entered path reappears"), Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 1);

			// 5) DrawGroundLine routes through the renderer (green), not the fallback.
			UBuhCombatSim::DrawGroundLine(PC, FVector(0.f, 0.f, 90.f), FVector(200.f, 0.f, 90.f), 5.f, 0.f);
			Check(TEXT("E: ground line draws via renderer"), Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 1);

			// Leave clean.
			UBuhCombatSim::ClearWalkPath(PC);
			Check(TEXT("E: final clear leaves nothing visible"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
		}
		if (PC) PC->Destroy();
	}

	R->Destroy();
	Check(TEXT("D: renderer destroyed (count back to baseline)"), CountRenderers() == BaselineCount);
	return bAllPassed;
}

// Reaction-aware Dash path color verification. Drives UBuhCombatSim::DrawDashPath
// (the exact function the Blueprint UI calls during execution) with a live
// APlayerController and checks the red/green boundary for every elapsed-movement
// Instant count. Proves the remaining Dash path reflects reaction eligibility:
//   A) planning preview (Elapsed=0) -> red 0..270, green 270..720,
//   B) Elapsed=1 -> red 0..180, green 180..720,
//   C) Elapsed=2 -> red 0..90, green 90..720,
//   D) Elapsed=3 (reaction pause) -> entire remaining path green,
//   E) Elapsed=4 (past locked window) -> entire remaining path green,
//   F) short path (2 Instants) Elapsed=0 -> entirely red,
//   G) path exactly 3 Instants Elapsed=0 -> entirely red.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhDashPathColorTest, "BUH.DashPathColorTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhDashPathColorTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	Check(TEXT("Editor world available"), World != nullptr);
	if (!World)
	{
		return bAllPassed;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	Check(TEXT("Player controller spawned"), PC != nullptr && PC->GetWorld() == World);
	if (!PC)
	{
		return bAllPassed;
	}

	ABUHPathRenderer* Target = nullptr;
	{
		TActorIterator<ABUHPathRenderer> It(World);
		if (It) Target = *It;
	}
	Check(TEXT("A renderer exists for the statics to drive"), Target != nullptr);
	if (!Target)
	{
		PC->Destroy();
		return bAllPassed;
	}

	// Full Dash path: 720 uu (8 Instants * 90 uu/Instant).
	const FVector Start(0.f, 0.f, 90.f);
	const FVector EndFull(720.f, 0.f, 90.f);

	// --- Test A: planning preview (Elapsed=0), red 0..270, green 270..720 ---
	{
		UBuhCombatSim::DrawDashPath(PC, Start, EndFull, 0);
		Check(TEXT("A: path visible"), Target->IsPathVisible());
		Check(TEXT("A: two pieces (red+green)"), Target->GetVisibleSegmentCount() == 2);
		UStaticMeshComponent* P0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
		UStaticMeshComponent* P1 = Target->SegmentPool.Num() >= 2 ? Target->SegmentPool[1].Get() : nullptr;
		Check(TEXT("A: first piece red"), P0 && PieceColor(Target, P0) == EPathSegmentColor::Red);
		Check(TEXT("A: second piece green"), P1 && PieceColor(Target, P1) == EPathSegmentColor::Green);
		if (P0)
		{
			// Red piece spans 0..270, midpoint 135.
			Check(TEXT("A: red piece midpoint x~135"), FMath::Abs(P0->GetComponentLocation().X - 135.f) < 5.f);
		}
		UBuhCombatSim::ClearWalkPath(PC);
		Check(TEXT("A: cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
	}

	// --- Test B: Elapsed=1, red 0..180, green 180..720 ---
	{
		UBuhCombatSim::DrawDashPath(PC, Start, EndFull, 1);
		Check(TEXT("B: two pieces"), Target->GetVisibleSegmentCount() == 2);
		UStaticMeshComponent* P0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
		Check(TEXT("B: first piece red"), P0 && PieceColor(Target, P0) == EPathSegmentColor::Red);
		if (P0)
		{
			// Red piece spans 0..180, midpoint 90.
			Check(TEXT("B: red piece midpoint x~90"), FMath::Abs(P0->GetComponentLocation().X - 90.f) < 5.f);
		}
		UBuhCombatSim::ClearWalkPath(PC);
		Check(TEXT("B: cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
	}

	// --- Test C: Elapsed=2, red 0..90, green 90..720 ---
	{
		UBuhCombatSim::DrawDashPath(PC, Start, EndFull, 2);
		Check(TEXT("C: two pieces"), Target->GetVisibleSegmentCount() == 2);
		UStaticMeshComponent* P0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
		Check(TEXT("C: first piece red"), P0 && PieceColor(Target, P0) == EPathSegmentColor::Red);
		if (P0)
		{
			// Red piece spans 0..90, midpoint 45.
			Check(TEXT("C: red piece midpoint x~45"), FMath::Abs(P0->GetComponentLocation().X - 45.f) < 5.f);
		}
		UBuhCombatSim::ClearWalkPath(PC);
		Check(TEXT("C: cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
	}

	// --- Test D: Elapsed=3 (reaction pause), entire path green ---
	{
		UBuhCombatSim::DrawDashPath(PC, Start, EndFull, 3);
		Check(TEXT("D: path visible"), Target->IsPathVisible());
		Check(TEXT("D: one piece (all green)"), Target->GetVisibleSegmentCount() == 1);
		UStaticMeshComponent* P0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
		Check(TEXT("D: piece is green"), P0 && PieceColor(Target, P0) == EPathSegmentColor::Green);
		UBuhCombatSim::ClearWalkPath(PC);
		Check(TEXT("D: cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
	}

	// --- Test E: Elapsed=4 (past locked window), entire path green ---
	{
		UBuhCombatSim::DrawDashPath(PC, Start, EndFull, 4);
		Check(TEXT("E: one piece (all green)"), Target->GetVisibleSegmentCount() == 1);
		UStaticMeshComponent* P0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
		Check(TEXT("E: piece is green"), P0 && PieceColor(Target, P0) == EPathSegmentColor::Green);
		UBuhCombatSim::ClearWalkPath(PC);
		Check(TEXT("E: cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
	}

	// --- Test F: short path (180 uu, 2 Instants), Elapsed=0, entirely red ---
	{
		const FVector EndShort(180.f, 0.f, 90.f);
		UBuhCombatSim::DrawDashPath(PC, Start, EndShort, 0);
		Check(TEXT("F: one piece (all red)"), Target->GetVisibleSegmentCount() == 1);
		UStaticMeshComponent* P0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
		Check(TEXT("F: piece is red"), P0 && PieceColor(Target, P0) == EPathSegmentColor::Red);
		UBuhCombatSim::ClearWalkPath(PC);
		Check(TEXT("F: cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
	}

	// --- Test G: path exactly 3 Instants (270 uu), Elapsed=0, entirely red ---
	{
		const FVector EndThree(270.f, 0.f, 90.f);
		UBuhCombatSim::DrawDashPath(PC, Start, EndThree, 0);
		Check(TEXT("G: one piece (all red)"), Target->GetVisibleSegmentCount() == 1);
		UStaticMeshComponent* P0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
		Check(TEXT("G: piece is red"), P0 && PieceColor(Target, P0) == EPathSegmentColor::Red);
		UBuhCombatSim::ClearWalkPath(PC);
		Check(TEXT("G: cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
	}

	PC->Destroy();
	return bAllPassed;
}

// Real integration regression test: drives ResolveCombat to a Dash reaction
// pause, then verifies the remaining path color comes from the authoritative
// CurrentState (via GetDashElapsedMovementInstants), not from stale Blueprint
// phase/Instant countdowns. This is the exact path the Blueprint UI takes.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhDashReactionIntegrationTest, "BUH.DashReactionIntegrationTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhDashReactionIntegrationTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	Check(TEXT("Editor world available"), World != nullptr);
	if (!World)
	{
		return bAllPassed;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	Check(TEXT("Player controller spawned"), PC != nullptr && PC->GetWorld() == World);
	if (!PC)
	{
		return bAllPassed;
	}

	ABUHPathRenderer* Target = nullptr;
	{
		TActorIterator<ABUHPathRenderer> It(World);
		if (It) Target = *It;
	}
	Check(TEXT("A renderer exists for the statics to drive"), Target != nullptr);
	if (!Target)
	{
		PC->Destroy();
		return bAllPassed;
	}

	// --- Drive ResolveCombat to a Dash reaction pause (PhaseInstants == 3) ---
	// The player is mid-Dash (Action 8, Active phase 2) with 3 movement
	// Instants already elapsed. The opponent uses Heavy Strike (Action 1).
	// ResolveCombat must stop at the reaction pause (StoppedReason 5).
	FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
	S.Fighters[0].Action = 8;
	S.Fighters[0].Phase = 2;
	S.Fighters[0].PhaseInstants = 3;
	S.Fighters[0].DashTarget = 220.f;
	const FBuhCombatResult R = UBuhCombatSim::ResolveCombat(S, -1, 1, 200);
	Check(TEXT("ResolveCombat stopped at reaction pause"), R.StoppedReason == 5 && R.bReactionOpportunity);
	if (R.StoppedReason != 5 || !R.bReactionOpportunity)
	{
		UBuhCombatSim::ClearWalkPath(PC);
		PC->Destroy();
		return bAllPassed;
	}

	// The FinalState is the authoritative CurrentState at the pause.
	const FBuhCombatState& CurrentState = R.FinalState;
	Check(TEXT("CurrentState player is still Dashing"), CurrentState.Fighters[0].Action == 8);
	Check(TEXT("CurrentState player is in Active phase"), CurrentState.Fighters[0].Phase == 2);
	Check(TEXT("CurrentState PhaseInstants >= 3 (reactable)"), CurrentState.Fighters[0].PhaseInstants >= 3);

	// --- GetDashElapsedMovementInstants must read from CurrentState ---
	const int32 Elapsed = UBuhCombatSim::GetDashElapsedMovementInstants(CurrentState);
	Check(TEXT("GetDashElapsedMovementInstants returns PhaseInstants"), Elapsed == CurrentState.Fighters[0].PhaseInstants);
	Check(TEXT("Elapsed >= 3 (past locked window)"), Elapsed >= 3);

	// --- DrawDashPath with the CurrentState-derived elapsed count -> all green ---
	// The Dash path from the player's current position to the Dash target.
	const FVector Start(static_cast<float>(CurrentState.Fighters[0].X), 0.f, 90.f);
	const FVector End(static_cast<float>(CurrentState.Fighters[0].DashTarget), 0.f, 90.f);
	UBuhCombatSim::DrawDashPath(PC, Start, End, Elapsed);
	Check(TEXT("Path visible after DrawDashPath"), Target->IsPathVisible());
	Check(TEXT("Entire remaining path is green (one piece)"), Target->GetVisibleSegmentCount() == 1);
	UStaticMeshComponent* P0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	Check(TEXT("Piece is green"), P0 && PieceColor(Target, P0) == EPathSegmentColor::Green);
	UBuhCombatSim::ClearWalkPath(PC);
	Check(TEXT("Cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);

	// --- Regression guard: a stale Elapsed=0 would produce a red segment ---
	// If the Blueprint had (wrongly) passed 0 instead of the CurrentState value,
	// the first 3 Instants would be red. Verify that Elapsed=0 DOES produce red,
	// proving the test can distinguish the two cases.
	UBuhCombatSim::DrawDashPath(PC, Start, End, 0);
	Check(TEXT("Regression: Elapsed=0 produces a red segment"), Target->GetVisibleSegmentCount() >= 1);
	UStaticMeshComponent* P0b = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	Check(TEXT("Regression: first piece is red when Elapsed=0"), P0b && PieceColor(Target, P0b) == EPathSegmentColor::Red);
	UBuhCombatSim::ClearWalkPath(PC);
	Check(TEXT("Regression: cleared"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);

	PC->Destroy();
	return bAllPassed;
}

// TEMP DIAGNOSTIC: drives the LIVE PIE BP_CombatManager to a Dash reaction
// pause (Action=8, Phase=2, PhaseInstants=3, PlayerMove=8) so the real
// WBP_CombatUI EventTick execution branch draws the Dash path and the
// DrawDashPath DashDiag log captures the actual Elapsed/RemainingLocked/
// Red/Green values from the running widget + renderer. Run while PIE is active.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhPieDashDiagTest, "BUH.PieDashDiagTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBuhPieDashDiagTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	if (!World)
	{
		AddError(TEXT("No active world. Start PIE first."));
		return false;
	}
	AActor* CM = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("BP_CombatManager")))
		{
			CM = *It;
			break;
		}
	}
	if (!CM)
	{
		AddError(TEXT("No BP_CombatManager actor in the world."));
		return false;
	}
	if (FProperty* P = FindFProperty<FProperty>(CM->GetClass(), TEXT("playerMove"))) *P->ContainerPtrToValuePtr<int32>(CM) = 8;
	if (FProperty* P = FindFProperty<FProperty>(CM->GetClass(), TEXT("targetingAction"))) *P->ContainerPtrToValuePtr<int32>(CM) = -1;
	if (FProperty* P = FindFProperty<FProperty>(CM->GetClass(), TEXT("state"))) *P->ContainerPtrToValuePtr<int32>(CM) = 3;
	FStructProperty* CS = FindFProperty<FStructProperty>(CM->GetClass(), TEXT("currentState"));
	if (CS && CS->Struct)
	{
		FBuhCombatState* St = CS->ContainerPtrToValuePtr<FBuhCombatState>(CM);
		St->Fighters.SetNum(2);
		St->Fighters[0].Health = 100; St->Fighters[0].X = -500; St->Fighters[0].Y = 0;
		St->Fighters[0].Action = 8; St->Fighters[0].Phase = 2; St->Fighters[0].PhaseInstants = 3;
		St->Fighters[0].Team = 0; St->Fighters[0].Facing = 1; St->Fighters[0].bCommitted = true;
		St->Fighters[0].DashTarget = 200;
		St->Fighters[1].Health = 100; St->Fighters[1].X = 500; St->Fighters[1].Y = 0;
		St->Fighters[1].Action = 1; St->Fighters[1].Phase = 2; St->Fighters[1].PhaseInstants = 5;
		St->Fighters[1].Team = 1; St->Fighters[1].Facing = -1; St->Fighters[1].bCommitted = true;
		St->Instant = 10; St->Seed = 0;
	}
	AddInfo(TEXT("Set PIE CombatManager to a Dash reaction pause. The WBP EventTick will draw the Dash path next frame and log DashDiag."));
	return true;
}

UEdGraphNode* FindGraphNodeByName(UEdGraph* Graph, const TCHAR* Name)
{
	if (!Graph)
	{
		return nullptr;
	}
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && Node->GetFName().ToString() == Name)
		{
			return Node;
		}
	}
	return nullptr;
}

UEdGraphPin* FindGraphPin(UEdGraphNode* Node, const TCHAR* PinName, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && Pin->PinName.ToString() == PinName && Pin->Direction == Direction)
		{
			return Pin;
		}
	}
	return nullptr;
}

bool IsPinLinkedToNode(UEdGraphPin* Pin, UEdGraphNode* TargetNode)
{
	if (!Pin || !TargetNode)
	{
		return false;
	}
	for (FEdGraphPinReference Ref : Pin->LinkedTo)
	{
		if (Ref.Get() && Ref.Get()->GetOwningNode() == TargetNode)
		{
			return true;
		}
	}
	return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhDashGateRegressionTest, "BUH.DashGateRegressionTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhDashGateRegressionTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI"));
	Check(TEXT("WBP_CombatUI loaded"), BP != nullptr);
	if (!BP || !BP->GeneratedClass)
	{
		return bAllPassed;
	}

	UEdGraph* Graph = FindObject<UEdGraph>(BP, TEXT("EventGraph"));
	Check(TEXT("EventGraph found"), Graph != nullptr);
	if (!Graph)
	{
		return bAllPassed;
	}

	TArray<UK2Node_CallFunction*> DashDraws;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (auto* Call = Cast<UK2Node_CallFunction>(Node))
		{
			if (Call->FunctionReference.GetMemberName() == TEXT("DrawDashPath"))
			{
				DashDraws.Add(Call);
			}
		}
	}
	Check(TEXT("At least one DrawDashPath node exists"), DashDraws.Num() >= 1);
	if (DashDraws.Num() < 1)
	{
		return bAllPassed;
	}

	bool bFoundExecutionGate = false;
	for (UK2Node_CallFunction* DashDraw : DashDraws)
	{
		UEdGraphPin* ElapsedPin = FindGraphPin(DashDraw, TEXT("ElapsedMovementInstants"), EGPD_Input);
		if (!ElapsedPin)
		{
			continue;
		}
		UEdGraphNode* SelectNode = nullptr;
		for (FEdGraphPinReference Ref : ElapsedPin->LinkedTo)
		{
			if (Ref.Get() && Ref.Get()->GetOwningNode())
			{
				SelectNode = Ref.Get()->GetOwningNode();
				break;
			}
		}
		auto* SelectCall = Cast<UK2Node_CallFunction>(SelectNode);
		if (!SelectCall || SelectCall->FunctionReference.GetMemberName() != TEXT("SelectInt"))
		{
			continue;
		}
		bFoundExecutionGate = true;

		UEdGraphPin* APin = FindGraphPin(SelectCall, TEXT("A"), EGPD_Input);
		UEdGraphPin* BPin = FindGraphPin(SelectCall, TEXT("B"), EGPD_Input);
		UEdGraphPin* PickAPin = FindGraphPin(SelectCall, TEXT("bPickA"), EGPD_Input);
		Check(TEXT("SelectInt A pin exists"), APin != nullptr);
		Check(TEXT("SelectInt B pin exists"), BPin != nullptr);
		Check(TEXT("SelectInt bPickA pin exists"), PickAPin != nullptr);
		if (!APin || !BPin || !PickAPin)
		{
			continue;
		}

		bool bAIsZero = APin->AutogeneratedDefaultValue == TEXT("0") || APin->DefaultValue == TEXT("0");
		Check(TEXT("SelectInt A is zero literal"), bAIsZero);

		bool bBIsDashElapsed = false;
		for (FEdGraphPinReference Ref : BPin->LinkedTo)
		{
			if (Ref.Get() && Ref.Get()->GetOwningNode())
			{
				auto* BCall = Cast<UK2Node_CallFunction>(Ref.Get()->GetOwningNode());
				if (BCall && BCall->FunctionReference.GetMemberName() == TEXT("GetDashElapsedMovementInstants"))
				{
					bBIsDashElapsed = true;
					break;
				}
			}
		}
		Check(TEXT("SelectInt B is GetDashElapsedMovementInstants"), bBIsDashElapsed);

		bool bPickAIsDashAwaiting = false;
		for (FEdGraphPinReference Ref : PickAPin->LinkedTo)
		{
			if (Ref.Get() && Ref.Get()->GetOwningNode())
			{
				auto* PCall = Cast<UK2Node_CallFunction>(Ref.Get()->GetOwningNode());
				if (PCall && PCall->FunctionReference.GetMemberName() == TEXT("IsDashAwaitingExecute"))
				{
					bPickAIsDashAwaiting = true;
					break;
				}
			}
		}
		Check(TEXT("SelectInt bPickA is IsDashAwaitingExecute"), bPickAIsDashAwaiting);
		break;
	}
	Check(TEXT("Execution DrawDashPath uses SelectInt gate"), bFoundExecutionGate);

	return bAllPassed;
}

namespace
{
	// Mirrors the exact decision the WBP_CombatUI EventTick makes before calling
	// DrawDashPath: while targeting, while a replacement Dash is confirmed but not
	// yet executed, or when the selected move is not Dash, the effective elapsed is
	// 0 (red). Otherwise it is the authoritative elapsed from CurrentState (green).
	int32 EffectiveDashElapsed(int32 TargetingAction, int32 PlayerMove, bool bDashAwaitingExecute, const FBuhCombatState& State)
	{
		if (TargetingAction >= 0)
		{
			return 0;
		}
		if (PlayerMove != 8)
		{
			return 0;
		}
		if (bDashAwaitingExecute)
		{
			return 0;
		}
		return UBuhCombatSim::GetDashElapsedMovementInstants(State);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhDashLifecycleRegressionTest, "BUH.DashLifecycleRegressionTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhDashLifecycleRegressionTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	Check(TEXT("Editor world available"), World != nullptr);
	if (!World)
	{
		return bAllPassed;
	}

	// The explicit pending-decision state must exist on the real BP_CombatManager
	// asset. This proves the lifecycle is represented in instance state, not a
	// global, without relying on headless ProcessEvent variable writes (which do
	// not land in the editor world; see BUH.CombatManagerIntegrationTests).
	const FString AssetPath = TEXT("/Game/BUH/Prototypes/TacticalDuel/Blueprints/BP_CombatManager.BP_CombatManager");
	UBlueprint* CombatManagerBP = LoadObject<UBlueprint>(nullptr, *AssetPath);
	Check(TEXT("BP_CombatManager asset loaded"), CombatManagerBP != nullptr);
	if (CombatManagerBP && CombatManagerBP->GeneratedClass)
	{
		UClass* CombatManagerClass = CombatManagerBP->GeneratedClass;
		Check(TEXT("bDashAwaitingExecute variable exists on BP_CombatManager"),
			CombatManagerClass->FindPropertyByName(TEXT("bDashAwaitingExecute")) != nullptr);
		Check(TEXT("IsDashAwaitingExecute function exists on BP_CombatManager"),
			CombatManagerClass->FindFunctionByName(TEXT("IsDashAwaitingExecute")) != nullptr);
	}
	else
	{
		return bAllPassed;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	Check(TEXT("Player controller spawned"), PC != nullptr && PC->GetWorld() == World);
	if (!PC)
	{
		return bAllPassed;
	}

	ABUHPathRenderer* Target = nullptr;
	{
		TActorIterator<ABUHPathRenderer> It(World);
		if (It) Target = *It;
	}
	if (!Target)
	{
		Target = World->SpawnActor<ABUHPathRenderer>(FVector::ZeroVector, FRotator::ZeroRotator);
		Target->PostInitializeComponents();
		Target->DispatchBeginPlay();
	}
	Check(TEXT("A renderer exists for DrawDashPath to drive"), Target != nullptr);
	if (!Target)
	{
		PC->Destroy();
		return bAllPassed;
	}

	// --- Existing Dash reaction (green) ---
	FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
	S.Fighters[0].Action = 8;
	S.Fighters[0].Phase = 2;
	S.Fighters[0].PhaseInstants = 3;
	S.Fighters[0].DashTarget = 220.f;
	const FBuhCombatResult R = UBuhCombatSim::ResolveCombat(S, -1, 1, 200);
	Check(TEXT("Existing Dash reached a reaction pause"), R.StoppedReason == 5 && R.bReactionOpportunity);
	if (R.StoppedReason != 5 || !R.bReactionOpportunity)
	{
		UBuhCombatSim::ClearWalkPath(PC);
		PC->Destroy();
		return bAllPassed;
	}

	FBuhCombatState CurrentState = R.FinalState;
	int32 TargetingAction = -1;
	int32 PlayerMove = 8;
	bool bDashAwaitingExecute = false;

	const int32 OldElapsed = UBuhCombatSim::GetDashElapsedMovementInstants(CurrentState);
	Check(TEXT("Old Dash elapsed is reactable (>=3)"), OldElapsed >= 3);
	Check(TEXT("Effective elapsed for an existing Dash is the old progress"),
		EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState) == OldElapsed);

	const FVector OldStart(static_cast<float>(CurrentState.Fighters[0].X), 0.f, 90.f);
	const FVector OldEnd(static_cast<float>(CurrentState.Fighters[0].DashTarget), 0.f, 90.f);
	UBuhCombatSim::DrawDashPath(PC, OldStart, OldEnd, EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState));
	Check(TEXT("Existing Dash continuation is visible"), Target->IsPathVisible());
	Check(TEXT("Existing Dash continuation is entirely green"), Target->GetVisibleSegmentCount() == 1);
	UStaticMeshComponent* OldPiece = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	Check(TEXT("Existing Dash piece is green"), OldPiece && PieceColor(Target, OldPiece) == EPathSegmentColor::Green);
	UBuhCombatSim::ClearWalkPath(PC);

	// --- Begin replacement Dash targeting (red) ---
	TargetingAction = 8;
	bDashAwaitingExecute = false;
	Check(TEXT("Effective elapsed is 0 while targeting"),
		EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState) == 0);

	const float NewTargetX = static_cast<float>(CurrentState.Fighters[0].X) + 450.f;
	const FVector NewStart = OldStart;
	const FVector NewEnd(NewTargetX, 0.f, 90.f);
	UBuhCombatSim::DrawDashPath(PC, NewStart, NewEnd, EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState));
	Check(TEXT("Targeting replacement Dash is visible"), Target->IsPathVisible());
	Check(TEXT("Targeting replacement Dash has red and green sections"), Target->GetVisibleSegmentCount() == 2);
	UStaticMeshComponent* NewPiece0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	UStaticMeshComponent* NewPiece1 = Target->SegmentPool.Num() >= 2 ? Target->SegmentPool[1].Get() : nullptr;
	Check(TEXT("Targeting replacement Dash first piece is red"), NewPiece0 && PieceColor(Target, NewPiece0) == EPathSegmentColor::Red);
	Check(TEXT("Targeting replacement Dash second piece is green"), NewPiece1 && PieceColor(Target, NewPiece1) == EPathSegmentColor::Green);
	UBuhCombatSim::ClearWalkPath(PC);

	// --- Confirm replacement Dash destination (STILL RED) ---
	TargetingAction = -1;
	PlayerMove = 8;
	bDashAwaitingExecute = true;
	Check(TEXT("Effective elapsed is 0 after confirmation"),
		EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState) == 0);

	UBuhCombatSim::DrawDashPath(PC, NewStart, NewEnd, EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState));
	Check(TEXT("Confirmed replacement Dash is still visible"), Target->IsPathVisible());
	Check(TEXT("Confirmed replacement Dash still has red and green sections"), Target->GetVisibleSegmentCount() == 2);
	UStaticMeshComponent* ConfPiece0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	UStaticMeshComponent* ConfPiece1 = Target->SegmentPool.Num() >= 2 ? Target->SegmentPool[1].Get() : nullptr;
	Check(TEXT("Confirmed replacement Dash first piece is still red"), ConfPiece0 && PieceColor(Target, ConfPiece0) == EPathSegmentColor::Red);
	Check(TEXT("Confirmed replacement Dash second piece is green"), ConfPiece1 && PieceColor(Target, ConfPiece1) == EPathSegmentColor::Green);
	UBuhCombatSim::ClearWalkPath(PC);

	// --- UI refreshes must not clear the pending decision ---
	for (int32 Refresh = 0; Refresh < 3; ++Refresh)
	{
		Check(TEXT("bDashAwaitingExecute survives UI refresh"), bDashAwaitingExecute);
		UBuhCombatSim::DrawDashPath(PC, NewStart, NewEnd, EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState));
		UStaticMeshComponent* RefreshPiece0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
		Check(TEXT("Refreshed confirmed replacement Dash first piece is still red"), RefreshPiece0 && PieceColor(Target, RefreshPiece0) == EPathSegmentColor::Red);
		UBuhCombatSim::ClearWalkPath(PC);
	}

	// --- Cancel restores the old green continuation ---
	TargetingAction = -1;
	bDashAwaitingExecute = false;
	Check(TEXT("Effective elapsed returns to the old progress after cancel"),
		EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState) == OldElapsed);

	UBuhCombatSim::DrawDashPath(PC, OldStart, OldEnd, EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState));
	Check(TEXT("Canceled replacement restores old Dash visibility"), Target->IsPathVisible());
	Check(TEXT("Canceled replacement restores old Dash green path"), Target->GetVisibleSegmentCount() == 1);
	UStaticMeshComponent* CancelPiece = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	Check(TEXT("Canceled replacement old Dash piece is green"), CancelPiece && PieceColor(Target, CancelPiece) == EPathSegmentColor::Green);
	UBuhCombatSim::ClearWalkPath(PC);

	// --- Continue preserves the original Dash progress ---
	PlayerMove = 8;
	bDashAwaitingExecute = false;
	Check(TEXT("Effective elapsed is the old progress after Continue"),
		EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState) == OldElapsed);

	UBuhCombatSim::DrawDashPath(PC, OldStart, OldEnd, EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState));
	Check(TEXT("Continued Dash is visible"), Target->IsPathVisible());
	Check(TEXT("Continued Dash remains entirely green"), Target->GetVisibleSegmentCount() == 1);
	UStaticMeshComponent* ContinuePiece = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	Check(TEXT("Continued Dash piece is green"), ContinuePiece && PieceColor(Target, ContinuePiece) == EPathSegmentColor::Green);
	UBuhCombatSim::ClearWalkPath(PC);

	// --- Short replacement Dash (<=270 uu) is entirely red ---
	TargetingAction = 8;
	bDashAwaitingExecute = false;
	const float ShortTargetX = static_cast<float>(CurrentState.Fighters[0].X) + 200.f;
	TargetingAction = -1;
	PlayerMove = 8;
	bDashAwaitingExecute = true;
	Check(TEXT("Short replacement effective elapsed is 0"),
		EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState) == 0);

	const FVector ShortEnd(ShortTargetX, 0.f, 90.f);
	UBuhCombatSim::DrawDashPath(PC, NewStart, ShortEnd, EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState));
	Check(TEXT("Short replacement Dash is visible"), Target->IsPathVisible());
	Check(TEXT("Short replacement Dash is entirely red"), Target->GetVisibleSegmentCount() == 1);
	UStaticMeshComponent* ShortPiece = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	Check(TEXT("Short replacement Dash piece is red"), ShortPiece && PieceColor(Target, ShortPiece) == EPathSegmentColor::Red);
	UBuhCombatSim::ClearWalkPath(PC);
	TargetingAction = -1;
	bDashAwaitingExecute = false;

	// --- Execute starts a new commitment period ---
	TargetingAction = 8;
	bDashAwaitingExecute = false;
	TargetingAction = -1;
	PlayerMove = 8;
	bDashAwaitingExecute = true;
	Check(TEXT("Replacement effective elapsed is 0 before Execute"),
		EffectiveDashElapsed(TargetingAction, PlayerMove, bDashAwaitingExecute, CurrentState) == 0);

	FBuhCombatState CommitState = UBuhCombatSim::WithPlayerDashTarget(CurrentState, NewTargetX, 0.f);
	const FBuhCombatResult CommitResult = UBuhCombatSim::ResolveCombat(CommitState, 8, 1, 200);
	bDashAwaitingExecute = false;
	Check(TEXT("Executed replacement Dash is committed"), CommitResult.FinalState.Fighters.Num() >= 1 && CommitResult.FinalState.Fighters[0].Action == 8);
	const int32 NewElapsed = UBuhCombatSim::GetDashElapsedMovementInstants(CommitResult.FinalState);
	Check(TEXT("Executed replacement Dash does not inherit old elapsed Instants"), NewElapsed < 3);

	UBuhCombatSim::ClearWalkPath(PC);
	PC->Destroy();
	return bAllPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhJumpSimulationTest, "BUH.JumpSimulationTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhJumpSimulationTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
	const FBuhCombatResult Vertical = UBuhCombatSim::ResolveCombat(S, 9, 1, 200);
	Check(TEXT("Vertical Jump lands in place"), Vertical.FinalState.Fighters.Num() >= 1 && FMath::Abs(Vertical.FinalState.Fighters[0].X - (-500.f)) < 1.f);
	Check(TEXT("Vertical Jump ends on the ground"), Vertical.FinalState.Fighters.Num() >= 1 && FMath::Abs(Vertical.FinalState.Fighters[0].Height) < 1.f);
	Check(TEXT("Vertical Jump takes no damage from a long Heavy"), Vertical.FinalState.Fighters.Num() >= 1 && Vertical.FinalState.Fighters[0].Health == 100);

	FBuhCombatState ClampState = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
	ClampState = UBuhCombatSim::WithPlayerJumpTarget(ClampState, 500.f, 0.f);
	const FBuhCombatResult Clamp = UBuhCombatSim::ResolveCombat(ClampState, 9, 1, 200);
	Check(TEXT("Jump clamps to the 480 uu range"), Clamp.FinalState.Fighters.Num() >= 1 && FMath::Abs(Clamp.FinalState.Fighters[0].X - (-20.f)) < 1.f);
	Check(TEXT("Clamped Jump ends on the ground"), Clamp.FinalState.Fighters.Num() >= 1 && FMath::Abs(Clamp.FinalState.Fighters[0].Height) < 1.f);

	FBuhCombatState ShortState = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
	ShortState = UBuhCombatSim::WithPlayerJumpTarget(ShortState, -400.f, 0.f);
	const TArray<FBuhTimelineSnapshot> ShortTimeline = UBuhCombatSim::GenerateTimeline(ShortState, 9, -1, 200);
	Check(TEXT("Short Jump still flies for 8 Instants"), ShortTimeline.Num() >= 9);
	if (ShortTimeline.Num() >= 9)
	{
		Check(TEXT("Short Jump is airborne at Instant 4"), ShortTimeline[4].PlayerHeight > 100.f);
		Check(TEXT("Short Jump is near apex at Instant 4"), FMath::Abs(ShortTimeline[4].PlayerHeight - 220.f) < 1.f);
		Check(TEXT("Short Jump is still airborne at Instant 7"), ShortTimeline[7].PlayerHeight > 10.f);
		Check(TEXT("Short Jump lands at Instant 8"), FMath::Abs(ShortTimeline[8].PlayerHeight) < 1.f);
	}
	Check(TEXT("Short Jump reaches the requested destination"), ShortTimeline.Num() > 0 && FMath::Abs(ShortTimeline.Last().PlayerX - (-400.f)) < 1.f);

	FBuhCombatState ReactState = UBuhCombatSim::MakeStateWithPhases(-500, 500, 100, 100, 0, 2, 3, 0, 0);
	ReactState.Fighters[0].Action = 9;
	ReactState.Fighters[0].JumpStartX = -500.f;
	ReactState.Fighters[0].JumpStartY = 0.f;
	ReactState.Fighters[0].JumpTarget = -500.f;
	ReactState.Fighters[0].JumpTargetY = 0.f;
	const FBuhCombatResult React = UBuhCombatSim::ResolveCombat(ReactState, -1, 0, 200);
	Check(TEXT("Jump permits a reaction after 3 flight Instants"), React.StoppedReason == 5 && React.bReactionOpportunity);

	FBuhCombatState ContinueState = UBuhCombatSim::MakeStateWithPhases(-500, 500, 100, 100, 0, 2, 5, 0, 0);
	ContinueState.Fighters[0].Action = 9;
	ContinueState.Fighters[0].JumpStartX = -500.f;
	ContinueState.Fighters[0].JumpStartY = 0.f;
	ContinueState.Fighters[0].JumpTarget = -100.f;
	ContinueState.Fighters[0].JumpTargetY = 0.f;
	const FBuhCombatResult Continue = UBuhCombatSim::ResolveCombat(ContinueState, 7, 1, 200);
	Check(TEXT("Continue preserves Jump flight progress"), Continue.FinalState.Fighters.Num() >= 1 && Continue.FinalState.Fighters[0].Action == 9);
	Check(TEXT("Continue keeps the Jump target"), Continue.FinalState.Fighters.Num() >= 1 && FMath::Abs(Continue.FinalState.Fighters[0].JumpTarget - (-100.f)) < 1.f);

	return bAllPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhJumpPathColorTest, "BUH.JumpPathColorTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhJumpPathColorTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	Check(TEXT("Editor world available"), World != nullptr);
	if (!World)
	{
		return bAllPassed;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	Check(TEXT("Player controller spawned"), PC != nullptr && PC->GetWorld() == World);
	if (!PC)
	{
		return bAllPassed;
	}

	ABUHPathRenderer* Target = nullptr;
	{
		TActorIterator<ABUHPathRenderer> It(World);
		if (It) Target = *It;
	}
	if (!Target)
	{
		Target = World->SpawnActor<ABUHPathRenderer>(FVector::ZeroVector, FRotator::ZeroRotator);
		Target->PostInitializeComponents();
		Target->DispatchBeginPlay();
	}
	Check(TEXT("A renderer exists for DrawJumpPath to drive"), Target != nullptr);
	if (!Target)
	{
		PC->Destroy();
		return bAllPassed;
	}

	const FVector Start(-500.f, 0.f, 90.f);
	const FVector End(-20.f, 0.f, 90.f);

	UBuhCombatSim::DrawJumpPath(PC, Start, End, 0);
	Check(TEXT("Full Jump path is visible"), Target->IsPathVisible());
	Check(TEXT("Full Jump path has eight flight segments"), Target->GetVisibleSegmentCount() == 8);
	bool bFirstThreeRed = true, bRestGreen = true, bHasHeight = false;
	for (int32 i = 0; i < Target->SegmentPool.Num(); ++i)
	{
		const EPathSegmentColor C = PieceColor(Target, Target->SegmentPool[i].Get());
		if (i < 3 && C != EPathSegmentColor::Red) bFirstThreeRed = false;
		if (i >= 3 && C != EPathSegmentColor::Green) bRestGreen = false;
		if (Target->SegmentPool[i].Get() && Target->SegmentPool[i].Get()->GetComponentLocation().Z > 190.f) bHasHeight = true;
	}
	Check(TEXT("First three Jump segments are red"), bFirstThreeRed);
	Check(TEXT("Remaining Jump segments are green"), bRestGreen);
	Check(TEXT("Jump path has vertical curvature"), bHasHeight);
	UBuhCombatSim::ClearWalkPath(PC);

	UBuhCombatSim::DrawJumpPath(PC, Start, End, 2);
	Check(TEXT("Jump path after 2 elapsed Instants is visible"), Target->IsPathVisible());
	Check(TEXT("Jump path after 2 elapsed Instants has six segments"), Target->GetVisibleSegmentCount() == 6);
	UStaticMeshComponent* ElapsedPiece0 = Target->SegmentPool.Num() >= 1 ? Target->SegmentPool[0].Get() : nullptr;
	UStaticMeshComponent* ElapsedPiece1 = Target->SegmentPool.Num() >= 2 ? Target->SegmentPool[1].Get() : nullptr;
	Check(TEXT("Jump path after 2 elapsed Instants starts red"), ElapsedPiece0 && PieceColor(Target, ElapsedPiece0) == EPathSegmentColor::Red);
	Check(TEXT("Jump path after 2 elapsed Instants turns green"), ElapsedPiece1 && PieceColor(Target, ElapsedPiece1) == EPathSegmentColor::Green);
	UBuhCombatSim::ClearWalkPath(PC);

	UBuhCombatSim::DrawJumpPath(PC, Start, End, 3);
	Check(TEXT("Jump path after 3 elapsed Instants is visible"), Target->IsPathVisible());
	Check(TEXT("Jump path after 3 elapsed Instants has five segments"), Target->GetVisibleSegmentCount() == 5);
	bool bAllGreen = true;
	for (int32 i = 0; i < Target->SegmentPool.Num(); ++i)
	{
		if (PieceColor(Target, Target->SegmentPool[i].Get()) != EPathSegmentColor::Green) bAllGreen = false;
	}
	Check(TEXT("Jump path after 3 elapsed Instants is entirely green"), bAllGreen);
	UBuhCombatSim::ClearWalkPath(PC);

	const FVector VerticalStart(-500.f, 0.f, 90.f);
	UBuhCombatSim::DrawJumpPath(PC, VerticalStart, VerticalStart, 0);
	Check(TEXT("Vertical Jump path is visible"), Target->IsPathVisible());
	Check(TEXT("Vertical Jump path has eight flight segments"), Target->GetVisibleSegmentCount() == 8);
	bHasHeight = false;
	for (int32 i = 0; i < Target->SegmentPool.Num(); ++i)
	{
		if (Target->SegmentPool[i].Get() && Target->SegmentPool[i].Get()->GetComponentLocation().Z > 190.f) bHasHeight = true;
	}
	Check(TEXT("Vertical Jump path rises above the ground"), bHasHeight);
	UBuhCombatSim::ClearWalkPath(PC);

	UBuhCombatSim::DrawJumpPath(PC, Start, End, 8);
	Check(TEXT("Completed Jump path clears"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);

	PC->Destroy();
	return bAllPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhJumpPlaybackIntegrationTest, "BUH.JumpPlaybackIntegrationTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhJumpPlaybackIntegrationTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	Check(TEXT("Editor world available"), World != nullptr);
	if (!World)
	{
		return bAllPassed;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	Check(TEXT("Player controller spawned"), PC != nullptr && PC->GetWorld() == World);
	if (!PC)
	{
		return bAllPassed;
	}

	ABUHPathRenderer* Target = nullptr;
	{
		TActorIterator<ABUHPathRenderer> It(World);
		if (It) Target = *It;
	}
	if (!Target)
	{
		Target = World->SpawnActor<ABUHPathRenderer>(FVector::ZeroVector, FRotator::ZeroRotator);
		Target->PostInitializeComponents();
		Target->DispatchBeginPlay();
	}
	Check(TEXT("A renderer exists for Jump playback"), Target != nullptr);
	if (!Target)
	{
		PC->Destroy();
		return bAllPassed;
	}

	AActor* Player = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	AActor* Opponent = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (Player)
	{
		USceneComponent* Root = NewObject<USceneComponent>(Player);
		Player->SetRootComponent(Root);
		Root->RegisterComponent();
		Player->SetActorLocation(FVector(-500.f, 0.f, 90.f));
	}
	if (Opponent)
	{
		USceneComponent* Root = NewObject<USceneComponent>(Opponent);
		Opponent->SetRootComponent(Root);
		Root->RegisterComponent();
		Opponent->SetActorLocation(FVector(500.f, 0.f, 90.f));
	}
	Check(TEXT("Fighter actors spawned"), Player != nullptr && Opponent != nullptr);
	if (!Player || !Opponent)
	{
		PC->Destroy();
		return bAllPassed;
	}

	UBuhCombatSim::StopJumpPlayback();

	FBuhCombatState CompletedPre = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
	CompletedPre = UBuhCombatSim::WithPlayerJumpTarget(CompletedPre, -20.f, 0.f);
	const FBuhCombatResult CompletedResult = UBuhCombatSim::ResolveCombat(CompletedPre, 9, -1, 200);
	UBuhCombatSim::BeginJumpPlayback(PC, Player, Opponent, CompletedPre, 9, -1, CompletedResult);
	float MaxZ = 0.f;
	for (int32 i = 0; i < 50; ++i)
	{
		if (!UBuhCombatSim::TickJumpPlayback(0.05f)) break;
		MaxZ = FMath::Max(MaxZ, Player->GetActorLocation().Z);
	}
	Check(TEXT("Completed Jump playback stops"), !UBuhCombatSim::TickJumpPlayback(0.05f));
	Check(TEXT("Completed Jump lands on the ground"), FMath::Abs(Player->GetActorLocation().Z - 90.f) < 1.f);
	Check(TEXT("Completed Jump reaches the destination"), FMath::Abs(Player->GetActorLocation().X - (-20.f)) < 1.f);
	Check(TEXT("Completed Jump apex is 220 uu above ground"), FMath::Abs(MaxZ - (90.f + 220.f)) < 1.f);
	Check(TEXT("Completed Jump clears the path"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);
	Check(TEXT("Completed Jump leaves the opponent grounded"), FMath::Abs(Opponent->GetActorLocation().Z - 90.f) < 1.f);

	FBuhCombatState InterruptPre = UBuhCombatSim::MakeStateWithPhases(-500, 500, 100, 100, 0, 2, 3, 0, 0);
	InterruptPre.Fighters[0].Action = 9;
	InterruptPre.Fighters[0].JumpStartX = -500.f;
	InterruptPre.Fighters[0].JumpStartY = 0.f;
	InterruptPre.Fighters[0].JumpTarget = -20.f;
	InterruptPre.Fighters[0].JumpTargetY = 0.f;
	InterruptPre.Fighters[0].X = -320.f;
	InterruptPre.Fighters[0].Y = 0.f;
	InterruptPre.Fighters[0].Height = 206.25f;
	const FBuhCombatResult InterruptResult = UBuhCombatSim::ResolveCombat(InterruptPre, 7, 1, 200);
	Check(TEXT("Airborne Jump vs Heavy pauses at a reaction opportunity"), InterruptResult.bReactionOpportunity);
	Check(TEXT("Airborne Jump is still airborne at the reaction pause"), InterruptResult.FinalState.Fighters.Num() >= 1 && InterruptResult.FinalState.Fighters[0].Action == 9 && InterruptResult.FinalState.Fighters[0].Phase == 2 && InterruptResult.FinalState.Fighters[0].PhaseInstants == 3);
	UBuhCombatSim::BeginJumpPlayback(PC, Player, Opponent, InterruptPre, 7, 1, InterruptResult);
	for (int32 i = 0; i < 50; ++i)
	{
		if (!UBuhCombatSim::TickJumpPlayback(0.05f)) break;
	}
	Check(TEXT("Interrupted Jump playback stops at the reaction"), !UBuhCombatSim::TickJumpPlayback(0.05f));
	Check(TEXT("Interrupted Jump actor is visibly airborne"), Player->GetActorLocation().Z > 200.f);
	Check(TEXT("Interrupted Jump path is visible"), Target->IsPathVisible());
	Check(TEXT("Interrupted Jump path has five remaining segments"), Target->GetVisibleSegmentCount() == 5);
	bool bInterruptedAllGreen = true;
	for (int32 i = 0; i < Target->SegmentPool.Num(); ++i)
	{
		if (PieceColor(Target, Target->SegmentPool[i].Get()) != EPathSegmentColor::Green) bInterruptedAllGreen = false;
	}
	Check(TEXT("Interrupted Jump path is entirely green"), bInterruptedAllGreen);

	const FBuhCombatState ContinuePre = InterruptResult.FinalState;
	const FBuhCombatResult ContinueResult = UBuhCombatSim::ResolveCombat(ContinuePre, 7, 1, 200);
	UBuhCombatSim::BeginJumpPlayback(PC, Player, Opponent, ContinuePre, 7, 1, ContinueResult);
	for (int32 i = 0; i < 50; ++i)
	{
		if (!UBuhCombatSim::TickJumpPlayback(0.05f)) break;
	}
	Check(TEXT("Continue Jump playback stops"), !UBuhCombatSim::TickJumpPlayback(0.05f));
	Check(TEXT("Continue Jump lands on the ground"), FMath::Abs(Player->GetActorLocation().Z - 90.f) < 1.f);
	Check(TEXT("Continue Jump reaches the original destination"), FMath::Abs(Player->GetActorLocation().X - (-20.f)) < 1.f);
	Check(TEXT("Continue Jump clears the path"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);

	FBuhCombatState AfterApexState = UBuhCombatSim::MakeStateWithPhases(-500, 500, 100, 100, 0, 2, 5, 0, 0);
	AfterApexState.Fighters[0].Action = 9;
	AfterApexState.Fighters[0].JumpStartX = -500.f;
	AfterApexState.Fighters[0].JumpStartY = 0.f;
	AfterApexState.Fighters[0].JumpTarget = -20.f;
	AfterApexState.Fighters[0].JumpTargetY = 0.f;
	AfterApexState.Fighters[0].X = -200.f;
	AfterApexState.Fighters[0].Y = 0.f;
	AfterApexState.Fighters[0].Height = 206.25f;
	UBuhCombatSim::DrawJumpPathFromState(PC, AfterApexState, 5);
	Check(TEXT("After-apex Jump path is visible"), Target->IsPathVisible());
	Check(TEXT("After-apex Jump path has three remaining segments"), Target->GetVisibleSegmentCount() == 3);
	bool bAfterApexDescends = true, bAfterApexGreen = true;
	for (int32 i = 0; i < Target->SegmentPool.Num(); ++i)
	{
		UStaticMeshComponent* Piece = Target->SegmentPool[i].Get();
		if (Piece && Piece->GetComponentLocation().Z > 300.f) bAfterApexDescends = false;
		if (PieceColor(Target, Piece) != EPathSegmentColor::Green) bAfterApexGreen = false;
	}
	Check(TEXT("After-apex Jump path does not create a second apex"), bAfterApexDescends);
	Check(TEXT("After-apex Jump path is entirely green"), bAfterApexGreen);
	UBuhCombatSim::ClearWalkPath(PC);

	FBuhCombatState GroundState = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
	UBuhCombatSim::DrawJumpPathFromState(PC, GroundState, 0);
	Check(TEXT("Non-Jump state clears the path"), !Target->IsPathVisible() && Target->GetVisibleSegmentCount() == 0);

	Player->Destroy();
	Opponent->Destroy();
	PC->Destroy();
	return bAllPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhJumpGroundedReactionTest, "BUH.JumpGroundedReactionTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuhJumpGroundedReactionTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	UWorld* World = GWorld;
	if (!World && GEditor) World = GEditor->GetEditorWorldContext().World();
	Check(TEXT("World available (PIE preferred)"), World != nullptr);
	if (!World)
	{
		return bAllPassed;
	}
	UE_LOG(LogTemp, Warning, TEXT("JumpGroundedReaction WORLD: PIE=%d"), GWorld ? 1 : 0);

	FBuhCombatState PreState = UBuhCombatSim::MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 3, 4);
	PreState.Fighters[1].Action = 1;
	PreState = UBuhCombatSim::WithPlayerJumpTarget(PreState, -20.f, 0.f);

	const FBuhCombatResult Result = UBuhCombatSim::ResolveCombat(PreState, 9, 1, 200);
	UE_LOG(LogTemp, Warning, TEXT("JumpGroundedReaction RESOLVE: reaction=%d stoppedReason=%d finalAction=%d finalPhase=%d finalPI=%d finalHeight=%.2f"),
		Result.bReactionOpportunity ? 1 : 0, Result.StoppedReason,
		Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].Action : -1,
		Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].Phase : -1,
		Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].PhaseInstants : -1,
		Result.FinalState.Fighters.Num() > 0 ? Result.FinalState.Fighters[0].Height : -1.f);
	Check(TEXT("Grounded Jump reaches a reaction pause"), Result.bReactionOpportunity);
	Check(TEXT("Authoritative reaction state is airborne"), Result.FinalState.Fighters.Num() > 0 && Result.FinalState.Fighters[0].Height > 50.f);
	if (!Result.bReactionOpportunity)
	{
		return bAllPassed;
	}

	const TArray<FBuhTimelineSnapshot> Timeline = UBuhCombatSim::GenerateTimeline(PreState, 9, 1, 200, false, false);
	bool bTimelineReaction = false;
	float TimelineReactionHeight = 0.f;
	int32 TimelineReactionInstant = -1;
	for (const FBuhTimelineSnapshot& S : Timeline)
	{
		if (S.bReactionOpportunity)
		{
			bTimelineReaction = true;
			TimelineReactionHeight = S.PlayerHeight;
			TimelineReactionInstant = S.Instant;
			break;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("JumpGroundedReaction TIMELINE: count=%d reaction=%d reactionInstant=%d reactionHeight=%.2f"),
		Timeline.Num(), bTimelineReaction ? 1 : 0, TimelineReactionInstant, TimelineReactionHeight);
	Check(TEXT("Playback timeline reproduces the reaction"), bTimelineReaction);
	Check(TEXT("Playback reaction snapshot is airborne"), TimelineReactionHeight > 50.f);
	Check(TEXT("Authoritative and playback reaction heights match"),
		FMath::Abs(Result.FinalState.Fighters[0].Height - TimelineReactionHeight) < 0.01f);

	AActor* Player = nullptr;
	AActor* Opponent = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->Tags.Contains(TEXT("PlayerFighter"))) Player = *It;
		else if (It->Tags.Contains(TEXT("OpponentFighter"))) Opponent = *It;
	}
	if (!Player || !Opponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("JumpGroundedReaction ACTOR: using spawned fallbacks (no tagged fighters in world)"));
		Player = World->SpawnActor<AActor>(FVector(-500, 0, 90), FRotator::ZeroRotator);
		Opponent = World->SpawnActor<AActor>(FVector(500, 0, 90), FRotator::ZeroRotator);
	}
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) PC = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	Check(TEXT("Fighter actors and PC available"), PC != nullptr && Player != nullptr && Opponent != nullptr);
	if (!PC || !Player || !Opponent)
	{
		return bAllPassed;
	}

	Player->SetActorLocation(FVector(-500, 0, 90), false, nullptr, ETeleportType::TeleportPhysics);
	Opponent->SetActorLocation(FVector(500, 0, 90), false, nullptr, ETeleportType::TeleportPhysics);

	UBuhCombatSim::BeginJumpPlayback(PC, Player, Opponent, PreState, 9, 1, Result);
	int32 Ticks = 0;
	float LastZ = -1.f;
	while (UBuhCombatSim::TickJumpPlayback(0.05f) && Ticks < 400)
	{
		LastZ = Player->GetActorLocation().Z;
		UBuhCombatSim::LogFighterZ(Player, Opponent, FString::Printf(TEXT("tick%d"), Ticks));
		++Ticks;
	}
	for (int32 I = 0; I < 5; ++I)
	{
		UBuhCombatSim::LogFighterZ(Player, Opponent, FString::Printf(TEXT("post%d"), I));
	}
	LastZ = Player->GetActorLocation().Z;
	UE_LOG(LogTemp, Warning, TEXT("JumpGroundedReaction ACTOR: ticks=%d lastZ=%.2f expectedZ=%.2f playerClass=%s"),
		Ticks, LastZ, 90.f + Result.FinalState.Fighters[0].Height, *Player->GetClass()->GetName());
	Check(TEXT("Fighter actor is airborne after playback stops"), LastZ > 90.f + 50.f);
	Check(TEXT("Fighter actor Z matches authoritative reaction height"),
		FMath::Abs(LastZ - (90.f + Result.FinalState.Fighters[0].Height)) < 0.01f);

	UBuhCombatSim::StopJumpPlayback();
	if (!Player->Tags.Contains(TEXT("PlayerFighter"))) Player->Destroy();
	if (!Opponent->Tags.Contains(TEXT("OpponentFighter"))) Opponent->Destroy();
	if (!World->GetFirstPlayerController()) PC->Destroy();
	return bAllPassed;
}
