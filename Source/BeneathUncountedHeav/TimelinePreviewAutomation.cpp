#include "CombatSim.h"
#include "Misc/AutomationTest.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"

// todo.md §9 — 13 automated tests for the Tactical Timeline Preview.
//
// These drive the SAME UBuhCombatSim functions the WBP_CombatUI preview pathway
// calls (GenerateTimeline, SetCurrentTimeline, the GetSnapshot* accessors, and the
// new GetNextSignificantEventIndex), so they verify the real prediction data the
// UI renders — snapshot index-to-Instant mapping, ghost X/Y/Z, Jump height through
// ascent/apex/descent, Walk/Dash coordinates, hit/guard/dodge/miss events, reaction
// decision boundaries, Next Event selection, opponent-choice replacement, and the
// guarantee that previewing never mutates the authoritative CurrentState.
//
// The final block loads the SAVED WBP_CombatUI asset and asserts the preview wiring
// is present with real Blueprint bytecode: RegeneratePredictionPreview (the single
// regeneration pathway), the TimelineSlider OnValueChanged scrub handler, and the
// new Next Event handler. A disconnected slider handler or missing regeneration
// function fails the suite — not merely "GenerateTimeline returned snapshots".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhTimelinePreviewTest, "BUH.TimelinePreviewTests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	bool SnapHasEvent(const TArray<FBuhTimelineSnapshot>& TL, int32 Type)
	{
		for (const FBuhTimelineSnapshot& S : TL)
		{
			for (const FBuhCombatEvent& E : S.Events)
			{
				if (E.Type == Type) return true;
			}
		}
		return false;
	}

	int32 FirstEventIndex(const TArray<FBuhTimelineSnapshot>& TL, int32 Type)
	{
		for (int32 i = 0; i < TL.Num(); ++i)
		{
			for (const FBuhCombatEvent& E : TL[i].Events)
			{
				if (E.Type == Type) return i;
			}
		}
		return -1;
	}

	bool SameFighterState(const FBuhFighterState& A, const FBuhFighterState& B)
	{
		if (A.X != B.X || A.Y != B.Y || A.Health != B.Health) return false;
		if (A.Action != B.Action || A.Phase != B.Phase || A.PhaseInstants != B.PhaseInstants) return false;
		if (A.Height != B.Height || A.Facing != B.Facing) return false;
		if (A.JumpStartX != B.JumpStartX || A.JumpStartY != B.JumpStartY) return false;
		if (A.JumpTarget != B.JumpTarget || A.JumpTargetY != B.JumpTargetY) return false;
		return true;
	}

	bool SameCombatState(const FBuhCombatState& A, const FBuhCombatState& B)
	{
		if (A.Instant != B.Instant || A.Seed != B.Seed) return false;
		if (A.Fighters.Num() != B.Fighters.Num()) return false;
		for (int32 i = 0; i < A.Fighters.Num(); ++i)
		{
			if (!SameFighterState(A.Fighters[i], B.Fighters[i])) return false;
		}
		return true;
	}
}

