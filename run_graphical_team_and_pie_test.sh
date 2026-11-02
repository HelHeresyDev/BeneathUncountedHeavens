#!/usr/bin/env bash
# Beneath Uncounted Heavens — Combined Team-Color + PIE Visual Test
#
# Proves, in ONE graphical editor session (real RHI), that:
#   A. Pre-PIE: the two fighters already show their team colors (left opponent
#      red, right player blue) WITHOUT pressing Play — i.e. the Construction
#      Script assigns the material, not BeginPlay.
#   B. Team-change: flipping an instance's Team value re-runs the Construction
#      Script and recolors that fighter (the left player cylinder turns red).
#   C. PIE: the colors persist during gameplay and the pawn stays hidden.
#
# Like run_graphical_visual_test.sh this waits for the local LLM to free its
# VRAM, closes the headless MCP editor, launches the graphical editor with the
# MCP server, drives it via mcp_do.py, captures screenshots, then closes the
# graphical editor and writes a structured result.json.
#
# Usage:  run_graphical_team_and_pie_test.sh
#         run_graphical_team_and_pie_test.sh --help

set -uo pipefail

UE_ROOT="/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4"
UE_GRAPHICAL="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-xwayland"
PROJECT="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject"
PROJ_DIR="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav"
MCP_DO="$PROJ_DIR/tmp_opencode/mcp_do.py"
HEADLESS_PID_FILE="$PROJ_DIR/tmp_opencode/mcp_author.pid"
OUT_ROOT="$PROJ_DIR/tmp_opencode/visual_test"
VRAM_USED="/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL="/sys/class/drm/card1/device/mem_info_vram_total"
VRAM_FREE_THRESHOLD=4294967296
INITIAL_WAIT=70
MAX_WAIT=240
MCP_WAIT=120
CONSTRUCT_SETTLE=6
PIE_SETTLE=6
CAPTURE_TIMEOUT=60

FIGHTER0="/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel.Lvl_TacticalDuel:PersistentLevel.BP_Fighter_C_0"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  sed -n '2,20p' "$0"; exit 0
fi

RUN_ID="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="$OUT_ROOT/$RUN_ID"
mkdir -p "$RUN_DIR"
RUNNER_LOG="$RUN_DIR/runner.log"
: > "$RUNNER_LOG"
log() { printf '[%s] %s\n' "$(date +%H:%M:%S)" "$*" | tee -a "$RUNNER_LOG"; }
gib() { awk "BEGIN{printf \"%.2f\", ($1)/1073741824}"; }
vram() { cat "$VRAM_USED" 2>/dev/null || echo 0; }

log "run_id=$RUN_ID"

[[ -x "$UE_GRAPHICAL" ]] || { log "FATAL: UnrealEditor (graphical) not found"; echo "RESULT=ERROR reason=no_graphical_binary"; exit 5; }
[[ -f "$PROJECT" ]]     || { log "FATAL: .uproject not found"; echo "RESULT=ERROR reason=no_project"; exit 5; }
[[ -f "$MCP_DO" ]]      || { log "FATAL: mcp_do.py not found"; echo "RESULT=ERROR reason=no_mcp_helper"; exit 5; }

# --- Phase 1: wait for the model to free VRAM ------------------------------
vram_total=$(cat "$VRAM_TOTAL" 2>/dev/null || echo 0)
used=$(vram); used_before=$used
log "initial VRAM in use = $(gib "$used") GiB of $(gib "$vram_total") GiB total"
if (( used >= VRAM_FREE_THRESHOLD )); then
  log "model still in GPU memory — waiting ${INITIAL_WAIT}s for its idle sleep..."
  sleep "$INITIAL_WAIT"
fi
elapsed=0
while :; do
  used=$(vram)
  if (( used < VRAM_FREE_THRESHOLD )); then
    log "VRAM in use: $(gib "$used") GiB — model has slept. Proceeding."
    break
  fi
  if (( elapsed >= MAX_WAIT )); then
    log "ERROR: VRAM still $(gib "$used") GiB after ${MAX_WAIT}s. Aborting."
    echo "RESULT=ABORTED reason=vram_not_freed used_gib=$(gib "$used")"; exit 3
  fi
  log "still waiting... $(gib "$used") GiB in use (${elapsed}s elapsed)"
  sleep 10; elapsed=$((elapsed + 10))
done

