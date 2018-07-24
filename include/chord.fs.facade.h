#pragma once

#include <set>

#include "chord.fs.client.h"
#include "chord.fs.service.h"
#include "chord.uri.h"

namespace chord {
namespace fs {

class Facade {
 private:
  const Context& context;
  std::unique_ptr<Client> fs_client;
  std::unique_ptr<Service> fs_service;

 private:
  void put_file(const chord::path& source, const chord::uri& target);
  void get_file(const chord::uri& source, const chord::path& target);
  bool is_directory(const chord::uri& target);

 public:
  Facade(Context& context, ChordFacade* chord);

  ::grpc::Service* grpc_service();

  void put(const chord::path& source, const chord::uri& target);

  void get(const chord::uri& source, const chord::path& target);

  void dir(const chord::uri& uri, std::iostream& iostream);

  void del(const chord::uri& uri);

  chord::take_consumer_t on_leave_callback() {
    return [&](const chord::TakeResponse& res) {
      if (!res.has_detail()) return;
      if (!res.detail().Is<chord::fs::MetaResponse>()) return;

      chord::fs::MetaResponse meta_res;
      res.detail().UnpackTo(&meta_res);
      if (meta_res.uri().empty()) {
        //TODO use logger
        std::cerr << "received TakeResponse.MetaResponse without uri";
      }

      std::cerr << "\nprocessing MetaResponse: " << meta_res.uri() << "\n";
      const auto data_set = MetadataBuilder::from(meta_res);
      for (const auto& data : data_set) {
        std::cerr << "... data    " << data.name << "\n";
        if (data.file_type == type::regular) {
          std::cerr << "... trying to get_file " << meta_res.uri() << " to ." << "\n";
          //this->get_file(uri{meta_res.uri()}, context.data_directory);
          this->get_file(uri{meta_res.uri()}, context.data_directory / uri{meta_res.uri()}.path());
        } else {
          std::cerr << "... skipping " << meta_res.uri() << "\n";
        }
      }

      //const std::set<Metadata> metadata = MetadataBuilder::from(meta_res);
      //const auto uri = uri::from(meta_res.uri());
      //// make_client().
      //fs_service->metadata_manager()->add(uri, metadata);
    };
  }
  /**
   * @note should be called only once - create on demand
   * @todo move to cc
   */
  chord::take_consumer_t take_consumer_callback() {
    return [&](const chord::TakeResponse& res) {
      if(!res.has_detail()) return;

      if(!res.detail().Is<chord::fs::MetaResponse>()) return;

      chord::fs::MetaResponse meta_res;
      res.detail().UnpackTo(&meta_res);
      if(meta_res.uri().empty()) {
        //TODO use logger
        std::cerr << "received TakeResponse.MetaResponse without uri";
        return;
      }

      const std::set<Metadata> metadata = MetadataBuilder::from(meta_res);
      const auto uri = uri::from(meta_res.uri());
      fs_service->metadata_manager()->add(uri, metadata);
    };
  }

  /**
   * @note should be called only once - create on demand
   * @todo move to cc
   */
  chord::take_producer_t take_producer_callback() {
    return [&](const auto& from, const auto& to) {
      const auto map = fs_service->metadata_manager()->get(from, to);
      std::vector<chord::TakeResponse> ret;
      for (const auto& m : map) {
        chord::TakeResponse res;
        chord::fs::MetaResponse meta;

        meta.set_uri(m.first);
        meta.set_ref_id(context.uuid());
        MetadataBuilder::addMetadata(m.second, meta);
        res.set_id(m.first);
        res.mutable_detail()->PackFrom(meta);
        ret.push_back(res);
      }
      return ret;
    };
  }
};

} //namespace fs
} //namespace chord
