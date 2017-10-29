#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iomanip>

#include "chord.uri.h"

using namespace std;
using namespace chord;

TEST(chord_uri, parse_path) {
  auto uri = chord::uri::from("chord://folder/filename.extension");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("folder/filename.extension", uri.path());
}
TEST(chord_uri, parse_success) {
  auto uri = chord::uri::from("scheme://user:password@host:50050/path?foo=bar&zoom=1&ignored#fragment");

  // scheme
  ASSERT_EQ("scheme", uri.scheme());
  // authorization
  ASSERT_EQ("user", uri.user());
  ASSERT_EQ("password", uri.password());
  ASSERT_EQ("host", uri.host());
  ASSERT_EQ(50050, uri.port());
  // path
  ASSERT_EQ("/path", uri.path());
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
    ASSERT_EQ("path/foo/bar", uri.path());
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
