//
// Created by covariance on 07.05.2020.
//

#include "big_integer.h"

// region inners
namespace big_integer_inner {
  static const std::function<uint32_t(uint32_t, uint32_t)>
    _and = [](uint32_t a, uint32_t b) { return a & b; },
    _or = [](uint32_t a, uint32_t b) { return a | b; },
    _xor = [](uint32_t a, uint32_t b) { return a ^ b; };
  static const big_integer ONE(1), TEN(10);
}

inline size_t big_integer::size() const {
  return data.size();
}

big_integer big_integer::convert(size_t blocks) const {
  big_integer result(*this);
  if (result.negative) {
    ++result;
    for (auto& block : result.data) { block = ~block; }
    result.data.resize(blocks, UINT32_MAX);
  } else {
    result.data.resize(blocks, 0);
  }
  return result;
}

big_integer bitwise(const big_integer& a, const big_integer& b,
                    const std::function<uint32_t(uint32_t, uint32_t)>& op) {
  size_t sz = std::max(a.size(), b.size()) + 1;
  big_integer result, tma = a.convert(sz), tmb = b.convert(sz);
  result.data.resize(sz);
  for (size_t i = 0; i < sz; ++i) {
    result.data[i] = op(tma.data[i], tmb.data[i]);
  }
  result.negative = op(tma.negative, tmb.negative);
  result.normalize();
  if (result.negative) {
    for (auto& block : result.data) {
      block = ~block;
    }
    --result;
  }
  result.normalize();
  return result;
}

inline void big_integer::normalize() {
  while (size() > 1 && data.back() == 0) { data.pop_back(); }
  if (zero()) {
    negative = false;
  }
}

inline bool big_integer::zero() const {
  return (data.size() == 1 && data[0] == 0);
}
// endregion

// region constructors
big_integer::big_integer()
  : data(1)
  , negative(false) {}

big_integer::big_integer(int x)
  : data(1, (uint32_t) std::abs((int64_t) x))
  , negative(x < 0) {}

big_integer::big_integer(uint32_t x)
  : data(1, x)
  , negative(false) {}

big_integer::big_integer(uint64_t x)
  : data(0)
  , negative(false) {
  data.push_back(static_cast<uint32_t>(x & (UINT32_MAX)));
  data.push_back(static_cast<uint32_t>(x >> 32u));
}

big_integer::big_integer(const std::string& s)
  : data(1, 0)
  , negative(false) {
  size_t i = (s.length() > 0 && (s[0] == '-' || s[0] == '+')) ? 0 : -1;
  while (++i < s.length()) { *this = (*this * big_integer_inner::TEN) + (s[i] - '0'); }
  negative = (s[0] == '-');
  normalize();
}
// endregion

// region support functions
void big_integer::swap(big_integer& other) {
  std::swap(data, other.data);
  std::swap(negative, other.negative);
}

std::string to_string(const big_integer& a) {
  if (a == big_integer()) { return "0"; }

  std::string ans;
  big_integer tmp(a), cycle_tmp;

  while (!tmp.zero()) {
    cycle_tmp = tmp / big_integer_inner::TEN;
    ans += char('0' + (tmp - cycle_tmp * big_integer_inner::TEN).data[0]);
    tmp = cycle_tmp;
  }
  if (a.negative) { ans += '-'; }
  reverse(ans.begin(), ans.end());
  return ans;
}
// endregion

// region unary operators
big_integer big_integer::operator+() const {
  return big_integer(*this);
}

big_integer big_integer::operator-() const {
  big_integer tmp(*this);
  tmp.negative = !tmp.negative;
  tmp.normalize();
  return tmp;
}

big_integer big_integer::operator~() const {
  return -(*this) - big_integer_inner::ONE;
}
// endregion

// region (inc/dec)rements
big_integer& big_integer::operator++() {
  return (*this += big_integer_inner::ONE);
}

const big_integer big_integer::operator++(int) {
  const big_integer ret = *this;
  *this += 1;
  return ret;
}

big_integer& big_integer::operator--() {
  return (*this -= big_integer_inner::ONE);
}

const big_integer big_integer::operator--(int) {
  const big_integer ret = *this;
  *this -= 1;
  return ret;
}
// endregion

// region boolean operators
bool operator==(const big_integer& a, const big_integer& b) {
  return (a.negative == b.negative && a.data == b.data);
}

bool operator!=(const big_integer& a, const big_integer& b) {
  return !(a == b);
}

bool operator<(const big_integer& a, const big_integer& b) {
  if (a.negative != b.negative) { return a.negative; }

  if (a.size() != b.size()) {
    return !a.negative ^ (a.size() > b.size());
  } else {
    for (size_t i = a.size(); i != 0; --i) {
      if (a.data[i - 1] != b.data[i - 1]) {
        return !a.negative ^ (a.data[i - 1] > b.data[i - 1]);
      }
    }
    return false;
  }
}

