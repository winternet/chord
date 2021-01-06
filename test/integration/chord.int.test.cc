#include "chord.int.test.h"
#include "chord.fs.metadata.h"
#include "chord.utils.h"

namespace chord {
namespace test {


std::string put(const path& src, const path& dst) {
  return "put " + src.string() + " " + to_string(chord::utils::as_uri(dst));
}

grpc::Status del(chord::IntPeer* peer, const chord::uri& dst, bool recursive) {
  controller::Client ctrl_client;
  return ctrl_client.control(peer->get_context().advertise_addr, "del "s + (recursive ? "--recursive ":"") + std::string(dst));
}

grpc::Status mkdir(chord::IntPeer* peer, int replication, const chord::uri& dst) {
  controller::Client ctrl_client;
  return ctrl_client.control(peer->get_context().advertise_addr, "mkdir --repl "+std::to_string(replication)+" "+std::string(dst));
}

void IntegrationTest::assert_metadata(const IntPeer* peer, const uri& target_uri) const {
  const auto metadata_mgr = peer->get_metadata_manager();
  ASSERT_TRUE(metadata_mgr->exists(target_uri));


  const auto parent_path = target_uri.path().parent_path();
  if(parent_path.empty()) return;

  ASSERT_THAT(peer, ParentContains(target_uri));
}

void IntegrationTest::assert_equal(const IntPeer* peer, std::shared_ptr<chord::test::TmpBase> source, const uri& target_uri, const bool check_parent) const {
  const auto metadata_mgr = peer->get_metadata_manager();
  const auto target_path = peer->get_context().data_directory/target_uri.path();

  ASSERT_TRUE(file::files_equal(p(source), target_path)) << "source[" << source->path << "] vs [" << target_path << "]";
  ASSERT_TRUE(metadata_mgr->exists(target_uri));

  if(check_parent) {
    ASSERT_THAT(peer, ParentContains(target_uri));
  }
}

void IntegrationTest::assert_deleted(const IntPeer* peer, const uri& target_uri, const bool check_parent) const {
  const auto metadata_mgr = peer->get_metadata_manager();
  const auto target_path = peer->get_context().data_directory/target_uri.path();

  ASSERT_FALSE(file::exists(target_path));
  ASSERT_FALSE(metadata_mgr->exists(target_uri));

  if(check_parent) {
    ASSERT_THAT(peer, Not(ParentContains(target_uri)));
  }
}

} // namespace test
} // namespace chord

