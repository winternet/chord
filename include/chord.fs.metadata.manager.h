#pragma once

// metadata strategy: rocksdb
#include <rocksdb/db.h>
#include <set>
#include <map>

#include "chord.context.h"
#include "chord.crypto.h"
#include "chord.fs.metadata.builder.h"
#include "chord.fs.metadata.h"
#include "chord.i.fs.metadata.manager.h"
#include "chord.uri.h"
#include "chord.log.h"

namespace chord {
namespace fs {

class MetadataManager : public IMetadataManager {
  static constexpr auto logger_name = "chord.fs.metadata.manager";

 private:
  Context &context;
  std::unique_ptr<rocksdb::DB> db;
  std::shared_ptr<spdlog::logger> logger;

  void check_status(const rocksdb::Status &status) {
    if(status.ok()) return;

    throw__exception(status.ToString());
  }

  std::map<std::string, Metadata> deserialize(const std::string& metadata) {
    std::map<std::string, Metadata> ret;

    if(metadata.empty()) return ret;

    std::stringstream ss{metadata};
    boost::archive::text_iarchive ia{ss};
    ia >> ret;
    return ret;
  }

  std::string serialize(const std::map<std::string, Metadata>& metadata) {
    std::stringstream ss;
    boost::archive::text_oarchive oa{ss};
    oa << metadata;
    return ss.str();
  }

  void initialize() {
    rocksdb::DB *db_tmp;
    rocksdb::Options options;
    options.create_if_missing = true;

    check_status(rocksdb::DB::Open(options, context.meta_directory, &db_tmp));
    db.reset(db_tmp);

    // make root
    add(uri{"chord:///"}, {{".", "", "", perms::all, type::directory}});
  }

  void __del(const chord::uri& uri) {
    logger->trace("[DEL] uri {}", uri);
    check_status(db->Delete(rocksdb::WriteOptions(), uri.path().canonical().string()));
  }

 public:
  explicit MetadataManager(Context &context)
    : context{context},
      logger{context.logging.factory().get_or_create(logger_name)}
      { initialize(); }

  MetadataManager(const MetadataManager&) = delete;

  std::set<Metadata> del(const chord::uri& directory) override {
    std::set<Metadata> retVal = get(directory);
    __del(directory);
    return retVal;
  }

  std::set<Metadata> del(const chord::uri& directory, const std::set<Metadata> &metadata, const bool removeIfEmpty) override {
    std::string value;
    const auto path = directory.path().canonical().string();
    check_status(db->Get(rocksdb::ReadOptions(), path, &value));

    logger->trace("[DEL] {}", directory);
    for(const auto& meta:metadata) {
      logger->trace("[DEL] `-  {}", meta);
    }

    std::set<Metadata> retVal;

    //map['path'] = metadata
    auto current = deserialize(value);
    for(const auto &m:metadata) {
      retVal.insert(current[m.name]);
      current.erase(m.name);
    }

    if(current.empty() && removeIfEmpty) {
      __del(directory);
    } else {
      value = serialize(current);
      check_status(db->Put(rocksdb::WriteOptions(), path, value));
    }

    return retVal;
  }

  std::set<Metadata> dir(const chord::uri& directory) override {
    std::string value;
    const auto status = db->Get(rocksdb::ReadOptions(), directory.path().canonical().string(), &value);

    if(status.ok()) {
      logger->trace("[DIR] {}", directory);
      const auto current = deserialize(value);
      std::set<Metadata> ret;
      for(const auto &m:current) {
        ret.insert(m.second);
        logger->trace("[DIR] `-  {}", m.second);
      }
      return ret;
    }

    throw__exception(std::string{"failed to dir: "} + status.ToString());
  }

  bool add(const chord::uri& directory, const std::set<Metadata>& metadata) override {
    std::string value;
    const auto path = directory.path().canonical().string();
    const auto status = db->Get(rocksdb::ReadOptions(), path, &value);

    if(!status.ok() && !status.IsNotFound()){
      check_status(status);
    }

    std::map<std::string, Metadata> current = deserialize(value);

    bool added = false;
    for (const auto& m : metadata) {
      // TODO check whether m already exists && equals 
      const auto [_, status] = current.insert_or_assign(m.name, m);
      added |= status;
    }

    // always override "." since replication might have changed
    // do not allow overrides from clients
    if(current.find(".") != current.end()) {
      current.insert_or_assign(".", create_directory(metadata));
    }

    logger->trace("[ADD] {}", directory);
    for (const auto& [path, meta]: current) {
      logger->trace("[ADD] `-  {}", meta);
    }

    value = serialize(current);

    check_status(db->Put(rocksdb::WriteOptions(), path, value));

    return added;
  }