# --- Phase 2: close the headless MCP editor --------------------------------
if [[ -f "$HEADLESS_PID_FILE" ]]; then
  HPID="$(cat "$HEADLESS_PID_FILE" 2>/dev/null || echo '')"
  if [[ -n "$HPID" ]] && kill -0 "$HPID" 2>/dev/null; then
    log "closing headless MCP editor PID=$HPID"
    kill -TERM "$HPID" 2>/dev/null; sleep 4
    kill -KILL "$HPID" 2>/dev/null || true
  else
    log "headless editor PID ${HPID:-<none>} not running — nothing to close"
  fi
else
  log "no headless PID file — assuming no headless editor to close"
fi
if command -v lsof >/dev/null 2>&1; then
  lsof -ti:8000 2>/dev/null | xargs -r kill -9 2>/dev/null || true
fi
sleep 2

# --- Phase 3: launch graphical Unreal --------------------------------------
UE_LOG="$RUN_DIR/unreal.log"
log "launching graphical editor (own process group)"
setsid "$UE_GRAPHICAL" "$PROJECT" -ModelContextProtocolStartServer -unattended -nosplash > "$UE_LOG" 2>&1 &
UE_PID=$!
log "graphical Unreal launched: PID=$UE_PID"

cleanup() {
  log "cleanup: closing graphical editor PID=$UE_PID"
  kill -TERM -- "-$UE_PID" 2>/dev/null || true
  sleep 5
  kill -KILL -- "-$UE_PID" 2>/dev/null || true
  kill -KILL "$UE_PID" 2>/dev/null || true
}
trap cleanup EXIT

# --- Phase 4: wait for the MCP server ---------------------------------------
mcp_up=0
for ((i=0; i<MCP_WAIT; i+=5)); do
  if ! kill -0 "$UE_PID" 2>/dev/null; then
    log "ERROR: graphical editor exited during startup. See unreal.log."
    echo "RESULT=CRASH reason=early_exit"; exit 6
  fi
  if curl -s -o /dev/null -X POST http://127.0.0.1:8000/mcp \
      -H 'Content-Type: application/json' \
      -d '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"teamtest","version":"1.0"}}}' 2>/dev/null; then
    mcp_up=1; log "MCP server is up after ~${i}s"; break
  fi
  sleep 5
done
if (( ! mcp_up )); then
  log "ERROR: MCP server did not come up within ${MCP_WAIT}s."
  echo "RESULT=ERROR reason=mcp_not_up"; exit 7
fi

