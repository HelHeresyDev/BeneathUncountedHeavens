#!/usr/bin/env bash
# LeftPanel/LeftPanelBG background rendering diagnostic.
# Follows feedback.md steps 1-4 and 7. Captures screenshots after each change.

set -uo pipefail

UE_ROOT="/home/depressedtsukasa/Apps/Linux_Unreal_Engine_5.8.4"
UE_GRAPHICAL="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-xwayland"
PROJECT="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav/BeneathUncountedHeav.uproject"
PROJ_DIR="/home/depressedtsukasa/Documents/Unreal Projects/BeneathUncountedHeav"
MCP_DO="$PROJ_DIR/tmp_opencode/mcp_do.py"
OUT_ROOT="$PROJ_DIR/tmp_opencode/leftpanel_diag"
VRAM_USED="/sys/class/drm/card1/device/mem_info_vram_used"
VRAM_TOTAL="/sys/class/drm/card1/device/mem_info_vram_total"
VRAM_FREE_THRESHOLD=4294967296
INITIAL_WAIT=70
MAX_WAIT=240
MCP_WAIT=120
PIE_SETTLE=6

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
      -d '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"leftpanel_diag","version":"1.0"}}}' 2>/dev/null; then
    mcp_up=1; log "MCP up after ~${i}s"; break
  fi
  sleep 5
done
if (( ! mcp_up )); then
  log "ERROR: MCP did not come up"
  echo "RESULT=ERROR reason=mcp_not_up"; exit 7
fi

mcp() { python3 "$MCP_DO" "$1" "$2" <(echo "$3") 2>&1; }

WBP="/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.WBP_CombatUI"
CDO="/Game/BUH/Prototypes/TacticalDuel/Blueprints/WBP_CombatUI.Default__WBP_CombatUI_C"
LVL="/Game/BUH/Prototypes/TacticalDuel/Lvl_TacticalDuel"
OBJ="editor_toolset.toolsets.object.ObjectTools"
UMG="UMGToolSet.UMGToolSet"
ASSET="editor_toolset.toolsets.asset.AssetTools"
EDITOR="EditorToolset.EditorAppToolset"

# --- Phase 5: open level, start PIE ---
log "opening Lvl_TacticalDuel"
mcp "$EDITOR" "OpenEditorForAsset" "{\"assetPath\":\"$LVL\"}" > "$RUN_DIR/open_level.json" || true
sleep 3

log "starting PIE"
mcp "$EDITOR" "StartPIE" '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":3}}' > "$RUN_DIR/start_pie.json" || true
log "waiting ${PIE_SETTLE}s for PIE to settle"
sleep "$PIE_SETTLE"

