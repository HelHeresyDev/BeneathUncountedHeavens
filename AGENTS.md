<!-- COMPACT: re-read this file in full before continuing work -->
# AGENTS.md — Beneath Uncounted Heavens (Unreal MCP capability test)

## Compact Policy
- **After every compaction**, re-read this file in full before continuing work.
- **Compaction summaries MUST include**: "Re-read AGENTS.md in full before continuing."
- **Before finishing any task**, complete the final verify step of the workflow below.
- Detailed toolset gotchas, the graphical visual-test/self-wake procedure, and known limitations live in **`notes.md`** — consult it when a specific problem arises, not every turn.

## Response Budget Policy
Prefer acting over prolonged internal deliberation. Inspect the relevant assets, form a concise plan, and begin using tools rather than spending the entire response budget reasoning. If a problem is complex, make incremental progress and verify each step instead of solving everything mentally first. When nearing the response budget, summarize current findings and continue through tools rather than ending without action.

## General
- **Think before acting.** Before every command or edit, pause and ask: does this still make sense given what's changed? Am I just repeating the last step on autopilot?
- Keep responses short (CLI). No preamble/postamble unless asked.
- Prefer editing existing assets; follow existing conventions. No code comments unless asked.
- When the user says "Screenshot", they have taken a screenshot themselves — inspect the most recent file in `/home/depressedtsukasa/Pictures/Screenshots` (read the newest image). `CaptureEditorImage` is only for grabbing the UE editor's own game view, not the desktop.
- **After any failure, research online** (Unreal 5.8 docs, GitHub issues) before the next approach.
- **If still stuck**, ask the user to pass the problem to ChatGPT. Sanitize for SFW if adult.
- **`pkill` is unreliable** on this system. To free a port, use `lsof -ti:<port> | xargs kill -9`.
- **Graphical PIE is always fresh.** The user launches a brand-new PIE session for every graphical test (full editor restart + fresh PIE). An "old/stale PIE" or "outdated Blueprint instance still running" is **NEVER** a valid explanation for a bug. If a graphical-PIE test shows wrong behavior, the cause is in the on-disk assets, the compiled C++, or the live graph — not a stale session. Do not offer "stale PIE" as a hypothesis.
- **Push after confirming a task is complete.** ChatGPT reads the public GitHub repository to give feedback and plan next steps, so once the workflow’s verify step passes and the task is confirmed complete, commit the intended changes and push them to the remote. Do not push work-in-progress, unverified behavior, or secrets; an explicit user instruction such as “no Git commits” still overrides this policy.

## Verify Before Claiming
Inspect before you claim. If you can check it cheaply, checking is mandatory.
- Prefer: "I checked X and observed Y, therefore Z." Avoid: "This should work, so it's fixed."
- If verification is not possible, label it as an inference and say what would need to be checked.
- Never invent file contents, command output, PIE results, or project state. Never claim a move "works" because the Blueprint compiled — run PIE and inspect the result.
- Never attribute a graphical-PIE failure to a "stale PIE" or "outdated Blueprint instance" — the user always starts a fresh PIE, so that explanation is invalid. Find the real cause in the assets, C++, or live graph.

## Check Existing Work
Before creating or replacing anything, check if it already exists. Don't treat lack of memory as evidence the work hasn't been done. Prefer modifying the smallest necessary part over rebuilding. `todo.md` tracks phase state; check it before starting work.

## Evidence Before Action
For any non-trivial debugging, coding, configuration, or project-state task, do not immediately modify assets. First:
1. Inspect the relevant existing implementation.
2. Inspect related assets, configuration, logs, or runtime (PIE) state.
3. Determine what is already implemented so completed work is not repeated.
4. Identify the observed evidence for the actual problem or missing behavior.
5. Form a concrete plan based on that evidence.
6. Only then modify the project.

A plausible explanation is not sufficient evidence. After a change, verify using the source of truth: PIE capture, log entries, or direct property inspection.

## Image Inspection Rule
When the user refers to an image, screenshot, or visual file:
- Never describe, interpret, or diagnose its contents until you have actually inspected it.
- A filename, path, or nearby context is not a substitute for viewing the image.
- If it cannot be inspected, explicitly say so.
- Clearly distinguish what is visibly present from conclusions inferred from it.

