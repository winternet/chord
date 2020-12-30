#pragma once

#include <set>
#include <fstream>
#include "chord.exception.h"
#include "chord.file.h"
#include "chord.fs.metadata.h"
#include "chord.path.h"
#include "chord.uri.h"
#include "chord.crypto.h"
#include "chord_fs.pb.h"
#include "chord.context.h"
#include "chord.common.h"

namespace chord {
namespace fs {

struct MetadataBuilder {

  /**
   * generate metadata from chord uri path
   *
   * @param context to prepend chord's data directory
   * @param chord_path the path from chord:// e.g. /folder/file
   */
  static std::set<Metadata> for_path(const Context& context, const path& chord_path, const Replication repl = {}) {
    const auto local_path = context.data_directory / chord_path;
    if (!file::exists(local_path)) {
      throw__exception("not found: " + local_path.string());
    }

    std::set<Metadata> ret;
    auto root = MetadataBuilder::from(context, local_path, repl);
    if(root.file_type == type::directory) root.name = ".";
    ret.insert(root);

    // add include contents if directory
    for(const auto &content : local_path.contents()) {
      ret.insert(MetadataBuilder::from(context, content, repl));
    }

    return ret;
  }

  static Metadata directory(const path& directory) {
    auto metadata = chord::fs::create_directory();
    metadata.name = directory.filename();
    return metadata;
  }

  /**
   * Get metadata from existing local path
   *
   * @todo implement owner / group
   */
  static Metadata from(const Context& context, const path& local_path, const Replication repl = {}) {
    if (!file::exists(local_path)) {
      throw__exception("not found: " + local_path.string());
    }

    std::ifstream istream(local_path);

    Metadata meta{local_path.filename(),
                  "",  // owner
                  "",  // group
                  perms::all,
                  file::is_directory(local_path) ? type::directory : type::regular,
                  file::file_size(local_path),
                  file::is_regular_file(local_path) ? chord::optional<chord::uuid>{chord::crypto::sha256(istream)} : chord::optional<chord::uuid>{},
                  {},
                  repl};

    for(const auto &content : local_path.contents()) {
      auto meta = MetadataBuilder::from(context, content, repl);
    }

    return meta;
  }

  static Metadata directory(const chord::uri& uri, const Replication repl = {}) {
    Metadata meta{
      uri.path().filename(),
        "",
        "",
        perms::all,
        type::directory,
        0,
        {},
        {},
        repl
    };
    return meta;
  }

  /**
   * @todo implement owner / group
   */
  static Metadata from(const chord::fs::Data& item) {
    // replication
    Replication repl(item.replication_idx(), item.replication_cnt());

    Metadata meta{item.filename(),
                  "",  // owner
                  "",  // group
                  static_cast<perms>(item.permissions()),
                  static_cast<type>(item.type()), 
                  item.size(),
                  !item.file_hash().empty() ? chord::optional<chord::uuid>{item.file_hash()} : chord::optional<chord::uuid>{},
                  item.has_node_ref() ? chord::common::make_node(item.node_ref()) : chord::optional<chord::node>{},
                  repl};

    return meta;
  }

  static std::set<Metadata> from(const chord::fs::MetaRequest* req) {
    if (req->metadata_size() <= 0)
      throw__exception("Failed to convert MetaRequest since metadata is missing");

    std::set<Metadata> returnValue;
    for( const auto &data : req->metadata()) {
      const Metadata meta = MetadataBuilder::from(data);
      returnValue.insert(meta);
    }

    return returnValue;
  }

  static chord::optional<chord::node> make_node(const chord::fs::MetaResponse& res) {
    if(res.has_node_ref())
      return {{res.node_ref().uuid(), res.node_ref().endpoint()}};
    return {};
  }

  static std::map<Replication, std::set<Metadata>> group_by_replication(const std::set<Metadata>& metadata) {
    std::map<Replication, std::set<Metadata>> grouped;
    for(const auto& meta:metadata) {
      const auto& repl = meta.replication;
      if(!repl) continue;
      if(repl.index >= repl.count-1) continue;
      grouped[repl].insert(meta);
    }
    return grouped;
  }

  static std::set<Metadata> from(const chord::fs::MetaResponse& res) {
    std::set<Metadata> ret;
    for (const auto& m : res.metadata()) {
      ret.insert(MetadataBuilder::from(m));
    }
    return ret;
  }

  static void addMetadata(const std::set<Metadata>& metadata, chord::fs::MetaResponse* response) {
    MetadataBuilder::addMetadata(metadata, *response);
  }

  static void addMetadata(const std::set<Metadata>& metadata, chord::fs::MetaResponse& response) {
    for (const auto& m : metadata) {
      chord::fs::Data* data = response.add_metadata();
      data->set_filename(m.name);
      data->set_type(value_of(m.file_type));
      data->set_size(m.file_size);
      data->set_permissions(value_of(m.permissions));
      if(m.file_hash) {
        data->set_file_hash(m.file_hash.value().string());
      }
      if(m.replication) {
        const auto repl = m.replication;
        data->set_replication_idx(repl.index);
        data->set_replication_cnt(repl.count);
      }
    }
  }
};
}  // namespace fs
}  // namespace chord