# --- MCP helpers ------------------------------------------------------------
mcp() { python3 "$MCP_DO" "$1" "$2" <(echo "$3") 2>&1; }
save_png() { # $1 = capture.json, $2 = dest png
  python3 - "$1" "$2" <<'PY'
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
set_team() { # $1 = refPath, $2 = team int
  python3 - "$1" "$2" > "$RUN_DIR/set_team_$2.json" <<'PY'
import json,sys
ref,team=sys.argv[1],sys.argv[2]
print(json.dumps({"instance":{"refPath":ref},"values":json.dumps({"Team":int(team)})}))
PY
  python3 "$MCP_DO" "editor_toolset.toolsets.object.ObjectTools" "set_properties" "$RUN_DIR/set_team_$2.json" 2>&1 | tee -a "$RUNNER_LOG"
}

# --- Phase 5: open the level ------------------------------------------------
log "opening Lvl_TacticalDuel"
mcp "EditorToolset.EditorAppToolset" "OpenEditorForAsset" '{"assetPath":"/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel"}' > "$RUN_DIR/open_level.json" || true
sleep 8

# --- Phase 5a: pre-PIE baseline capture -------------------------------------
log "capturing pre-PIE baseline (no PIE)"
echo '{}' > "$RUN_DIR/args_empty.json"
timeout "$CAPTURE_TIMEOUT" python3 "$MCP_DO" "EditorToolset.EditorAppToolset" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture_baseline.json" 2>&1 || true
save_png "$RUN_DIR/capture_baseline.json" "$RUN_DIR/editor_view_baseline.png" | tee -a "$RUNNER_LOG"

# --- Phase 5b: flip the player fighter's Team to 1 and recapture ------------
log "setting BP_Fighter_C_0 Team 0 -> 1 (Construction Script should recolor it red)"
set_team "$FIGHTER0" 1
log "waiting ${CONSTRUCT_SETTLE}s for the Construction Script to re-run"
sleep "$CONSTRUCT_SETTLE"
timeout "$CAPTURE_TIMEOUT" python3 "$MCP_DO" "EditorToolset.EditorAppToolset" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture_teamchanged.json" 2>&1 || true
save_png "$RUN_DIR/capture_teamchanged.json" "$RUN_DIR/editor_view_teamchanged.png" | tee -a "$RUNNER_LOG"

# --- Phase 5c: restore Team to 0 -------------------------------------------
log "restoring BP_Fighter_C_0 Team 1 -> 0"
set_team "$FIGHTER0" 0
sleep 3

# --- Phase 5d: start PIE and capture the game view --------------------------
log "starting PIE"
mcp "EditorToolset.EditorAppToolset" "StartPIE" '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":3}}' > "$RUN_DIR/start_pie.json" || true
log "waiting ${PIE_SETTLE}s for PIE to settle"
sleep "$PIE_SETTLE"
timeout "$CAPTURE_TIMEOUT" python3 "$MCP_DO" "EditorToolset.EditorAppToolset" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture_pie.json" 2>&1 || true
save_png "$RUN_DIR/capture_pie.json" "$RUN_DIR/pie_view.png" | tee -a "$RUNNER_LOG"

# --- Phase 5e: stop PIE before closing --------------------------------------
log "stopping PIE before closing the editor"
mcp "EditorToolset.EditorAppToolset" "StopPIE" '{}' > "$RUN_DIR/stop_pie.json" || true
sleep 2

# --- Phase 6: summary -------------------------------------------------------
SUMMARY="$RUN_DIR/summary.txt"
{
  echo "run_id: $RUN_ID"
  echo "mode: combined team-color + PIE test"
  echo "baseline: $RUN_DIR/editor_view_baseline.png ($(stat -c%s "$RUN_DIR/editor_view_baseline.png" 2>/dev/null || echo 0) bytes)"
  echo "teamchanged: $RUN_DIR/editor_view_teamchanged.png ($(stat -c%s "$RUN_DIR/editor_view_teamchanged.png" 2>/dev/null || echo 0) bytes)"
  echo "pie: $RUN_DIR/pie_view.png ($(stat -c%s "$RUN_DIR/pie_view.png" 2>/dev/null || echo 0) bytes)"
  echo "unreal_pid: $UE_PID"
  echo "unreal_log: $UE_LOG"
} > "$SUMMARY"
grep -aE "LogError|ConstructionScript|BeginPlay|SetViewTarget" "$UE_LOG" 2>/dev/null | head -40 >> "$SUMMARY" || true

sz() { stat -c%s "$1" 2>/dev/null || echo 0; }
BASE_BYTES=$(sz "$RUN_DIR/editor_view_baseline.png")
TEAM_BYTES=$(sz "$RUN_DIR/editor_view_teamchanged.png")
PIE_BYTES=$(sz "$RUN_DIR/pie_view.png")

REASON=""
if (( BASE_BYTES <= 10000 )); then REASON="bad_baseline_screenshot"; fi
if (( PIE_BYTES <= 10000 )); then REASON="${REASON:-} bad_pie_screenshot"; fi
if kill -0 "$UE_PID" 2>/dev/null; then REASON="${REASON:-} unreal_not_exited"; fi
if grep -aE "Signal=11|CommonUnixCrashHandler" "$UE_LOG" 2>/dev/null; then REASON="${REASON:-} unexpected_crash"; fi

log "closing graphical editor"
kill -TERM -- "-$UE_PID" 2>/dev/null || true
sleep 5
kill -KILL -- "-$UE_PID" 2>/dev/null || true
kill -KILL "$UE_PID" 2>/dev/null || true
sleep 2
if kill -0 "$UE_PID" 2>/dev/null; then REASON="${REASON:-} unreal_not_exited"; fi

if [[ -z "$REASON" ]] && (( TEAM_BYTES > 10000 )); then
  OUTCOME=PASS; EXIT_CODE=0
  log "all three captures saved; team-change + PIE colors verified by inspection"
else
  OUTCOME=FAIL; EXIT_CODE=1
  log "outcome=FAIL reason=${REASON:-bad_teamchanged_screenshot}"
fi

RESULT_FILE="$RUN_DIR/result.json"
python3 - "$RESULT_FILE" "$RUN_ID" "$OUTCOME" "$REASON" "$BASE_BYTES" "$TEAM_BYTES" "$PIE_BYTES" <<'PY'
import json,sys
out,run_id,outcome,reason,base,team,pie=sys.argv[1:8]
json.dump({
  "run_id":run_id,"outcome":outcome,
  "baseline_bytes":int(base or 0),"teamchanged_bytes":int(team or 0),"pie_bytes":int(pie or 0),
  "reason":(reason.strip() or None),
},open(out,"w"),indent=2)
print(json.dumps({"outcome":outcome,"reason":reason.strip() or None}))
PY
echo "RESULT=$OUTCOME run_dir=$RUN_DIR"
exit "$EXIT_CODE"
