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
#include "chord.log.h"

namespace chord {
namespace fs {

class MetadataManager {
  static constexpr auto logger_name = "chord.fs.metadata.manager";

 private:
  Context &context;
  std::unique_ptr<leveldb::DB> db;
  std::shared_ptr<spdlog::logger> logger;

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
    : context{context},
      logger{log::get_or_create(logger_name)}
      { initialize(); }

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
    logger->trace("adding metadata for {}", directory);

    for (const auto& m : metadata) {
      current.insert({m.name, m});
    }

    for (const auto& m: current) {
      logger->trace("{} : {}", m.first, m.second);
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
   *   2) save hash to Metadata (deserialization needed)
   */
  std::map<chord::uri, std::set<Metadata> > get(const chord::uuid& from, const chord::uuid& to) {
    std::map<chord::uri, std::set<Metadata> > ret;
    std::unique_ptr<leveldb::Iterator> it{db->NewIterator(leveldb::ReadOptions())};
    for(it->SeekToFirst(); it->Valid(); it->Next()) {
      const std::string& _path = it->key().ToString();
      const chord::uri uri("chord", {_path});
      const chord::uuid hash = chord::crypto::sha256(uri);
      logger->trace("\nuri {}\nhash_uri  {}\nfrom      {}\nto        {}", uri, hash, from, to);

      if(hash.between(from, to)) {
        const auto map = deserialize(it->value().ToString());
        //TODO refactor this, either always save uri
        //     or save scheme in metadata
        ret[uri] = extract_metadata_set(map);
      }
    }
    for(const auto& o : ret) {
      logger->trace("data in uri: {}", o.first);
      for(const auto &m : o.second) {
        logger->trace("meta: {}", m);
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
    for (const auto& m: map) {
      logger->trace("{} : {}", m.first, m.second);
    }
    return extract_metadata_set(map);
  }

};

} //namespace fs
} //namespace chord
