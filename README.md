# Beneath Uncounted Heavens

**Beneath Uncounted Heavens** is a third-person 3D cultivation sandbox RPG with tactical, instant-based combat. The public repository currently contains the **TacticalDuel** prototype, which is used to validate the full Unreal Engine 5.8.4 + Linux + MCP development loop.

> One soul. Countless lives. Infinite paths.

## Why this repository is public

The repository is public so that ChatGPT and other assistants can read the design documents, source code, Blueprint references, test scripts, and troubleshooting notes without manual file transfer.

Public visibility is **viewing only**. The `LICENSE` grants no permission to copy, modify, distribute, or use the project without written permission from the copyright holder.

## Current prototype

The playable prototype lives at:

```
/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel
```

It contains:

- Two tagged fighter actors (`BP_Fighter`)
- An authoritative combat manager (`BP_CombatManager`)
- A combat UI widget (`WBP_CombatUI`)
- A reusable ground path renderer (`ABUHPathRenderer`)
- Team, hit, block, ghost, and path materials

### Combat moves

Moves are integer action IDs in `UBuhCombatSim`:

| ID | Name | Role |
|----|------|------|
| 0 | Quick | Fast strike |
| 1 | Heavy | Slow high-damage strike |
| 2 | Guard | Blocks incoming strikes |
| 3 | Dodge | Evades and repositions |
| 4 | Walk Toward | Directional walk toward the opponent |
| 5 | Walk Away | Directional walk away from the opponent |
| 6 | Walk | Confirmed 2D ground walk to a mouse-selected destination |
| 7 | Continue | Continue an interrupted walk |
| 8 | Dash | Fast movement with a reaction window after 3 locked Instants |

### Resolver rules

- Faster startup interrupts slower startup.
- Equal startup means both actions connect.
- Guard blocks strikes.
- Prediction and execution use the same resolver; the UI does not hardcode outcomes.
- `BP_CombatManager` state IDs: `0=Planning`, `1=Resolving`, `2=Done`.

### Path renderer

`ABUHPathRenderer` draws persistent green/red path segments on the ground. Green means the remaining path is still legally reachable; red means the path is locked or no longer reachable at the current reaction point.

## Repository layout

```
BeneathUncountedHeav/
├── BeneathUncountedHeav.uproject
├── AGENTS.md
├── README.md
├── LICENSE
├── designbible.md
├── todo.md
├── notes.md
├── opencode.json
├── Config/
├── Content/
│   ├── BUH/Prototypes/TacticalDuel/
│   ├── Characters/Mannequins/
│   └── DemoTemplate/
├── Source/BeneathUncountedHeav/
│   ├── BeneathUncountedHeav.Build.cs
│   ├── BeneathUncountedHeav.cpp
│   ├── BeneathUncountedHeav.h
│   ├── CombatSim.cpp
│   ├── CombatSim.h
│   ├── CombatSimAutomation.cpp
│   ├── CombatManagerIntegrationAutomation.cpp
│   ├── PathRenderer.cpp
│   ├── PathRenderer.h
│   ├── PathRendererAutomation.cpp
│   ├── BUHCombatUI.cpp
│   └── BUHCombatUI.h
└── tests/
    ├── run_combat_flow_test.sh
    ├── run_graphical_visual_test.sh
    ├── run_unreal_test.sh
    └── ...
```

`Content/Characters/Mannequins/` and `Content/DemoTemplate/` are Unreal Engine sample content included for the prototype.

## Documentation map

| File | Purpose |
|------|---------|
| `designbible.md` | Full creative direction, cosmology, reincarnation loop, and combat design |
| `todo.md` | Current working contract and phase status |
| `AGENTS.md` | Always-in-context agent rules, environment, MCP protocol, and workflow |
| `notes.md` | Detailed troubleshooting history, verified fixes, and known limitations |
| `chatgpt_report.md` | Report written for external assistant review |
| `phase9b_report.md` | Phase 9B validation report |
| `phase9c_report.md` | Phase 9C validation report |
| `verification_report.md` | Full prototype verification report |
| `feedback.md` | Captured engine log feedback |

## Requirements

- Unreal Engine **5.8.4** on Linux
- Engine path: `/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4`
- Project path: `/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav`
- AMD GPU stack (RX 7900 XTX on the development machine); avoid CUDA-specific assumptions
- Python 3.14 is available for offline analysis but is not required by the game

No Git LFS is used. All `.uasset` and `.umap` files are ordinary Git blobs.

## Build C++

From the project root:

