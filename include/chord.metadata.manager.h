#pragma once

// metadata strategy: leveldb
#include <leveldb/db.h>

#include "chord.context.h"
#include "chord.fs.metadata.builder.h"
#include "chord.fs.metadata.h"
#include "chord.uri.h"

#define MANAGER_LOG(level, method) LOG(level) << "[meta.mgr][" << #method << "] "

namespace chord {
namespace fs {

class MetadataManager {

 private:
  Context &context;
  std::unique_ptr<leveldb::DB> db;

  void check_status(const leveldb::Status &status) {
    if(status.ok()) return;

    throw chord::exception(status.ToString());
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
  }

 public:
  explicit MetadataManager(Context &context)
    : context{context} { initialize(); }

  MetadataManager(const MetadataManager&) = delete;

  void del(const chord::uri& directory) {
    //Metadata current{directory.path().canonical().string()};
    check_status(db->Delete(leveldb::WriteOptions(), directory.path().canonical().string()));
  }

  void del(const chord::uri& directory, const set<Metadata> &metadata) {
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

  set<Metadata> dir(const chord::uri& directory) {
    std::string value;
    auto status = db->Get(leveldb::ReadOptions(), directory.path().canonical().string(), &value);

    if(status.ok()) {
      auto current = deserialize(value);
      set<Metadata> ret;
      for(const auto &m:current) {
        ret.insert(m.second);
      }
      return ret;
    } else if(status.IsNotFound()) {
      throw chord::exception("not found");
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

  std::set<Metadata> get(const chord::uri& directory) {
    std::string value;
    check_status(db->Get(leveldb::ReadOptions(), directory.path().string(), &value));

    const auto map = deserialize(value);

    set<Metadata> ret;
    for(const auto& m:map) ret.insert(m.second);
    return ret;
  }

};

} //namespace fs
} //namespace chord
