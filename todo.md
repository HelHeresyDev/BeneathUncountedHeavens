## **STATUS: Both issues fixed and verified**

- State ID mismatch: reaction branch now checks `State == 3` (was `State == 1`).
- Action discard: `MakeStateWithPhases` now accepts and preserves `PlayerAction`/`OpponentAction`.
- All 25 BUH automation tests pass (0 failures).
- See `notes.md` §9.56 for details.

## **Code investigation: two problems found**

I accessed your GitHub repository, [Beneath Uncounted Heavens](https://github.com/HelHeresyDev/BeneathUncountedHeavens?utm_source=chatgpt.com), and examined the latest commit, `2a85b6a`, along with `CombatSim.cpp`, `CombatSim.h`, the UI automation tests, and the Blueprint implementation notes.

The latest fix has **two significant problems that can explain your screenshot**.

### **1. The reaction preview appears to check the wrong combat state**

The latest `RegeneratePredictionPreview` implementation is documented as:

- State `0`: Planning
- State `1`: Reaction

However, earlier verified `BP_CombatManager.ExecuteMove` documentation establishes:

- State `0`: Planning
- State `1`: Resolving/playback
- State `3`: Reaction opportunity

If these values haven't changed, the new reaction branch will never execute at the moment shown in your screenshot.

That would leave `GCurrentTimeline` empty, producing exactly what you're seeing: **"No preview," no opponent timeline, and an immovable slider**.

This is a strong candidate for the immediate failure, although the current saved Blueprint graph still needs a direct readback to confirm the branch condition.

### **2. The reaction preview discards the actual combat state**

This is a definite design bug in the latest implementation.

According to `notes.md` §9.55, the new reaction branch reconstructs combat state using `MakeStateWithPhases()` instead of reading the existing `CurrentState`.

Here's what that function actually does in `CombatSim.cpp`:

```
FBuhCombatState S = MakeState(
    PlayerX, OpponentX,
    PlayerHealth, OpponentHealth,
    Seed, PlayerY, OpponentY
);

S.Fighters[0].Phase = PlayerPhase;
S.Fighters[0].PhaseInstants = PlayerPhaseInstants;
S.Fighters[1].Phase = OpponentPhase;
S.Fighters[1].PhaseInstants = OpponentPhaseInstants;
return S;
```

It does **not** preserve the fighter's current action, committed status, destination, or reaction bookkeeping.

That matters because `Continue` (action 7) is explicitly designed to preserve an existing movement action. It does not create a new Walk.

The newly reconstructed state has `Action = -1` for both fighters. Passing Continue to that state cannot properly resume the existing Walk.

Worse, `StepCombatInstant()` accesses:

```
BuhCombat::MoveData[F.Action]
```

When a reconstructed fighter has a nonzero phase but `Action == -1`, this can cause an invalid array access.

**The correct reaction preview must use a copy of the authoritative&#x20;**`CurrentState`**, not reconstruct one from positions and phase counters.**

### **3. Why the slider no longer moves**

The slider problem is likely a consequence of the missing timeline, not a separate mouse-input bug.

In `CombatSim.cpp`:

```
int32 UBuhCombatSim::GetFinalInstantIndex()
{
    if (!bHasTimeline()) return 0;
    return FMath::Max(0, GetTimelineLength() - 1);
}
```

The Blueprint sets the slider's maximum using this value. When there's no preview, the maximum becomes zero. With both minimum and maximum at zero, there's nowhere for the thumb to move.

That matches your screenshot.

### **4. Why all 25 automated tests passed**

I also inspected `TimelinePreviewAutomation.cpp`.

The test labeled `BUH.PIE.WalkContinueReaction` explicitly acknowledges that it cannot verify the real PIE-world manager state and UI behavior. It calls Blueprint functions, logs the limitation, and returns `true`.

Similarly, the widget-wiring test primarily checks that `RegeneratePredictionPreview` exists and contains compiled bytecode. It does not verify that the correct reaction branch executes.

Consequently, **25 passing tests do not establish that reaction previews work**.

## **What I recommend fixing**

The correction should be surgical.

**First:** In `WBP_CombatUI.RegeneratePredictionPreview`, use the actual reaction condition. Prefer the manager's `bReactionOpportunity` and `ReactionFighter == 0`, rather than assuming `State == 1`.

**Second:** Remove `MakeStateWithPhases` from the reaction-preview pathway. Supply the actual `BP_CombatManager.CurrentState` to `GenerateAndStoreReactionPreview`. If Blueprint access remains problematic, expose a small C++ helper that reads the manager's `CurrentState` through Unreal property reflection. The project's existing automation code already demonstrates that this is possible.

**Third:** Ensure that the resulting preview is stored before updating the slider range. Preserve the existing `ScrubTimeline` implementation, which was previously verified in graphical PIE.

**Fourth:** Add regression coverage for the actual manager-to-preview pathway, including assertions that Continue preserves the original Walk target, the opponent's committed Heavy, and `ReactedThreatFighter`.

### **My conclusion**

I would not rewrite the slider, movement system, or combat resolver.

The strongest evidence points to a faulty reaction-state branch introduced in the latest Blueprint change, compounded by an invalid reconstruction of the frozen combat state.

The C++ simulation already supports continuing a movement against a committed opponent attack. The missing piece is passing it the **correct existing state**.

I have not executed Unreal or independently decoded the saved `.uasset` graph, so the precise runtime branch failure still needs confirmation. But the `MakeStateWithPhases` defect is directly established by the source code, and the state-ID mismatch is strongly supported by the commit history.

This gives us a much narrower repair target than the previous generic debugging workpack.