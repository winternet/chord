#include <string>
#include <regex>
#include <map>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <experimental/filesystem>

#include "chord.uri.h"
#include "chord.exception.h"

namespace chord {

  using namespace std;

  uri::builder uri::builder::scheme(const string& scheme) { _scheme = scheme; return *this;}
  uri::builder uri::builder::host(const string& host) { _host = host; return *this; }
  uri::builder uri::builder::port(const int port) { _port = port; return *this; }
  uri::builder uri::builder::user(const string& user) { _user = user; return *this; }
  uri::builder uri::builder::password(const string& password) { _password = password; return *this; }
  uri::builder uri::builder::path(const string& path) { _path = path; return *this;}
  uri::builder uri::builder::query(const map<string, string>& query) { _query = query; return *this; }
  uri::builder uri::builder::add_query(const string& key, const string& value) { _query[key] = value; return *this; }
  uri::builder uri::builder::fragment(const string& fragment) { _fragment = fragment; return *this; }
  uri::builder uri::builder::directory(const string& directory) { _directory = directory; return *this; }
  uri::builder uri::builder::filename(const string& filename) { _filename = filename; return *this; }
  uri::builder uri::builder::extension(const string& extension) { _extension = extension; return *this; }

  uri uri::builder::build() {
    uri ret({_user, _password, _host, _port});
    ret.scheme(_scheme);
    ret.path(_path);
    ret.query(_query);
    ret.fragment(_fragment);
    ret.directory(_directory);
    ret.filename(_filename);
    ret.extension(_extension);
    return ret;
  }

  uri::authority::authority(string host, int port)
    :_host{host}, _port{port}
  {}
  uri::authority::authority(string user, string password, string host, int port)
    :_user{user}, _password{password}, _host{host}, _port{port}
  {}
  const string& uri::authority::user() const { return _user; }
  const string& uri::authority::password() const { return _password; }
  const string& uri::authority::host() const { return _host; }
  const int&    uri::authority::port() const { return _port; }

  void uri::authority::user(const string user) { _user = user; }
  void uri::authority::password(const string password) { _password = password; }
  void uri::authority::host(const string host) { _host = host; }
  void uri::authority::port(const int port) { _port = port; }

  uri::uri(const class authority& authority)
    : _authority{authority} {}

  uri::~uri() {}

  string uri::pattern() { 
    return "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)?([^#]*)?(#(.*))?"s;
  }
  regex uri::uri_regex() { 
    return regex{uri::pattern()};
  }

  void uri::scheme(const string scheme) { _scheme = scheme; }
  void uri::authority(const class authority auth) { _authority = auth; }
  void uri::path(const string path) { _path = path; }
  void uri::query(const map<string, string> query) { _query = query; }
  void uri::fragment(const string fragment) { _fragment = fragment; }
  void uri::directory(const string directory) { _directory = directory; }
  void uri::filename(const string filename) { _filename = filename; }
  void uri::extension(const string extension) { _extension = extension; }

  const string& uri::scheme() const { return _scheme; }
  const string& uri::path() const { return _path; }
  map<string, string>& uri::query() { return _query; }
  const string& uri::user() const { return _authority.user(); }
  const string& uri::password() const { return _authority.password(); }
  const string& uri::host() const { return _authority.host(); }
  const int&    uri::port() const { return _authority.port(); }
  const string& uri::fragment() const { return _fragment; }
  const string& uri::directory() const { return _directory; }
  const string& uri::filename() const { return _filename; }
  const string& uri::extension() const { return _extension; }

  string uri::decode(const string& str) {
    ostringstream decoded;
    for(auto it=begin(str); it != end(str); it++) {
      if(*it == '%') {
        decoded << (char)strtol(string(it+1, it+3).c_str(), nullptr, 16);
        it+=2;
      } else {
        decoded << *it;
      }
    }
    return decoded.str();
  }

  string uri::encode(const string& str) {
    ostringstream encoded;
    encoded.fill('0');
    encoded << hex;
    for(auto c:str) {
      // Keep alphanumeric and other accepted characters intact
      if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
        encoded << c;
      } else {
        // Any other characters are percent-encoded
        encoded << uppercase;
        encoded << '%' << setw(2) << int(c);
        encoded << nouppercase;
      }
    }
    return encoded.str();
  }

  uri uri::from(const string& str) {
    namespace fs = std::experimental::filesystem::v1;
    auto reg = uri_regex();
    smatch match;
    if(!regex_match(str, reg)) {
      throw chord::exception("failed to parse uri \'" + str + "\'");
    }
    regex_search(str, match, reg);

    auto uri_builder = uri::builder{};

    auto _scheme = match.str(2);
    auto _authority = match.str(4);
    {
      smatch auth_match;
      auto auth_reg = regex{"^(.*):(.*)@([^:]*)?[:]?([0-9]*)?$"};
      auto has_authority = regex_match(_authority, auth_reg);
      if(has_authority) {
        regex_search(_authority, auth_match, auth_reg);
        uri_builder.user(auth_match.str(1));
        uri_builder.password(auth_match.str(2));
        uri_builder.host(auth_match.str(3));
        if(!auth_match.str(4).empty())
          uri_builder.port(stoi(auth_match.str(4)));
      } else {
        uri_builder.host(_authority);
      }
    }

    auto _path = match.str(5);

    auto _query = match.str(6);
    {
      smatch query_matches;
      auto query_reg = regex{"([^?&]*)=([^?&]*)"};
      auto beg = _query.cbegin();
      while(regex_search(beg, _query.cend(), query_matches, query_reg)) {
        string key = query_matches.str(1);
        string val = query_matches.str(2);
        uri_builder.add_query(key, val);
        beg = query_matches[2].second;
      }
    }
    auto _fragment = match.str(8);

    uri_builder.scheme(_scheme);
    uri_builder.path(_path);
    fs::path p {_path};

    uri_builder.directory(p.parent_path());
    uri_builder.filename(p.filename());
    uri_builder.extension(p.extension());

    uri_builder.fragment(_fragment);
    return uri_builder.build();
  }

}
