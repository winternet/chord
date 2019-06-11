#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "chord.uri.h"
#include "chord.exception.h"

using namespace std;
using namespace chord;

TEST(chord_uri, parse_url_root) {
  auto uri = chord::uri::from("chord:///");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("/", uri.path().filename());
  chord::uri::from(to_string(uri));
}

TEST(chord_uri, parse_url_filename_only) {
  auto uri = chord::uri::from("chord:/file.ext");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("file.ext", uri.path().filename());
  ASSERT_EQ(".ext", uri.path().extension());
}

TEST(chord_uri, url_builder_canonical) {
  chord::uri::builder uri_builder;
  auto uri = uri_builder
      .scheme("https")
      .path("/folder/subfolder/./././////../subfolder/bar.ext")
      .build();
  ASSERT_EQ("/folder/subfolder/bar.ext", uri.path().canonical());
}

TEST(chord_uri, parse_url_returns_canonical) {
  auto uri = chord::uri::from("chord://hostname/folder///////./subfolder/./../subfolder/bar.ext");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("hostname", uri.auth()->host());
  ASSERT_EQ("/folder/subfolder/bar.ext", uri.path().canonical());
  ASSERT_EQ("/folder/subfolder", uri.path().canonical().parent_path());
  ASSERT_EQ("bar.ext", uri.path().filename());
  ASSERT_EQ(".ext", uri.path().extension());
}

TEST(chord_uri, parse_url) {
  auto uri = chord::uri::from("chord://hostname/folder/subfolder/bar.ext");
  ASSERT_EQ("chord", uri.scheme());
  ASSERT_EQ("hostname", uri.auth()->host());
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
  ASSERT_EQ("user", uri.auth()->user());
  ASSERT_EQ("password", uri.auth()->password());
  ASSERT_EQ("host", uri.auth()->host());
  ASSERT_EQ(50050, uri.auth()->port().value());
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

TEST(chord_uri, string_ctor) {
  auto u = uri{"scheme://user:password@host:50050/path/sub/index.html?foo=bar&zoom=1&ignored#fragment"};

  // scheme
  ASSERT_EQ("scheme", u.scheme());
  // authorization
  ASSERT_EQ("user", u.auth()->user());
  ASSERT_EQ("password", u.auth()->password());
  ASSERT_EQ("host", u.auth()->host());
  ASSERT_EQ(50050, u.auth()->port().value());
  // path
  ASSERT_EQ(path{"/path/sub/index.html"s}, u.path());
  ASSERT_EQ(path{"/path/sub"s}, u.path().parent_path());
  ASSERT_EQ(path{"index.html"s}, u.path().filename());
  ASSERT_EQ(".html", u.path().extension());
  // queries
  ASSERT_EQ("bar", u.query().at("foo"));
  ASSERT_EQ("1", u.query().at("zoom"));
  // ignored not in query
  ASSERT_EQ(u.query().end(), u.query().find("ignored"));
  // fragment
  ASSERT_EQ("fragment", u.fragment());
}

TEST(chord_uri, parse_path_only) {
  auto uri = chord::uri::from("chord:path/foo/bar");
  ASSERT_EQ("chord", uri.scheme());
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

TEST(chord_uri, authority) {
  auto auth = uri::authority("localhost", 50050);
  ASSERT_EQ("localhost", auth.host());
  ASSERT_EQ(50050, auth.port().value());
}

TEST(chord_uri, authority_with_user_pass) {
  auto auth = uri::authority("user", "pass", "localhost", 50050);
  ASSERT_EQ("localhost", auth.host());
  ASSERT_EQ(50050, auth.port().value());
  ASSERT_EQ("user", auth.user());
  ASSERT_EQ("pass", auth.password());
}

TEST(chord_uri, builder_with_authority) {
  auto uri = uri::builder()
      .scheme("https")
      .user("user")
      .password("password")
      .host("localhost")
      .port(50050)
      .path("/foo/bar")
      .add_query("key1", "val1")
      .add_query("key2", "val2")
      .fragment("fragment")
      .build();
  ASSERT_EQ("https://user:password@localhost:50050/foo/bar?key1=val1&key2=val2#fragment", to_string(uri));
}

TEST(chord_uri, builder_append_path) {
  auto uri = uri::builder("chord:///etc/foo").append_path({"/bar.md"}).build();
  ASSERT_EQ("chord:///etc/foo/bar.md", to_string(uri));
}

TEST(chord_uri, builder_append_path_trailing_slash) {
  auto uri = uri::builder("chord:///etc/foo/").append_path({"/bar.md"}).build();
  ASSERT_EQ("chord:///etc/foo/bar.md", to_string(uri));
}

TEST(chord_uri, builder_without_scheme_without_path) {
  auto builder = uri::builder();
  ASSERT_THROW(builder.build(), chord::exception);
}

TEST(chord_uri, builder_without_scheme) {
  chord::uri::builder uri_builder;
  auto uri = uri_builder.path("/folder/subfolder/./././////../subfolder/bar.ext");
  ASSERT_THROW(uri_builder.build(), chord::exception);
}

TEST(chord_uri, builder_without_path) {
  chord::uri::builder uri_builder;
  ASSERT_THROW(uri_builder.scheme("http").build(), chord::exception);
}
