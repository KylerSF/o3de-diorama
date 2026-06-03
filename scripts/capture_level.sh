#!/bin/bash
#
# Headless video capture of a Diorama level, via Xvfb + the project GameLauncher +
# ffmpeg. A debugging/recording aid: record what a level actually renders without a
# monitor (the editor hangs under Xvfb on this setup; the GameLauncher does not).
#
# Usage:
#   scripts/capture_level.sh <LevelName> <out.mp4> [seconds]
#
# Env (override as needed):
#   DIORAMA_PROJECT   default: $HOME/PROJECTS/DioramaSandbox
#   CAP_DISPLAY       default: :210
#   CAP_W, CAP_H      default: 1280 720
#
# Notes / caveats (see Docs and the memory file headless-gamelauncher-capture):
#   - The level MUST contain a camera entity (an Atom Camera component) that becomes
#     the active render view. A freshly authored/copied level whose camera is not the
#     project's startup camera will NOT auto-activate, so put the make_active_camera.lua
#     script (Assets/Diorama/Examples) on the camera; it calls CameraRequestBus
#     MakeActiveView on the first tick. Without an active view the launcher renders gray.
#   - The level (and any changed scripts) MUST be processed into the cache first. There
#     is no live AssetProcessor in this setup, so this script runs AssetProcessorBatch
#     before launching. It does NOT delete the cached spawnable (doing so with no live
#     AP left the level empty -> black, the original black-frame cause).
#   - This sets /O3DE/Autoexec/ConsoleCommands/LoadLevel in Registry/load_level.setreg
#     to the target for one run, then restores it to "defaultlevel".
#   - It hard-kills any prior GameLauncher/Xvfb first (overlapping launchers writing
#     the shared Game.log was a real source of confusion).
set -u
LEVEL="${1:?usage: capture_level.sh <LevelName> <out.mp4> [seconds]}"
OUT="${2:?usage: capture_level.sh <LevelName> <out.mp4> [seconds]}"
SECS="${3:-8}"
PROJ="${DIORAMA_PROJECT:-$HOME/PROJECTS/DioramaSandbox}"
ENGINE="${O3DE_ENGINE_PATH:-/opt/O3DE/26.05.0}"
DISP="${CAP_DISPLAY:-:210}"
W="${CAP_W:-1280}"; H="${CAP_H:-720}"
LAUNCHER="$PROJ/build/linux/bin/profile/DioramaSandbox.GameLauncher"
APBATCH="$ENGINE/bin/Linux/profile/Default/AssetProcessorBatch"
GAMELOG="$PROJ/user/log/Game.log"
LL="$PROJ/Registry/load_level.setreg"

for p in $(pgrep -f DioramaSandbox.GameLauncher); do kill -9 "$p" 2>/dev/null; done
for p in $(pgrep -f "Xvfb ${DISP}"); do kill -9 "$p" 2>/dev/null; done
sleep 5
rm -f "/tmp/.X${DISP#:}-lock"

# Point the autoexec LoadLevel at our target FIRST, then process. The launcher merges
# BOTH the live Registry/load_level.setreg AND a cached bootstrap.<launcher>.<config>.setreg
# that AssetProcessorBatch bakes from it. If these disagree the launcher fires two
# LoadLevel commands (the cached one second, so it wins) and the wrong level renders.
# Setting the value before AP makes both agree on $LEVEL.
python3 -c "import json;p='$LL';d=json.load(open(p));d.setdefault('O3DE',{}).setdefault('Autoexec',{}).setdefault('ConsoleCommands',{})['LoadLevel']='$LEVEL';json.dump(d,open(p,'w'),indent=4)"

# Build the level + any changed assets into the cache (no live AP in this setup);
# this also re-bakes the cached bootstrap autoexec with $LEVEL.
if [ -x "$APBATCH" ]; then
  echo "running AssetProcessorBatch (building $LEVEL + changed assets)..."
  "$APBATCH" --project-path="$PROJ" >/tmp/diorama_apbatch.log 2>&1
  grep -iE "Failed to Process|Successfully Processed" /tmp/diorama_apbatch.log | tail -2
fi

Xvfb "$DISP" -screen 0 "${W}x${H}x24" -nolisten tcp >/tmp/diorama_xvfb.log 2>&1 & XPID=$!
sleep 3
: > "$GAMELOG" 2>/dev/null || true
DISPLAY="$DISP" "$LAUNCHER" --project-path="$PROJ" --rhi=vulkan --r_fullscreen=0 --r_width="$W" --r_height="$H" >/tmp/diorama_launcher.log 2>&1 & GPID=$!

for i in $(seq 1 60); do
  grep -qiE "$LEVEL.*loaded" "$GAMELOG" 2>/dev/null && break
  kill -0 $GPID 2>/dev/null || { echo "launcher exited before level load"; break; }
  sleep 3
done
sleep 8   # settle: re-bake reload + animation populate

if kill -0 $GPID 2>/dev/null; then
  # -draw_mouse 0: do not record the pointer. Under Xvfb (no window manager) the
  # default X11 root cursor is a large "X" parked at screen center; the launcher
  # cannot hide it (logs "ShowCursor failed"), so suppress it at the grab instead.
  DISPLAY="$DISP" ffmpeg -y -f x11grab -draw_mouse 0 -video_size "${W}x${H}" -framerate 30 -i "$DISP" -t "$SECS" -pix_fmt yuv420p "$OUT" >/tmp/diorama_ffmpeg.log 2>&1
  echo "ffmpeg rc=$?"
else
  echo "launcher not running at capture time (see /tmp/diorama_launcher.log)"
fi

kill -9 $GPID 2>/dev/null; kill -9 $XPID 2>/dev/null
python3 -c "import json;p='$LL';d=json.load(open(p));d['O3DE']['Autoexec']['ConsoleCommands']['LoadLevel']='defaultlevel';json.dump(d,open(p,'w'),indent=4)"
# Re-bake the cached bootstrap back to defaultlevel so a plain launch isn't left
# pointing at the demo level.
[ -x "$APBATCH" ] && "$APBATCH" --project-path="$PROJ" >/tmp/diorama_apbatch_restore.log 2>&1
echo "restored load_level -> defaultlevel"
ls -la "$OUT" 2>/dev/null
