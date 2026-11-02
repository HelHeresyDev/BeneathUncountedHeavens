#!/usr/bin/env bash
# Beneath Uncounted Heavens — Manual PIE Click-Through Runner (Walk Execution Repair §5)
#
# Launches the FULL graphical editor, opens Lvl_TacticalDuel, starts PIE, captures
# the initial-state screenshot, then KEEPS THE EDITOR OPEN so a human can click
# through the consecutive-action sequence (Move->Walk, set destination, Execute,
# Attack->Quick, Execute, Reset). It blocks until the human closes the editor,
# then frees the GPU and exits so the model can wake.
#
# Run via a BLOCKING bash tool call (long timeout) so the session waits (no
# inference) and llama.cpp idle-sleeps, freeing VRAM for the graphical editor.
#
# Usage:  run_manual_pie_clickthrough.sh

set -uo pipefail

UE_ROOT="/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4"
UE_GRAPHICAL="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-xwayland"
PROJECT="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject"
PROJ_DIR="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav"
MCP_DO="$PROJ_DIR/tmp_opencode/mcp_do.py"
HEADLESS_PID_FILE="$PROJ_DIR/tmp_opencode/mcp_author.pid"
OUT_ROOT="$PROJ_DIR/tmp_opencode/clickthrough"
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

[[ -x "$UE_GRAPHICAL" ]] || { log "FATAL: graphical binary missing"; echo "RESULT=ERROR reason=no_graphical_binary"; exit 5; }
[[ -f "$PROJECT" ]]     || { log "FATAL: .uproject missing"; echo "RESULT=ERROR reason=no_project"; exit 5; }
[[ -f "$MCP_DO" ]]      || { log "FATAL: mcp_do.py missing"; echo "RESULT=ERROR reason=no_mcp_helper"; exit 5; }

# --- Phase 1: wait for the model to free VRAM ------------------------------
vram_total=$(cat "$VRAM_TOTAL" 2>/dev/null || echo 0)
used=$(vram); used_before=$used
log "initial VRAM = $(gib "$used") GiB of $(gib "$vram_total") GiB (threshold $(gib "$VRAM_FREE_THRESHOLD") GiB)"
if (( used >= VRAM_FREE_THRESHOLD )); then
  log "model still on GPU — waiting ${INITIAL_WAIT}s for idle sleep..."
  sleep "$INITIAL_WAIT"
fi
elapsed=0
while :; do
  used=$(vram)
  if (( used < VRAM_FREE_THRESHOLD )); then
    log "VRAM $(gib "$used") GiB (down from $(gib "$used_before")) — model slept. Proceeding."
    break
  fi
  if (( elapsed >= MAX_WAIT )); then
    log "ERROR: VRAM still $(gib "$used") GiB after ${MAX_WAIT}s. Aborting."
    echo "RESULT=ABORTED reason=vram_not_freed"; exit 3
  fi
  log "waiting... $(gib "$used") GiB (${elapsed}s)"
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
    log "headless editor not running"
  fi
else
  log "no headless PID file"
fi
if command -v lsof >/dev/null 2>&1; then
  lsof -ti:8000 2>/dev/null | xargs -r kill -9 2>/dev/null || true
fi
sleep 2

# --- Phase 3: launch graphical Unreal --------------------------------------
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

# --- Phase 4: wait for MCP --------------------------------------------------
mcp_up=0
for ((i=0; i<MCP_WAIT; i+=5)); do
  if ! kill -0 "$UE_PID" 2>/dev/null; then
    log "ERROR: graphical editor exited during startup. See unreal.log."
    echo "RESULT=CRASH reason=early_exit"; exit 6
  fi
  if curl -s -o /dev/null -X POST http://127.0.0.1:8000/mcp \
      -H 'Content-Type: application/json' \
      -d '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"clickthrough","version":"1.0"}}}' 2>/dev/null; then
    mcp_up=1; log "MCP up after ~${i}s"; break
  fi
  sleep 5
done
if (( ! mcp_up )); then
  log "ERROR: MCP not up within ${MCP_WAIT}s."
  echo "RESULT=ERROR reason=mcp_not_up"; exit 7
fi

# --- Phase 5: open level, start PIE, capture initial state ------------------
mcp() { python3 "$MCP_DO" "$1" "$2" <(echo "$3") 2>&1; }

log "opening Lvl_TacticalDuel"
mcp "EditorToolset.EditorAppToolset" "OpenEditorForAsset" '{"assetPath":"/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel"}' > "$RUN_DIR/open_level.json" || true
sleep 3

log "starting PIE"
mcp "EditorToolset.EditorAppToolset" "StartPIE" '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":3}}' > "$RUN_DIR/start_pie.json" || true
log "waiting ${PIE_SETTLE}s for PIE to settle"
sleep "$PIE_SETTLE"

log "capturing initial state"
echo '{}' > "$RUN_DIR/args_empty.json"
timeout "$CAPTURE_TIMEOUT" python3 "$MCP_DO" "EditorToolset.EditorAppToolset" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture.json" 2>&1 || true
SHOT="$RUN_DIR/initial.png"
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

# --- Phase 6: KEEP THE EDITOR OPEN for the human click-through --------------
log "PIE is running. Click through the sequence, then CLOSE the editor window."
log "Sequence: Move -> Walk (set destination) -> Execute; Attack -> Quick -> Execute; Reset."
echo "CLICKTHROUGH_READY run_dir=$RUN_DIR shot=$SHOT"

# Block until the human closes the graphical editor.
wait "$UE_PID" 2>/dev/null || true
log "graphical editor closed by user — click-through window ended"

# --- Phase 7: write result, exit (cleanup trap frees the GPU) ---------------
RESULT_FILE="$RUN_DIR/result.json"
python3 - "$RESULT_FILE" "$RUN_ID" "CLICKTHROUGH_DONE" "$SHOT" "$(stat -c%s "$SHOT" 2>/dev/null || echo 0)" "$UE_LOG" "$RUNNER_LOG" <<'PY'
import json,sys
out,run_id,outcome,shot,shot_bytes,ue_log,runner_log=sys.argv[1:8]
json.dump({"run_id":run_id,"outcome":outcome,"screenshot":shot,
           "screenshot_bytes":int(shot_bytes or 0),"unreal_log":ue_log,
           "runner_log":runner_log},open(out,"w"),indent=2)
print(json.dumps({"run_id":run_id,"outcome":outcome}))
PY
echo "RESULT=CLICKTHROUGH_DONE run_dir=$RUN_DIR"
exit 0
