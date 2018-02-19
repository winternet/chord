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

  Metadata deserialize(const std::string& metadata) {
    Metadata ret;
    std::stringstream ss{metadata};
    boost::archive::text_iarchive ia(ss);
    ia >> ret;
    return ret;
  }

  std::string serialize(const Metadata& metadata) {
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

  void del(const chord::uri& directory, const Metadata &metadata) {
    std::string value;
    Metadata current{directory.path().canonical().string()};
    check_status(db->Get(leveldb::ReadOptions(), current.name, &value));

    current = deserialize(value);
    current.files.erase(metadata);
    
    value = serialize(current);
    check_status(db->Put(leveldb::WriteOptions(), current.name, value));
  }

  void mod(const chord::uri &directory, const Metadata& metadata) {
    throw chord::exception("currently not supported");
  }
  /*

  void dir(const chord::uri& directory, Metadata& metadata) {
    std::string value;
    Metadata current{directory.path().canonical().string()};
    auto status = db->Get(leveldb::ReadOptions(), current.name, &value);

    if(status.ok()) {
      Metadata current = deserialize(value);
      metadata = current;
    } else if(status.IsNotFound()) {
      throw chord::exception("not found");
    }
  }
  */

  void add(const chord::uri& directory, const Metadata& metadata) {
    std::string value;
    Metadata current{directory.path().canonical().string()};
    auto status = db->Get(leveldb::ReadOptions(), current.name, &value);

    if(status.ok()) {
      current = deserialize(value);
      //MANAGER_LOG(trace, add) << "current mentadata for " << directory << "\n" << current;
    } else if(!status.IsNotFound()){
      check_status(status);
    }
    
    current.files.insert(metadata);

    //MANAGER_LOG(trace, add) << "new metadata for " << directory << "\n" << current;
    value = serialize(current);

    check_status(db->Put(leveldb::WriteOptions(), current.name, value));
  }

  Metadata get(const chord::uri& directory) {
    std::string value;
    check_status(db->Get(leveldb::ReadOptions(), directory.path().string(), &value));

    return deserialize(value);
  }

};

} //namespace fs
} //namespace chord
