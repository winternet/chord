#!/bin/bash
#set -euo pipefail
set -eo pipefail

CHORD_FUSE="./bin/chord_fuse"
CHORD_FUSE_WORKING_DIR=$(pwd)

if [ -z ${CHORD_FUSE_ADAPTER+$CHORD_FUSE} ]; then
  CHORD_FUSE_ADAPTER="$CHORD_FUSE"
fi

CHORD_FUSE_CONFIG=../config/fuse.yml
#CHORD_FUSE_CONFIG=fuse.yml
CHORD_FUSE_MOUNTPOINT="$CHORD_FUSE_WORKING_DIR/mountpoint" 

oneTimeSetUp() {
  echo "• launching chord_fuse [$CHORD_FUSE_ADAPTER] (debug, single_threaded)"
  mkdir -p $CHORD_FUSE_MOUNTPOINT
  $CHORD_FUSE_ADAPTER -d "$CHORD_FUSE_MOUNTPOINT" -- -c $CHORD_FUSE_CONFIG &>/dev/null &

  echo "waiting 2s..."
  sleep 2

  if ! pidof $CHORD_FUSE_ADAPTER; then
    echo "☠ failed to launch adapter - check configuration and output:"
    $CHORD_FUSE_ADAPTER -d "$CHORD_FUSE_MOUNTPOINT" -- -c $CHORD_FUSE_CONFIG 
    exit 1
  fi
}

oneTimeTearDown() {
  cd $CHORD_FUSE_WORKING_DIR
  echo "• unmounting"
  fusermount -u -z -q "$CHORD_FUSE_MOUNTPOINT"
  echo "• deleting mountpoint"
  rm -r "$CHORD_FUSE_MOUNTPOINT" &>/dev/null

  # TOOD make data and meta configurable via parameters so that
  # we dont have to rely on a matching/correct $CHORD_FUSE_CONFIG file
  echo "• deleting data0"
  rm -rf /tmp/data0
  echo "• deleting meta0"
  rm -rf /tmp/meta0
  echo "• deleting logs"
  rm -rf /tmp/logs
}

_() {
  local _COMMAND=$1
  echo "❱ $_COMMAND"
  local _RET=$($_COMMAND)
  local _EXT=$?

  if [ -n "$_RET" ]; then
    echo "$_RET"
  fi

  assertSame "expected command to succeed" $_EXT 0

  if [ ! -z $2 ]; then
    eval "$2='$_RET'"
  fi
}

test_empty() {
  local RET
  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "must contain current folder ." "$RET" "."
  assertContains "must contain current folder .." "$RET" ".."
  assertSame $(echo "$RET" | wc -l) 3
}

test_touch_empty_file() {
  local RET
  local FILENAME="empty_file.md"
  local FILE_PATH="$CHORD_FUSE_MOUNTPOINT/$FILENAME"

  assertFalse "file already exists" '[ -f $FILE_PATH ]'
  _ "touch $FILE_PATH" 
  assertTrue "file does not exists" '[ -f $FILE_PATH ]'

  _ "ls $FILE_PATH" RET
  assertContains "$RET" "$FILE_PATH"

  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "$RET" "$FILENAME"
}

test_touch_file_with_contents() {
  local RET
  local FILENAME="root.md"
  local FILE_PATH="$CHORD_FUSE_MOUNTPOINT/$FILENAME"

  assertFalse "file already exists" '[ -f $FILE_PATH ]'
  echo "file with contents" > $FILE_PATH
  assertTrue "file does not exists" '[ -f $FILE_PATH ]'

  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "$RET" "$FILENAME"

  _ "ls $FILE_PATH" RET
  assertContains "$RET" "$FILENAME"

  _ "cat $FILE_PATH" RET
  assertSame "$RET" "file with contents"

  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "$RET" "$FILENAME"
}

test_mkdir() {
  local RET
  local DIRNAME="folder"
  local DIR_PATH="$CHORD_FUSE_MOUNTPOINT/$DIRNAME"

  assertFalse "directory already exists" '[ -d $DIR_PATH ]'
  _ "mkdir $DIR_PATH"
  assertTrue "directory does not exist" '[ -d $DIR_PATH ]'

  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "$RET" "$DIRNAME"

  _ "ls -al $DIR_PATH" RET
  assertContains "must contain current folder ." "$RET" "."
  assertContains "must contain current folder .." "$RET" ".."
}

test_add_file_to_dir() {
  local RET
  local FILENAME="subfile.md"
  local PARENT_PATH="$CHORD_FUSE_MOUNTPOINT/folder"
  local FILE_PATH="$PARENT_PATH/$FILENAME"

  assertFalse "file already exists" '[ -f $FILE_PATH ]'
  _ "touch $FILE_PATH"
  assertTrue "file does not exist" '[ -f $FILE_PATH ]'

  _ "ls -al $PARENT_PATH" RET
  assertContains "$RET" "$FILENAME"
}

test_append_content_to_subfile() {
  local RET
  local FILENAME="subfile.md"
  local PARENT_PATH="$CHORD_FUSE_MOUNTPOINT/folder"
  local FILE_PATH="$PARENT_PATH/$FILENAME"

  assertTrue "file does not exist" '[ -f $FILE_PATH ]'
  echo "foobar" > $FILE_PATH
  _ "cat $FILE_PATH" RET
  assertSame "$RET" "foobar"
}

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source $DIR/shunit2

#oneTimeSetUp
#test_touch_file_with_contents
#oneTimeTearDown
