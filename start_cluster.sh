#!/bin/bash
PREFIX="/home/muffin/workspace/chord/"
EXECUTABLE="${PREFIX}/build/chord"
CONFIG="${PREFIX}/config/"
TMUX_SESSION_NAME="chord_cluster"

function cleanup {
  echo "stopping chord cluster"
  CHORD_PROCESSES=($(ps -o pid -C chord --no-headers))
  echo "found ${#CHORD_PROCESSES[@]} chord processes"
  for pid in ${CHORD_PROCESSES[@]};
  do
    echo "killing chord pid ${pid}"
    kill ${pid}
  done

  # cleanup meta
  rm -rf meta?
  rm -rf data?
}

cleanup

for i in `seq 0 2`;
do
  echo "starting node $i"
  nohup ${EXECUTABLE} -c ${CONFIG}/node${i}.yml > node${i}.log &
  sleep 2
done    

tail -f node*.log