## Project Overview
**Beneath Uncounted Heavens** is a third-person 3D cultivation sandbox RPG with tactical combat inspired by *Your Only Move Is HUSTLE*. The current assignment is an **engine capability test**, not full production: build a minimal, functional 3D tactical duel prototype (two fighters, three player moves, move prediction, working combat UI) to prove the full dev cycle works via Unreal MCP. Spec: `todo.md` (the working contract — re-read it when in doubt about scope or boundaries).

## Environment
- Unreal Engine 5.8.4 at `/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4`. Do NOT move the engine into the project directory.
- Project root: `/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/` (this is the opencode cwd; the `.uproject` lives directly inside it — no nested project subdir).
- GPU: AMD RX 7900 XTX (AMD stack). Avoid CUDA-specific assumptions.
- Linux-first, offline-first. Nothing leaves the machine by default.
- Python 3.14 available for offline analysis; not required for the game.
- **Headless editor + Qwen coexist safely.** A `UnrealEditor-Cmd -nullrhi -unattended -ModelContextProtocolStartServer` process adds ~0 GiB VRAM. It is safe to **leave the headless editor running alongside Qwen/llama.cpp** — no need to wait for llama.cpp to sleep, and no need to close the editor after editing.

## MCP Access & Protocol (CRITICAL)
All Unreal work goes through the **Unreal MCP server** at `http://127.0.0.1:8000/mcp` (wired in `opencode.json`).
- The server only exists if the editor was launched with `-ModelContextProtocolStartServer`. After an editor restart the agent's `unreal-mcp_*` meta-tools may be absent.
- **Reliable path**: direct HTTP JSON-RPC via `tmp_opencode/mcp_do.py` (toolset, tool, args-file). It performs a fresh handshake each call — use it whenever the agent's `unreal-mcp_*` meta-tools are absent.
- Protocol: `POST http://127.0.0.1:8000/mcp`, JSON-RPC. Handshake `initialize` → `notifications/initialized` → `tools/list` / `tools/call`. Meta-tools: `list_toolsets`, `describe_toolset`, `call_tool`.
- `call_tool` args: `toolset_name` (optional for top-level meta-tools), `tool_name`, `arguments` object matching the tool schema.
- After relaunch the editor opens to `Lvl_IntroRoom`; switch to the prototype level with `EditorToolset.EditorAppToolset.OpenEditorForAsset` on `/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel`.

## MCP Essentials (the few gotchas that affect every operation)
- **Save**: `AssetTools.save_assets` on **package paths** with param `asset_paths` (array). Saving a BP fails while PIE is running — `StopPIE` first.
- **PIE**: `StartPIE` needs `options` = `{"bSimulate": false, "playMode": "PlayMode_InViewPort", "warmupSeconds": 3}`. Stop with `StopPIE`; check `IsPIERunning`.
- **Capture**: `CaptureEditorImage` shows the PIE game view — use it for PIE evidence. Under headless `-nullrhi` it returns a 1x1 placeholder; verify state via property inspection instead.
- **ObjectTools** cannot read **object-reference** properties (`Mesh`, `StaticMesh`, `SetMaterial`); use `ActorTools.get_actor_transform` for actor positions.
- **List all PIE actors**: `SceneTools.find_actors` with empty `name`/`tag`/`collision_channels`.
- **Blueprint DSL** (`write_graph_dsl`): attaches bodies to found entry nodes (does not wipe the graph). Own-BP functions are `CallFunction|Name`; use keyword args (`:Param value`) or the first positional arg wires to the `self` pin. Full DSL gotchas are in `notes.md`.

## Start / Test Commands
```sh
# Launch editor with MCP (required for the server to exist)
nohup /home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4/Engine/Binaries/Linux/UnrealEditor-xwayland \
  "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject" \
  -ModelContextProtocolStartServer > /home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/tmp_opencode/unreal_editor.log 2>&1 &

# Headless variant (safe to leave running alongside Qwen)
nohup /home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd \
  "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject" \
  -nullrhi -unattended -nosplash -ModelContextProtocolStartServer > /home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/tmp_opencode/mcp_author.log 2>&1 &

# MCP calls (meta-tool absent path)
python3 /home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/tmp_opencode/mcp_do.py "<Toolset>.<ToolsetName>" "<tool_name>" /home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/tmp_opencode/args.json

# Start / stop PIE, capture, logs — via call_tool on EditorToolset.EditorAppToolset / LogsToolset
#   StartPIE   options {"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":3}
#   StopPIE, IsPIERunning, CaptureEditorImage, GetLogEntries (category:"")
```
There is no separate test runner. Validation = compile the BP, run PIE, capture the game view, and inspect log entries + actor properties.
- **XWayland launcher**: every *graphical* launch (desktop shortcut, the nohup command above, `tests/run_graphical_visual_test.sh`) goes through `…/Binaries/Linux/UnrealEditor-xwayland`, a wrapper that sets `SDL_VIDEODRIVER=x11` + `QT_QPA_PLATFORM=xcb` and unsets `WAYLAND_DISPLAY`, then execs the real `UnrealEditor`. This keeps the editor off native Wayland, which otherwise freezes GNOME Shell on graceful close. The real ELF must stay named `UnrealEditor` (UE target-receipt validation rejects a renamed binary and re-launches in a loop). Headless `UnrealEditor-Cmd` launches are unaffected. Rollback: delete the wrapper and point launchers back at `UnrealEditor`.

