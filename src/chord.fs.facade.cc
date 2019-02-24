#include <fstream>

#include "chord.log.h"
#include "chord.exception.h"
#include "chord.fs.facade.h"

using namespace std;

namespace chord {
namespace fs {

Facade::Facade(Context& context, ChordFacade* chord)
    : context{context},
      fs_client{make_unique<fs::Client>(context, chord)},
      fs_service{make_unique<fs::Service>(context, chord)},
      logger{log::get_or_create(logger_name)}
{}

Facade::Facade(Context& context, fs::Client* fs_client, fs::Service* fs_service)
  : context{context},
    fs_client{fs_client},
    fs_service{fs_service},
    logger{log::get_or_create(logger_name)}
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

void Facade::put(const chord::path &source, const chord::uri &target, Replication repl) {
  if (chord::file::is_directory(source)) {
    for(const auto &child : source.recursive_contents()) {
      // dont put empty folders for now
      if(file::is_directory(child)) continue;

      const auto relative_path = child - source.parent_path();
      const auto new_target = target.path().canonical() / relative_path;
      put_file(child, {target.scheme(), new_target}, repl);
    }
  } else {
    const auto new_target = is_directory(target) ? target.path().canonical() / source.filename() : target.path().canonical();
    put_file(source, {target.scheme(), new_target}, repl);
  }
}

void Facade::get_shallow_copies(const chord::node& leaving_node) {
  auto shallow_copies = fs_service->metadata_manager()->get(leaving_node);

  // integrate the metadata
  {
    for (const auto& [uri, meta_set] : shallow_copies) {
      std::set<Metadata> deep_copies;
      for_each(meta_set.begin(), meta_set.end(), [&](Metadata m) {
        if (m.file_type == type::regular && m.name == uri.path().filename()) {
          get_file(uri, context.data_directory / uri.path());
        }
        m.node_ref = {};
        deep_copies.insert(m);
      });
      fs_service->metadata_manager()->add(uri, deep_copies);
    }
  }
}

void Facade::get_and_integrate(const chord::fs::MetaResponse& meta_res) {
  if (meta_res.uri().empty()) {
    throw__exception(
        "failed to get and integrate metaresponse due to missing uri " +
        meta_res.uri());
  }

  const auto uri = chord::uri{meta_res.uri()};
  std::set<Metadata> data_set;

  // integrate the metadata
  {
    for (auto data : MetadataBuilder::from(meta_res)) {
      // unset reference id since the node leaves
      data.node_ref = {};
      data_set.insert(data);
    }
    fs_service->metadata_manager()->add(uri, data_set);
  }

  // get the files
  for (const auto& data : data_set) {
    // uri might be a directory containing data.name as child
    // or uri might point to a file with the metadata containing
    // the file's name, we consider only those leaves
    if (data.file_type == type::regular && data.name == uri.path().filename()) {
      get_file(uri, context.data_directory / uri.path());
    }
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
}


void Facade::put_file(const path& source, const chord::uri& target, Replication repl) {
  // validate input...
  if(repl.count > Replication::MAX_REPL_CNT) 
    throw__exception("replication count above "+to_string(Replication::MAX_REPL_CNT)+" is not allowed");

  try {
    std::ifstream file;
    file.exceptions(ifstream::failbit | ifstream::badbit);
    file.open(source, std::fstream::binary);

    const auto status = fs_client->put(target, file, repl);
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

    const auto status = fs_client->get(source, file);
    if(!status.ok()) throw__grpc_exception(std::string{"failed to get "} + to_string(source), status);
  } catch (const std::ios_base::failure& exception) {
    throw__exception("failed to issue get_file " + std::string{exception.what()});
  }
}

}  // namespace fs
}  // namespace chord
