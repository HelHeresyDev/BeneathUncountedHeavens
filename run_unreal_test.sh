#!/usr/bin/env bash
# Beneath Uncounted Heavens — Unreal VRAM-Swapping Test Runner
#
# Frees GPU VRAM by waiting for the active LLM model to sleep, then runs an
# Unreal Engine automation test headless, captures logs, and tears down only
# the Unreal process it launched. When this script returns, the next OpenCode
# inference wakes the model again.
#
# Usage:
#   run_unreal_test.sh <TestName>      run one named automation test
#   run_unreal_test.sh --list          list available automation tests
#   run_unreal_test.sh --smoke         run a trivial built-in smoke test
#   run_unreal_test.sh --help          show usage
#
# All artifacts land under <project>/tmp_opencode/unreal_test/<run_id>/.

set -uo pipefail

# --- Configuration ---------------------------------------------------------
UE_ROOT="/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4"
UE_CMD="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd"
PROJECT="/home/depressedtsukasa/Documents/Beneath Uncounted Heavens/BeneathUncountedHeav/BeneathUncountedHeav.uproject"
OUT_ROOT="/home/depressedtsukasa/Documents/Beneath Uncounted Heavens/tmp_opencode/unreal_test"
VRAM_USED="/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL="/sys/class/drm/card1/device/mem_info_vram_total"
VRAM_FREE_THRESHOLD=4294967296   # 4 GiB — below this the model is off the GPU
INITIAL_WAIT=70                  # seconds to let the model hit its 60s idle sleep
MAX_WAIT=240                     # hard cap on the VRAM-wait phase
UNREAL_TIMEOUT=120               # max seconds for the Unreal run itself
POLL=10                          # VRAM poll interval (seconds)

# --- Argument parsing ------------------------------------------------------
MODE="test"
TEST_NAME=""
usage() { sed -n '2,16p' "$0"; }
if [[ $# -ge 1 ]]; then
  case "$1" in
    --list)  MODE="list" ;;
    --smoke) MODE="smoke"; TEST_NAME="Smoke" ;;
    -h|--help) usage; exit 0 ;;
    *)       TEST_NAME="$1" ;;
  esac
fi
if [[ "$MODE" == "test" && -z "$TEST_NAME" ]]; then
  echo "ERROR: no test name given. Usage: $0 <TestName>|--list|--smoke|--help" >&2
  exit 2
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

log "run_id=$RUN_ID mode=$MODE test=${TEST_NAME:-<none>}"
log "UE_CMD=$UE_CMD"
log "PROJECT=$PROJECT"

# --- List mode: offline test discovery (no Unreal launch) ------------------
# 'Automation List' headless is unreliable (the ListAllTests callback needs a live
# worker session and does not fire), so we extract test names straight from the
# engine + project source instead. This is fast and needs no GPU.
if [[ "$MODE" == "list" ]]; then
  PROJECT_DIR="$(dirname "$PROJECT")"
  SUMMARY="$RUN_DIR/summary.txt"
  {
    echo "run_id: $RUN_ID"
    echo "mode: list (offline source scan)"
    echo "source: IMPLEMENT_SIMPLE_AUTOMATION_TEST in engine + project"
  } > "$SUMMARY"
  grep -rhoE "IMPLEMENT_SIMPLE_AUTOMATION_TEST\([^,]+,[[:space:]]*\"[^\"]+\"" \
    "$UE_ROOT/Engine/Source/" "$PROJECT_DIR/Source/" 2>/dev/null \
    | sed -E 's/.*"([^"]+)".*/\1/' | sort -u >> "$SUMMARY" || true
  count=$(tail -n +4 "$SUMMARY" | grep -c . || echo 0)
  log "list mode: extracted $count test names to $SUMMARY"
  echo "RESULT=OK mode=list tests=$count run_dir=$RUN_DIR"
  exit 0
fi

# --- Preflight -------------------------------------------------------------
[[ -x "$UE_CMD" ]] || { log "FATAL: UnrealEditor-Cmd not found/executable"; echo "RESULT=ERROR reason=no_ue_binary"; exit 5; }
[[ -f "$PROJECT" ]] || { log "FATAL: .uproject not found"; echo "RESULT=ERROR reason=no_project"; exit 5; }
if pgrep -f "UnrealEditor.*-ModelContextProtocolStartServer" >/dev/null 2>&1; then
  log "NOTE: a manually-opened Unreal Editor (MCP) is running. We will NOT touch it — we only kill our own process group."
fi

# --- Phase 1: wait for the model to free VRAM ------------------------------
vram_total=$(cat "$VRAM_TOTAL" 2>/dev/null || echo 0)
used=$(vram)
used_before=$used
log "initial VRAM in use = $(gib "$used") GiB of $(gib "$vram_total") GiB total (threshold $(gib "$VRAM_FREE_THRESHOLD") GiB)"
log "ollama ps (informational): $(ollama ps 2>/dev/null | tail -n +2 | tr '\n' ' ' || echo '<ollama unavailable>')"

if (( used >= VRAM_FREE_THRESHOLD )); then
  log "model still in GPU memory — waiting ${INITIAL_WAIT}s for its idle sleep..."
  sleep "$INITIAL_WAIT"
fi

