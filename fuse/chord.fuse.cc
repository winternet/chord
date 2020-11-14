#include "chord.fuse.adapter.h"

int main(int argc, char *argv[])
{
  using namespace chord::fuse;

  Adapter adapter(argc, argv);

  int status = adapter.run(argc, argv);

  return status;
}
