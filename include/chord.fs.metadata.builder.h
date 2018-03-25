#pragma once

#include <set>
#include "chord.exception.h"
#include "chord.file.h"
#include "chord.fs.metadata.h"
#include "chord.path.h"
#include "chord_fs.pb.h"

namespace chord {
namespace fs {

using std::set;
using chord::fs::MetaRequest;
using chord::fs::Data;
using chord::path;
using chord::file::exists;
using chord::file::is_directory;

struct MetadataBuilder {

  static set<Metadata> for_path(const path& local_path) {
    if (!file::exists(local_path)) {
      throw chord::exception("not found: " + local_path.string());
    }

    set<Metadata> ret = {};
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
      throw chord::exception("not found: " + local_path.string());
    }

    Metadata meta{local_path.filename(),
                  "",  // owner
                  "",  // group
                  perms::all,
                  is_directory(local_path) ? type::directory : type::regular};

    for(const auto &content : local_path.contents()) {
      auto meta = MetadataBuilder::from(content);
    }

    return meta;
  }

  static Metadata from(const Data& item) {
    Metadata meta{item.filename(),
                  "",  // owner
                  "",  // group
                  static_cast<perms>(item.permissions()),
                  static_cast<type>(item.type())};

    return meta;
  }

  static set<Metadata> from(const MetaRequest* req) {
    if (req->metadata_size() <= 0)
      throw chord::exception(
          "Failed to convert MetaRequest since metadata is missing");

    set<Metadata> returnValue;
    for( const auto &data : req->metadata()) {
      const Metadata meta = MetadataBuilder::from(data);
      returnValue.insert(meta);
    }

    return returnValue;
  }

  static set<Metadata> from(const MetaResponse& res) {
    if (res.metadata_size() <= 0)
      throw chord::exception(
          "Failed to convert MetaRequest since metadata is missing");

    set<Metadata> ret;
    for (const auto& m : res.metadata()) {
      ret.insert(MetadataBuilder::from(m));
    }
    return ret;
  }
};
}  // namespace fs
}  // namespace chord

