#include <string>
#include <regex>
#include <iomanip>
#include <experimental/filesystem>

#include "chord.path.h"
#include "chord.uri.h"

using namespace std;
namespace fs = std::experimental::filesystem;

namespace chord {

uri::builder::builder(const std::string scheme, const chord::path path)
    : _scheme{scheme}, _path{path} {}

uri::builder uri::builder::scheme(const string &scheme) {
  _scheme = scheme;
  return *this;
}

uri::builder uri::builder::host(const string &host) {
  _host = host;
  return *this;
}

uri::builder uri::builder::port(const int port) {
  _port = port;
  return *this;
}

uri::builder uri::builder::user(const string &user) {
  _user = user;
  return *this;
}

uri::builder uri::builder::password(const string &password) {
  _password = password;
  return *this;
}

uri::builder uri::builder::path(const string &path) {
  _path = chord::path{path};
  return *this;
}

uri::builder uri::builder::query(const map<string, string> &query) {
  _query = query;
  return *this;
}

uri::builder uri::builder::add_query(const string &key, const string &value) {
  _query[key] = value;
  return *this;
}

uri::builder uri::builder::fragment(const string &fragment) {
  _fragment = fragment;
  return *this;
}

uri uri::builder::build() {
  if(_scheme.empty() || _path.empty()) {
    throw chord::exception{"Neither scheme nor path must be empty!"};
  }
  uri ret{_scheme, _path};

  //--- authority
  if(!_host.empty()) {
    authority auth{_host};
    auth.user(_user);
    auth.password(_password);
    if(_port) auth.port(_port.value());

    ret.auth(auth);
  }

  ret.query(_query);
  ret.fragment(_fragment);
  return ret;
}

uri::authority::authority(string host)
    : _host{host} {}

uri::authority::authority(string host, int port)
    : _host{host}, _port{port} {}

uri::authority::authority(string user, string password, string host, int port)
    : _host{host}, _user{user}, _password{password}, _port{port} {}

const std::string &uri::authority::user() const { return _user; }

const std::string &uri::authority::password() const { return _password; }

const std::string &uri::authority::host() const { return _host; }

const std::experimental::optional<int> &uri::authority::port() const { return _port; }

void uri::authority::user(const string user) { _user = user; }

void uri::authority::password(const string password) { _password = password; }

void uri::authority::host(const string host) { _host = host; }

void uri::authority::port(const int port) { _port = port; }

ostream &operator<<(std::ostream &os, const uri::authority &authority) {
  if(!authority.user().empty()) {
    os << authority.user();
    if(!authority.password().empty()) {
      os << ':' << authority.password();
    }
    os << '@';
  }
  os << authority.host();
  if(authority.port())
    os << ':'  << to_string(authority.port().value());

  return os;
}

uri::uri(const string& str) {
  *this = uri::from(str);
}

uri::uri(const string scheme, const chord::path path)
: _scheme{scheme}, _path{path} {}

string uri::pattern() {
  return "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)?([^#]*)?(#(.*))?"s;
}

regex uri::uri_regex() {
  return regex{uri::pattern()};
}

void uri::scheme(const string scheme) { _scheme = scheme; }

void uri::auth(const class authority auth) { _authority = auth; }

void uri::path(const chord::path path) { _path = path; }

void uri::query(const map<string, string> query) { _query = query; }

void uri::fragment(const string fragment) { _fragment = fragment; }

const string &uri::scheme() const { return _scheme; }

const std::experimental::optional<uri::authority> &uri::auth() const { return _authority; }

const chord::path &uri::path() const { return _path; }

const map<string, string> &uri::query() const { return _query; }

const string &uri::fragment() const { return _fragment; }

string uri::decode(const string &str) {
  ostringstream decoded;
  for (auto it = begin(str); it!=end(str); it++) {
    if (*it=='%') {
      decoded << (char) strtol(string(it + 1, it + 3).c_str(), nullptr, 16);
      it += 2;
    } else {
      decoded << *it;
    }
  }
  return decoded.str();
}

string uri::encode(const string &str) {
  ostringstream encoded;
  encoded.fill('0');
  encoded << hex;
  for (auto c:str) {
    // Keep alphanumeric and other accepted characters intact
    if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~') {
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

uri uri::from(const string &str) {
  auto reg = uri_regex();
  smatch match;
  if (!regex_match(str, reg)) {
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
    if (has_authority) {
      regex_search(_authority, auth_match, auth_reg);
      uri_builder.user(auth_match.str(1));
      uri_builder.password(auth_match.str(2));
      uri_builder.host(auth_match.str(3));
      if (!auth_match.str(4).empty())
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
    while (regex_search(beg, _query.cend(), query_matches, query_reg)) {
      string key = query_matches.str(1);
      string val = query_matches.str(2);
      uri_builder.add_query(key, val);
      beg = query_matches[2].second;
    }
  }
  auto _fragment = match.str(8);

  uri_builder.scheme(_scheme);

  uri_builder.path(_path);

  uri_builder.fragment(_fragment);
  return uri_builder.build();
}

ostream &operator<<(ostream &os, const uri &uri) {
  os << to_string(uri);
  return os;
}

uri::operator std::string() const {
  return to_string(*this);
}

std::string to_string(const uri &uri) {
  stringstream ss;
  //--- important scheme etc.
  ss << uri.scheme() << ':';
  if(uri.auth()) ss << "//" << uri.auth().value();
  ss << uri.path();

  //--- query param handling
  const auto &query = uri.query();
  if(!query.empty()) ss << '?';

  for(auto it = begin(query); it != end(query); it++ ) {
    ss << it->first << "=" << it->second;
    if(next(it) != end(query)) ss << '&';
  }

  //--- fragment
  if(!uri.fragment().empty()) ss << '#' << uri.fragment();
  return ss.str();
}

}
