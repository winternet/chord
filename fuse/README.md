FUSE - Filesystem Adapter
=========================

Usage
-----

```sh
CHORD_FUSE=../build/bin/chord_fuse bash ../fuse/sh/fuse_up.sh
CHORD_FUSE=../build/bin/chord_fuse bash ../fuse/sh/fuse_down.sh
```

Execute Tests
-------------

ctest -V -R chord_test_fuse