## Project Structure
```
BeneathUncountedHeav/                 ← opencode cwd = project root
├── BeneathUncountedHeav.uproject
├── opencode.json                     ← MCP url http://127.0.0.1:8000/mcp
├── todo.md                           ← phase spec (working contract)
├── AGENTS.md                         ← always-in-context essentials (this file)
├── notes.md                          ← detailed gotchas, visual-test/self-wake, known limitations
└── Content/BUH/Prototypes/TacticalDuel/
    ├── Lvl_TacticalDuel.umap         ← arena: floor, dedicated CameraActor, 2 tagged fighters, BP_CombatManager
    ├── Blueprints/
    │   ├── BP_Fighter.uasset         ← capsule root + BodyMesh StaticMeshComponent, Team (0/1)
    │   ├── BP_CombatManager.uasset   ← resolver + SelectMove/ExecuteMove/ResetEncounter + BeginPlay (sets view target to CameraActor)
    │   └── WBP_CombatUI.uasset       ← health/move/prediction text, 5 buttons, RefreshUI
    └── Materials/
        ├── M_FighterBase.uasset      ← base material with Color parameter
        ├── MI_Player / MI_Opponent   ← team colors
        └── MI_Hit / MI_Block         ← flash materials
```

## Architecture & Combat Design
- Moves are ints: `0=Quick Strike` (6 startup, 10 dmg), `1=Heavy Strike` (13 startup, 24 dmg), `2=Guard` (0 startup, blocks). Opponent always uses Heavy Strike (`OpponentMove=1`).
- Resolver rules: faster startup interrupts slower; equal startup → both connect; Guard blocks. Prediction and execution share the same resolver (no hardcoded UI predictions).
- `BP_CombatManager` state ints: `0=Planning`, `1=Resolving`, `2=Done`.
- `WBP_CombatUI` hides planning buttons when `State != 0` and shows only Reset when `State == 2` (handles Phase 5 "hide controls" + "allow reset").
- `ExecuteMove` resolves damage, lunges each fighter ±200 on X, and flashes `MI_Block` (if that side Guarded) or `MI_Hit` (if that side took damage). `ResetEncounter` restores positions `(±500,0,90)` and team materials.

## Workflow (after every asset change)
1. **Post-compact check**: If compacted since last read, re-read this file in full.
2. **Edit** the asset via the correct MCP toolset (BlueprintTools / ObjectTools / SceneTools / UMGToolSet).
3. **Compile** the affected Blueprint(s); inspect for compile errors.
4. **Validate in PIE**: `StartPIE`, `CaptureEditorImage`, inspect the game view + `GetLogEntries` + actor properties. Confirm the actual values, not just "no crash".
5. **Save** with `AssetTools.save_assets` (package paths, `asset_paths`). Stop PIE before saving a BP.
6. **If issues found**: go back to step 2.
7. **Verify**: re-read this file in full. Confirm steps 2–6 completed. Update `todo.md` checkboxes.
8. **Record the fix**: After verifying a fix worked, add an entry to `notes.md` §9 documenting the problem, cause, verified fix, and lesson learned. Do not skip this step — future sessions rely on `notes.md` to avoid re-diagnosing solved problems.
9. **Clean notes.md**: Remove or update any out of date or incorrect entries. 

Do NOT report "done" until all steps pass.

## Keep Docs DRY
- `todo.md` is the phase spec and tracks phase state — update checkboxes as work completes.
- `AGENTS.md` (this file) holds only what every operation needs: environment, MCP protocol, essentials, structure, combat design, workflow.
- `notes.md` holds the detailed toolset gotchas, the graphical visual-test/self-wake procedure, and known limitations. Consult it when a specific problem arises. Don't duplicate across the three.
