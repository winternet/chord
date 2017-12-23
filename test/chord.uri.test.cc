#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iomanip>

#include "chord.uri.h"
#include <experimental/filesystem>

using namespace std;
using namespace chord;

TEST(chord_uri, parse_url_filename_only) {
  auto uri = chord::uri::from("chord:/file.ext");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("file.ext", uri.path().filename());
  ASSERT_EQ(".ext", uri.path().extension());
}

TEST(chord_uri, url_builder_canonical) {
  chord::uri::builder uri_builder;
  auto uri = uri_builder.path("/folder/subfolder/./././////../subfolder/bar.ext").build();
  ASSERT_EQ("/folder/subfolder/bar.ext", uri.path().canonical());
}

TEST(chord_uri, parse_url_returns_canonical) {
  auto uri = chord::uri::from("chord://hostname/folder///////./subfolder/./../subfolder/bar.ext");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("hostname", uri.host());
  ASSERT_EQ("/folder/subfolder/bar.ext", uri.path().canonical());
  ASSERT_EQ("/folder/subfolder", uri.path().canonical().parent_path());
  ASSERT_EQ("bar.ext", uri.path().filename());
  ASSERT_EQ(".ext", uri.path().extension());
}

TEST(chord_uri, parse_url) {
  auto uri = chord::uri::from("chord://hostname/folder/subfolder/bar.ext");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("hostname", uri.host());
  ASSERT_EQ(path{"/folder/subfolder/bar.ext"s}, uri.path());
  ASSERT_EQ(path{"/folder/subfolder"s}, uri.path().parent_path());
  ASSERT_EQ(path{"bar.ext"s}, uri.path().filename());
  ASSERT_EQ(".ext", uri.path().extension());
}

TEST(chord_uri, parse_urn) {
  auto uri = chord::uri::from("chord:folder:filename.extension");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("folder:filename.extension", uri.path());
}

TEST(chord_uri, parse_success) {
  auto uri = chord::uri::from("scheme://user:password@host:50050/path/sub/index.html?foo=bar&zoom=1&ignored#fragment");

  // scheme
  ASSERT_EQ("scheme", uri.scheme());
  // authorization
  ASSERT_EQ("user", uri.user());
  ASSERT_EQ("password", uri.password());
  ASSERT_EQ("host", uri.host());
  ASSERT_EQ(50050, uri.port());
  // path
  ASSERT_EQ(path{"/path/sub/index.html"s}, uri.path());
  ASSERT_EQ(path{"/path/sub"s}, uri.path().parent_path());
  ASSERT_EQ(path{"index.html"s}, uri.path().filename());
  ASSERT_EQ(".html", uri.path().extension());
  // queries
  ASSERT_EQ("bar", uri.query().at("foo"));
  ASSERT_EQ("1", uri.query().at("zoom"));
  // ignored not in query
  ASSERT_EQ(uri.query().end(), uri.query().find("ignored"));
  // fragment
  ASSERT_EQ("fragment", uri.fragment());
}

TEST(chord_uri, parse_path_only) {
  auto uri = chord::uri::from("path/foo/bar");
  ASSERT_EQ(path{"path/foo/bar"s}, uri.path());
}

TEST(chord_uri, encode_unescaped) {
  auto unescaped = uri::encode("abc1234-_.~");
  ASSERT_EQ(unescaped, "abc1234-_.~");
}

TEST(chord_uri, encode_escaped) {
  auto escaped = uri::encode("abc1234!#$&'()*+,/:;=?@[]");
  ASSERT_EQ(escaped, "abc1234%21%23%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D");
}

TEST(chord_uri, encode_decode_eqals_source) {
  auto source = "abc1234!#$&'()*+,/:;=?@[]"s;
  auto encoded = uri::encode(source);
  auto decoded = uri::decode(encoded);
  ASSERT_EQ(source, decoded);
}

TEST(chord_uri, encode_decode_url) {
  auto source = "http://www.cplusplus.com/reference/map/map/operator[]/"s;
  auto encoded = uri::encode(source);
  ASSERT_EQ("http%3A%2F%2Fwww.cplusplus.com%2Freference%2Fmap%2Fmap%2Foperator%5B%5D%2F", encoded);
  auto decoded = uri::decode(encoded);
  ASSERT_EQ(source, decoded);
}
