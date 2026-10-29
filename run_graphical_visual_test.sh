#!/usr/bin/env bash
# Beneath Uncounted Heavens — Graphical Visual-Test Runner (Phase 4)
#
# The headless (-nullrhi) editor renders nothing, so it cannot prove the two
# fighters actually draw on screen. This script launches the FULL graphical
# editor (real RHI), opens the tactical-duel level, starts PIE, and captures a
# screenshot showing both fighters — but only after the local LLM model has
# freed its VRAM, so the two never contend for the GPU.
#
# Sequence:
#   1. Wait for the model's idle unload (VRAM drops below threshold).
#   2. Close the headless MCP editor owned by this workflow (mcp_author.pid).
#   3. Launch graphical UnrealEditor with -ModelContextProtocolStartServer.
#   4. Wait for the MCP server on port 8000.
#   5. Drive via mcp_do.py: open Lvl_TacticalDuel, StartPIE, CaptureEditorImage.
#   6. Save the screenshot + logs + a summary under tmp_opencode/visual_test/<run_id>/.
#   7. Close ONLY the graphical editor this script launched and verify it exited.
#   8. Write a structured result.json and exit with an outcome code. The OpenCode
#      tool call that launched this script receives the result; the session's next
#      inference (an HTTP request to the llama.cpp server) wakes the model. No nested
#      `opencode run` — that is what broke the self-wake before.
#
# Usage:  run_graphical_visual_test.sh
#         run_graphical_visual_test.sh --help

set -uo pipefail

# --- Configuration ---------------------------------------------------------
UE_ROOT="/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4"
UE_GRAPHICAL="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-xwayland"
PROJECT="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject"
PROJ_DIR="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav"
MCP_DO="$PROJ_DIR/tmp_opencode/mcp_do.py"
HEADLESS_PID_FILE="$PROJ_DIR/tmp_opencode/mcp_author.pid"
OUT_ROOT="$PROJ_DIR/tmp_opencode/visual_test"
VRAM_USED="/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL="/sys/class/drm/card1/device/mem_info_vram_total"
VRAM_FREE_THRESHOLD=4294967296   # 4 GiB — below this the model is off the GPU
INITIAL_WAIT=70                  # seconds to let the model hit its 60s idle sleep
MAX_WAIT=240                     # hard cap on the VRAM-wait phase
MCP_WAIT=120                     # max seconds to wait for the MCP server
PIE_SETTLE=6                     # seconds for PIE to settle before the capture
CAPTURE_TIMEOUT=60               # max seconds for the capture MCP call

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  sed -n '2,24p' "$0"; exit 0
fi

# --- Output layout ---------------------------------------------------------
RUN_ID="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="$OUT_ROOT/$RUN_ID"
mkdir -p "$RUN_DIR"
RUNNER_LOG="$RUN_DIR/runner.log"
: > "$RUNNER_LOG"
log() { printf '[%s] %s\n' "$(date +%H:%M:%S)" "$*" | tee -a "$RUNNER_LOG"; }
gib() { awk "BEGIN{printf \"%.2f\", ($1)/1073741824}"; }
vram() { cat "$VRAM_USED" 2>/dev/null || echo 0; }

log "run_id=$RUN_ID"
log "UE_GRAPHICAL=$UE_GRAPHICAL"
log "PROJECT=$PROJECT"

# --- Preflight -------------------------------------------------------------
[[ -x "$UE_GRAPHICAL" ]] || { log "FATAL: UnrealEditor (graphical) not found/executable"; echo "RESULT=ERROR reason=no_graphical_binary"; exit 5; }
[[ -f "$PROJECT" ]]     || { log "FATAL: .uproject not found"; echo "RESULT=ERROR reason=no_project"; exit 5; }
[[ -f "$MCP_DO" ]]      || { log "FATAL: mcp_do.py not found"; echo "RESULT=ERROR reason=no_mcp_helper"; exit 5; }

# --- Phase 1: wait for the model to free VRAM ------------------------------
vram_total=$(cat "$VRAM_TOTAL" 2>/dev/null || echo 0)
used=$(vram); used_before=$used
log "initial VRAM in use = $(gib "$used") GiB of $(gib "$vram_total") GiB total (threshold $(gib "$VRAM_FREE_THRESHOLD") GiB)"
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
    log "ERROR: VRAM still $(gib "$used") GiB after ${MAX_WAIT}s. Model did not free the GPU. Aborting (not launching Unreal)."
    echo "RESULT=ABORTED reason=vram_not_freed used_gib=$(gib "$used")"; exit 3
  fi
  log "still waiting... $(gib "$used") GiB in use (${elapsed}s elapsed)"
  sleep 10; elapsed=$((elapsed + 10))
done

