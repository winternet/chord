#include <fstream>

#include "chord.exception.h"
#include "chord.fs.facade.h"

using namespace std;

namespace chord {
namespace fs {

Facade::Facade(Context& context, ChordFacade* chord)
    : context{context},
      fs_client{make_unique<fs::Client>(context, chord)},
      fs_service{make_unique<fs::Service>(context, chord)}
{}

::grpc::Service* Facade::grpc_service() {
  return fs_service.get();
}

bool Facade::is_directory(const chord::uri& target) {
  set<Metadata> metadata;
  const auto status = fs_client->dir(target, metadata);
  if (!status.ok()) throw__grpc_exception("failed to dir " + to_string(target), status);
  // check if exists?
  return !metadata.empty();
}

void Facade::put(const chord::path &source, const chord::uri &target) {
  if (chord::file::is_directory(source)) {
    for(const auto &child : source.recursive_contents()) {
      // dont put empty folders for now
      if(file::is_directory(child)) continue;

      const auto relative_path = child - source.parent_path();
      const auto new_target = target.path().canonical() / relative_path;
      put_file(child, {target.scheme(), new_target});
    }
  } else {
    const auto new_target = is_directory(target) ? target.path().canonical() / source.filename() : target.path().canonical();
    put_file(source, {target.scheme(), new_target});
  }
}

void Facade::get(const chord::uri &source, const chord::path& target) {

  std::set<Metadata> metadata;
  fs_client->meta(source, Client::Action::DIR, metadata);
  for(const auto& meta:metadata) {
    // subtract and append, e.g.
    // 1.1) source.path() == /file.txt
    // 2.1) meta.name == /file.txt
    // => /file.txt
    // 1.2) source.path() == /folder/
    // 1.2) meta.name == file.txt
    // => /folder/file.txt
    const auto new_source = (source.path().canonical() - path{meta.name}) / path{meta.name};

    if(meta.file_type == type::regular) {
      // issue get_file
      const auto new_target = file::exists(target) ? target / path{meta.name} : target;
      get_file({source.scheme(), new_source}, new_target);
    } else if(meta.file_type == type::directory && meta.name != ".") {
      // issue recursive metadata call
      get({source.scheme(), new_source}, target / path{meta.name});
    }
  }
}

void Facade::dir(const chord::uri &uri, iostream &iostream) {
  std::set<Metadata> metadata;
  const auto status = fs_client->dir(uri, metadata);
  if(!status.ok()) throw__grpc_exception("failed to dir " + to_string(uri), status);
  iostream << metadata;
}

void Facade::del(const chord::uri &uri) {
  const auto status = fs_client->del(uri);
  if(!status.ok()) throw__grpc_exception("failed to del file " + to_string(uri), status);

  //status = fs_client->meta(uri, Client::Action::DEL);
  //if(!status.ok()) throw__grpc_exception("failed to del metadata " + to_string(uri), status);
}


void Facade::put_file(const path& source, const chord::uri& target) {
  try {
    std::ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(source, std::fstream::binary);

    const auto status = fs_client->put(target, file);
    if (!status.ok()) throw__grpc_exception("failed to put " + to_string(target), status);
  } catch (const std::ios_base::failure& exception) {
    throw__exception("failed to issue put_file " + std::string{exception.what()});
  }
}

void Facade::get_file(const chord::uri& source, const chord::path& target) {
  try {
    // assert target path exists
    std::cerr << "\n\n*** get_file: source: " << to_string(source) << " target: " << target;
    const auto parent = target.parent_path();
    if(!file::exists(parent)) file::create_directories(parent);

    std::ofstream file;
    file.exceptions(ofstream::failbit | ofstream::badbit);
    file.open(target, std::fstream::binary);

    const auto status = fs_client->get(source, file);
    if(!status.ok()) throw__grpc_exception(std::string{"failed to get "} + to_string(source), status);
  } catch (const std::ios_base::failure& exception) {
    throw__exception("failed to issue get_file " + std::string{exception.what()});
  }
}

}  // namespace fs
}  // namespace chord
