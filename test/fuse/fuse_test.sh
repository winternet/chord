#!/bin/bash
#set -euo pipefail
set -eo pipefail

# this scripts directory
WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
CHORD_FUSE_WORKING_DIR=$WORKDIR

if [ -z ${CHORD_FUSE} ]; then
  CHORD_FUSE="./bin/chord_fuse"
fi

if [ -z ${CHORD_FUSE_ADAPTER+$CHORD_FUSE} ]; then
  CHORD_FUSE_ADAPTER="$CHORD_FUSE"
fi

#CHORD_FUSE_CONFIG=../config/fuse.yml
CHORD_FUSE_CONFIG=$WORKDIR/fuse.yml
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
  local _RET
  local _EXT
  echo "❱ $_COMMAND"
  _RET=$($_COMMAND)
  _EXT=$?

  if [ -n "$_RET" ]; then
    echo "$_RET"
  fi


  if [ ! -z $2 ]; then
    eval "$2='$_RET'"
  fi

  if [ ! -z $3 ]; then
    eval "$3='$_EXT'"
  else
    assertSame 'expected command to succeed' 0 $_EXT
  fi
}

test_empty() {
  local RET
  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains 'must contain current folder .' "$RET" "."
  assertContains 'must contain current folder ..' "$RET" ".."
  assertSame 3 $(echo "$RET" | wc -l)
}

test_touch_empty_file() {
  local RET
  local FILENAME="empty_file.md"
  local FILE_PATH="$CHORD_FUSE_MOUNTPOINT/$FILENAME"

  assertFalse 'file already exists' '[ -f $FILE_PATH ]'
  _ "touch $FILE_PATH" 
  assertTrue 'file does not exists' '[ -f $FILE_PATH ]'

  _ "ls $FILE_PATH" RET
  assertContains "$RET" "$FILE_PATH"

  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "$RET" "$FILENAME"

  _ "stat -c%s $FILE_PATH" RET
  assertSame "0" "$RET"
}

test_touch_file_with_contents() {
  local RET
  local FILENAME="root.md"
  local FILE_PATH="$CHORD_FUSE_MOUNTPOINT/$FILENAME"

  assertFalse 'file already exists' '[ -f $FILE_PATH ]'
  echo "file with contents" > $FILE_PATH
  assertTrue 'file does not exists' '[ -f $FILE_PATH ]'

  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "$RET" "$FILENAME"

  _ "ls $FILE_PATH" RET
  assertContains "$RET" "$FILENAME"

  _ "cat $FILE_PATH" RET
  assertSame "file with contents" "$RET" 

  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "$RET" "$FILENAME"

  _ "stat -c%s $FILE_PATH" RET
  assertSame "19" "$RET"
}

test_readdir__not_existant() {
  local RET
  local EXT
  local DIRNAME="does_not_exist"
  local DIR_PATH="${CHORD_FUSE_MOUNTPOINT}/$DIRNAME"

  _ "ls -al $DIR_PATH" RET EXT
  assertTrue 'expected to fail with exit code 2' '[ $EXT -eq 2 ]'
}

test_mkdir() {
  local RET
  local DIRNAME="folder"
  local DIR_PATH="$CHORD_FUSE_MOUNTPOINT/$DIRNAME"

  assertFalse 'directory already exists' '[ -d $DIR_PATH ]'
  _ "mkdir $DIR_PATH"
  assertTrue 'directory does not exist' '[ -d $DIR_PATH ]'

  _ "ls -al $CHORD_FUSE_MOUNTPOINT" RET
  assertContains "$RET" "$DIRNAME"

  _ "ls -al $DIR_PATH" RET
  assertContains 'must contain current folder .' "$RET" "."
  assertContains 'must contain current folder ..' "$RET" ".."
}

test_add_file_to_empty_dir() {
  local RET
  local FILENAME="subfile.md"
  local PARENT_PATH="$CHORD_FUSE_MOUNTPOINT/folder"
  local FILE_PATH="$PARENT_PATH/$FILENAME"

  assertFalse 'file already exists' '[ -f $FILE_PATH ]'
  _ "touch $FILE_PATH"
  assertTrue 'file does not exist' '[ -f $FILE_PATH ]'

  _ "ls -al $PARENT_PATH" RET
  assertContains "$RET" "$FILENAME"
}

test_append_content_to_subfile() {
  local RET
  local FILENAME="subfile.md"
  local PARENT_PATH="$CHORD_FUSE_MOUNTPOINT/folder"
  local FILE_PATH="$PARENT_PATH/$FILENAME"

  assertTrue 'file does not exist' '[ -f $FILE_PATH ]'
  echo "foobar" >> $FILE_PATH

  _ "stat -c%s $FILE_PATH" RET
  assertSame "7" "$RET"

  _ "cat $FILE_PATH" RET
  assertSame "foobar" "$RET" 

  echo "foobar" >> $FILE_PATH

  _ "stat -c%s $FILE_PATH" RET
  assertSame "14" "$RET"
}

test_truncate_file() {
  local RET
  local FILENAME="subfile.md"
  local PARENT_PATH="$CHORD_FUSE_MOUNTPOINT/folder"
  local FILE_PATH="$PARENT_PATH/$FILENAME"

  assertTrue "file does not exist" '[ -f $FILE_PATH ]'
  : > $FILE_PATH
  echo "❱ : > $FILE_PATH"
  _ "stat -c%s $FILE_PATH" RET

  assertTrue 'expected file to be truncated to 0' '[ $RET -eq 0 ]'
}

source $WORKDIR/shunit2

#oneTimeSetUp
#test_touch_file_with_contents
#oneTimeTearDown