# --- Phase 2: close the headless MCP editor owned by this workflow ---------
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
# Free port 8000 in case a stale listener remains (pkill is unreliable here).
if command -v lsof >/dev/null 2>&1; then
  lsof -ti:8000 2>/dev/null | xargs -r kill -9 2>/dev/null || true
fi
sleep 2

# --- Phase 3: launch graphical Unreal in its own process group -------------
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

# --- Phase 4: wait for the MCP server on port 8000 -------------------------
mcp_up=0
for ((i=0; i<MCP_WAIT; i+=5)); do
  if ! kill -0 "$UE_PID" 2>/dev/null; then
    log "ERROR: graphical editor exited during startup. See unreal.log."
    echo "RESULT=CRASH reason=early_exit"; exit 6
  fi
  if curl -s -o /dev/null -X POST http://127.0.0.1:8000/mcp \
      -H 'Content-Type: application/json' \
      -d '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"visualtest","version":"1.0"}}}' 2>/dev/null; then
    mcp_up=1; log "MCP server is up after ~${i}s"; break
  fi
  sleep 5
done
if (( ! mcp_up )); then
  log "ERROR: MCP server did not come up within ${MCP_WAIT}s."
  echo "RESULT=ERROR reason=mcp_not_up"; exit 7
fi

# --- Phase 5: open the level, start PIE, capture the game view -------------
mcp() { python3 "$MCP_DO" "$1" "$2" <(echo "$3") 2>&1; }

log "opening Lvl_TacticalDuel"
mcp "EditorToolset.EditorAppToolset" "OpenEditorForAsset" '{"assetPath":"/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel"}' > "$RUN_DIR/open_level.json" || true
sleep 3

log "starting PIE"
mcp "EditorToolset.EditorAppToolset" "StartPIE" '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":3}}' > "$RUN_DIR/start_pie.json" || true
log "waiting ${PIE_SETTLE}s for PIE to settle"
sleep "$PIE_SETTLE"

log "capturing the editor (PIE game view)"
echo '{}' > "$RUN_DIR/args_empty.json"
timeout "$CAPTURE_TIMEOUT" python3 "$MCP_DO" "EditorToolset.EditorAppToolset" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture.json" 2>&1 || true

# Decode the base64 PNG out of the MCP response.
SHOT="$RUN_DIR/fighters.png"
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

# --- Phase 6: save logs + summary ------------------------------------------
SUMMARY="$RUN_DIR/summary.txt"
{
  echo "run_id: $RUN_ID"
  echo "mode: graphical visual test"
  echo "screenshot: $SHOT"
  echo "screenshot_bytes: $(stat -c%s "$SHOT" 2>/dev/null || echo 0)"
  echo "unreal_pid: $UE_PID"
  echo "unreal_log: $UE_LOG"
} > "$SUMMARY"
grep -aE "LogError|LogWarning.*Fighter|BeginPlay|SetViewTarget" "$UE_LOG" 2>/dev/null | head -40 >> "$SUMMARY" || true

if [[ -s "$SHOT" ]] && (( $(stat -c%s "$SHOT") > 10000 )); then
  log "captured a real screenshot ($(stat -c%s "$SHOT") bytes) — both fighters should be visible"
  echo "RESULT=PASS run_dir=$RUN_DIR shot=$SHOT"
else
  log "screenshot missing or too small — capture may have failed (headless-style 1x1 or no RHI)"
  echo "RESULT=FAIL reason=bad_screenshot run_dir=$RUN_DIR"
fi

# --- Phase 7: close the graphical editor, write the result, exit -----------
# Close the editor first so its VRAM is freed before the model reloads —
# otherwise the two contend for the GPU and OOM.
log "closing graphical editor"
kill -TERM -- "-$UE_PID" 2>/dev/null || true
sleep 5
kill -KILL -- "-$UE_PID" 2>/dev/null || true
kill -KILL "$UE_PID" 2>/dev/null || true
sleep 2

# Verify Unreal fully exited before we hand control back.
REASON=""
if kill -0 "$UE_PID" 2>/dev/null; then
  log "ERROR: graphical editor still alive after cleanup"
  REASON="unreal_not_exited"
fi

# Determine the outcome from the screenshot.
SHOT_BYTES=$(stat -c%s "$SHOT" 2>/dev/null || echo 0)
if [[ -s "$SHOT" ]] && (( SHOT_BYTES > 10000 )) && [[ -z "$REASON" ]]; then
  OUTCOME=PASS; EXIT_CODE=0
  log "captured a real screenshot ($SHOT_BYTES bytes) — both fighters should be visible"
else
  OUTCOME=FAIL; EXIT_CODE=1
  [[ -z "$REASON" ]] && REASON="bad_screenshot"
  log "screenshot missing/too small or unreal did not exit — outcome=FAIL"
fi

# Write the structured result file the OpenCode tool call will surface.
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
