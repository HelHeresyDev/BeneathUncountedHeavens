#!/usr/bin/env bash
# Beneath Uncounted Heavens — Graphical UI-Probe Self-Wake Runner
#
# Runs the C++ BUH.PIE.UIProbe automation test inside a full graphical editor
# (real RHI) so UMG widget access does not segfault the way it does under
# -nullrhi. PIE is started via MCP (the same path the proven visual-test script
# uses) instead of GEditor->PlayMap from inside the test, which has been observed
# to crash the Vulkan swapchain during a viewport resize.
#
# Usage:  run_graphical_ui_probe.sh
#         run_graphical_ui_probe.sh --help

set -uo pipefail

UE_ROOT="/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4"
UE_GRAPHICAL="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-xwayland"
PROJECT="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject"
PROJ_DIR="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav"
MCP_DO="$PROJ_DIR/tmp_opencode/mcp_do.py"
HEADLESS_PID_FILE="$PROJ_DIR/tmp_opencode/mcp_author.pid"
OUT_ROOT="$PROJ_DIR/tmp_opencode/ui_probe_graphical"
VRAM_USED="/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL="/sys/class/drm/card1/device/mem_info_vram_total"
VRAM_FREE_THRESHOLD=4294967296
INITIAL_WAIT=70
MAX_WAIT=240
MCP_WAIT=120
PIE_SETTLE=6
TEST_WAIT=180

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  sed -n '2,13p' "$0"; exit 0
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
log "UE_GRAPHICAL=$UE_GRAPHICAL"
log "PROJECT=$PROJECT"

[[ -x "$UE_GRAPHICAL" ]] || { log "FATAL: UnrealEditor (graphical) not found/executable"; echo "RESULT=ERROR reason=no_graphical_binary"; exit 5; }
[[ -f "$PROJECT" ]]     || { log "FATAL: .uproject not found"; echo "RESULT=ERROR reason=no_project"; exit 5; }
[[ -f "$MCP_DO" ]]      || { log "FATAL: mcp_do.py not found"; echo "RESULT=ERROR reason=no_mcp_helper"; exit 5; }

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
    log "ERROR: VRAM still $(gib "$used") GiB after ${MAX_WAIT}s. Model did not free the GPU. Aborting."
    echo "RESULT=ABORTED reason=vram_not_freed used_gib=$(gib "$used")"; exit 3
  fi
  log "still waiting... $(gib "$used") GiB in use (${elapsed}s elapsed)"
  sleep 10; elapsed=$((elapsed + 10))
done

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

UE_LOG="$RUN_DIR/unreal.log"
log "launching graphical editor with console-command UI probe"
setsid "$UE_GRAPHICAL" "$PROJECT" \
  -Map=/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel \
  -ExecCmds="Play, wait 3, BUH.RunUIProbe, wait 3, Quit" \
  -unattended -nosplash > "$UE_LOG" 2>&1 &
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

log "waiting for BUH.RunUIProbe to finish (max ${TEST_WAIT}s)"
for ((i=0; i<TEST_WAIT; i+=5)); do
  if grep -a "BUH.UIProbeConsole Done" "$UE_LOG" 2>/dev/null; then
    log "console probe completed after ~${i}s"
    break
  fi
  if grep -aE "Signal=11|CommonUnixCrashHandler" "$UE_LOG" 2>/dev/null; then
    log "editor crashed while waiting for the probe"
    break
  fi
  if ! kill -0 "$UE_PID" 2>/dev/null; then
    log "graphical editor exited while waiting for the probe"
    break
  fi
  sleep 5
done

REASON=""
if kill -0 "$UE_PID" 2>/dev/null; then
  REASON="unreal_not_exited"
fi
if grep -aE "Signal=11|CommonUnixCrashHandler" "$UE_LOG" 2>/dev/null; then
  log "ERROR: unexpected Unreal crash detected in unreal.log"
  REASON="unexpected_crash"
fi

TEST_OK=0
if grep -a "BUH.UIProbeConsole Done" "$UE_LOG" 2>/dev/null; then
  TEST_OK=1
fi

SUMMARY="$RUN_DIR/summary.txt"
{
  echo "run_id: $RUN_ID"
  echo "mode: graphical UI probe via MCP PIE"
  echo "unreal_pid: $UE_PID"
  echo "unreal_log: $UE_LOG"
  echo "test_ok: $TEST_OK"
  echo "reason: ${REASON:-none}"
} > "$SUMMARY"
grep -aE "BUH.UIProbeConsole|Signal=11|LogError" "$UE_LOG" 2>/dev/null | head -80 >> "$SUMMARY" || true

if (( TEST_OK )) && [[ -z "$REASON" ]]; then
  OUTCOME=PASS; EXIT_CODE=0
  log "BUH.PIE.UIProbe passed in the graphical editor"
else
  OUTCOME=FAIL; EXIT_CODE=1
  [[ -z "$REASON" ]] && REASON="test_failed"
  log "BUH.PIE.UIProbe failed or the editor crashed — outcome=FAIL"
fi

RESULT_FILE="$RUN_DIR/result.json"
python3 - "$RESULT_FILE" "$RUN_ID" "$OUTCOME" "$UE_LOG" "$RUNNER_LOG" "$REASON" <<'PY'
import json,sys
out,run_id,outcome,ue_log,runner_log,reason=sys.argv[1:7]
data={
  "run_id":run_id,
  "outcome":outcome,
  "unreal_log":ue_log,
  "runner_log":runner_log,
  "reason":reason or None,
}
json.dump(data,open(out,"w"),indent=2)
print(json.dumps(data))
PY
echo "RESULT=$OUTCOME run_dir=$RUN_DIR"
exit "$EXIT_CODE"
