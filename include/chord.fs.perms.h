#pragma once

namespace chord {
namespace fs {

/// Bitmask type
enum class perms : unsigned {
    none		=  0,
    owner_read	=  0400,
    owner_write	=  0200,
    owner_exec	=  0100,
    owner_all		=  0700,
    group_read	=   040,
    group_write	=   020,
    group_exec	=   010,
    group_all		=   070,
    others_read	=    04,
    others_write	=  02,
    others_exec	=    01,
    others_all	=    07,
    all		=  0777,
    sticky_bit	= 01000,
    unknown		=  0xFFFF,
};

constexpr auto value_of(perms a) noexcept {
  using __utype = typename std::underlying_type<perms>::type;
  return static_cast<__utype>(a);
}
constexpr perms operator&(perms a, perms b) noexcept {
  using __utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<__utype>(a) & static_cast<__utype>(b));
}

constexpr perms operator|(perms a, perms b) noexcept {
  using __utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<__utype>(a) | static_cast<__utype>(b));
}

constexpr perms operator^(perms a, perms b) noexcept {
  using __utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(static_cast<__utype>(a) ^ static_cast<__utype>(b));
}

constexpr perms operator~(perms a) noexcept {
  using __utype = typename std::underlying_type<perms>::type;
  return static_cast<perms>(~static_cast<__utype>(a));
}

inline perms& operator&=(perms& a, perms b) noexcept { return a = a & b; }

inline perms& operator|=(perms& a, perms b) noexcept { return a = a | b; }

inline perms& operator^=(perms& a, perms b) noexcept { return a = a ^ b; }

}  // namespace fs
}  // namespace chord
