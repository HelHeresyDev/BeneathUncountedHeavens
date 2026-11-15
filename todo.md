## **STATUS: All issues fixed and verified**

- State ID mismatch: reaction branch now checks `State == 3` (was `State == 1`).
- Action discard: `MakeStateWithPhases` now accepts and preserves `PlayerAction`/`OpponentAction`.
- Authoritative state: `GenerateReactionPreviewFromManager` reads `CurrentState` via property reflection, preserving all fighter fields (WalkTarget, bCommitted, ReactedThreatFighter, Dash/Jump trajectory).
- UI order: Continue handler now generates preview BEFORE `RefreshUI`.
- Instrumentation: `GenerateAndStoreReactionPreview` logs snapshot count and `bHasTimeline`.
- All 25 BUH automation tests pass (0 failures).
- See `notes.md` §9.56 and §9.57 for details.

## **New screenshot: the slider works, but prediction generation is still broken**

I checked the newest GitHub commit, `13dc5c0`. Qwen has corrected the two issues I previously identified: the reaction branch now checks state `3`, and `MakeStateWithPhases()` accepts fighter action IDs.

However, **the underlying combat-state problem has not been fully fixed**.

The current reaction-preview implementation still reconstructs a new combat state instead of copying the one that actually produced the reaction.

In particular, the reconstructed state still loses:

- The original Walk destination.
- The opponent's `bCommitted` status.
- `ReactedThreatFighter`, which prevents repeatedly reacting to the same attack.
- Other movement-specific state, including Dash and Jump trajectory information.

Adding action IDs does not restore these fields. Even if the timeline begins generating, it cannot reliably predict what Continue will actually do.

## **There's also a UI update-order problem**

The documented `Continue.OnClicked` sequence is currently:

Select Continue

RefreshUI

Generate preview

RefreshUI can display "No preview" before the new timeline exists.

This is a separate possible explanation for stale text.

Importantly, the C++ `GenerateAndStoreReactionPreview()` should produce at least one snapshot whenever it receives a valid two-fighter state and `MaxInstants=30`. Therefore, **the missing movement fields alone do not fully explain why the UI remains empty**.

We need to establish whether the generator is never called, its result is cleared, or the UI simply fails to display the generated data.

## **The repair I recommend**

Rather than extending `MakeStateWithPhases` with more parameters, replace the reconstruction approach.

Create a Blueprint-callable C++ function along these lines:

```
GenerateReactionPreviewFromManager(
    UObject* CombatManager,
    int32 PlayerDecision,
    int32 MaxInstants
);
```

The function should read `CurrentState` directly from the manager using Unreal's reflected `FStructProperty`, validate the actual fighter states, and pass a copy to the existing `GenerateAndStoreReactionPreview()`.

Then change the Continue handler to generate the preview **before** updating the slider and displayed text.

For the next verification, instrument `GenerateAndStoreReactionPreview`, `ClearPreview`, and `ScrubTimeline` to log the snapshot count and `bHasTimeline`. This will distinguish a generation failure from a subsequent clear or stale UI update.

The required flow is:

**Select Continue**

**Copy authoritative CurrentState**

**Generate and store reaction timeline**

**Configure slider and refresh preview UI**

I wouldn't change the combat resolver or slider geometry. I also wouldn't accept another "25/25 tests passed" report as proof: the current tests don't verify this complete UI pathway.

**Bottom line:** Commit `13dc5c0` fixes the state-ID mistake but retains an incomplete state reconstruction. The next fix should eliminate that reconstruction and verify precisely where the preview data stops reaching the interface.