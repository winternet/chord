#!/bin/bash
PREFIX="/home/muffin/workspace/chord/"
EXECUTABLE="${PREFIX}/build/chord"
CONFIG="${PREFIX}/config/"
TMUX_SESSION_NAME="chord_cluster"

# cleanup pids
echo "stopping chord cluster"
CHORD_PROCESSES=($(ps -o pid -C chord --no-headers))
echo "found ${#CHORD_PROCESSES[@]} chord processes"
for pid in ${CHORD_PROCESSES[@]};
do
  echo "killing chord pid ${pid}"
  kill ${pid}
done