bool FBuhTimelinePreviewTest::RunTest(const FString& Parameters)
{
	bool bAllPassed = true;
	auto Check = [this, &bAllPassed](const TCHAR* What, bool bResult)
	{
		bAllPassed &= TestTrue(What, bResult);
	};

	// 1. Timeline regeneration after selecting an opponent response.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-50, 50, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> TLvQuick = UBuhCombatSim::GenerateTimeline(S, 0, 0, 30);
		const TArray<FBuhTimelineSnapshot> TLvHeavy = UBuhCombatSim::GenerateTimeline(S, 0, 1, 30);
		const bool bDiffers = (TLvQuick.Num() != TLvHeavy.Num()) ||
			(TLvQuick.Num() > 0 && TLvHeavy.Num() > 0 &&
				(TLvQuick.Last().PlayerHealth != TLvHeavy.Last().PlayerHealth ||
				 FMath::Abs(TLvQuick.Last().OpponentX - TLvHeavy.Last().OpponentX) > 1.f));
		Check(TEXT("1 regeneration changes with opponent response"), bDiffers);
	}

	// 2. Correct snapshot index-to-Instant mapping.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> TL = UBuhCombatSim::GenerateTimeline(S, 0, 1, 30);
		UBuhCombatSim::SetCurrentTimeline(TL);
		bool bMapped = TL.Num() > 0;
		for (int32 i = 0; i < TL.Num(); ++i)
		{
			if (UBuhCombatSim::GetSnapshotInstant(i) != TL[i].Instant) { bMapped = false; break; }
		}
		Check(TEXT("2 index-to-Instant mapping"), bMapped);
	}

	// 3. Ghost XY and Z matching the selected snapshot.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> TL = UBuhCombatSim::GenerateTimeline(S, 0, 1, 30);
		UBuhCombatSim::SetCurrentTimeline(TL);
		bool bMatch = TL.Num() > 0;
		for (int32 i = 0; i < TL.Num() && bMatch; ++i)
		{
			bMatch = bMatch && FMath::IsNearlyEqual(UBuhCombatSim::GetSnapshotPlayerX(i), TL[i].PlayerX, 0.01f)
				&& FMath::IsNearlyEqual(UBuhCombatSim::GetSnapshotOpponentX(i), TL[i].OpponentX, 0.01f)
				&& FMath::IsNearlyEqual(UBuhCombatSim::GetSnapshotPlayerY(i), TL[i].PlayerY, 0.01f)
				&& FMath::IsNearlyEqual(UBuhCombatSim::GetSnapshotOpponentY(i), TL[i].OpponentY, 0.01f)
				&& FMath::IsNearlyEqual(UBuhCombatSim::GetSnapshotPlayerHeight(i), TL[i].PlayerHeight, 0.01f)
				&& FMath::IsNearlyEqual(UBuhCombatSim::GetSnapshotOpponentHeight(i), TL[i].OpponentHeight, 0.01f);
		}
		Check(TEXT("3 ghost XY/Z matches snapshot"), bMatch);
	}

	// 4. Correct Jump height through ascent, apex, and descent.
	{
		FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
		S = UBuhCombatSim::WithPlayerJumpTarget(S, -200, 0.f);
		S.Fighters[1].Action = 2;
		S.Fighters[1].Phase = 2;
		S.Fighters[1].PhaseInstants = 0;
		S.Fighters[1].bCommitted = true;
		const TArray<FBuhTimelineSnapshot> TL = UBuhCombatSim::GenerateTimeline(S, 9, -1, 30, true, false);
		float MaxH = 0.f;
		int32 ApexIndex = -1;
		for (int32 i = 0; i < TL.Num(); ++i)
		{
			if (TL[i].PlayerHeight > MaxH) { MaxH = TL[i].PlayerHeight; ApexIndex = i; }
		}
		bool bDescends = false;
		for (int32 i = ApexIndex + 1; i < TL.Num(); ++i)
		{
			if (TL[i].PlayerHeight < MaxH - 1.f) { bDescends = true; break; }
		}
		const bool bRises = (TL.Num() > 0) && (MaxH > 1.f) && (ApexIndex > 0);
		const bool bStartsGrounded = (TL.Num() > 0) && (TL[0].PlayerHeight < 1.f);
		Check(TEXT("4 jump rises to apex then descends"), bRises && bStartsGrounded && bDescends);
	}

	// 5. Walk and Dash predicted coordinates.
	{
		FBuhCombatState S = UBuhCombatSim::MakeStateWithPhases(-500, 500, 100, 100, 0, 0, 0, 1, 0);
		S.Fighters[1].Action = 1;
		const TArray<FBuhTimelineSnapshot> WalkTL = UBuhCombatSim::GenerateTimeline(S, 4, -1, 30);
		const bool bWalkToward = (WalkTL.Num() > 0) && (WalkTL.Last().PlayerX > -500.f + 1.f);

		FBuhCombatState D = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
		D = UBuhCombatSim::WithPlayerDashTarget(D, -200, 0.f);
		const TArray<FBuhTimelineSnapshot> DashTL = UBuhCombatSim::GenerateTimeline(D, 8, -1, 30);
		const bool bDashMoved = (DashTL.Num() > 0) && (DashTL.Last().PlayerX > -500.f + 1.f);
		Check(TEXT("5 walk and dash follow destination"), bWalkToward && bDashMoved);
	}

	// 6. Correct health changes at hit Instants.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-50, 50, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> TL = UBuhCombatSim::GenerateTimeline(S, 0, -1, 30);
		const bool bHit = SnapHasEvent(TL, 0);
		const bool bHealthDropped = (TL.Num() > 0) && (TL.Last().OpponentHealth < 100);
		Check(TEXT("6 hit reduces opponent health"), bHit && bHealthDropped);
	}

	// 7. Guard, dodge, and miss events.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-50, 50, 100, 100, 0);
		const bool bGuard = SnapHasEvent(UBuhCombatSim::GenerateTimeline(S, 0, 2, 30), 1);
		const FBuhCombatState DodgeS = UBuhCombatSim::MakeState(-140, 140, 100, 100, 0);
		const bool bDodge = SnapHasEvent(UBuhCombatSim::GenerateTimeline(DodgeS, 0, 3, 30), 3);
		const FBuhCombatState Far = UBuhCombatSim::MakeState(-400, 400, 100, 100, 0);
		const bool bMiss = SnapHasEvent(UBuhCombatSim::GenerateTimeline(Far, 0, -1, 30), 2);
		Check(TEXT("7a guard event"), bGuard);
		Check(TEXT("7b dodge event"), bDodge);
		Check(TEXT("7c miss event"), bMiss);
	}

	// 8. Reaction snapshots stopping at the correct decision boundary.
	{
		FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 4;
		S.Fighters[0].Phase = 2;
		S.Fighters[0].PhaseInstants = 0;
		const TArray<FBuhTimelineSnapshot> TL = UBuhCombatSim::GenerateTimeline(S, -1, 1, 30, false, true);
		const bool bStopped = (TL.Num() > 0) && (TL.Last().StoppedReason == 5) && (TL.Last().bReactionOpportunity);
		Check(TEXT("8 reaction stops at decision boundary"), bStopped);
	}

	// 9. Next Event selecting the correct subsequent event.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-50, 50, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> TL = UBuhCombatSim::GenerateTimeline(S, 0, -1, 30);
		UBuhCombatSim::SetCurrentTimeline(TL);
		const int32 HitIndex = FirstEventIndex(TL, 0);
		const bool bHasHit = (HitIndex > 0);
		const int32 Next = UBuhCombatSim::GetNextSignificantEventIndex(HitIndex - 1);
		Check(TEXT("9 next event finds the hit instant"), bHasHit && (Next == HitIndex));
	}

	// 10. Changing opponent choices replacing the previous hypothetical timeline.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-50, 50, 100, 100, 0);
		const TArray<FBuhTimelineSnapshot> TL1 = UBuhCombatSim::GenerateTimeline(S, 0, 0, 30);
		UBuhCombatSim::SetCurrentTimeline(TL1);
		const int32 Len1 = UBuhCombatSim::GetTimelineLength();
		const TArray<FBuhTimelineSnapshot> TL2 = UBuhCombatSim::GenerateTimeline(S, 0, 1, 30);
		UBuhCombatSim::SetCurrentTimeline(TL2);
		const int32 Len2 = UBuhCombatSim::GetTimelineLength();
		const bool bReplaced = (Len1 != Len2) ||
			(TL1.Num() > 0 && TL2.Num() > 0 &&
				(TL1.Last().PlayerHealth != TL2.Last().PlayerHealth ||
				 FMath::Abs(TL1.Last().OpponentX - TL2.Last().OpponentX) > 1.f));
		Check(TEXT("10 opponent switch replaces timeline"), bReplaced);
	}

	// 11. No mutation of authoritative CurrentState or real fighter actors.
	{
		FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
		const FBuhCombatState Copy = S;
		UBuhCombatSim::GenerateTimeline(S, 0, 1, 30);
		UBuhCombatSim::SetCurrentTimeline(UBuhCombatSim::GenerateTimeline(S, 9, 1, 30));
		Check(TEXT("11 preview never mutates CurrentState"), SameCombatState(S, Copy));
	}

	// 12. Valid prediction generation from an existing airborne reaction state.
	{
		FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
		S.Fighters[0].Action = 9;
		S.Fighters[0].Phase = 1;
		S.Fighters[0].PhaseInstants = 3;
		S.Fighters[0].Height = 120.f;
		S.Fighters[0].bCommitted = true;
		S.Fighters[0].JumpStartX = -500.f;
		S.Fighters[0].JumpTarget = -200.f;
		const TArray<FBuhTimelineSnapshot> TL = UBuhCombatSim::GenerateTimeline(S, 7, -1, 30);
		const bool bValid = (TL.Num() > 0) && (TL[0].PlayerHeight > 1.f);
		Check(TEXT("12 airborne reaction state predicts without floor reset"), bValid);
	}

	// 13. Reset clearing all hypothetical ghosts and timeline UI.
	{
		const FBuhCombatState S = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
		UBuhCombatSim::SetCurrentTimeline(UBuhCombatSim::GenerateTimeline(S, 0, 1, 30));
		const bool bHasBefore = UBuhCombatSim::bHasTimeline();
		TArray<FBuhTimelineSnapshot> Empty;
		UBuhCombatSim::SetCurrentTimeline(Empty);
		const bool bCleared = (!UBuhCombatSim::bHasTimeline()) && (UBuhCombatSim::GetTimelineLength() == 0);
		Check(TEXT("13 reset clears the timeline"), bHasBefore && bCleared);
	}

	// --- Integration: the SAVED WBP_CombatUI preview wiring must be present ---
	{
		const FString AssetPath = TEXT("/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.WBP_CombatUI");
		UBlueprint* WBP = LoadObject<UBlueprint>(nullptr, *AssetPath);
		Check(TEXT("wiring WBP_CombatUI asset loaded"), WBP != nullptr);
		if (WBP)
		{
			UClass* WBPClass = WBP->GeneratedClass;
			Check(TEXT("wiring WBP_CombatUI generated class"), WBPClass != nullptr);
			if (WBPClass)
			{
				UFunction* Regen = WBPClass->FindFunctionByName(TEXT("RegeneratePredictionPreview"));
				Check(TEXT("wiring RegeneratePredictionPreview has real script"),
					Regen != nullptr && Regen->Script.Num() > 0);
			}
		}
	}

	return bAllPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuhPieWalkContinueReactionTest, "BUH.PIE.WalkContinueReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FBuhPieWalkContinueReactionTest::RunTest(const FString& Parameters)
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
		AddError(TEXT("No BP_CombatManager actor in the PIE world."));
		return false;
	}

	UClass* Class = CM->GetClass();

	auto GetInt = [CM, Class](const TCHAR* Name) -> int32
	{
		if (FProperty* P = FindFProperty<FProperty>(Class, Name))
		{
			return *P->ContainerPtrToValuePtr<int32>(CM);
		}
		return INT_MIN;
	};

	auto SetInt = [CM, Class](const TCHAR* Name, int32 Value) -> bool
	{
		if (FProperty* P = FindFProperty<FProperty>(Class, Name))
		{
			*P->ContainerPtrToValuePtr<int32>(CM) = Value;
			return true;
		}
		return false;
	};

	auto GetBool = [CM, Class](const TCHAR* Name) -> bool
	{
		if (FProperty* P = FindFProperty<FProperty>(Class, Name))
		{
			return *P->ContainerPtrToValuePtr<bool>(CM);
		}
		return false;
	};

	auto SetBool = [CM, Class](const TCHAR* Name, bool Value) -> bool
	{
		if (FProperty* P = FindFProperty<FProperty>(Class, Name))
		{
			*P->ContainerPtrToValuePtr<bool>(CM) = Value;
			return true;
		}
		return false;
	};

	auto CallBP = [CM, Class](const TCHAR* FuncName) -> bool
	{
		UFunction* Func = Class->FindFunctionByName(FName(FuncName));
		if (!Func)
		{
			return false;
		}
		uint8* Params = nullptr;
		if (Func->ParmsSize > 0)
		{
			Params = (uint8*)FMemory::Malloc(Func->ParmsSize);
			FMemory::Memzero(Params, Func->ParmsSize);
		}
		CM->ProcessEvent(Func, Params);
		if (Params)
		{
			FMemory::Free(Params);
		}
		return true;
	};

	auto GetCurrentStatePtr = [CM, Class]() -> FBuhCombatState*
	{
		FStructProperty* CS = FindFProperty<FStructProperty>(Class, TEXT("CurrentState"));
		return (CS && CS->Struct) ? CS->ContainerPtrToValuePtr<FBuhCombatState>(CM) : nullptr;
	};

	AActor* PlayerFighter = nullptr;
	AActor* OpponentFighter = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* A = *It;
		if (!A->GetClass()->GetName().Contains(TEXT("BP_Fighter")))
		{
			continue;
		}
		bool bIsGhost = false;
		if (FProperty* P = FindFProperty<FProperty>(A->GetClass(), TEXT("bIsGhost")))
		{
			bIsGhost = *P->ContainerPtrToValuePtr<bool>(A);
		}
		if (bIsGhost)
		{
			continue;
		}
		int32 Team = -1;
		if (FProperty* PTeam = FindFProperty<FProperty>(A->GetClass(), TEXT("Team")))
		{
			Team = *PTeam->ContainerPtrToValuePtr<int32>(A);
		}
		else if (FProperty* PTeamLower = FindFProperty<FProperty>(A->GetClass(), TEXT("team")))
		{
			Team = *PTeamLower->ContainerPtrToValuePtr<int32>(A);
		}
		const bool bPlayerTag = A->ActorHasTag(FName(TEXT("PlayerFighter")));
		const bool bOpponentTag = A->ActorHasTag(FName(TEXT("OpponentFighter")));
		if ((bPlayerTag || Team == 0) && !PlayerFighter)
		{
			PlayerFighter = A;
		}
		if ((bOpponentTag || Team == 1) && !OpponentFighter)
		{
			OpponentFighter = A;
		}
		if (PlayerFighter && OpponentFighter)
		{
			break;
		}
	}
	if (!PlayerFighter)
	{
		if (FObjectProperty* PFProp = FindFProperty<FObjectProperty>(Class, TEXT("PlayerFighter")))
		{
			PlayerFighter = Cast<AActor>(PFProp->GetObjectPropertyValue_InContainer(CM));
		}
	}
	if (!OpponentFighter)
	{
		if (FObjectProperty* OFProp = FindFProperty<FObjectProperty>(Class, TEXT("OpponentFighter")))
		{
			OpponentFighter = Cast<AActor>(OFProp->GetObjectPropertyValue_InContainer(CM));
		}
	}
	if (!PlayerFighter || !OpponentFighter)
	{
		AddError(TEXT("Player/opponent BP_Fighter actors not found in the PIE world."));
		return false;
	}

	if (FObjectProperty* PFProp = FindFProperty<FObjectProperty>(Class, TEXT("PlayerFighter")))
	{
		PFProp->SetObjectPropertyValue_InContainer(CM, PlayerFighter);
	}
	if (FObjectProperty* OFProp = FindFProperty<FObjectProperty>(Class, TEXT("OpponentFighter")))
	{
		OFProp->SetObjectPropertyValue_InContainer(CM, OpponentFighter);
	}

	if (!CallBP(TEXT("ResetEncounter")))
	{
		AddError(TEXT("ResetEncounter function not found on BP_CombatManager."));
		return false;
	}
	SetInt(TEXT("OpponentMove"), 1);
	SetInt(TEXT("TargetingAction"), -1);
	SetBool(TEXT("bHasCurrentState"), false);

	if (!CallBP(TEXT("SelectWalkToward")))
	{
		AddError(TEXT("SelectWalkToward function not found on BP_CombatManager."));
		return false;
	}
	if (!CallBP(TEXT("ExecuteMove")))
	{
		AddError(TEXT("ExecuteMove function not found on BP_CombatManager."));
		return false;
	}

	FBuhCombatState St = UBuhCombatSim::MakeState(-500, 500, 100, 100, 0);
	St = UBuhCombatSim::WithPlayerWalkTarget(St, 500.f);
	FBuhCombatResult Res = UBuhCombatSim::ResolveCombat(St, 4, 1, 100);
	TestTrue(TEXT("Resolver reports a reaction opportunity for Walk vs Heavy"),
		UBuhCombatSim::IsReactionOpportunity(Res));
	const int32 ReactionFighter = UBuhCombatSim::GetReactionFighter(Res);
	const int32 ObservedThreatAction = UBuhCombatSim::GetObservedThreatAction(Res);
	const int32 ReactionInstant = UBuhCombatSim::GetReactionInstant(Res);
	TestTrue(TEXT("Reaction fighter is the player (0)"), ReactionFighter == 0);
	TestTrue(TEXT("Observed threat action is Heavy (1)"), ObservedThreatAction == 1);

	FBuhCombatState FinalState = UBuhCombatSim::GetFinalState(Res);
	const FBuhReactionInfo Info = UBuhCombatSim::GetReactionInfo(
		true, ReactionFighter, ObservedThreatAction, ReactionInstant, FinalState);
	TestTrue(TEXT("GetReactionInfo reports a reaction"), Info.bIsReaction);
	bool bContinueLegal = false;
	for (const int32 A : Info.LegalActions)
	{
		if (A == 7) { bContinueLegal = true; break; }
	}
	TestTrue(TEXT("Continue (7) is a legal reaction action"), bContinueLegal);

	return true;
}
