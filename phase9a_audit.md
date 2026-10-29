# Phase 9A Audit: Existing Combat Architecture

## 1. CombatSim.h — Data Structures

### FBuhFighterState
- `Health` (int32): Current HP.
- `X` (float): Position on the 1D arena axis.
- `Action` (int32): Currently committed action id (0=Quick, 1=Heavy, 2=Guard, 3=Dodge).
- `Phase` (int32): 0=ready, 1=startup, 2=active, 3=recovery.
- `PhaseInstants` (int32): Instants spent in the current phase.
- `Team` (int32): 0=player, 1=opponent.
- `Facing` (int32): 1=right, -1=left.

### FBuhCombatState
- `Fighters` (TArray<FBuhFighterState>): Always exactly 2 fighters.
- `Instant` (int32): Current simulation instant.
- `Seed` (int32): Deterministic seed for AI variety.

### FBuhCombatEvent
- `Instant`, `Type`, `Fighter`, `Damage`, `Detail`.
- Type: 0=hit, 1=guard-block, 2=miss, 3=dodge-evade, 4=ai-commit, 5=KO.

### FBuhCombatResult
- `Events` (TArray<FBuhCombatEvent>), `FinalState` (FBuhCombatState), `StoppedReason` (int32), `bPlayerReady`, `bOpponentReady`, `MinDistanceSeen`.

### FBuhTimelineSnapshot
- Per-instant record: positions, facings, actions, phases, phase instants, healths, ready flags, and events for that instant.

## 2. CombatSim.cpp — Core Logic

### MoveData[4]
| Id | Name    | Startup | Active | Recovery | Damage | Range  |
|----|---------|---------|--------|----------|--------|--------|
| 0  | Quick   | 6       | 1      | 8        | 10     | 300    |
| 1  | Heavy   | 13      | 1      | 8        | 24     | 400    |
| 2  | Guard   | 0       | 10     | 0        | 0      | 9999   |
| 3  | Dodge   | 3       | 2      | 3        | 0      | 9999   |

### Constants
- `Approach = 30`: Automatic positional closure per instant when fighters are farther than `MinSeparation`.
- `MinSeparation = 100`: Minimum distance before automatic approach stops.
- `AI_MOVE = 1`: The AI always commits Heavy Strike.
- `DodgeDistance = 300`: Distance threshold for dodge evasion check.

### StepCombatInstant (lines 55–155)
1. Advances each fighter's phase (startup→active→recovery→ready).
2. **Automatic approach (lines 78–84)**: If distance > MinSeparation, both fighters move toward each other by `Approach` (30 units). This is the "lunge" that must be removed.
3. Resolves active-window hits: if a fighter just entered active phase and is in range, it deals damage (or is blocked/evaded).
4. Returns a list of `FBuhOutcome` for the instant.

### ResolveCombat (lines 157–275)
1. Commits decisions for fighters that are ready (Phase 0) at the start.
2. Steps through instants, calling `StepCombatInstant`.
3. Applies outcomes (damage, block, evade, miss).
4. Stops when both fighters are ready (StoppedReason=2), or one fighter is ready (StoppedReason=1), or KO (StoppedReason=3), or max instants reached (StoppedReason=4).
5. When the opponent becomes ready and the player is not, the AI independently commits Heavy (AI_MOVE=1) and combat continues.

### GenerateTimeline (lines 1103–1250)
- Same logic as `ResolveCombat` but records a `FBuhTimelineSnapshot` each instant instead of returning a single result.
- Used for the UI timeline preview and ghost scrubbing.

### ChooseOpponentMove (lines 318–361)
- AI logic based on observable inputs only (distance, healths, opponent phase, player's last revealed move).
- Out of range: Guard or Dodge.
- Player attacked last: defend or counter.
- Player defended/idled: press the attack.

### MakeState
- Creates initial state: fighters at -500 and +500, health 100 each, facing each other, Phase 0.

## 3. BUHCombatUI.h/cpp — UI Bridge

- `UUserWidget` subclass with BlueprintCallable functions:
  - `SetTimelineInstantText`: Updates the instant label.
  - `ResetTimelineSlider`: Resets the slider value.
  - `MoveGhostsToInstant`: Moves the two fighter actors to the snapshot positions for the given instant.
  - `SetupTimelineAndMoveGhosts`: Generates a timeline for a player/opponent move pair and scrubs to the given instant.
- Console commands: `BUH.ScrubTimeline` and `BUH.InspectLeftPanel`.

## 4. BP_CombatManager (Blueprint)

- Has `SelectMove`, `ExecuteMove`, `ResetEncounter` functions and a `BeginPlay` that sets the view target.
- State ints: 0=Planning, 1=Resolving, 2=Done.
- Connects the WBP buttons to the C++ combat sim.

## 5. WBP_CombatUI (Blueprint)

- Four-region layout: TopBar, LeftPanel, RightPanel, BottomTimeline.
- Buttons: BtnQuick, BtnHeavy, BtnGuard, BtnDodge, BtnExecute, BtnReset, and opponent move buttons.
- Text: health, state, move, prediction, timeline instant, distance.
- Slider: TimelineSlider for scrubbing the timeline.
- Hides planning buttons when State != 0, shows only Reset when State == 2.

## 6. Key Findings for Phase 9A

1. **Automatic approach/lunge**: `StepCombatInstant` lines 78–84 move both fighters toward each other by `Approach` (30) every instant when distance > `MinSeparation` (100). This must be removed so that Quick and Heavy no longer auto-close distance.

2. **No Walk action**: The four moves are Quick(0), Heavy(1), Guard(2), Dodge(3). There is no Walk. A new action id (4=Walk) must be added.

3. **No reaction window system**: Combat pauses only when both fighters are ready. There is no concept of a fighter being "reactable" while another fighter is in startup. The reaction window system must be introduced.

4. **No action commitment tracking**: Fighters commit actions at the start of `ResolveCombat`, but there is no separate "committed" flag. The `Action` field is set to the decision and `Phase` is set to 1. A `bCommitted` flag and `PendingAction` need to be added.

5. **No observable threat detection**: The UI shows distance and health but does not detect "opponent is in startup and will reach active within N instants." This must be added.

6. **AI always commits Heavy**: `AI_MOVE = 1` is hardcoded. The AI does not react to player actions during startup. This needs to change so the AI can react at reaction windows.

7. **GenerateTimeline duplicates ResolveCombat logic**: Both functions have nearly identical step loops. They should share a common stepping function to avoid divergence.

8. **42 automated tests** in `CombatSimAutomation.cpp` cover the existing combat and timeline behavior. These must be preserved or updated to reflect the new movement/reaction rules.
