#pragma once

#include <set>
#include "chord.exception.h"
#include "chord.file.h"
#include "chord.fs.metadata.h"
#include "chord.path.h"
#include "chord_fs.pb.h"

namespace chord {
namespace fs {

struct MetadataBuilder {

  static std::set<Metadata> for_path(const path& local_path) {
    if (!file::exists(local_path)) {
      throw__exception("not found: " + local_path.string());
    }

    std::set<Metadata> ret = {};
    auto root = MetadataBuilder::from(local_path);
    if(root.file_type == type::directory) root.name = ".";
    ret.insert(root);

    // add include contents if directory
    for(const auto &content : local_path.contents()) {
      ret.insert(MetadataBuilder::from(content));
    }

    return ret;
  }

  static Metadata from(const path& local_path) {
    if (!file::exists(local_path)) {
      throw__exception("not found: " + local_path.string());
    }

    Metadata meta{local_path.filename(),
                  "",  // owner
                  "",  // group
                  perms::all,
                  file::is_directory(local_path) ? type::directory : type::regular};

    for(const auto &content : local_path.contents()) {
      auto meta = MetadataBuilder::from(content);
    }

    return meta;
  }

  static Metadata from(const chord::fs::Data& item) {
    Metadata meta{item.filename(),
                  "",  // owner
                  "",  // group
                  static_cast<perms>(item.permissions()),
                  static_cast<type>(item.type())};

    return meta;
  }

  static std::set<Metadata> from(const chord::fs::MetaRequest* req) {
    if (req->metadata_size() <= 0)
      throw__exception(
          "Failed to convert MetaRequest since metadata is missing");

    std::set<Metadata> returnValue;
    for( const auto &data : req->metadata()) {
      const Metadata meta = MetadataBuilder::from(data);
      returnValue.insert(meta);
    }

    return returnValue;
  }

  static std::set<Metadata> from(const chord::fs::MetaResponse& res) {
    if (res.metadata_size() <= 0)
      throw__exception(
          "Failed to convert MetaRequest since metadata is missing");

    std::set<Metadata> ret;
    for (const auto& m : res.metadata()) {
      ret.insert(MetadataBuilder::from(m));
    }
    return ret;
  }
};
}  // namespace fs
}  // namespace chord

