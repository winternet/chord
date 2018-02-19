#pragma once

#include "chord.exception.h"
#include "chord.file.h"
#include "chord.fs.metadata.h"
#include "chord.path.h"
#include "chord_fs.pb.h"

namespace chord {
namespace fs {

using chord::fs::MetaRequest;
using chord::fs::Data;
using chord::path;
using chord::file::exists;
using chord::file::is_directory;

struct MetadataBuilder {

  static Metadata from(const path& local_path) {
    if (!file::exists(local_path)) {
      throw chord::exception("not found: " + local_path.string());
    }

    Metadata meta{local_path.filename()};
    //TODO(muffin): implement this someday
    meta.permissions = perms::all;
    //TODO(muffin): extend
    meta.file_type = is_directory(local_path) ? type::directory : type::regular;

    return meta;
  }

  static Metadata from(const Data& item) {
    Metadata meta(item.filename());
    meta.permissions = static_cast<perms>(item.permissions());
    meta.file_type = static_cast<type>(item.type());

    return meta;
  }

  static Metadata from(const MetaRequest* req) {
    if (!req->has_metadata())
      throw chord::exception(
          "Failed to convert MetaRequest since metadata is missing");

    Metadata ret = MetadataBuilder::from(req->metadata());

    for (const auto &file : req->files()) {
      ret.files.emplace(MetadataBuilder::from(file));
    }
    return ret;
  }

  static Metadata from(const MetaResponse& res) {
    if (!res.has_metadata())
      throw chord::exception(
          "Failed to convert MetaRequest since metadata is missing");

    Metadata ret = MetadataBuilder::from(res.metadata());

    for (const auto &file : res.files()) {
      ret.files.emplace(MetadataBuilder::from(file));
    }
    return ret;
  }
};
}  // namespace fs
}  // namespace chord
