#pragma once

namespace chord {
namespace fs {

enum class type : unsigned {
    none		  =  0,
    not_found	=  (unsigned)-1,
    regular	  =  1,
    directory = 2,
    unknown		=  0xFFFF,
};

constexpr auto value_of(type a) noexcept {
  using __utype = typename std::underlying_type<perms>::type;
  return static_cast<__utype>(a);
}

constexpr type operator&(type a, type b) noexcept {
  using __utype = typename std::underlying_type<type>::type;
  return static_cast<type>(static_cast<__utype>(a) & static_cast<__utype>(b));
}

constexpr type operator|(type a, type b) noexcept {
  using __utype = typename std::underlying_type<type>::type;
  return static_cast<type>(static_cast<__utype>(a) | static_cast<__utype>(b));
}

constexpr type operator^(type a, type b) noexcept {
  using __utype = typename std::underlying_type<type>::type;
  return static_cast<type>(static_cast<__utype>(a) ^ static_cast<__utype>(b));
}

constexpr type operator~(type a) noexcept {
  using __utype = typename std::underlying_type<type>::type;
  return static_cast<type>(~static_cast<__utype>(a));
}

inline type& operator&=(type& a, type b) noexcept { return a = a & b; }

inline type& operator|=(type& a, type b) noexcept { return a = a | b; }

inline type& operator^=(type& a, type b) noexcept { return a = a ^ b; }

}  // namespace fs
}  // namespace chord
