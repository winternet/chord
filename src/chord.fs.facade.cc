#include <fstream>

#include "chord.exception.h"
#include "chord.fs.facade.h"

using namespace std;

namespace chord {
namespace fs {

Facade::Facade(Context& context, ChordFacade* chord)
    : fs_client{make_unique<fs::Client>(context, chord)},
      fs_service{make_unique<fs::Service>(context, chord)}
{}

::grpc::Service* Facade::grpc_service() {
  return fs_service.get();
}

void Facade::put(const chord::path &source, const chord::uri &target) {
  if (chord::file::is_directory(source)) {
    for(const auto &child : source.recursive_contents()) {
      // dont put empty folders for now
      if(file::is_directory(child)) continue;

      auto relative_path = child - source;
      auto exact_target_path = target.path() / relative_path;
      put_file(child, {target.scheme(), exact_target_path.canonical()});
    }
  } else {
    put_file(source, target);
  }
}

void Facade::get(const chord::uri &source, const chord::path& target) {

  std::set<Metadata> metadata;
  fs_client->meta(source, Client::Action::DIR, metadata);
  for(const auto& meta:metadata) {
    auto new_source = source.path().canonical() / path{meta.name};
    if(meta.file_type == type::regular) {
      // issue get_file
      get_file({source.scheme(), new_source}, target / path{meta.name});
    } else if(meta.file_type == type::directory && meta.name != ".") {
      // issue recursive metadata call
      get({source.scheme(), new_source}, target / path{meta.name});
    } else {
    }
  }
}

void Facade::dir(const chord::uri &uri, iostream &iostream) {
  std::set<Metadata> metadata;
  auto status = fs_client->dir(uri, metadata);
  if(!status.ok()) throw__grpc_exception("failed to dir " + to_string(uri), status);
  iostream << metadata;
}

void Facade::del(const chord::uri &uri) {
  auto status = fs_client->del(uri);
  if(!status.ok()) throw__grpc_exception("failed to del file " + to_string(uri), status);

  //status = fs_client->meta(uri, Client::Action::DEL);
  //if(!status.ok()) throw__grpc_exception("failed to del metadata " + to_string(uri), status);
}


void Facade::put_file(const path& source, const chord::uri& target) {
  try {
    std::ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(source, std::fstream::binary);

    auto status = fs_client->put(target, file);
    if (!status.ok()) throw__grpc_exception("failed to put " + to_string(target), status);
  } catch (const std::ios_base::failure& exception) {
    throw__exception("failed to issue put_file " + std::string{exception.what()});
  }
}

void Facade::get_file(const chord::uri& source, const chord::path& target) {
  try {
    // assert target path exists
    const auto parent = target.parent_path();
    if(!file::exists(parent)) file::create_directories(parent);

    std::ofstream file;
    file.exceptions(ofstream::failbit | ofstream::badbit);
    file.open(target, std::fstream::binary);

    auto status = fs_client->get(source, file);
    if(!status.ok()) throw__grpc_exception(std::string{"failed to get "} + to_string(source), status);
  } catch (const std::ios_base::failure& exception) {
    throw__exception("failed to issue get_file " + std::string{exception.what()});
  }
}

}  // namespace fs
}  // namespace chord
