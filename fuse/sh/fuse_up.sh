#!/bin/bash
echo "• launching chord_fuse (debug)"
#
# not that the fuse.yml MUST contain absolute paths otherwise permission issues arise - for now
#
./chord_fuse -d  ~/Chord -- -c ../config/fuse.yml 