capture() {
  local label="$1"
  local shot="$RUN_DIR/${label}.png"
  log "capturing ${label}"
  echo '{}' > "$RUN_DIR/args_empty.json"
  timeout 60 python3 "$MCP_DO" "$EDITOR" "CaptureEditorImage" "$RUN_DIR/args_empty.json" > "$RUN_DIR/capture_${label}.json" 2>&1 || true
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

# --- Step 1: Compare brush properties of LeftPanel, LeftPanelBG, and TopBar ---
log "Step 1: comparing brush properties"

log "  inspecting LeftPanel Border"
mcp "$OBJ" "get_properties" "{\"instance\":{\"refPath\":\"$CDO\"},\"properties\":[\"background\",\"brushColor\",\"drawAs\",\"visibility\",\"renderOpacity\",\"padding\"]}" > "$RUN_DIR/leftpanel_props.json" 2>&1 || true

log "  inspecting LeftPanelBG Image"
mcp "$OBJ" "get_properties" "{\"instance\":{\"refPath\":\"$CDO\"},\"properties\":[\"brush\",\"drawAs\",\"visibility\",\"renderOpacity\"]}" > "$RUN_DIR/leftpanelbg_props.json" 2>&1 || true

log "  inspecting TopBar Border"
mcp "$OBJ" "get_properties" "{\"instance\":{\"refPath\":\"$CDO\"},\"properties\":[\"background\",\"brushColor\",\"drawAs\",\"visibility\",\"renderOpacity\",\"padding\"]}" > "$RUN_DIR/topbar_props.json" 2>&1 || true

log "  inspecting LeftPanelBG slot"
mcp "$OBJ" "get_properties" "{\"instance\":{\"refPath\":\"$CDO:WidgetTree.RootCanvas.CanvasPanelSlot_0\"},\"properties\":[\"layoutData\",\"bAutoSize\",\"zOrder\"]}" > "$RUN_DIR/leftpanelbg_slot.json" 2>&1 || true

log "  inspecting LeftPanel slot"
mcp "$OBJ" "get_properties" "{\"instance\":{\"refPath\":\"$CDO:WidgetTree.RootCanvas.CanvasPanelSlot_17\"},\"properties\":[\"layoutData\",\"bAutoSize\",\"zOrder\"]}" > "$RUN_DIR/leftpanel_slot.json" 2>&1 || true

log "  inspecting TopBar slot"
mcp "$OBJ" "get_properties" "{\"instance\":{\"refPath\":\"$CDO:WidgetTree.RootCanvas.CanvasPanelSlot_16\"},\"properties\":[\"layoutData\",\"bAutoSize\",\"zOrder\"]}" > "$RUN_DIR/topbar_slot.json" 2>&1 || true

capture "step1_baseline"

# --- Step 2: Assign WhiteSquareTexture to LeftPanelBG, DrawAs=Image, opaque red, 300x300 at (0,80) ---
log "Step 2: setting LeftPanelBG to WhiteSquareTexture, DrawAs=Image, opaque red, 300x300 at (0,80)"

log "  setting LeftPanelBG brush"
mcp "$OBJ" "set_properties" "{\"instance\":{\"refPath\":\"$CDO\"},\"values\":\"{\\\"brush\\\":{\\\"resourceObject\\\":\\\"/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture\\\",\\\"imageSize\\\":{\\\"x\\\":300,\\\"y\\\":300},\\\"tintColor\\\":{\\\"specifyColor\\\":true,\\\"color\\\":{\\\"r\\\":1,\\\"g\\\":0,\\\"b\\\":0,\\\"a\\\":1}},\\\"drawAs\\\":\\\"Image\\\"}}\"}" > "$RUN_DIR/set_leftpanelbg_brush.json" 2>&1 || true

log "  setting LeftPanelBG slot to 300x300 at (0,80) top-left anchors"
mcp "$OBJ" "set_properties" "{\"instance\":{\"refPath\":\"$CDO:WidgetTree.RootCanvas.CanvasPanelSlot_0\"},\"values\":\"{\\\"layoutData\\\":{\\\"position\\\":{\\\"x\\\":0,\\\"y\\\":80},\\\"size\\\":{\\\"x\\\":300,\\\"y\\\":300},\\\"stretch\\\":\\\"None\\\"},\\\"anchors\\\":{\\\"min\\\":{\\\"x\\\":0,\\\"y\\\":0},\\\"max\\\":{\\\"x\\\":0,\\\"y\\\":0}},\\\"alignment\\\":{\\\"x\\\":0,\\\"y\\\":0},\\\"bAutoSize\\\":false},\\\"zOrder\\\":0}\"}" > "$RUN_DIR/set_leftpanelbg_slot.json" 2>&1 || true

log "  compiling WBP"
mcp "$UMG" "CompileWidgetBlueprint" "{\"widgetBlueprint\":{\"refPath\":\"$WBP\"}}" > "$RUN_DIR/compile_step2.json" 2>&1 || true

log "  saving WBP"
mcp "$ASSET" "save_assets" "{\"asset_paths\":[\"$WBP\"]}" > "$RUN_DIR/save_step2.json" 2>&1 || true

capture "step2_whitesquare_red"

# --- Step 3: Test LeftPanel Border with same white texture, DrawAs=Image ---
log "Step 3: setting LeftPanel Border to WhiteSquareTexture, DrawAs=Image, opaque red"

log "  setting LeftPanel background"
mcp "$OBJ" "set_properties" "{\"instance\":{\"refPath\":\"$CDO\"},\"values\":\"{\\\"background\\\":{\\\"resourceObject\\\":\\\"/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture\\\",\\\"imageSize\\\":{\\\"x\\\":0,\\\"y\\\":0},\\\"tintColor\\\":{\\\"specifyColor\\\":true,\\\"color\\\":{\\\"r\\\":1,\\\"g\\\":0,\\\"b\\\":0,\\\"a\\\":1}},\\\"drawAs\\\":\\\"Image\\\"}}\"}" > "$RUN_DIR/set_leftpanel_bg.json" 2>&1 || true

log "  compiling WBP"
mcp "$UMG" "CompileWidgetBlueprint" "{\"widgetBlueprint\":{\"refPath\":\"$WBP\"}}" > "$RUN_DIR/compile_step3.json" 2>&1 || true

log "  saving WBP"
mcp "$ASSET" "save_assets" "{\"asset_paths\":[\"$WBP\"]}" > "$RUN_DIR/save_step3.json" 2>&1 || true

capture "step3_border_whitesquare"

# --- Step 4: Fix ZOrder and negative bottom offset ---
log "Step 4: fixing ZOrder and negative bottom offset"

log "  setting LeftPanelBG zOrder=0, stretched vertical slot"
mcp "$OBJ" "set_properties" "{\"instance\":{\"refPath\":\"$CDO:WidgetTree.RootCanvas.CanvasPanelSlot_0\"},\"values\":\"{\\\"layoutData\\\":{\\\"position\\\":{\\\"x\\\":0,\\\"y\\\":60},\\\"size\\\":{\\\"x\\\":300,\\\"y\\\":580},\\\"stretch\\\":\\\"Fill\\\"},\\\"anchors\\\":{\\\"min\\\":{\\\"x\\\":0,\\\"y\\\":0},\\\"max\\\":{\\\"x\\\":0,\\\"y\\\":1}},\\\"alignment\\\":{\\\"x\\\":0,\\\"y\\\":0},\\\"bAutoSize\\\":false},\\\"zOrder\\\":0}\"}" > "$RUN_DIR/set_leftpanelbg_zorder.json" 2>&1 || true

log "  setting LeftPanel zOrder=1"
mcp "$OBJ" "set_properties" "{\"instance\":{\"refPath\":\"$CDO:WidgetTree.RootCanvas.CanvasPanelSlot_17\"},\"values\":\"{\\\"layoutData\\\":{\\\"position\\\":{\\\"x\\\":0,\\\"y\\\":60},\\\"size\\\":{\\\"x\\\":300,\\\"y\\\":580},\\\"stretch\\\":\\\"Fill\\\"},\\\"anchors\\\":{\\\"min\\\":{\\\"x\\\":0,\\\"y\\\":0},\\\"max\\\":{\\\"x\\\":0,\\\"y\\\":1}},\\\"alignment\\\":{\\\"x\\\":0,\\\"y\\\":0},\\\"bAutoSize\\\":false},\\\"zOrder\\\":1}\"}" > "$RUN_DIR/set_leftpanel_zorder.json" 2>&1 || true

log "  compiling WBP"
mcp "$UMG" "CompileWidgetBlueprint" "{\"widgetBlueprint\":{\"refPath\":\"$WBP\"}}" > "$RUN_DIR/compile_step4.json" 2>&1 || true

log "  saving WBP"
mcp "$ASSET" "save_assets" "{\"asset_paths\":[\"$WBP\"]}" > "$RUN_DIR/save_step4.json" 2>&1 || true

capture "step4_zorder_fixed"

# --- Phase 6: summary ---
SUMMARY="$RUN_DIR/summary.txt"
{
  echo "run_id: $RUN_ID"
  echo "mode: leftpanel/LeftPanelBG background diagnostic"
  echo "screenshot_step1: $RUN_DIR/step1_baseline.png"
  echo "screenshot_step1_bytes: $(stat -c%s "$RUN_DIR/step1_baseline.png" 2>/dev/null || echo 0)"
  echo "screenshot_step2: $RUN_DIR/step2_whitesquare_red.png"
  echo "screenshot_step2_bytes: $(stat -c%s "$RUN_DIR/step2_whitesquare_red.png" 2>/dev/null || echo 0)"
  echo "screenshot_step3: $RUN_DIR/step3_border_whitesquare.png"
  echo "screenshot_step3_bytes: $(stat -c%s "$RUN_DIR/step3_border_whitesquare.png" 2>/dev/null || echo 0)"
  echo "screenshot_step4: $RUN_DIR/step4_zorder_fixed.png"
  echo "screenshot_step4_bytes: $(stat -c%s "$RUN_DIR/step4_zorder_fixed.png" 2>/dev/null || echo 0)"
  echo "unreal_pid: $UE_PID"
  echo "unreal_log: $UE_LOG"
} > "$SUMMARY"
grep -aE "LogError|LogTemp|BUH" "$UE_LOG" 2>/dev/null | head -40 >> "$SUMMARY" || true

ALL_PASS=1
for f in step1_baseline step2_whitesquare_red step3_border_whitesquare step4_zorder_fixed; do
  bytes=$(stat -c%s "$RUN_DIR/${f}.png" 2>/dev/null || echo 0)
  if (( bytes < 10000 )); then
    log "WARNING: ${f}.png is only ${bytes} bytes (may be a 1x1 placeholder)"
    ALL_PASS=0
  else
    log "${f}.png: ${bytes} bytes"
  fi
done

if (( ALL_PASS )); then
  log "all four screenshots captured"
  echo "RESULT=PASS run_dir=$RUN_DIR"
  exit 0
else
  log "some screenshots may be placeholders"
  echo "RESULT=PARTIAL run_dir=$RUN_DIR"
  exit 1
fi
