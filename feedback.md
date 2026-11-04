Yes. Your `notes.md` is a useful reference, but I found several outdated entries, some contradictions, and a few important lessons that should be added.

I would keep the current structure. There's no need to replace the file or download an Unreal manual. What it needs is a targeted accuracy cleanup, especially now that Qwen has apparently fixed the right-panel hit-testing issue.

## 1. Changes I would make

| Priority | Section                   | Recommended change                                                                                                                                                                  |
| -------- | ------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| High     | §3.1 Blueprint DSL        | Correct the statement that a successful `write_graph_dsl` call proves compilation. It proves the edit was accepted; compilation and runtime behavior require separate verification. |
| High     | §4.1 UUserWidget          | Correct the explanation of `NativeConstruct` and native ticking.                                                                                                                    |
| High     | §6.2 Headless testing     | Remove the implication that `-nullrhi` caused Blueprint functions to have no observable effects. §9.6 identified the editor-world test context as the actual issue.                 |
| High     | §8 Unresolved limitations | Update the Execute-disable and TopBar entries. Both still cite a DSL limitation for which §3.1 now documents a workaround.                                                          |
| High     | §9.10 Pawn removal        | Update the previous pawn-destruction procedure to reflect the latest cursor fix.                                                                                                    |
| Medium   | §4.2 RootCanvas           | Resolve the contradiction between Fill coefficient 0 causing clipping and coefficient 0 being harmless in the current widget.                                                       |
| Medium   | §9.12 Predictive ghosts   | Archive superseded implementation details, including the old ghost targeting and pending C++ rebuild.                                                                               |
| Medium   | §2.6 ObjectTools          | Clarify that deleting an actor from the level is not a substitute for hiding a runtime prediction ghost.                                                                            |

### Two especially important corrections

The UUserWidget explanation is inaccurate. Unreal's `UUserWidget` does support `NativeConstruct`, and its Blueprint `Construct` event is called when the underlying Slate widget is constructed. The actual problem was that `WBP_CombatUI` did not inherit from your custom `UBUHCombatUI` class, so diagnostics implemented in that unrelated subclass could not run. Native ticking is a separate issue.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://dev.epicgames.com\&sz=32)

Epic Developer Community+1



The Blueprint DSL note creates false confidence. I would replace:

> A successful `write_graph_dsl` (no error) is the compile proof.

With:

> A successful `write_graph_dsl` call indicates the graph edit was accepted. Always compile the Blueprint, inspect errors, save the asset, and verify the behavior separately. Read-back DSL is not guaranteed to match the submitted syntax.

That correction matters because we've repeatedly encountered Blueprints that compiled successfully but didn't work when you actually clicked the interface.

## 2. What I would add

### A. A current-status entry for hypothetical predictions

Section §9.15 now reports that the zero-width `RightPanel` was fixed and that the opponent buttons receive mouse events. That's progress, but the evidence specifically mentions the Dodge hover diagnostic. It does not yet prove all four buttons successfully select and generate predictions.

Add this distinction:

Opponent prediction UI — partially resolved

- RightPanel hit-testing: fixed; Dodge hover verified in graphical PIE.
- All four button click handlers: require complete manual verification.
- Hypothetical selection and timeline generation: not yet confirmed working end-to-end.
- Do not reopen the hit-testing investigation without new evidence.

That will prevent Qwen from declaring the entire prediction feature complete merely because the buttons are now clickable.

### B. The persistent mouse cursor fix

The current file doesn't document the recent solution that restored the cursor. Add a short entry covering the working `UI Only` input mode, focusable `WBP_CombatUI`, removal of the First Person mouse-look mapping context, and the distinction between tactical combat and eventual exploration input.

Also update §9.10: the latest cursor work replaced the previous pawn-destruction sequence with a delayed lookup and destruction. The old procedure should be labeled historical rather than presented as the current implementation.

### C. A rule for verifying fixes

I'd add this near the beginning:

> Verification rule: Record what was changed, how it was tested, and what remains unverified. Source inspection proves implementation structure; Blueprint compilation proves compilation; graphical screenshots prove rendering; only actual interaction testing proves mouse-driven UI behavior.

This is probably the single most valuable addition for Qwen.

## 3. What I would remove or consolidate

The document is currently 442 lines. That's not excessive, but several sections repeat the same historical problems.

I would shorten §9.12–§9.14 into a more concise ghost-system reference while preserving the exact MCP gotchas. I would also remove obsolete statements such as the pending C++ rebuild once the newer successful build is verified.

In §9.3, clarify that removing extra fighters was historical cleanup. The current design intentionally includes two real fighters, two prediction ghosts, and one destination ghost. That distinction is important so Qwen doesn't delete the prediction actors again.

I'd also replace the blanket port-8000 cleanup command using `kill -9` with instructions to identify the owning process first and terminate only a confirmed stale Unreal MCP process.

## My recommendation

Keep `notes.md`, but perform one documentation-only correction pass.

Preserve the technical syntax, tested commands, and MCP workarounds. Correct the contradictory guidance, archive obsolete procedures, and add the cursor and right-panel fixes with accurate verification status.

I would also have Qwen mark individual entries with last verified dates where behavior might change. That would be particularly helpful for Unreal MCP limitations, which you've already seen can be resolved or disproven by later investigations.

No project code needs to change as part of this cleanup, and I wouldn't reorganize `AGENTS.md` or `todo.md` at the same time.