#pragma once

#include <set>
#include "chord.exception.h"
#include "chord.file.h"
#include "chord.fs.metadata.h"
#include "chord.path.h"
#include "chord_fs.pb.h"
#include "chord.context.h"

namespace chord {
namespace fs {

struct MetadataBuilder {

  /**
   * generate metadata from chord uri path
   *
   * @param context to prepend chord's data directory
   * @param chord_path the path from chord:// e.g. /folder/file
   */
  static std::set<Metadata> for_path(const Context& context, const path& chord_path) {
    const auto local_path = context.data_directory / chord_path;
    if (!file::exists(local_path)) {
      throw__exception("not found: " + local_path.string());
    }

    std::set<Metadata> ret = {};
    auto root = MetadataBuilder::from(context, local_path);
    if(root.file_type == type::directory) root.name = ".";
    ret.insert(root);

    // add include contents if directory
    for(const auto &content : local_path.contents()) {
      ret.insert(MetadataBuilder::from(context, content));
    }

    return ret;
  }

  /**
   * Get metadata from existing local path
   *
   * @todo implement owner / group
   */
  static Metadata from(const Context& context, const path& local_path) {
    if (!file::exists(local_path)) {
      throw__exception("not found: " + local_path.string());
    }

    Metadata meta{local_path.filename(),
                  "",  // owner
                  "",  // group
                  perms::all,
                  file::is_directory(local_path) ? type::directory : type::regular};

    for(const auto &content : local_path.contents()) {
      auto meta = MetadataBuilder::from(context, content);
    }

    return meta;
  }

  /**
   * @todo implement owner / group
   */
  static Metadata from(const chord::fs::Data& item, chord::optional<chord::node> node_ref = {}) {
    Metadata meta{item.filename(),
                  "",  // owner
                  "",  // group
                  static_cast<perms>(item.permissions()),
                  static_cast<type>(item.type()), 
                  node_ref};

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

  static std::set<Metadata> from(const chord::fs::MetaResponse& res) {
    std::set<Metadata> ret;
    for (const auto& m : res.metadata()) {
      ret.insert(MetadataBuilder::from(m, make_node(res)));
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
      data->set_permissions(value_of(m.permissions));
    }
  }
};
}  // namespace fs
}  // namespace chord

