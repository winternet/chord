#include "chord.int.test.h"
#include "chord.fs.metadata.h"

namespace chord {
namespace test {


std::string put(const path& src, const path& dst) {
  return "put " + src.string() + " " + to_string(uri{"chord", dst});
}

void del(chord::IntPeer* peer, const chord::uri& dst) {
  controller::Client ctrl_client;
  ctrl_client.control(peer->get_context().advertise_addr, "del "+std::string(dst));
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

