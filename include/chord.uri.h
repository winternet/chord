#pragma once

#include <string>
#include <regex>
#include <map>
#include <cctype>

#include "chord.path.h"

namespace chord {

class uri {
 public:
  class builder {
    std::map<std::string, std::string> _query;
    std::string _scheme, _fragment, _user, _password, _host;
    chord::path _path;
    int _port;

   public:
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

  class Authority {
   private:
    std::string _user, _password, _host;
    int _port;

   public:
    Authority(std::string host, int port);

    Authority(std::string user, std::string password, std::string host, int port);

    const std::string &user() const;

    const std::string &password() const;

    const std::string &host() const;

    const int &port() const;

    void user(std::string user);

    void password(std::string password);

    void host(std::string host);

    void port(int port);
  };

 private:
  Authority _authority;
  std::map<std::string, std::string> _query;

  std::string _scheme, _fragment;
  chord::path _path;

 public:
  explicit uri(const Authority &authority);

  virtual ~uri() = default;

  static std::string pattern();

  static std::regex uri_regex();

  void scheme(std::string scheme);

  void authority(Authority auth);

  void path(chord::path path);

  void query(std::map<std::string, std::string> query);

  void fragment(std::string fragment);

  const std::string &scheme() const;

  const chord::path &path() const;

  std::map<std::string, std::string> &query();

  const std::string &user() const;

  const std::string &password() const;

  const std::string &host() const;

  const int &port() const;

  const std::string &fragment() const;

  static std::string decode(const std::string &str);

  static std::string encode(const std::string &str);

  static uri from(const std::string &str);
};
}
