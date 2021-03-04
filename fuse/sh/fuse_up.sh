#!/bin/bash
echo "• launching chord_fuse (debug)"
#
# note that the fuse.yml MUST contain absolute paths otherwise permission issues arise - for now
#
./$CHORD_FUSE -d  ~/Chord -- -c ../config/fuse.yml 
