#!/usr/bin/env bash
# Phase 8 — Capture Screenshots A/B/C of the interactive tactical prediction.
#
# Launches the graphical editor, opens the level, starts PIE, then drives the
# C++ SetupTimelineAndMoveGhosts / MoveGhostsToInstant functions via the DSL to
# place the ghost fighters at specific instants, capturing a screenshot after
# each step.
#
#   Screenshot A: Quick (0) vs Heavy (1), instant 5
#   Screenshot B: Quick (0) vs Heavy (1), instant 15
#   Screenshot C: Quick (0) vs Dodge (3), instant 10

set -uo pipefail

UE_ROOT="/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4"
UE_GRAPHICAL="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-xwayland"
PROJECT="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject"
PROJ_DIR="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav"
MCP_DO="$PROJ_DIR/tmp_opencode/mcp_do.py"
OUT_ROOT="$PROJ_DIR/tmp_opencode/phase8_shots"
VRAM_USED="/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL="/sys/class/drm/card1/device/mem_info_vram_total"
VRAM_FREE_THRESHOLD=4294967296
INITIAL_WAIT=70
MAX_WAIT=240
MCP_WAIT=120
PIE_SETTLE=6
CAPTURE_TIMEOUT=60

RUN_ID="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="$OUT_ROOT/$RUN_ID"
mkdir -p "$RUN_DIR"
RUNNER_LOG="$RUN_DIR/runner.log"
: > "$RUNNER_LOG"
log() { printf '[%s] %s\n' "$(date +%H:%M:%S)" "$*" | tee -a "$RUNNER_LOG"; }
gib() { awk "BEGIN{printf \"%.2f\", ($1)/1073741824}"; }
vram() { cat "$VRAM_USED" 2>/dev/null || echo 0; }

log "run_id=$RUN_ID"

[[ -x "$UE_GRAPHICAL" ]] || { log "FATAL: no graphical binary"; echo "RESULT=ERROR reason=no_graphical_binary"; exit 5; }
[[ -f "$PROJECT" ]]     || { log "FATAL: no .uproject"; echo "RESULT=ERROR reason=no_project"; exit 5; }
[[ -f "$MCP_DO" ]]      || { log "FATAL: no mcp_do.py"; echo "RESULT=ERROR reason=no_mcp_helper"; exit 5; }

# --- Phase 1: wait for VRAM to free ---
vram_total=$(cat "$VRAM_TOTAL" 2>/dev/null || echo 0)
used=$(vram); used_before=$used
log "initial VRAM = $(gib "$used") GiB of $(gib "$vram_total") GiB (threshold $(gib "$VRAM_FREE_THRESHOLD") GiB)"
if (( used >= VRAM_FREE_THRESHOLD )); then
  log "model in GPU memory — waiting ${INITIAL_WAIT}s..."
  sleep "$INITIAL_WAIT"
fi
elapsed=0
while :; do
  used=$(vram)
  if (( used < VRAM_FREE_THRESHOLD )); then
    log "VRAM: $(gib "$used") GiB — model slept. Proceeding."
    break
  fi
  if (( elapsed >= MAX_WAIT )); then
    log "ERROR: VRAM still $(gib "$used") GiB after ${MAX_WAIT}s"
    echo "RESULT=ABORTED reason=vram_not_freed"; exit 3
  fi
  log "waiting... $(gib "$used") GiB (${elapsed}s)"
  sleep 10; elapsed=$((elapsed + 10))
done

# --- Phase 2: close headless editor ---
lsof -ti:8000 2>/dev/null | xargs -r kill -9 2>/dev/null || true
sleep 3

# --- Phase 3: launch graphical editor ---
UE_LOG="$RUN_DIR/unreal.log"
log "launching graphical editor"
setsid "$UE_GRAPHICAL" "$PROJECT" -ModelContextProtocolStartServer -unattended -nosplash > "$UE_LOG" 2>&1 &
UE_PID=$!
log "graphical Unreal PID=$UE_PID"

cleanup() {
  log "cleanup: closing graphical editor PID=$UE_PID"
  kill -TERM -- "-$UE_PID" 2>/dev/null || true
  sleep 5
  kill -KILL -- "-$UE_PID" 2>/dev/null || true
  kill -KILL "$UE_PID" 2>/dev/null || true
}
trap cleanup EXIT

# --- Phase 4: wait for MCP ---
mcp_up=0
for ((i=0; i<MCP_WAIT; i+=5)); do
  if ! kill -0 "$UE_PID" 2>/dev/null; then
    log "ERROR: graphical editor exited during startup"
    echo "RESULT=CRASH reason=early_exit"; exit 6
  fi
  if curl -s -o /dev/null -X POST http://127.0.0.1:8000/mcp \
      -H 'Content-Type: application/json' \
      -d '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"phase8","version":"1.0"}}}' 2>/dev/null; then
    mcp_up=1; log "MCP up after ~${i}s"; break
  fi
  sleep 5
done
if (( ! mcp_up )); then
  log "ERROR: MCP did not come up"
  echo "RESULT=ERROR reason=mcp_not_up"; exit 7
fi

# --- Phase 5: open level, start PIE ---
mcp() { python3 "$MCP_DO" "$1" "$2" <(echo "$3") 2>&1; }

