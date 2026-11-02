# Beneath Uncounted Heavens — Fighter Mesh Cleanup and Editor-Visible Team Colors

## RESOLVED (2026-11-02)

All success criteria are met and verified graphically (`20261102_101311`):

- **Editor, before PIE:** left opponent cylinder red, right player cylinder blue — colors come from the `BP_Fighter` **Construction Script**, not `BeginPlay`. No PIE running.
- **Team-change:** flipping an instance's `Team` 0→1 recolors that cylinder red (Construction Script re-runs on the property change). Verified by capture.
- **PIE:** left red / right blue persist during gameplay; the default pawn is hidden; the combat UI (menu, prediction, Execute) is intact.
- **No duplicate `BodyMesh1`:** direct `ActorTools.get_components` on the CDO (`BP_Fighter.Default__BP_Fighter_C`) and both level instances returns only `BodyMesh` — the diagnostic report's `BodyMesh1` claim was disproven (it had been chased via a rejected colon-form refPath). Nothing to remove; the class is already clean.
- **Explicit references:** `ApplyMaterial`, `BeginPlay`, and the Construction Script all target the mesh via `Variables|Default|GetBodyMesh`, never `GetComponentByClass`.
- **Runtime preserved:** `BeginPlay` no longer assigns team materials (Construction Script handles it); `ResetEncounter` still restores team colors via the fixed `ApplyMaterial`; hit/block flashes intact.
- **Regression:** `BP_Fighter` compiled + saved; all four combat automation tests (`BUH.CombatSimTests`, `BUH.CombatFlowTests`, `BUH.CombatManagerIntegrationTests`, `BUH.WalkPreviewTests`) pass.

Lessons recorded in `notes.md` §2.6 (set_properties `values`-as-string, CDO refPath, top-level meta-tools) and §9.11 (Construction Script vs BeginPlay).

---

The fighter colors now render correctly in graphical PIE:

- **Team 0 (Player): Blue**
- **Team 1 (Opponent): Red**
- Left cylinder is the opponent; right cylinder is the player.

However, the cylinders remain gray in the Unreal Editor before pressing Play because their materials are assigned during `BeginPlay`.

Additionally, the latest diagnostic report confirms that `BP_Fighter` contains a duplicate `StaticMeshComponent` named `BodyMesh1`, which previously caused material assignments to target the wrong mesh.

**Goal:** Make fighter colors visible both before and during PIE, permanently fix the duplicate-component problem, and preserve all existing combat functionality.

## 1. Inspect the existing BP_Fighter

Before modifying anything:

- Inspect the Blueprint's component hierarchy.
- Identify `BodyMesh` and `BodyMesh1`.
- Determine which component is the intended visible cylinder.
- Identify any Blueprint graphs or C++ code referencing either component.
- Check the placed fighter instances in `Lvl_TacticalDuel`.
- Confirm how the `Team` variable is currently assigned.

Do not assume that removing a duplicate component from a placed actor also removes it from the Blueprint class.

Create a backup of the affected Blueprint asset before making structural changes.

## 2. Permanently remove the duplicate mesh

Remove the unnecessary `BodyMesh1` component from the `BP_Fighter` Blueprint asset itself, not merely the existing level instances.

Preserve the legitimate `BodyMesh` component and its:

- Cylinder StaticMesh.
- Transform and scale.
- Collision settings.
- Mobility.
- Existing gameplay references.

If removing `BodyMesh1` through MCP is unsupported, use a safe Unreal Editor or editor-side C++ approach.

Do not directly manipulate the binary `.uasset` structure.

Verify that newly spawned `BP_Fighter` instances contain exactly one intended cylinder mesh component.

Do not remove the component until its purpose and references have been checked.

## 3. Use explicit mesh references

Replace ambiguous calls such as:

`GetComponentByClass(StaticMeshComponent)`

with an explicit reference to the intended `BodyMesh` component wherever fighter materials are assigned.

Avoid selecting components based on enumeration order.

If a direct Blueprint component reference is unavailable through MCP, use a reliable, explicitly verified component lookup rather than selecting the first StaticMeshComponent.

## 4. Assign colors in the Construction Script

Make the correct team color visible in the level editor before PIE.

Use the `BP_Fighter` Construction Script to assign:

- Team 0 → `MI_Player` (blue).
- Team 1 → `MI_Opponent` (red).

Ensure the `Team` variable is instance-editable so each placed fighter can have its team configured in the Details panel.

For `Lvl_TacticalDuel`, verify:

- PlayerFighter has Team 0.
- OpponentFighter has Team 1.

Use the existing material instances rather than creating duplicate materials.

Changing the team in the editor should update the cylinder's color without requiring PIE.

## 5. Preserve runtime behavior

Check whether `BeginPlay` still needs to assign team materials.

If Construction Script assignment is sufficient for the existing gameplay, avoid duplicating unnecessary initialization.

If teams can change dynamically during gameplay, provide a single reusable `ApplyTeamMaterial` function called from the appropriate lifecycle events.

Ensure that:

- ResetEncounter does not restore gray materials.
- Hit and block flashes still work.
- Temporary visual effects restore the correct team color.
- Ghost fighters remain visually distinguishable.
- Material assignments do not change combat state.

Keep this change narrowly focused on mesh components and team presentation.

## 6. Verify in the editor and PIE

Perform these checks:

**Editor, before pressing Play**
- Left fighter is red.
- Right fighter is blue.
- Changing an instance's Team value updates its color.
- Both cylinders have the correct mesh and material slot.
- No duplicate BodyMesh1 exists in the Blueprint class.

**Graphical PIE**
- Both fighters retain their colors.
- The arena and camera render correctly.
- No unwanted white player pawn appears.
- ResetEncounter preserves team colors.
- Combat flashes do not permanently overwrite materials.
- Ghost prediction visuals remain functional.

Use graphical Unreal for visual verification. Do not treat headless `-nullrhi` screenshots as evidence of rendered colors.

## 7. Regression checks

Compile and save the affected Blueprints and level.

Run the existing combat automation tests.

Ensure the changes do not affect:

- Fighter positions.
- Health.
- Combat execution.
- Prediction and ghost actors.
- The hierarchical combat menu.
- Walk state persistence.

Do not modify unrelated combat mechanics or UI layout.

## 8. Documentation

Update `notes.md` with the verified lesson:

When multiple StaticMeshComponents exist on an actor, `GetComponentByClass` may return a different component from the one visibly rendered. Prefer explicit component references for gameplay-critical operations.

Also document the distinction between Construction Script material assignment and BeginPlay assignment.

Keep the note brief and avoid duplicating existing documentation.

## Constraints

- Unreal Engine 5.8.4 on Linux.
- Preserve existing blue/red material instances.
- No new combat mechanics.
- No unnecessary asset recreation.
- No unrelated refactoring.
- No Git commit without authorization.

**Success criterion:** The two fighters are visibly red and blue in the Unreal Editor before PIE, remain correctly colored during gameplay, and newly created fighter instances no longer inherit the duplicate-mesh problem.

Report any part that cannot be safely completed with the available MCP tools rather than claiming success without verification.