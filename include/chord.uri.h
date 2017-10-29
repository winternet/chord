#pragma once
#include <string>
#include <regex>
#include <map>
#include <cctype>
#include <iomanip>
#include <sstream>

#include "chord.exception.h"

namespace chord {
  using namespace std;
  class uri {
    public:
      class builder {
        map<string, string> _query;
        string _scheme, _path, _fragment, _user, _password, _host;
        int _port;

        public:
        builder scheme(const string& scheme) { _scheme = scheme; return *this;}
        builder host(const string& host) { _host = host; return *this; }
        builder port(const int port) { _port = port; return *this; }
        builder user(const string& user) { _user = user; return *this; }
        builder password(const string& password) { _password = password; return *this; }
        builder path(const string& path) { _path = path; return *this;}
        builder query(const map<string, string>& query) { _query = query; return *this; }
        builder add_query(const string& key, const string& value) { _query[key] = value; return *this; }
        builder fragment(const string& fragment) { _fragment = fragment; return *this; }

        uri build() {
          uri ret({_user, _password, _host, _port});
          ret.scheme(_scheme);
          ret.path(_path);
          ret.query(_query);
          ret.fragment(_fragment);
          return ret;
        }
      };

      class authority {
        private:
          string _user, _password, _host;
          int _port;

        public:
          authority(string host, int port)
            :_host{host}, _port{port}
          {}
          authority(string user, string password, string host, int port)
            :_user{user}, _password{password}, _host{host}, _port{port}
          {}
          const string& user() const { return _user; }
          const string& password() const { return _password; }
          const string& host() const { return _host; }
          const int& port() const { return _port; }

          void user(const string user) { _user = user; }
          void password(const string password) { _password = password; }
          void host(const string host) { _host = host; }
          void port(const int port) { _port = port; }
      };

    private:
      authority _authority;
      map<string, string> _query;

      string _scheme, _path, _fragment;

    public:
      uri(const authority& authority):
      _authority{authority} {}

      virtual ~uri() {}

      static string pattern() { 
        static std::string p{"^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)?([^#]*)?(#(.*))?"}; 
        return p; }
      static regex uri_regex() { 
        static regex r{pattern()};
        return r;
      }

      void scheme(const string scheme) { _scheme = scheme; }
      void authority(const authority auth) { _authority = auth; }
      void path(const string path) { _path = path; }
      void query(const map<string, string> query) { _query = query; }
      void fragment(const string fragment) { _fragment = fragment; }

      const string& scheme() const { return _scheme; }
      const string& path() const { return _path; }
      map<string, string>& query() { return _query; }
      const string& user() const { return _authority.user(); }
      const string& password() const { return _authority.password(); }
      const string& host() const { return _authority.host(); }
      const int& port() const { return _authority.port(); }
      const string& fragment() const { return _fragment; }

      static string decode(const string& str) {
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

      static string encode(const string& str) {
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

      static uri from(const string& str) {
        auto reg = uri_regex();
        smatch match;
        if(!regex_match(str, reg)) {
          throw chord::exception("failed to parse uri \'" + str + "\'");
        }
        regex_search(str, match, reg);

        auto uri_builder = uri::builder{};

        auto _scheme = match.str(2);
        auto _authority = match.str(4);
        auto has_authority = false;
        {
          smatch auth_match;
          auto auth_reg = regex{"^(.*):(.*)@([^:]*)?[:]?([0-9]*)?$"};
          has_authority = regex_match(_authority, auth_reg);
          if(has_authority) {
            regex_search(_authority, auth_match, auth_reg);
            uri_builder.user(auth_match.str(1));
            uri_builder.password(auth_match.str(2));
            uri_builder.host(auth_match.str(3));
            if(!auth_match.str(4).empty())
              uri_builder.port(stoi(auth_match.str(4)));
          }
        }

        auto _path = ""s;
        {
          if(has_authority) {
            _path = match.str(5);
          } else {
            _path = match.str(4) + match.str(5);
          }
        }

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
        uri_builder.fragment(_fragment);
        return uri_builder.build();
      }
  };
};
