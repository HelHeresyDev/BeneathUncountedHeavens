# Beneath Uncounted Heavens — Preserve Opponent Predictions While Selecting Walk

The previous fix works: selecting hypothetical opponent Dodge with no player action produces a visible opponent Dodge ghost.

However, the user reports a new regression:

**When the opponent Dodge ghost is visible and the player selects Walk, the opponent's prediction ghost disappears.**

This is incorrect during planning.

The player must be able to select a hypothetical opponent action independently, then choose or change their own action while preserving that hypothetical response.

## 1. Reproduce and diagnose the transition

Start with:

- PlayerMove = -1 (no selected action)
- HypotheticalOpponentMove = 3 (Dodge)
- Orange opponent prediction ghost visible

Then select Move → Walk without pressing Execute.

Before and after selecting Walk, log:

- PlayerMove
- HypotheticalOpponentMove
- CurrentTimeline length and selected Instant
- GhostOpponent bHidden
- GhostOpponent world position
- Real opponent world position
- Whether RefreshUI runs
- Whether the prediction timeline is regenerated

Determine whether:

A. Walk selection clears HypotheticalOpponentMove.

B. RefreshUI hides the prediction ghosts.

C. The timeline regenerates but positions the ghost at Instant 0, overlapping the real opponent.

D. Another handler moves or hides the ghost after RefreshUI.

Do not assume disappearing means HiddenInGame=true. Verify its transform and visibility.

## 2. Preserve hypothetical selection

Selecting Walk must not clear the existing hypothetical opponent response.

Expected:

Before Walk:
PlayerMove = -1
HypotheticalOpponentMove = 3

After selecting Walk:
PlayerMove = 6
HypotheticalOpponentMove = 3

The opponent Dodge selection remains highlighted.

The active prediction should now represent Walk vs. Dodge.

Only clear the hypothetical selection when the player explicitly clears it, resets the encounter, or commits an action and the relevant hypothetical preview is no longer valid.

## 3. Centralize ghost visibility

The previous fix explicitly unhides the ghosts after RefreshUI inside each opponent-button handler.

This is fragile because other UI events also call RefreshUI.

Make RefreshUI or one dedicated preview-state function the authoritative controller of ghost visibility.

Desired rules:

- No hypothetical selected: cyan/orange prediction ghosts hidden.
- Hypothetical selected with no player action: show opponent-only prediction.
- Hypothetical selected with a player action: show the combined prediction.
- Walk planning: yellow destination ghost also visible.
- Execute/Reset: clear obsolete hypothetical previews appropriately.

Do not require PlayerMove >= 0 for opponent-only predictions.

Avoid adding another temporary unhide operation solely to the Walk button.

## 4. Regenerate predictions when the player action changes

When the player selects Walk while Dodge is already the hypothetical opponent response:

1. Keep HypotheticalOpponentMove=3.
2. Read the current authoritative combat state.
3. Apply the selected Walk target to a copied state.
4. Generate the Walk vs. Dodge hypothetical timeline.
5. Store the new timeline.
6. Select a meaningful preview Instant.
7. Move the cyan and orange ghosts to their predicted positions.
8. Update the timeline slider, prediction text, and selected response indicator.
9. Refresh the UI without hiding the active preview.

The ghost should not be left using an obsolete opponent-only timeline.

## 5. Preserve meaningful preview timing

The previous Dodge fix initialized the ghost at a meaningful final Instant so the displacement was immediately visible.

Do not accidentally reset the display to Instant 0 when selecting Walk.

If the selected preview Instant changes, keep the timeline slider synchronized.

The predicted opponent may have moved only a small distance, so distinguish a hidden ghost from one overlapping the real fighter.

Do not exaggerate or hardcode Dodge displacement.

Use the existing CombatSim timeline.

## 6. Preserve Walk slider behavior

While Walk vs. Dodge is active:

- Yellow ghost shows the intended Walk destination.
- Cyan ghost shows the player's predicted position.
- Orange ghost shows the opponent's predicted Dodge position.

Adjusting the Walk slider should:

- Update PlayerWalkTarget.
- Move the yellow destination ghost.
- Recompute Walk vs. Dodge.
- Update cyan/orange ghosts and the timeline.
- Preserve the hypothetical Dodge selection.

Neither real fighter may move during planning.

## 7. Test all relevant transitions

Verify:

A. None → hypothetical Dodge: orange Dodge ghost appears.

B. Dodge hypothetical → select Walk: orange ghost remains visible or updates to the new predicted outcome.

C. Walk selected → change Walk slider: yellow destination and hypothetical positions update correctly.

D. Walk vs. Dodge → switch hypothetical to Heavy: prediction regenerates for Walk vs. Heavy.

E. Walk vs. Heavy → switch player action to Quick: prediction regenerates for Quick vs. Heavy.

F. Press Back while retaining a valid hypothetical selection: either preserve the opponent-only preview or explicitly clear the selection according to a consistent, documented rule. Do not silently hide a selected hypothesis.

G. Execute: obsolete hypothetical ghosts clear and real combat uses the authoritative resolver.

H. Reset: real positions and preview visibility return to their initial states.

## 8. Verification

Compile and save modified Blueprints.

Run existing combat and prediction automation tests.

In graphical PIE, capture:

- Opponent-only Dodge preview.
- Walk vs. Dodge preview.
- Two different Walk destinations with Dodge still selected.

Inspect actual ghost transforms, not just visibility.

If MCP cannot simulate clicks, provide a short manual verification checklist and do not claim that interactive testing passed automatically.

Preserve the working persistent cursor, team colors, and menu hierarchy.

No new combat mechanics, Dash, Jump, 3D targeting, unrelated HUD redesign, or Git commits without authorization.

**Success criterion:** Selecting or adjusting Walk never silently cancels an active hypothetical opponent Dodge. The preview updates to the correct Walk vs. Dodge simulation, with appropriate visible ghosts and synchronized timeline state.