#pragma once

#include "chord.exception.h"
#include "chord.fs.metadata.h"
#include "chord_fs.pb.h"

namespace chord {
namespace fs {

using chord::fs::MetaRequest;
using chord::fs::Data;

struct MetadataBuilder {
  static Metadata from(const Data& item) {
    Metadata meta(item.filename());
    meta.permissions = static_cast<perms>(item.permission());
    meta.file_type = static_cast<type>(item.type());

    return meta;
  }

  static Metadata from(const MetaRequest* req) {
    if (!req->has_metadata())
      throw chord::exception(
          "Failed to convert MetaRequest since metadata is missing");

    Metadata ret = MetadataBuilder::from(req->metadata());

    for (auto file : req->files()) {
      ret.files.emplace(MetadataBuilder::from(file));
    }
    return ret;
  }
};
}  // namespace fs
}  // namespace chord
