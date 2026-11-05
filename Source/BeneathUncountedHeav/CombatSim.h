#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatSim.generated.h"

class UWidget;

USTRUCT(BlueprintType)
struct BENEATHUNCOUNTEDHEAV_API FBuhFighterState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Health = 100;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float X = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Action = -1;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Phase = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 PhaseInstants = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Team = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Facing = 1;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") bool bCommitted = false;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 PendingAction = -1;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 WalkDir = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 WalkRemaining = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float WalkTarget = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 ReactedThreatFighter = -1;
};

USTRUCT(BlueprintType)
struct BENEATHUNCOUNTEDHEAV_API FBuhCombatState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") TArray<FBuhFighterState> Fighters;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Instant = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Seed = 0;
};

USTRUCT(BlueprintType)
struct BENEATHUNCOUNTEDHEAV_API FBuhCombatEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Instant = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Type = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Fighter = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Damage = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") FString Detail;
};

USTRUCT(BlueprintType)
struct BENEATHUNCOUNTEDHEAV_API FBuhCombatResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") FBuhCombatState FinalState;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") TArray<FBuhCombatEvent> Events;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") bool bPlayerReady = false;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") bool bOpponentReady = false;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 StoppedReason = 0;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float MinDistanceSeen = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") bool bReactionOpportunity = false;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 ReactionInstant = -1;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 ReactionFighter = -1;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 ObservedThreatAction = -1;
};

USTRUCT(BlueprintType)
struct BENEATHUNCOUNTEDHEAV_API FBuhTimelineSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 Instant = 0;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") float PlayerX = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") float OpponentX = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 PlayerFacing = 1;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 OpponentFacing = -1;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 PlayerAction = -1;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 OpponentAction = -1;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 PlayerPhase = 0;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 OpponentPhase = 0;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 PlayerPhaseInstants = 0;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 OpponentPhaseInstants = 0;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 PlayerHealth = 100;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 OpponentHealth = 100;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") bool bPlayerReady = false;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") bool bOpponentReady = false;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") bool bReactionOpportunity = false;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 ReactionFighter = -1;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 ObservedThreatAction = -1;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") TArray<FBuhCombatEvent> Events;
};

USTRUCT(BlueprintType)
struct BENEATHUNCOUNTEDHEAV_API FBuhReactionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") bool bIsReaction = false;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 CurrentInstant = 0;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 ObservedThreatAction = -1;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 PlayerCurrentAction = -1;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 ThreatRemainingPrep = 0;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") TArray<int32> LegalActions;
};

UCLASS()
class BENEATHUNCOUNTEDHEAV_API UBuhCombatSim : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatResult ResolveCombat(FBuhCombatState InState, int32 PlayerDecision, int32 OpponentDecision, int32 MaxInstants, bool bSuppressReactionPause = false, bool bSuppressPlayerReadyBreak = false);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState MakeState(int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState MakeStateWithPhases(int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed, int32 PlayerPhase, int32 PlayerPhaseInstants, int32 OpponentPhase, int32 OpponentPhaseInstants);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState WithPlayerWalkTarget(const FBuhCombatState& InState, float TargetX);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString RunAllTests();

	static void RunTimelineTests11to17(FString& Report, int32& Passed, int32& Failed);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalPlayerX(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalOpponentX(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalPlayerHealth(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalOpponentHealth(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalPlayerPhase(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalPlayerPhaseInstants(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalOpponentPhase(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalOpponentPhaseInstants(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState GetFinalState(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static bool IsReactionOpportunity(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetReactionInstant(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetReactionFighter(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetObservedThreatAction(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhReactionInfo GetReactionInfo(
		bool bReactionOpportunity,
		int32 ReactionFighter,
		int32 ObservedThreatAction,
		int32 ReactionInstant,
		const FBuhCombatState& CurrentState);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString PredictText(const FBuhCombatState& InState, int32 PlayerDecision, int32 OpponentDecision);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static TArray<FBuhTimelineSnapshot> GenerateTimeline(const FBuhCombatState& InState, int32 PlayerDecision, int32 OpponentDecision, int32 MaxInstants, bool bSuppressPlayerReadyBreak = false);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetCurrentTimeline(const TArray<FBuhTimelineSnapshot>& InTimeline);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhTimelineSnapshot GetSnapshotAt(int32 Instant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetTimelineLength();

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static bool bHasTimeline();

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetSnapshotInstant(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetSnapshotPlayerX(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetSnapshotOpponentX(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetSnapshotPlayerFacing(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetSnapshotOpponentFacing(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetSnapshotPlayerHealth(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetSnapshotOpponentHealth(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetSnapshotPlayerPhase(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetSnapshotOpponentPhase(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetSnapshotInstantText(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetSnapshotInfoText(int32 Index);

	// Deterministic opponent AI. Chooses from the same four moves as the player
	// (0 Quick, 1 Heavy, 2 Guard, 3 Dodge) using only observable state: distance,
	// healths, the opponent's own readiness, and the player's LAST revealed move.
	// It never reads the player's current secret action (State.Fighters[0].Action).
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 ChooseOpponentMove(const FBuhCombatState& State, int32 LastPlayerMove, int32 Seed);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void MoveGhostsToInstant(int32 Instant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 MoveGhostsToFinalInstant();

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalInstantIndex();

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetupTimelineAndMoveGhosts(int32 PlayerMove, int32 OpponentMove, int32 Instant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetMoveNameText(int32 Move);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetMoveDurationText(int32 Move);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetBranchSummaryText(int32 PlayerMove, int32 OpponentMove, int32 CurrentInstant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetPreviewWalkTarget(float TargetX);

	static float GPlayerWalkTarget;

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetTimelineLengthText(int32 CurrentInstant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetPreviewDistanceText(int32 Instant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 ComputeOpponentMoveWithVars(
		bool bHasCurrentState,
		const FBuhCombatState& CurrentState,
		int32 PlayerX,
		int32 OpponentX,
		int32 PlayerHealth,
		int32 OpponentHealth,
		int32 Seed,
		int32 StoredOpponentMove,
		int32 LastPlayerMove);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatResult ExecuteMoveWithVars(
		bool bHasCurrentState,
		const FBuhCombatState& CurrentState,
		int32 PlayerX,
		int32 OpponentX,
		int32 PlayerHealth,
		int32 OpponentHealth,
		int32 Seed,
		int32 PlayerMove,
		int32 OpponentMove,
		float WalkTarget);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void PrintWidgetGeometry(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetSlotLayoutOnAsset(FName WidgetName, float MinX, float MinY, float MaxX, float MaxY, float AlignX, float AlignY, float Left, float Top, float Right, float Bottom, bool bAutoSize);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void LogFighterMaterial(AActor* Fighter, int32 ExpectedTeam);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetBlueMaterialOnFighter(AActor* Fighter);
};