```sh
/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4/Engine/Build/BatchFiles/Linux/Build.sh \
  "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject" \
  Linux Development
```

The C++ module is `BeneathUncountedHeav`.

## Launch the editor

Graphical editor with MCP:

```sh
nohup /home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4/Engine/Binaries/Linux/UnrealEditor-xwayland \
  "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject" \
  -ModelContextProtocolStartServer > /home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/tmp_opencode/unreal_editor.log 2>&1 &
```

Headless editor with MCP:

```sh
nohup /home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd \
  "/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject" \
  -nullrhi -unattended -nosplash -ModelContextProtocolStartServer > /home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/tmp_opencode/mcp_author.log 2>&1 &
```

After launch, open:

```
/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel
```

The `UnrealEditor-xwayland` binary is a wrapper that forces X11/XCB environment variables so the graphical editor does not freeze GNOME Shell on Wayland. The real ELF must remain named `UnrealEditor`.

## MCP workflow

`opencode.json` configures a local Unreal MCP server at:

```
http://127.0.0.1:8000/mcp
```

The server exists only when the editor is launched with `-ModelContextProtocolStartServer`.

The local helper `tmp_opencode/mcp_do.py` is **not committed** because `tmp_opencode/` is ignored. It performs a fresh JSON-RPC handshake on every call and is the reliable path when the agent's `unreal-mcp_*` meta-tools are absent.

Common MCP operations:

- `AssetTools.save_assets` — save by package path array
- `EditorToolset.EditorAppToolset.StartPIE` — start PIE
- `EditorToolset.EditorAppToolset.StopPIE` — stop PIE
- `EditorToolset.EditorAppToolset.CaptureEditorImage` — capture the PIE game view
- `LogsToolset.GetLogEntries` — read engine logs
- `AutomationTestToolset.DiscoverTests` / `ListTests` / `RunTests` / `GetTestResults` — run automation tests

Headless `-nullrhi` PIE cannot tick widget `EventTick`, show the mouse cursor, or provide an `APlayerController`. Visual cursor and path-color confirmation requires a graphical PIE session.

## Tests

### Automation suites

| Test | Covers |
|------|--------|
| `BUH.CombatSimTests` | Resolver, timeline generation, prediction, and movement math |
| `BUH.CombatFlowTests` | Full encounter flows and stop reasons |
| `BUH.CombatManagerIntegrationTests` | `BP_CombatManager` integration with the C++ resolver |
| `BUH.WalkPreviewTests` | Walk targeting, preview line, and confirmation behavior |
| `BUH.PathRendererTests` | Path renderer lifecycle, segment pool, and leak checks |
| `BUH.DashPathColorTests` | Dash red/green boundary for every elapsed-Instant count |
| `BUH.DashReactionIntegrationTest` | Real reaction pause producing a fully green remaining Dash path |

`BUH.PieDashDiagTest` is a temporary live-PIE diagnostic and is normally excluded from regression runs.

All seven stable `BUH.*` suites pass with 0 failures as of the current commit.

### Shell test scripts

`tests/` contains helper scripts for manual and graphical validation:

- `run_combat_flow_test.sh`
- `run_graphical_editor_view_test.sh`
- `run_graphical_team_and_pie_test.sh`
- `run_graphical_visual_test.sh`
- `run_leftpanel_diagnostic.sh`
- `run_leftpanel_screenshot.sh`
- `run_manual_pie_clickthrough.sh`
- `run_phase8_console_shots.sh`
- `run_phase8_direct_shots.sh`
- `run_phase8_screenshots.sh`
- `run_phase8_slider_shots.sh`
- `run_ui_screenshot.sh`
- `run_unreal_test.sh`

These scripts are diagnostics and capture helpers, not a CI test runner.

## Console commands

`UBuhCombatSim` registers several console commands for manual timeline inspection:

- `BUH.SetupTimeline <PlayerMove> <OpponentMove> <Instant>`
- `BUH.MoveGhosts <Instant>`
- `BUH.PrintSnapshot <Index>`
- `BUH.DebugSnapshot <Index>`
- `BUH.DiagnoseUI`
- `BUH.SetSlot <WidgetName> ...`
- `BUH.SetSlotAsset <WidgetName> ...`
- `BUH.LogFighterMats`
- `BUH.SetBlueMat`

`UBUHCombatUI` registers:

- `BUH.ScrubTimeline`
- `BUH.InspectLeftPanel`

## License

Copyright (c) 2026 HelHeresyDev. All rights reserved. See `LICENSE`.
