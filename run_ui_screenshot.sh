#!/usr/bin/env bash
# Capture a PIE screenshot at a specified viewport resolution.
# Usage: run_ui_screenshot.sh [RES_X] [RES_Y]
# Defaults to 1280x720 if no arguments provided.

set -uo pipefail

RES_X="${1:-1280}"
RES_Y="${2:-720}"

UE_ROOT="/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4"
UE_GRAPHICAL="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-xwayland"
PROJECT="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject"
PROJ_DIR="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav"
MCP_DO="$PROJ_DIR/tmp_opencode/mcp_do.py"
OUT_ROOT="$PROJ_DIR/tmp_opencode/ui_test"
VRAM_USED="/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL="/sys/class/drm/card1/device/mem_info_vram_total"
VRAM_FREE_THRESHOLD=4294967296
INITIAL_WAIT=70
MAX_WAIT=240
MCP_WAIT=120
PIE_SETTLE=6

RUN_ID="$(date +%Y%m%d_%H%M%S)_${RES_X}x${RES_Y}"
RUN_DIR="$OUT_ROOT/$RUN_ID"
mkdir -p "$RUN_DIR"
RUNNER_LOG="$RUN_DIR/runner.log"
: > "$RUNNER_LOG"
log() { printf '[%s] %s\n' "$(date +%H:%M:%S)" "$*" | tee -a "$RUNNER_LOG"; }
gib() { awk "BEGIN{printf \"%.2f\", ($1)/1073741824}"; }
vram() { cat "$VRAM_USED" 2>/dev/null || echo 0; }

log "run_id=$RUN_ID resolution=${RES_X}x${RES_Y}"

[[ -x "$UE_GRAPHICAL" ]] || { log "FATAL: no graphical binary"; echo "RESULT=ERROR reason=no_graphical_binary"; exit 5; }
[[ -f "$PROJECT" ]]     || { log "FATAL: no .uproject"; echo "RESULT=ERROR reason=no_project"; exit 5; }

# --- Phase 1: wait for VRAM to free ---
vram_total=$(cat "$VRAM_TOTAL" 2>/dev/null || echo 0)
used=$(vram)
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

# --- Phase 2: close any headless editor ---
lsof -ti:8000 2>/dev/null | xargs -r kill -9 2>/dev/null || true
sleep 3

# --- Phase 3: launch graphical editor at specified resolution ---
UE_LOG="$RUN_DIR/unreal.log"
log "launching graphical editor at ${RES_X}x${RES_Y}"
setsid "$UE_GRAPHICAL" "$PROJECT" -ModelContextProtocolStartServer -unattended -nosplash -ResX="$RES_X" -ResY="$RES_Y" > "$UE_LOG" 2>&1 &
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
      -d '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"ui_test","version":"1.0"}}}' 2>/dev/null; then
    mcp_up=1; log "MCP up after ~${i}s"; break
  fi
  sleep 5
done
if (( ! mcp_up )); then
  log "ERROR: MCP did not come up"
  echo "RESULT=ERROR reason=mcp_not_up"; exit 7
fi

mcp() { python3 "$MCP_DO" "$1" "$2" <(echo "$3") 2>&1; }
EDITOR="EditorToolset.EditorAppToolset"
LVL="/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel"

# --- Phase 5: open level, start PIE ---
log "opening Lvl_TacticalDuel"
mcp "$EDITOR" "OpenEditorForAsset" "{\"assetPath\":\"$LVL\"}" > "$RUN_DIR/open_level.json" || true
sleep 3

log "starting PIE"
mcp "$EDITOR" "StartPIE" '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":3}}' > "$RUN_DIR/start_pie.json" || true
log "waiting ${PIE_SETTLE}s for PIE to settle"
sleep "$PIE_SETTLE"

# --- Phase 6: capture screenshot ---
SHOT="$RUN_DIR/ui_screenshot.png"
log "capturing screenshot"
echo '{}' > "$RUN_DIR/args_empty.json"
timeout 60 python3 "$MCP_DO" "$EDITOR" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture.json" 2>&1 || true
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

BYTES=$(stat -c%s "$SHOT" 2>/dev/null || echo 0)
if (( BYTES >= 10000 )); then
  log "screenshot: ${BYTES} bytes"
  echo "RESULT=PASS run_dir=$RUN_DIR screenshot=$SHOT resolution=${RES_X}x${RES_Y}"
  exit 0
else
  log "WARNING: screenshot is only ${BYTES} bytes (may be a 1x1 placeholder)"
  echo "RESULT=PARTIAL run_dir=$RUN_DIR resolution=${RES_X}x${RES_Y}"
  exit 1
fi