bool operator>(const big_integer& a, const big_integer& b) {
  return (b < a);
}

bool operator<=(const big_integer& a, const big_integer& b) {
  return !(a > b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
  return !(a < b);
}
// endregion

// region usual binary operators
big_integer operator+(const big_integer& a, const big_integer& b) {
  if (a.negative != b.negative) {
    return (a.negative ? b - (-a) : a - (-b));
  }

  size_t size = std::max(a.size(), b.size());
  uint64_t sum, carry = 0;
  big_integer res;
  res.data.resize(size + 1, 0);
  for (size_t i = 0; i != size; ++i) {
    sum = carry;
    if (i < a.size()) { sum += a.data[i]; }
    if (i < b.size()) { sum += b.data[i]; }
    carry = (sum > UINT32_MAX ? 1 : 0);
    res.data[i] = static_cast<uint32_t>(sum & UINT32_MAX);
  }
  res.data[size] = static_cast<uint32_t>(carry);
  res.negative = a.negative;
  res.normalize();
  return res;
}

big_integer operator-(const big_integer& a, const big_integer& b) {
  if (a.negative != b.negative) {
    return (a.negative ? -((-a) + b) : a + (-b));
  }
  if (a.negative) { return (-b) - (-a); }
  if (a < b) { return -(b - a); }

  uint32_t sub, carry = 0;
  big_integer res;
  res.data.resize(a.size());

  for (size_t i = 0; i != a.size(); ++i) {
    if (i >= b.size()) {
      res.data[i] = (a.data[i] >= carry) ? a.data[i] - carry : UINT32_MAX;
      carry = (a.data[i] >= carry) ? 0 : 1;
    } else {
      if (a.data[i] >= b.data[i]) {
        sub = (a.data[i] - b.data[i] >= carry) ? a.data[i] - b.data[i] - carry : UINT32_MAX;
      } else {
        sub = UINT32_MAX - (b.data[i] - a.data[i]) + 1 - carry;
      }
      carry = a.data[i] >= b.data[i] ? ((a.data[i] - b.data[i] >= carry) ? 0 : 1) : 1;
      res.data[i] = sub;
    }
  }
  res.normalize();
  return res;
}

big_integer operator*(const big_integer& a, const big_integer& b) {
  if (a.zero() || b.zero()) { return big_integer(); }

  big_integer res;
  res.data.resize(a.size() + b.size(), 0);
  uint32_t carry;

  for (size_t i = 0; i != a.size(); ++i) {
    carry = 0;
    for (size_t j = 0; j != b.size(); ++j) {
      uint64_t mul = static_cast<uint64_t>(a.data[i]) * static_cast<uint64_t>(b.data[j]) +
                     static_cast<uint64_t>(res.data[i + j]) + static_cast<uint64_t>(carry);
      res.data[i + j] = static_cast<uint32_t>(mul & UINT32_MAX);
      carry = static_cast<uint32_t>(mul >> 32u);
    }
    res.data[i + b.size()] += carry;
  }
  res.negative = a.negative ^ b.negative;
  res.normalize();
  return res;
}

//region Division
#define uint128_t unsigned __int128

void diff(big_integer& a, const big_integer& b, size_t index) {
  size_t start = a.size() - index;
  bool borrow = false;
  union {
    int64_t sgn = 0;
    uint64_t uns;
  } c;
  for (size_t i = 0; i != index; ++i) {
    c.sgn = static_cast<int64_t>(a.data[start + i])
            - static_cast<int64_t>(i < b.size() ? b.data[i] : 0)
            - static_cast<int64_t>(borrow);

    borrow = c.sgn < 0;
    c.uns &= UINT32_MAX;
    a.data[start + i] = static_cast<uint32_t>(c.uns);
  }
}

big_integer operator/(const big_integer& a, const big_integer& b) {
  big_integer divident = a, divisor = b;
  divident.negative = false;
  divisor.negative = false;
  if (divident < divisor) { return 0; }

  big_integer result;
  if (divisor.size() == 1) {
    uint64_t rest = 0, x;
    for (size_t i = 0; i != divident.size(); ++i) {
      x = (rest << 32u) | static_cast<uint64_t>(divident.data[divident.size() - 1 - i]);
      result.data.push_back(static_cast<uint32_t>(x / divisor.data[0]));
      rest = x % divisor.data[0];
    }
    reverse(result.data.begin(), result.data.end());
    result.normalize();

    result.negative = a.negative ^ b.negative;
    return result;
  }

  big_integer tmp_big;
  divident.negative = divisor.negative = false;
  uint32_t tmp_32;

  divident.data.push_back(0);

  size_t m = divisor.size() + 1, n = divident.size();
  result.data.resize(n - m + 1);

  for (size_t i = m - 1, j = result.data.size() - 1; i != n; ++i, --j) {
    uint128_t x = (static_cast<uint128_t>(divident.data[divident.size() - 1]) << 64u) |
                  (static_cast<uint128_t>(divident.data[divident.size() - 2]) << 32u) |
                  (static_cast<uint128_t>(divident.data[divident.size() - 3]));
    uint128_t y = (static_cast<uint128_t>(divisor.data[divisor.size() - 1]) << 32u) |
                  (static_cast<uint128_t>(divisor.data[divisor.size() - 2]));

    tmp_32 = static_cast<uint32_t>(x / y);
    tmp_big = divisor * tmp_32;

    bool flag = true;
    for (size_t k = 1; k <= divident.size(); k++) {
      if (divident.data[divident.size() - k] != (m - k < tmp_big.size() ? tmp_big.data[m - k] : 0)) {
        flag = divident.data[divident.size() - k] > (m - k < tmp_big.size() ? tmp_big.data[m - k] : 0);
        break;
      }
    }

    if (!flag) {
      tmp_32--;
      tmp_big -= divisor;
    }
    result.data[j] = tmp_32;
    diff(divident, tmp_big, m);
    if (!divident.data.back()) { divident.data.pop_back(); }
  }

  result.negative = a.negative ^ b.negative;
  result.normalize();
  return result;
}

#undef uint128_t
//endregion

big_integer operator%(const big_integer& a, const big_integer& b) {
  return a - (a / b) * b;
}
// endregion

// region bitwise binary operators
big_integer operator&(const big_integer& a, const big_integer& b) {
  return bitwise(a, b, big_integer_inner::_and);
}

big_integer operator|(const big_integer& a, const big_integer& b) {
  return bitwise(a, b, big_integer_inner::_or);
}

big_integer operator^(const big_integer& a, const big_integer& b) {
  return bitwise(a, b, big_integer_inner::_xor);
}

big_integer operator<<(const big_integer& a, int b) {
  big_integer result(a);
  size_t block_shift = static_cast<size_t>(b) >> 5u,
    inner_shift = static_cast<size_t>(b) & 31u;
  uint32_t carry = 0, tmp;
  for (uint32_t& i : result.data) {
    tmp = (i >> (32 - inner_shift));
    i = ((i << inner_shift) | carry);
    carry = tmp;
  }
  if (carry > 0) { result.data.push_back(carry); }
  result.data.resize(result.data.size() + block_shift);
  for (size_t i = result.data.size(); i > block_shift; i--) {
    result.data[i - 1] = result.data[i - block_shift - 1];
  }
  std::fill(result.data.begin(), result.data.begin() + block_shift, 0);
  return result;
}

big_integer operator>>(const big_integer& a, int b) {
  big_integer result(a);
  size_t block_shift = static_cast<size_t> (b) >> 5u,
    inner_shift = static_cast<size_t> (b) & 31u;
  uint32_t carry = 0, tmp, offset = (1u << inner_shift) - 1;
  for (size_t i = block_shift; i < result.data.size(); ++i) { result.data[i - block_shift] = result.data[i]; }
  result.data.resize(result.data.size() - block_shift);
  for (auto i = result.data.rbegin(); i != result.data.rend(); ++i) {
    tmp = (*i & offset) << (32 - inner_shift);
    *i = ((*i >> inner_shift) | carry);
    carry = tmp;
  }
  result.normalize();
  if (result.negative) { result--; }
  return result;
}
// endregion

// region derived operators
big_integer& big_integer::operator=(const big_integer& one) {
  big_integer tmp(one);
  swap(tmp);
  return *this;
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
  return *this = *this + rhs;
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
  return *this = *this - rhs;
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
  return *this = *this * rhs;
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
  return *this = *this / rhs;
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
  return *this = *this % rhs;
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
  return *this = *this & rhs;
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
  return *this = *this | rhs;
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
  return *this = *this ^ rhs;
}

big_integer& big_integer::operator<<=(int rhs) {
  return *this = *this << rhs;
}

big_integer& big_integer::operator>>=(int rhs) {
  return *this = *this >> rhs;
}
// endregion

// region io operators
std::ostream& operator<<(std::ostream& s, const big_integer& a) {
  return s << to_string(a);
}

std::istream& operator>>(std::istream& in, big_integer& a) {
  std::string s;
  in >> s;
  a = big_integer(s);
  return in;
}
// endregion