  std::map<chord::uri, std::set<Metadata> > get_all() override {
    std::map<chord::uri, std::set<Metadata> > ret;
    std::unique_ptr<rocksdb::Iterator> it{db->NewIterator(rocksdb::ReadOptions())};
    for(it->SeekToFirst(); it->Valid(); it->Next()) {
      const std::string& _path = it->key().ToString();

      const chord::uri uri("chord", {_path});
      const auto map = deserialize(it->value().ToString());
      ret[uri] = extract_metadata_set(map);
    }
    return ret;
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
  std::map<chord::uri, std::set<Metadata> > get(const chord::uuid& from, const chord::uuid& to) override {
    std::map<chord::uri, std::set<Metadata> > ret;
    std::unique_ptr<rocksdb::Iterator> it{db->NewIterator(rocksdb::ReadOptions())};
    for(it->SeekToFirst(); it->Valid(); it->Next()) {
      const std::string& _path = it->key().ToString();

      const chord::uri uri("chord", {_path});
      const chord::uuid hash = chord::crypto::sha256(uri);

      if(uuid::between(from, hash, to)) {
        const auto map = deserialize(it->value().ToString());
        //TODO refactor this, either always save uri
        //     or save scheme in metadata
        ret[uri] = extract_metadata_set(map);
      }
    }
    for(const auto& o : ret) {
      logger->trace("[GET] {}", o.first);
      for(const auto &m : o.second) {
        logger->trace("[GET] `-  {}", m);
      }
    }
    return ret;
  }

  std::set<Metadata> extract_metadata_set(const std::map<std::string, Metadata>& map) {
    std::set<Metadata> ret;
    for(const auto& m:map) ret.insert(m.second);
    return ret;
  }

  //TODO write unit tests
  //TODO check whether to return only leaf nodes
  //TODO 
  std::map<chord::uri, std::set<Metadata>> get_shallow_copies(const chord::node& node) override {
    std::map<chord::uri, std::set<Metadata>> ret;
    std::unique_ptr<rocksdb::Iterator> it{db->NewIterator(rocksdb::ReadOptions())};

    for(it->SeekToFirst(); it->Valid(); it->Next()) {
      const std::string& path = it->key().ToString();
      const auto map = deserialize(it->value().ToString());
      const chord::uri uri("chord", {path});
      for(const auto& [_, meta] : map) {
        if(meta.node_ref && meta.node_ref == node) {
          ret[uri].insert(meta);
        }
      }
    }
    return ret;
  }

  std::map<chord::uri, std::set<Metadata>> get_replicated(const std::uint32_t min_idx) override {
    std::map<chord::uri, std::set<Metadata>> ret;
    std::unique_ptr<rocksdb::Iterator> it{db->NewIterator(rocksdb::ReadOptions())};

    for(it->SeekToFirst(); it->Valid(); it->Next()) {
      const std::string& path = it->key().ToString();
      const auto map = deserialize(it->value().ToString());
      const chord::uri uri("chord", {path});
      for(const auto& [_, meta] : map) {
        if(meta.file_type != type::directory && meta.replication.count > 1 && meta.replication.count >= min_idx) {
          ret[uri].insert(meta);
        }
      }
    }
    return ret;
  }

  bool exists(const chord::uri& uri) override {
    std::string value;
    return db->Get(rocksdb::ReadOptions(), uri.path().string(), &value).ok();
  }

  std::set<Metadata> get(const chord::uri& directory) override {
    std::string value;
    check_status(db->Get(rocksdb::ReadOptions(), directory.path().string(), &value));

    const auto map = deserialize(value);
    for (const auto& [path, meta]: map) {
      logger->trace("[GET] `-  {}", meta);
    }
    return extract_metadata_set(map);
  }

};

} //namespace fs
} //namespace chord
