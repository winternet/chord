#pragma once

#include <cctype>
#include <map>
#include <regex>
#include <string>

#include "chord.exception.h"
#include "chord.optional.h"
#include "chord.path.h"

namespace chord {

class uri {
 public:
  class builder {
    std::map<std::string, std::string> _query;
    std::string _scheme, _fragment, _user, _password, _host;
    chord::path _path;
    optional<int> _port;

   public:
    builder() = default;

    builder(const std::string scheme, const chord::path path);

    builder scheme(const std::string &scheme);

    builder host(const std::string &host);

    builder port(int port);

    builder user(const std::string &user);

    builder password(const std::string &password);

    builder path(const std::string &path);

    builder query(const std::map<std::string, std::string> &query);

    builder add_query(const std::string &key, const std::string &value);

    builder fragment(const std::string &fragment);

    uri build();
  };

  class authority {
   private:
    std::string _host;
    std::string _user, _password;
    optional<int> _port;

   public:
    explicit authority(std::string host);
    authority(std::string host, int port);

    authority(std::string user, std::string password, std::string host,
              int port);

    const std::string &user() const;

    const std::string &password() const;

    const std::string &host() const;

    const optional<int> &port() const;

    void user(std::string user);

    void password(std::string password);

    void host(std::string host);

    void port(int port);

    friend std::ostream &operator<<(std::ostream &os, const authority &authority);
  };

 private:
  optional<authority> _authority;
  std::map<std::string, std::string> _query;

  std::string _scheme, _fragment;
  chord::path _path;

 public:
  uri(const std::string& str);

  uri(const std::string scheme, const chord::path path);

  virtual ~uri() = default;

  static std::string pattern();

  static std::regex uri_regex();

  void scheme(std::string scheme);

  void auth(authority auth);

  void path(chord::path path);

  void query(std::map<std::string, std::string> query);

  void fragment(std::string fragment);

  const std::string &scheme() const;

  const optional<uri::authority> &auth() const;

  const chord::path &path() const;

  const std::map<std::string, std::string> &query() const;

  const std::string &fragment() const;

  static std::string decode(const std::string &str);

  static std::string encode(const std::string &str);

  static uri from(const std::string &str);

  operator std::string() const;

  friend std::ostream &operator<<(std::ostream &os, const uri &uri);

  friend std::string to_string(const uri &uri);
};
}  // namespace chord
