#!/bin/bash
#echo "• killing chord_fuse"
#killall -9 chord_fuse
echo "• unmounting"
fusermount -u -z -q ~/Chord

echo "• deleting data0"
rm -rf ~/workspace/chord/data0
echo "• deleting meta0"
rm -rf ~/workspace/chord/meta0
echo "• deleting logs"
rm -rf ~/workspace/chord/logs
