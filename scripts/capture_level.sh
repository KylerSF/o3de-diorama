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
#   - The level MUST contain a camera entity (an Atom Camera component); levels built
#     only for the editor viewport have none and render black in the standalone
#     launcher. The twin-stick DefaultLevel has one and captures correctly.
#   - This sets /O3DE/Autoexec/ConsoleCommands/LoadLevel in Registry/load_level.setreg
#     to the target for one run, then restores it to "defaultlevel".
#   - It hard-kills any prior GameLauncher/Xvfb first (overlapping launchers writing
#     the shared Game.log was a real source of confusion).
set -u
LEVEL="${1:?usage: capture_level.sh <LevelName> <out.mp4> [seconds]}"
OUT="${2:?usage: capture_level.sh <LevelName> <out.mp4> [seconds]}"
SECS="${3:-8}"
PROJ="${DIORAMA_PROJECT:-$HOME/PROJECTS/DioramaSandbox}"
DISP="${CAP_DISPLAY:-:210}"
W="${CAP_W:-1280}"; H="${CAP_H:-720}"
LAUNCHER="$PROJ/build/linux/bin/profile/DioramaSandbox.GameLauncher"
GAMELOG="$PROJ/user/log/Game.log"
LL="$PROJ/Registry/load_level.setreg"

for p in $(pgrep -f DioramaSandbox.GameLauncher); do kill -9 "$p" 2>/dev/null; done
for p in $(pgrep -f "Xvfb ${DISP}"); do kill -9 "$p" 2>/dev/null; done
sleep 5
rm -f "/tmp/.X${DISP#:}-lock"

python3 -c "import json;p='$LL';d=json.load(open(p));d.setdefault('O3DE',{}).setdefault('Autoexec',{}).setdefault('ConsoleCommands',{})['LoadLevel']='$LEVEL';json.dump(d,open(p,'w'),indent=4)"
rm -rf "$PROJ/Cache/linux/levels/$(echo "$LEVEL" | tr '[:upper:]' '[:lower:]')" 2>/dev/null

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
  DISPLAY="$DISP" ffmpeg -y -f x11grab -video_size "${W}x${H}" -framerate 30 -i "$DISP" -t "$SECS" -pix_fmt yuv420p "$OUT" >/tmp/diorama_ffmpeg.log 2>&1
  echo "ffmpeg rc=$?"
else
  echo "launcher not running at capture time (see /tmp/diorama_launcher.log)"
fi

kill -9 $GPID 2>/dev/null; kill -9 $XPID 2>/dev/null
python3 -c "import json;p='$LL';d=json.load(open(p));d['O3DE']['Autoexec']['ConsoleCommands']['LoadLevel']='defaultlevel';json.dump(d,open(p,'w'),indent=4)"
echo "restored load_level -> defaultlevel"
ls -la "$OUT" 2>/dev/null
