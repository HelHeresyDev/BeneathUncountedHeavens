# BUH — Fix competing Dash path renderers

The new graphical PIE screenshots show that the interrupted Dash path now correctly turns green. However, when the player selects a NEW Dash, its preview stays green instead of showing the usual red commitment section.

The latest graphical log proves the cause.

At 2026-11-09 20:54:08.219, immediately after a new Dash click:

1. `DrawDashPath` receives `Elapsed=0`, `RemainingLocked=3` and writes `Colors=[R]`. CORRECT.
2. `GetDashElapsed` then reads the OLD active Dash state: `Action=8 Phase=2 PhaseInstants=7 -> Elapsed=7`.
3. Another `DrawDashPath` writes `Colors=[G]`, overwriting the new Dash preview. WRONG.

This red-then-green sequence repeats every tick, including while the player moves the targeting cursor.

**Fix the two competing path writers. Do not change the working elapsed-Instant calculation.**

## Required behavior

- A newly selected Dash always starts a fresh preview with 3 locked Instants: red first, green afterward.
- An interrupted Dash that can be continued shows an entirely green remaining path.
- Selecting a new Dash during a reaction displays the NEW Dash preview, not the existing Dash continuation preview.
- Canceling the new Dash restores the previous Dash continuation preview, if applicable.
- Committing a new Dash starts a genuinely new Dash action with a new commitment period. It must not inherit the previous Dash's elapsed Instants.
- Continue resumes the existing Dash without restarting its commitment period.

## Implementation

Inspect all calls to `DrawDashPath`, including `WBP_CombatUI.EventTick`, Dash targeting updates, and reaction handling.

Give the rendering system one authoritative owner per update.

Suggested priority:

1. If actively targeting a NEW Dash, render that new path with elapsed=0.
2. Else, if showing an interruptible existing Dash, render its remaining path using the authoritative elapsed Instants.
3. Else, render the appropriate other action path or clear it.

Make these branches mutually exclusive. Avoid a second EventTick call overwriting a freshly drawn path. Preserve cursor-responsive target updates.

Keep preview state distinct from authoritative combat state until Execute. Do not mutate CurrentState just to change the preview color.

Add a regression test covering: interrupted Dash → select new Dash → red/green preview → cancel → original green continuation path → select new Dash → Execute → new commitment begins.

Verify through logging that exactly one effective path update occurs per UI refresh, and that the new Dash branch cannot be overwritten by the old reaction branch.

## Cursor bug, also confirmed

The graphical log records:

`DashDiag CURSOR [DashAfterInput]: bShowMouseCursor=0 ShouldShowMouseCursor=0 InputMode=Game and UI`

This appears on Dash clicks, including the second click at 20:54:08.217.

Identify why the actual player controller has both cursor flags false immediately after setting input mode. Correct that transition without relying on a per-tick visibility workaround. Verify that clicking Dash and targeting the ground keeps the cursor visible.

## Verification

Compile and save affected assets, run existing BUH tests and the new regression test, and verify the actual graphical PIE path if available.

Report the exact competing Blueprint/C++ call sites removed or gated, plus the cursor correction.

Do not add Jump, rewrite the combat system, or make unrelated UI changes. No Git commits.

Remove the large yellow diagnostic version banner and excessive per-frame `DashDiag` logging once the behavior is confirmed.

**Success: a continued Dash remains green; a newly selected Dash shows its initial red commitment section; neither overwrites the other.**