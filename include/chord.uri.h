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

    builder port(const int port);

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
    std::string _user, _password, _host;
    int _port;

   public:
    authority(std::string host, int port);

    authority(std::string user, std::string password, std::string host, int port);

    const std::string &user() const;

    const std::string &password() const;

    const std::string &host() const;

    const int &port() const;

    void user(const std::string user);

    void password(const std::string password);

    void host(const std::string host);

    void port(const int port);
  };

 private:
  authority _authority;
  std::map<std::string, std::string> _query;

  std::string _scheme, _fragment;
  chord::path _path;

 public:
  uri(const authority &authority);

  virtual ~uri();

  static std::string pattern();

  static std::regex uri_regex();

  //static chord::path canonical(const chord::path& path);

  void scheme(const std::string scheme);

  void authority(const authority auth);

  void path(const chord::path path);

  void query(const std::map<std::string, std::string> query);

  void fragment(const std::string fragment);

  const std::string &scheme() const;

  const chord::path &path() const;

  std::map<std::string, std::string> &query();

  const std::string &user() const;

  const std::string &password() const;

  const std::string &host() const;

  const int &port() const;

  const std::string &fragment() const;
  //const std::string directory() const;
  //const std::string filename() const;
  //const std::string extension() const;

  static std::string decode(const std::string &str);

  static std::string encode(const std::string &str);

  static uri from(const std::string &str);
};
}