elapsed=0
while :; do
  used=$(vram)
  if (( used < VRAM_FREE_THRESHOLD )); then
    log "VRAM in use: $(gib "$used") GiB (down from $(gib "$used_before") GiB) — model has slept, ~$(gib $((vram_total - used))) GiB now free. Proceeding."
    break
  fi
  if (( elapsed >= MAX_WAIT )); then
    log "ERROR: VRAM still $(gib "$used") GiB after ${MAX_WAIT}s. Model did not free the GPU. Aborting (not launching Unreal)."
    echo "RESULT=ABORTED reason=vram_not_freed used_gib=$(gib "$used")"
    exit 3
  fi
  log "still waiting... $(gib "$used") GiB in use (${elapsed}s elapsed)"
  sleep "$POLL"
  elapsed=$((elapsed + POLL))
done

# --- Phase 2: build the Unreal command -------------------------------------
UE_LOG="$RUN_DIR/unreal.log"
REPORT="$RUN_DIR/report.xml"
if [[ "$MODE" == "list" ]]; then
  CMD=("$UE_CMD" "$PROJECT" -nullrhi -unattended -nosplash "-ExecCmds=Automation List;Quit" "-TestExit=Automation Test Queue Empty")
else
  CMD=("$UE_CMD" "$PROJECT" -nullrhi -unattended -nosplash "-ExecCmds=Automation RunTests $TEST_NAME;Quit" "-TestExit=Automation Test Queue Empty" "-ReportOutputPath=$REPORT")
fi
log "launching: ${CMD[*]}"

# --- Phase 3: launch in its own process group (so we never kill a manual editor)
setsid "${CMD[@]}" > "$RUN_DIR/stdout.log" 2>&1 &
UE_PID=$!
log "Unreal launched: PID=$UE_PID (own session/process group)"

sleep 3
if ! kill -0 "$UE_PID" 2>/dev/null; then
  log "ERROR: Unreal exited immediately (bad args or crash). See stdout.log / unreal.log."
  echo "RESULT=CRASH reason=early_exit"
  exit 6
fi

# --- Phase 4: wait for exit with a timeout ---------------------------------
start=$(date +%s)
timed_out=0
while kill -0 "$UE_PID" 2>/dev/null; do
  now=$(date +%s)
  if (( now - start >= UNREAL_TIMEOUT )); then
    log "TIMEOUT: Unreal still running after ${UNREAL_TIMEOUT}s. Killing process group $UE_PID."
    kill -TERM -- "-$UE_PID" 2>/dev/null
    sleep 5
    kill -KILL -- "-$UE_PID" 2>/dev/null
    timed_out=1
    break
  fi
  sleep 2
done

wait "$UE_PID" 2>/dev/null
rc=$?
log "Unreal finished: exit_code=$rc timed_out=$timed_out"

# --- Phase 5: verify the process is gone -----------------------------------
if kill -0 "$UE_PID" 2>/dev/null; then
  log "WARN: PID $UE_PID still alive; force-killing process group."
  kill -KILL -- "-$UE_PID" 2>/dev/null
  sleep 2
fi
if kill -0 "$UE_PID" 2>/dev/null; then
  log "FATAL: could not kill Unreal (PID $UE_PID). Manual intervention needed."
  echo "RESULT=CRASH reason=unkillable pid=$UE_PID"
  exit 4
fi
log "verified: Unreal process group fully exited"

# --- Phase 6: summarize results --------------------------------------------
SUMMARY="$RUN_DIR/summary.txt"
{
  echo "run_id: $RUN_ID"
  echo "mode: $MODE"
  echo "test: ${TEST_NAME:-<none>}"
  echo "unreal_exit_code: $rc"
  echo "timed_out: $timed_out"
  echo "artifacts: $RUN_DIR"
} > "$SUMMARY"

LOG="$RUN_DIR/stdout.log"
if [[ "$MODE" == "list" ]]; then
  # 'Automation List' prints each test name on a 'LogAutomationController: Display:' line.
  grep -aE "LogAutomationController: Display: " "$LOG" \
    | sed -E 's/^.*LogAutomationController: Display: //' \
    | grep -avE "^Found [0-9]+ automation|^Loading results|^Successfully wrote|^Clearing|^Test (Started|Completed)" \
    | head -400 >> "$SUMMARY" || true
  log "list mode: captured test listing to $SUMMARY"
  echo "RESULT=OK mode=list run_dir=$RUN_DIR"
else
  # Parse the captured log for pass/fail (Result={Success} / Result={Failure}).
  passed=$(grep -acE "Result=\{Success\}" "$LOG" 2>/dev/null); passed=${passed:-0}
  failed=$(grep -acE "Result=\{Failure\}" "$LOG" 2>/dev/null); failed=${failed:-0}
  echo "tests_passed: $passed" >> "$SUMMARY"
  echo "tests_failed: $failed" >> "$SUMMARY"
  grep -aE "Test Completed|Result=|Automation Test Queue Empty|Automation Testing Complete|LogError" "$LOG" 2>/dev/null | head -40 >> "$SUMMARY" || true
  if (( timed_out )); then
    echo "RESULT=TIMEOUT test=$TEST_NAME run_dir=$RUN_DIR"
  elif (( rc == 0 && failed == 0 && passed > 0 )); then
    echo "RESULT=PASS test=$TEST_NAME passed=$passed run_dir=$RUN_DIR"
  elif (( rc == 0 )); then
    echo "RESULT=OK_NO_MATCH test=$TEST_NAME exit=0 passed=$passed failed=$failed run_dir=$RUN_DIR"
  else
    echo "RESULT=FAIL test=$TEST_NAME exit=$rc passed=$passed failed=$failed run_dir=$RUN_DIR"
  fi
fi

log "done. Next OpenCode inference will wake the model."
exit 0
