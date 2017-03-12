#pragma once

#include <boost/multiprecision/cpp_int.hpp>

typedef boost::multiprecision::uint256_t uuid_t;

std::string to_string(const uuid_t& uuid);
