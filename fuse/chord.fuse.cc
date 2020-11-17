#include <algorithm>
#include "chord.fuse.adapter.h"

void print_help(const char* program) {
  spdlog::error("Failed to parse arguments: missing '--' separator between fuse and chord arguments");
  spdlog::error("{} <fuse_args> -- <chord_args>", program);
  spdlog::error("<fuse_args>: mountpoint");
  spdlog::error("<chord_args>: see ./chord --help");
}

char** find_delimiter(int argc, char** argv, char** args) {
  std::memcpy(args, argv, sizeof(char*)*argc);
  return std::find_if(args, args+argc, [](const char* str) {
      return std::strcmp(str, "--") == 0;
  });
}

int chord_arguments(int argc, char** argv, char** args) {
  auto** ptr = find_delimiter(argc, argv, args);

  if(ptr == args+argc) {
    print_help(args[0]);
    return -2;
  }
  // found
  for(auto i=1; ptr+i < args+argc; ++i) args[i] = *(ptr+i);
  argc = (args+argc)-(ptr);
  return argc;
}

int fuse_arguments(int argc, char** argv, char** args) {
  auto** ptr = find_delimiter(argc, argv, args);

  if(ptr == args+argc) {
    print_help(args[0]);
    return -2;
  }
  // found
  argc = (ptr)-(args);
  return argc;
}

int main(int argc, char *argv[])
{
  using namespace chord::fuse;

  auto** chord_args = new char*[argc];
  auto chord_argc = chord_arguments(argc, argv, chord_args);
  Adapter adapter(chord_argc, chord_args);
  delete[] chord_args;


  auto** fuse_args = new char*[argc];
  auto fuse_argc = fuse_arguments(argc, argv, fuse_args);
  int status = adapter.run(fuse_argc, fuse_args);
  delete[] fuse_args;

  return status;
}
