#!/usr/bin/env bash
# Beneath Uncounted Heavens — Graphical Editor-View (pre-PIE) Capture
#
# Verifies that the two fighters render their TEAM colors in the level editor
# BEFORE pressing Play. The team color is now assigned in the BP_Fighter
# Construction Script (runs when the actor is constructed, in the editor and at
# runtime), so the editor viewport should already show blue (player) and red
# (opponent) cylinders with no PIE involved.
#
# Sequence:
#   1. Wait for the model's idle unload (VRAM drops below threshold).
#   2. Close the headless MCP editor owned by this workflow (mcp_author.pid).
#   3. Launch graphical UnrealEditor with -ModelContextProtocolStartServer.
#   4. Wait for the MCP server on port 8000.
#   5. Drive via mcp_do.py: open Lvl_TacticalDuel, wait for construction,
#      CaptureEditorImage (NO PIE).
#   6. Save the screenshot + logs + a summary under tmp_opencode/visual_test/<run_id>/.
#   7. Close ONLY the graphical editor this script launched and verify it exited.
#   8. Write a structured result.json and exit with an outcome code.

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
LEVEL_SETTLE=12        # seconds for the level to load + construction scripts to run
CAPTURE_TIMEOUT=60

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

log "run_id=$RUN_ID (editor-view pre-PIE)"

[[ -x "$UE_GRAPHICAL" ]] || { log "FATAL: no graphical binary"; echo "RESULT=ERROR reason=no_graphical_binary"; exit 5; }
[[ -f "$PROJECT" ]]     || { log "FATAL: no .uproject"; echo "RESULT=ERROR reason=no_project"; exit 5; }
[[ -f "$MCP_DO" ]]      || { log "FATAL: no mcp_do.py"; echo "RESULT=ERROR reason=no_mcp_helper"; exit 5; }

# --- Phase 1: wait for the model to free VRAM ------------------------------
vram_total=$(cat "$VRAM_TOTAL" 2>/dev/null || echo 0)
used=$(vram); used_before=$used
log "initial VRAM in use = $(gib "$used") GiB of $(gib "$vram_total") GiB (threshold $(gib "$VRAM_FREE_THRESHOLD") GiB)"
if (( used >= VRAM_FREE_THRESHOLD )); then
  log "model still in GPU memory — waiting ${INITIAL_WAIT}s for its idle sleep..."
  sleep "$INITIAL_WAIT"
fi
elapsed=0
while :; do
  used=$(vram)
  if (( used < VRAM_FREE_THRESHOLD )); then
    log "VRAM in use: $(gib "$used") GiB (down from $(gib "$used_before") GiB) — model has slept. Proceeding."
    break
  fi
  if (( elapsed >= MAX_WAIT )); then
    log "ERROR: VRAM still $(gib "$used") GiB after ${MAX_WAIT}s. Aborting."
    echo "RESULT=ABORTED reason=vram_not_freed used_gib=$(gib "$used")"; exit 3
  fi
  log "still waiting... $(gib "$used") GiB in use (${elapsed}s elapsed)"
  sleep 10; elapsed=$((elapsed + 10))
done

# --- Phase 2: close the headless MCP editor ---------------------------------
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

# --- Phase 3: launch graphical Unreal ---------------------------------------
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
      -d '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"editortest","version":"1.0"}}}' 2>/dev/null; then
    mcp_up=1; log "MCP server is up after ~${i}s"; break
  fi
  sleep 5
done
if (( ! mcp_up )); then
  log "ERROR: MCP server did not come up within ${MCP_WAIT}s."
  echo "RESULT=ERROR reason=mcp_not_up"; exit 7
fi

# --- Phase 5: open the level, wait, capture the editor view (NO PIE) --------
mcp() { python3 "$MCP_DO" "$1" "$2" <(echo "$3") 2>&1; }

log "opening Lvl_TacticalDuel"
mcp "EditorToolset.EditorAppToolset" "OpenEditorForAsset" '{"assetPath":"/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel"}' > "$RUN_DIR/open_level.json" || true
log "waiting ${LEVEL_SETTLE}s for level load + construction scripts (no PIE)"
sleep "$LEVEL_SETTLE"

log "capturing the editor viewport (pre-PIE)"
echo '{}' > "$RUN_DIR/args_empty.json"
timeout "$CAPTURE_TIMEOUT" python3 "$MCP_DO" "EditorToolset.EditorAppToolset" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture.json" 2>&1 || true

SHOT="$RUN_DIR/editor_view.png"
python3 - "$RUN_DIR/capture.json" "$SHOT" <<'PY'
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

# --- Phase 6: save logs + summary -------------------------------------------
SUMMARY="$RUN_DIR/summary.txt"
{
  echo "run_id: $RUN_ID"
  echo "mode: graphical editor-view (pre-PIE) capture"
  echo "screenshot: $SHOT"
  echo "screenshot_bytes: $(stat -c%s "$SHOT" 2>/dev/null || echo 0)"
  echo "unreal_pid: $UE_PID"
  echo "unreal_log: $UE_LOG"
} > "$SUMMARY"
grep -aE "LogError|LogWarning.*Fighter|ConstructionScript|BeginPlay" "$UE_LOG" 2>/dev/null | head -40 >> "$SUMMARY" || true

# --- Phase 7: close the graphical editor, write the result, exit ------------
log "closing graphical editor"
kill -TERM -- "-$UE_PID" 2>/dev/null || true
sleep 5
kill -KILL -- "-$UE_PID" 2>/dev/null || true
kill -KILL "$UE_PID" 2>/dev/null || true
sleep 2

REASON=""
if kill -0 "$UE_PID" 2>/dev/null; then
  log "ERROR: graphical editor still alive after cleanup"
  REASON="unreal_not_exited"
fi
if grep -aE "Signal=11|CommonUnixCrashHandler" "$UE_LOG" 2>/dev/null; then
  log "ERROR: unexpected Unreal crash detected in unreal.log"
  REASON="unexpected_crash"
fi

SHOT_BYTES=$(stat -c%s "$SHOT" 2>/dev/null || echo 0)
if [[ -s "$SHOT" ]] && (( SHOT_BYTES > 10000 )) && [[ -z "$REASON" ]]; then
  OUTCOME=PASS; EXIT_CODE=0
  log "captured a real editor-view screenshot ($SHOT_BYTES bytes)"
else
  OUTCOME=FAIL; EXIT_CODE=1
  [[ -z "$REASON" ]] && REASON="bad_screenshot"
  log "screenshot missing/too small, unreal did not exit, or crash — outcome=FAIL"
fi

RESULT_FILE="$RUN_DIR/result.json"
python3 - "$RESULT_FILE" "$RUN_ID" "$OUTCOME" "$SHOT" "$SHOT_BYTES" "$UE_LOG" "$RUNNER_LOG" "$REASON" <<'PY'
import json,sys
out,run_id,outcome,shot,shot_bytes,ue_log,runner_log,reason=sys.argv[1:9]
data={
  "run_id":run_id,
  "outcome":outcome,
  "screenshot":shot,
  "screenshot_bytes":int(shot_bytes or 0),
  "unreal_log":ue_log,
  "runner_log":runner_log,
  "reason":reason or None,
}
json.dump(data,open(out,"w"),indent=2)
print(json.dumps(data))
PY
echo "RESULT=$OUTCOME run_dir=$RUN_DIR"
exit "$EXIT_CODE"