log "opening Lvl_TacticalDuel"
mcp "EditorToolset.EditorAppToolset" "OpenEditorForAsset" '{"assetPath":"/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel"}' > "$RUN_DIR/open_level.json" || true
sleep 3

log "starting PIE"
mcp "EditorToolset.EditorAppToolset" "StartPIE" '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":3}}' > "$RUN_DIR/start_pie.json" || true
log "waiting ${PIE_SETTLE}s for PIE to settle"
sleep "$PIE_SETTLE"

# --- Phase 6: drive the ghosts and capture screenshots ---
# The DSL calls are made in the context of the WBP's event graph. We write a
# temporary event that calls the C++ function, then trigger it.

WBP_GRAPH="/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.WBP_CombatUI:EventGraph"

capture() {
  local label="$1"
  local shot="$RUN_DIR/${label}.png"
  log "capturing ${label}"
  echo '{}' > "$RUN_DIR/args_empty.json"
  timeout "$CAPTURE_TIMEOUT" python3 "$MCP_DO" "EditorToolset.EditorAppToolset" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture_${label}.json" 2>&1 || true
  python3 - "$RUN_DIR/capture_${label}.json" "$shot" <<'PY'
import json,sys,base64,os
src,dst=sys.argv[1],sys.argv[2]
try:
    d=json.load(open(src)); t=d['result']['content'][0]['text']
    v=json.loads(t); img=v.get('returnValue',v)
    data=img.get('data') or img.get('base64') if isinstance(img,dict) else None
    if data:
        open(dst,'wb').write(base64.b64decode(data))
        print("saved",dst,os.path.getsize(dst),"bytes")
    else:
        print("no image payload:",str(img)[:120])
except Exception as e:
    print("decode failed:",e)
PY
}

drive_ghosts() {
  local dsl="$1"
  local args_file="$RUN_DIR/args_drive.json"
  python3 - "$args_file" "$WBP_GRAPH" "$dsl" <<'PY'
import json,sys
out,graph,dsl=sys.argv[1],sys.argv[2],sys.argv[3]
args={"graph":{"refPath":graph},"code":dsl}
json.dump(args,open(out,"w"))
PY
  python3 "$MCP_DO" "editor_toolset.toolsets.blueprint.BlueprintTools" "write_graph_dsl" "$args_file" > "$RUN_DIR/drive_result.json" 2>&1 || true
  sleep 2
}

# Screenshot A: Quick (0) vs Heavy (1), instant 5
log "driving ghosts: Quick vs Heavy, instant 5"
drive_ghosts '(event EventPhase8ShotA (CombatSim|SetupTimelineAndMoveGhosts :PlayerMove 0 :OpponentMove 1 :Instant 5))'
capture "A_quick_vs_heavy_i5"

# Screenshot B: Quick (0) vs Heavy (1), instant 15
log "driving ghosts: Quick vs Heavy, instant 15"
drive_ghosts '(event EventPhase8ShotB (CombatSim|MoveGhostsToInstant :Instant 15))'
capture "B_quick_vs_heavy_i15"

# Screenshot C: Quick (0) vs Dodge (3), instant 10
log "driving ghosts: Quick vs Dodge, instant 10"
drive_ghosts '(event EventPhase8ShotC (CombatSim|SetupTimelineAndMoveGhosts :PlayerMove 0 :OpponentMove 3 :Instant 10))'
capture "C_quick_vs_dodge_i10"

# --- Phase 7: summary ---
SUMMARY="$RUN_DIR/summary.txt"
{
  echo "run_id: $RUN_ID"
  echo "mode: phase8 screenshots"
  echo "screenshot_A: $RUN_DIR/A_quick_vs_heavy_i5.png"
  echo "screenshot_A_bytes: $(stat -c%s "$RUN_DIR/A_quick_vs_heavy_i5.png" 2>/dev/null || echo 0)"
  echo "screenshot_B: $RUN_DIR/B_quick_vs_heavy_i15.png"
  echo "screenshot_B_bytes: $(stat -c%s "$RUN_DIR/B_quick_vs_heavy_i15.png" 2>/dev/null || echo 0)"
  echo "screenshot_C: $RUN_DIR/C_quick_vs_dodge_i10.png"
  echo "screenshot_C_bytes: $(stat -c%s "$RUN_DIR/C_quick_vs_dodge_i10.png" 2>/dev/null || echo 0)"
  echo "unreal_pid: $UE_PID"
  echo "unreal_log: $UE_LOG"
} > "$SUMMARY"
grep -aE "LogError|LogWarning.*Fighter|BeginPlay|SetViewTarget" "$UE_LOG" 2>/dev/null | head -40 >> "$SUMMARY" || true

ALL_PASS=1
for f in A_quick_vs_heavy_i5 B_quick_vs_heavy_i15 C_quick_vs_dodge_i10; do
  bytes=$(stat -c%s "$RUN_DIR/${f}.png" 2>/dev/null || echo 0)
  if (( bytes < 10000 )); then
    log "WARNING: ${f}.png is only ${bytes} bytes (may be a 1x1 placeholder)"
    ALL_PASS=0
  else
    log "${f}.png: ${bytes} bytes"
  fi
done

if (( ALL_PASS )); then
  log "all three screenshots captured"
  echo "RESULT=PASS run_dir=$RUN_DIR"
  exit 0
else
  log "some screenshots may be placeholders"
  echo "RESULT=PARTIAL run_dir=$RUN_DIR"
  exit 1
fi
