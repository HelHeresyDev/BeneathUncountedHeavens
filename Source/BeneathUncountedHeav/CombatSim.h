#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatSim.generated.h"

class UWidget;
class UUserWidget;

USTRUCT(BlueprintType)
struct BENEATHUNCOUNTEDHEAV_API FBuhFighterState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") int32 Health = 100;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float X = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float Y = 0.f;
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
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float WalkTargetY = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float DashTarget = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float DashTargetY = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float Height = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float JumpStartX = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float JumpStartY = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float JumpTarget = -9999.f;
	UPROPERTY(BlueprintReadWrite, Category = "CombatSim") float JumpTargetY = 0.f;
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
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") float PlayerY = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") float OpponentY = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") float PlayerHeight = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") float OpponentHeight = 0.f;
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
	UPROPERTY(BlueprintReadOnly, Category = "CombatSim") int32 StoppedReason = 0;
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
	static FBuhCombatState MakeState(int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed, float PlayerY = 0.f, float OpponentY = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState MakeStateWithPhases(int32 PlayerX, int32 OpponentX, int32 PlayerHealth, int32 OpponentHealth, int32 Seed, int32 PlayerPhase, int32 PlayerPhaseInstants, int32 OpponentPhase, int32 OpponentPhaseInstants, float PlayerY = 0.f, float OpponentY = 0.f, int32 PlayerAction = -1, int32 OpponentAction = -1);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState WithPlayerWalkTarget(const FBuhCombatState& InState, float TargetX, float TargetY = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState WithPlayerWalkTargetFromPreview(const FBuhCombatState& InState);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState WithPlayerDashTarget(const FBuhCombatState& InState, float TargetX, float TargetY = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState WithPlayerDashTargetFromPreview(const FBuhCombatState& InState);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState WithPlayerJumpTarget(const FBuhCombatState& InState, float TargetX, float TargetY = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState WithPlayerJumpTargetFromPreview(const FBuhCombatState& InState);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString RunAllTests();

	static void RunTimelineTests11to17(FString& Report, int32& Passed, int32& Failed);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalPlayerX(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetFinalOpponentX(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetFinalPlayerY(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetFinalOpponentY(const FBuhCombatResult& Result);

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

	// Reads the manager's reaction properties via reflection and calls GetReactionInfo.
	// Unambiguous single call that avoids the Blueprint global name-search collisions
	// between the manager's auto-generated property getters and CombatSim/CustomizableObjectInstance.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhReactionInfo GetReactionInfoFromManager(UObject* Manager);

	// Returns "D: <distance>" using the real fighter positions, with safe actor
	// validation. Returns "D: ?" when either fighter is missing (editor world).
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetActualDistanceText(UObject* Manager);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString PredictText(const FBuhCombatState& InState, int32 PlayerDecision, int32 OpponentDecision);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static TArray<FBuhTimelineSnapshot> GenerateTimeline(const FBuhCombatState& InState, int32 PlayerDecision, int32 OpponentDecision, int32 MaxInstants, bool bSuppressPlayerReadyBreak = false, bool bStopAtDecisionBoundary = false, bool bOpponentNoAction = false);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetCurrentTimeline(const TArray<FBuhTimelineSnapshot>& InTimeline);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhTimelineSnapshot GetSnapshotAt(int32 Instant);

	// Stage 2: the full authoritative FBuhCombatState stored at snapshot index
	// (parallel to GetSnapshotAt). Empty state when out of range. Branching at an
	// Instant resumes combat from this complete state, never from snapshot fields.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FBuhCombatState GetStateAt(int32 Index);

	// Stage 2: store a full-state array in parallel with the current timeline.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetCurrentTimelineWithStates(const TArray<FBuhTimelineSnapshot>& InTimeline, const TArray<FBuhCombatState>& InStates);

	// Stage 2: branch the current timeline at snapshot index BranchIndex by having
	// the opponent begin NewOpponentAction at that Instant. The player continues
	// their in-progress action (Walk/Dash/Jump). The opponent may only begin a new
	// action when legally ready (Phase 0); otherwise bLegal=false and OutReason
	// explains why. Returns the combined timeline (history before the branch + the
	// branch from that Instant on), which is also installed as the current timeline
	// with its full states. OutBranchInstant is the Instant the branch begins at.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static TArray<FBuhTimelineSnapshot> GenerateBranchTimeline(int32 BranchIndex, int32 NewOpponentAction, int32& OutBranchInstant, bool& bLegal, FString& OutReason);

	// Stage 2: the Instant at which the current timeline branched (-1 = no branch).
	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static int32 GetBranchInstant();

	// Stage 2: store the "no opponent action" base timeline separately from the
	// current (possibly branched) timeline so repeated opponent-action branches
	// always start from the same legal base.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetBaseTimeline(const TArray<FBuhTimelineSnapshot>& InTimeline, const TArray<FBuhCombatState>& InStates);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void ClearBaseTimeline();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static bool bHasBaseTimeline();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static int32 GetBaseTimelineLength();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static TArray<FBuhTimelineSnapshot> GetBaseTimeline();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static TArray<FBuhCombatState> GetBaseTimelineStates();

	// Stage 2: restore the current timeline to the stored base (clears the branch).
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void RestoreBaseTimeline();

	// Stage 2: generate the base "no opponent action" timeline for the given state
	// and player move, store it as the base, and install it as the current timeline.
	// Returns the base snapshots.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static TArray<FBuhTimelineSnapshot> GenerateAndStoreBaseTimeline(const FBuhCombatState& InState, int32 PlayerMove, int32 MaxInstants);

	// Stage 2: branch the stored base timeline at snapshot index BranchIndex by
	// having the opponent begin NewOpponentAction at that Instant. Unlike
	// GenerateBranchTimeline, this always branches from the base, never from a
	// previous branch continuation. Installs the combined result as current.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static TArray<FBuhTimelineSnapshot> GenerateBranchFromBase(int32 BranchIndex, int32 NewOpponentAction, int32& OutBranchInstant, bool& bLegal, FString& OutReason);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static bool bHasBranchTimeline();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static int32 GetBranchTimelineLength();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static bool bHasReactionSnapshot();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static FBuhTimelineSnapshot GetReactionSnapshot();

	// Reaction preview: generate a timeline from the frozen CurrentState with
	// the player's reaction decision and no new opponent action. Installs the
	// result as the current preview timeline (does not touch the base timeline).
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static TArray<FBuhTimelineSnapshot> GenerateAndStoreReactionPreview(const FBuhCombatState& InState, int32 PlayerDecision, int32 MaxInstants);

	// Reaction preview from manager: reads CurrentState directly from the
	// CombatManager actor via property reflection, then generates and stores
	// the reaction preview timeline. Returns the snapshot count.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GenerateReactionPreviewFromManager(UObject* CombatManager, int32 PlayerDecision, int32 MaxInstants);

	// Stage 2: clear both the base and the current preview timeline.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void ClearPreview();

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetTimelineLength();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static int32 ClampInstantFromFloat(float Value);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static bool bHasTimeline();

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 GetSnapshotInstant(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetSnapshotPlayerX(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetSnapshotOpponentX(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetSnapshotPlayerY(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetSnapshotOpponentY(int32 Index);

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

	// §4 Next Event: returns the next snapshot index strictly after AfterIndex that
	// carries a significant combat event (hit/guard/miss/dodge/KO/reaction, Types
	// 0,1,2,3,5,6) or a non-zero StoppedReason (recovery completion / end of the
	// predicted sequence). Returns -1 when no such instant remains.
	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static int32 GetNextSignificantEventIndex(int32 AfterIndex);

	// Deterministic opponent AI. Chooses from the same four moves as the player
	// (0 Quick, 1 Heavy, 2 Guard, 3 Dodge) using only observable state: distance,
	// healths, the opponent's own readiness, and the player's LAST revealed move.
	// It never reads the player's current secret action (State.Fighters[0].Action).
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 ChooseOpponentMove(const FBuhCombatState& State, int32 LastPlayerMove, int32 Seed);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void MoveGhostsToInstant(UObject* WorldContextObject, int32 Instant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static int32 MoveGhostsToFinalInstant(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static int32 GetFinalInstantIndex();

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetupTimelineAndMoveGhosts(UObject* WorldContextObject, int32 PlayerMove, int32 OpponentMove, int32 Instant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetMoveNameText(int32 Move);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetMoveDurationText(int32 Move);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FString GetBranchSummaryText(int32 PlayerMove, int32 OpponentMove, int32 CurrentInstant);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetPreviewWalkTarget(float TargetX, float TargetY = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetPreviewDashTarget(float TargetX, float TargetY = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetPreviewJumpTarget(float TargetX, float TargetY = 0.f);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetPreviewWalkTarget();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetPreviewWalkTargetY();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetPreviewDashTarget();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetPreviewDashTargetY();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetPreviewJumpTarget();

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetPreviewJumpTargetY();

	static float GPlayerWalkTarget;
	static float GPlayerWalkTargetY;
	static float GPlayerDashTarget;
	static float GPlayerDashTargetY;
	static float GPlayerJumpTarget;
	static float GPlayerJumpTargetY;

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetExecDashTarget(float TargetX, float TargetY = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetExecJumpTarget(float TargetX, float TargetY = 0.f);

	static float GExecDashTarget;
	static float GExecDashTargetY;
	static float GExecJumpTarget;
	static float GExecJumpTargetY;

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
		int32 LastPlayerMove,
		float PlayerY = 0.f,
		float OpponentY = 0.f);

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
		float WalkTarget,
		float WalkTargetY = 0.f,
		float PlayerY = 0.f,
		float OpponentY = 0.f);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void PrintWidgetGeometry(UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetSlotLayoutOnAsset(FName WidgetName, float MinX, float MinY, float MaxX, float MaxY, float AlignX, float AlignY, float Left, float Top, float Right, float Bottom, bool bAutoSize);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void LogFighterMaterial(AActor* Fighter, int32 ExpectedTeam);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetBlueMaterialOnFighter(AActor* Fighter);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FVector ScreenToGround(APlayerController* PC, FVector2D ScreenPos);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FVector2D WorldToScreen(APlayerController* PC, FVector WorldPos);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static FVector2D ScreenToGroundXY(APlayerController* PC, FVector2D ScreenPos);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float ScreenToGroundX(APlayerController* PC, FVector2D ScreenPos);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float ScreenToGroundY(APlayerController* PC, FVector2D ScreenPos);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void DrawGroundLine(APlayerController* PC, FVector Start, FVector End, float Thickness, float Duration);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void DrawWalkPath(APlayerController* PC, FVector Start, FVector End);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void DrawDashPath(APlayerController* PC, FVector Start, FVector End, int32 ElapsedMovementInstants = 0);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void DrawJumpPath(APlayerController* PC, FVector Start, FVector End, int32 ElapsedFlightInstants = 0);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void DrawJumpPathFromState(APlayerController* PC, const FBuhCombatState& S, int32 ElapsedFlightInstants = 0);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static FVector GetClampedJumpDestination(const FVector& Start, const FVector& Cursor);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetFinalPlayerHeight(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetFinalOpponentHeight(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetFinalPlayerWorldZ(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetFinalOpponentWorldZ(const FBuhCombatResult& Result);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetSnapshotPlayerHeight(int32 Index);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static float GetSnapshotOpponentHeight(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void BeginJumpPlayback(APlayerController* PC, AActor* PlayerFighter, AActor* OpponentFighter, const FBuhCombatState& PreState, int32 PlayerDecision, int32 OpponentDecision, const FBuhCombatResult& Result);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static bool TickJumpPlayback(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void StopJumpPlayback();

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void LogFighterZ(AActor* PlayerFighter, AActor* OpponentFighter, FString Tag);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void HandleTargetingTick(APlayerController* PC, AActor* PlayerFighter, int32 TargetingAction);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void TriggerReactionJump(AActor* Context);

	static TArray<FBuhTimelineSnapshot> GJumpPlaybackTimeline;
	static TWeakObjectPtr<AActor> GJumpPlaybackPlayer;
	static TWeakObjectPtr<AActor> GJumpPlaybackOpponent;
	static TWeakObjectPtr<APlayerController> GJumpPlaybackPC;
	static FBuhCombatState GJumpPlaybackPreState;
	static int32 GJumpPlaybackPlayerDecision;
	static int32 GJumpPlaybackOpponentDecision;
	static int32 GJumpPlaybackIndex;
	static float GJumpPlaybackAccumulator;
	static bool GJumpPlaybackActive;
	static float GJumpPlaybackJumpStartX;
	static float GJumpPlaybackJumpStartY;
	static float GJumpPlaybackJumpTargetX;
	static float GJumpPlaybackJumpTargetY;

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static int32 GetDashElapsedMovementInstants(const FBuhCombatState& S);

	UFUNCTION(BlueprintPure, Category = "CombatSim")
	static int32 GetJumpElapsedFlightInstants(const FBuhCombatState& S);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void LogCursorState(APlayerController* PC, FString Tag);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void ClearWalkPath(APlayerController* PC);

	// After a move resolves, update the persistent green Walk path from the
	// authoritative final state. If the player is mid-Walk (Action 6) with
	// remaining distance to WalkTarget, draw the remaining line from the
	// fighter's current position to the destination (preserving an interrupted
	// Walk through a reaction pause). Otherwise clear the path. Uses the
	// fighter's Action and WalkTarget, never PlayerMove or the DestPlayer actor.
	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void UpdateWalkPath(APlayerController* PC, const FBuhCombatState& FinalState);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static bool GetGroundHitUnderCursor(APlayerController* PC, FVector2D ScreenPos, FVector& OutHitLocation);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetGroundHitX(APlayerController* PC, FVector2D ScreenPos);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static float GetGroundHitY(APlayerController* PC, FVector2D ScreenPos);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void LogCombatState(const FBuhCombatState& S, const FString& Context);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void DiagnoseWidgetCast(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "CombatSim")
	static void SetWidgetManager(UUserWidget* Widget, AActor* Manager);

};
