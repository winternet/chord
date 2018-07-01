#pragma once

// metadata strategy: leveldb
#include <leveldb/db.h>
#include <set>
#include <map>

#include "chord.context.h"
#include "chord.crypto.h"
#include "chord.fs.metadata.builder.h"
#include "chord.fs.metadata.h"
#include "chord.uri.h"

namespace chord {
namespace fs {

class MetadataManager {

 private:
  Context &context;
  std::unique_ptr<leveldb::DB> db;

  void check_status(const leveldb::Status &status) {
    if(status.ok()) return;

    throw__exception(status.ToString());
  }

  std::map<std::string, Metadata> deserialize(const std::string& metadata) {
    std::map<std::string, Metadata> ret;

    if(metadata.empty()) return ret;

    std::stringstream ss{metadata};
    boost::archive::text_iarchive ia(ss);
    ia >> ret;
    return ret;
  }

  std::string serialize(const std::map<std::string, Metadata>& metadata) {
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << metadata;
    return ss.str();
  }

  void initialize() {
    leveldb::DB *db_tmp;
    leveldb::Options options;
    options.create_if_missing = true;

    check_status(leveldb::DB::Open(options, context.meta_directory, &db_tmp));
    db.reset(db_tmp);

    // make root
    add({"chord:///"}, {{".", "", "", perms::all, type::directory}});
  }

 public:
  explicit MetadataManager(Context &context)
    : context{context} { initialize(); }

  MetadataManager(const MetadataManager&) = delete;

  void del(const chord::uri& directory) {
    //Metadata current{directory.path().canonical().string()};
    check_status(db->Delete(leveldb::WriteOptions(), directory.path().canonical().string()));
  }

  void del(const chord::uri& directory, const std::set<Metadata> &metadata) {
    std::string value;
    auto path = directory.path().canonical().string();
    check_status(db->Get(leveldb::ReadOptions(), path, &value));

    //map['path'] = metadata
    auto current = deserialize(value);
    for(const auto &m:metadata) {
      current.erase(m.name);
    }
    
    value = serialize(current);
    check_status(db->Put(leveldb::WriteOptions(), path, value));
  }

  std::set<Metadata> dir(const chord::uri& directory) {
    std::string value;
    auto status = db->Get(leveldb::ReadOptions(), directory.path().canonical().string(), &value);

    if(status.ok()) {
      auto current = deserialize(value);
      std::set<Metadata> ret;
      for(const auto &m:current) {
        ret.insert(m.second);
      }
      return ret;
    }

    if(status.IsNotFound()) {
      throw__exception("not found");
    }
  }

  void add(const chord::uri& directory, const std::set<Metadata>& metadata) {
    std::string value;
    auto path = directory.path().canonical().string();
    auto status = db->Get(leveldb::ReadOptions(), path, &value);

    if(!status.ok() && !status.IsNotFound()){
      check_status(status);
    }

    std::map<std::string, Metadata> current = deserialize(value);

    for (const auto& m : metadata) {
      current.insert({m.name, m});
    }

    value = serialize(current);

    check_status(db->Put(leveldb::WriteOptions(), path, value));
  }

  /**
   * @brief Get all metadata in range (from...to)
   *
   * @param from uuid (exclusive)
   * @param to uuid (exclusive)
   * @todo re-hashing with full-table scan is slow
   *   1) use an 'index' or
   *   2) save hash to Metadata
   */
  std::map<uuid, std::set<Metadata> > get(const chord::uuid& from, const chord::uuid& to) {
    std::map<uuid, std::set<Metadata> > ret;
    auto *it = db->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()) {
      const std::string& uri = it->key().ToString();
      const uuid hash = chord::crypto::sha256(uri);
      if(hash.between(from, to)) {
        const auto map = deserialize(it->value().ToString());
        ret[hash] = extract_metadata_set(map);
      }
    }
    return ret;
  }

  std::set<Metadata> extract_metadata_set(const std::map<std::string, Metadata>& map) {
    std::set<Metadata> ret;
    for(const auto& m:map) ret.insert(m.second);
    return ret;
  }

  std::set<Metadata> get(const chord::uri& directory) {
    std::string value;
    check_status(db->Get(leveldb::ReadOptions(), directory.path().string(), &value));

    const auto map = deserialize(value);
    return extract_metadata_set(map);
  }

};

} //namespace fs
} //namespace chord
