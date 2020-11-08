#include "chord.fuse.adapter.h"

int main(int argc, char *argv[])
{
  using namespace chord::fuse;

  Adapter adapter;

  int status = adapter.run(argc, argv);

  return status;
}
