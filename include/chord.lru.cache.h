#pragma once

#include <map>
#include <list>
#include <mutex>
#include <functional>
#include <stdexcept>

namespace chord {

template<typename K, typename V>
class LRUCache {
  public:
    using mutex_t = std::mutex;
    using list_t = std::list<std::pair<K,V>>;
    using list_iterator_t = typename list_t::iterator;
    using list_const_iterator_t = typename list_t::const_iterator;
    using map_t = std::map<K, list_iterator_t>;
    using map_iterator_t = typename map_t::iterator;
    using map_const_iterator_t = typename map_t::const_iterator;
  private:
    mutable mutex_t mtx;
    list_t _list;
    map_t _map;

    std::size_t _capacity;

  public:
    LRUCache(const std::size_t capacity) : _capacity{capacity}  {}
    ~LRUCache() { clear(); }

    void clear() {
      std::lock_guard<mutex_t> lck(mtx);
      _list.clear();
      _map.clear();
    }
    std::size_t capacity() const { return _capacity; }
    std::size_t size() const { return _list.size(); }
    bool empty() const { return _list.empty(); }

    bool exists(const K& k) const {
      std::lock_guard<mutex_t> lck(mtx);
      return _exists(k);
    }

    void touch(const K& k) {
      std::lock_guard<mutex_t> lck(mtx);
      _touch(k);
    }

    V get(const K& k) {
      std::lock_guard<mutex_t> lck(mtx);
      return _get(k);
    }

    V compute_if_absent(const K& k, std::function<V (const K&)> mapping_function) {
      std::lock_guard<mutex_t> lck(mtx);
      if(!_exists(k)) _put(k, mapping_function(k));
      return _get(k);
    }

    bool erase(const K& k) {
      std::lock_guard<mutex_t> lck(mtx);
      return _erase(_map.find(k));
    }

    void put(const K& k, const V& v) {
      std::lock_guard<mutex_t> lck(mtx);
      _put(k, v);
    }

  private:
    bool _erase(const K& k) {
      return _erase(static_cast<map_const_iterator_t>(_map.find(k)));
    }

    void _put(const K& k, const V& v) {
      _erase(static_cast<map_const_iterator_t>(_map.find(k)));

      const auto it = _list.insert(_list.begin(), std::make_pair(k,v));
      _map.insert(std::make_pair(k, it));

      while(_list.size() > _capacity) {
        const auto last = _list.rbegin();
        _erase(last->first);
      }
    }
    
    V _get(const K& k) {
      const auto it = _touch(k);
      if(it == _map.end()) throw std::out_of_range("element not found");
      return it->second->second;
    }

    bool _erase(const map_const_iterator_t it) {
      if(it == _map.end()) return false;
      _list.erase(it->second);
      _map.erase(it);
      return true;
    }

    bool _exists(const K& k) const {
      return _map.find(k) != _map.end();
    }

    map_iterator_t _touch(const K& k) {
      const auto it = _map.find(k);
      if(it != _map.end()) _list.splice(_list.begin(), _list, it->second);
      return it;
    }
};

} // namespace chord
