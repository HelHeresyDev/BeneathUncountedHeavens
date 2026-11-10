#include "PathRenderer.h"
#include "CombatSim.h"
#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"
#include "UObject/Field.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

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

	UEdGraphNode* CompareNode = FindGraphNodeByName(Graph, TEXT("K2Node_MacroInstance_24"));
	Check(TEXT("CompareInt gate node exists"), CompareNode != nullptr);
	Check(TEXT("Old IfThenElse_0 gate removed"), FindGraphNodeByName(Graph, TEXT("K2Node_IfThenElse_0")) == nullptr);

	UEdGraphNode* BranchNode = FindGraphNodeByName(Graph, TEXT("K2Node_IfThenElse_23"));
	Check(TEXT("Execution Branch node exists"), BranchNode != nullptr);

	UEdGraphPin* LessPin = FindGraphPin(CompareNode, TEXT("<"), EGPD_Output);
	Check(TEXT("CompareInt has < output pin"), LessPin != nullptr);
	Check(TEXT("< output feeds execution Branch"), IsPinLinkedToNode(LessPin, BranchNode));

	UEdGraphPin* BranchExec = FindGraphPin(BranchNode, TEXT("execute"), EGPD_Input);
	Check(TEXT("Branch execute pin exists"), BranchExec != nullptr);
	Check(TEXT("Branch execute comes from CompareInt <"), IsPinLinkedToNode(BranchExec, CompareNode));

	UEdGraphNode* TargetingGet = FindGraphNodeByName(Graph, TEXT("K2Node_VariableGet_117"));
	Check(TEXT("TargetingAction get node exists"), TargetingGet != nullptr);
	UEdGraphPin* CompareInput = FindGraphPin(CompareNode, TEXT("Input"), EGPD_Input);
	Check(TEXT("CompareInt Input comes from TargetingAction"), IsPinLinkedToNode(CompareInput, TargetingGet));

	UEdGraphNode* ZeroLiteral = FindGraphNodeByName(Graph, TEXT("K2Node_CallFunction_230"));
	Check(TEXT("Zero literal node exists"), ZeroLiteral != nullptr);
	UEdGraphPin* CompareWith = FindGraphPin(CompareNode, TEXT("CompareWith"), EGPD_Input);
	Check(TEXT("CompareInt CompareWith comes from zero literal"), IsPinLinkedToNode(CompareWith, ZeroLiteral));

	UEdGraphNode* OldOperator = FindGraphNodeByName(Graph, TEXT("K2Node_PromotableOperator_38"));
	Check(TEXT("Old Equal operator still exists for targeting branch"), OldOperator != nullptr);
	UEdGraphPin* OldOut = FindGraphPin(OldOperator, TEXT("ReturnValue"), EGPD_Output);
	Check(TEXT("Old Equal operator no longer feeds execution gate"), OldOut && !IsPinLinkedToNode(OldOut, BranchNode));

	UEdGraphNode* DashDraw = FindGraphNodeByName(Graph, TEXT("K2Node_CallFunction_272"));
	Check(TEXT("Execution DrawDashPath node exists"), DashDraw != nullptr);
	UEdGraphPin* BranchThen = FindGraphPin(BranchNode, TEXT("then"), EGPD_Output);
	Check(TEXT("Branch then feeds execution DrawDashPath"), IsPinLinkedToNode(BranchThen, DashDraw));

	return bAllPassed;
}
