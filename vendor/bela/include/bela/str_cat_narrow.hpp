// PORT from absl::StrCat
//
// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// -----------------------------------------------------------------------------
// File: str_cat.h
// -----------------------------------------------------------------------------
//
// This package contains functions for efficiently concatenating and appending
// strings: `StrCat()` and `StrAppend()`. Most of the work within these routines
// is actually handled through use of a special AlphaNum type, which was
// designed to be used as a parameter type that efficiently manages conversion
// to strings and avoids copies in the above operations.
//
// Any routine accepting either a string or a number may accept `AlphaNum`.
// The basic idea is that by accepting a `const AlphaNum &` as an argument
// to your function, your callers will automagically convert bools, integers,
// and floating point values to strings for you.
//
// NOTE: Use of `AlphaNum` outside of the //absl/strings package is unsupported
// except for the specific case of function parameters of type `AlphaNum` or
// `const AlphaNum &`. In particular, instantiating `AlphaNum` directly as a
// stack variable is not supported.
//
// Conversion from 8-bit values is not accepted because, if it were, then an
// attempt to pass ':' instead of ":" might result in a 58 ending up in your
// result.
//
// Bools convert to "0" or "1". Pointers to types other than `char *` are not
// valid inputs. No output is generated for null `char *` pointers.
//
// Floating point numbers are formatted with six-digit precision, which is
// the default for "std::cout <<" or printf "%g" (the same as "%.6g").
//
// You can convert to hexadecimal output rather than decimal output using the
// `Hex` type contained here. To do so, pass `Hex(my_int)` as a parameter to
// `StrCat()` or `StrAppend()`. You may specify a minimum hex field width using
// a `PadSpec` enum.
//
// -----------------------------------------------------------------------------
#ifndef BELA_NARROW_STRINGCAT_HPP
#define BELA_NARROW_STRINGCAT_HPP
#include <string>
#include <array>
#include <string_view>
#include <charconv> // C++17
#include "codecvt.hpp"

namespace bela::narrow {
namespace strings_internal {
// AlphaNumBuffer allows a way to pass a string to StrCat without having to do
// memory allocation.  It is simply a pair of a fixed-size character array, and
// a size.  Please don't use outside of absl, yet.
template <size_t max_size> struct AlphaNumBuffer {
  std::array<char, max_size> data;
  size_t size;
};
} // namespace strings_internal

// Enum that specifies the number of significant digits to return in a `Hex` or
// `Dec` conversion and fill character to use. A `kZeroPad2` value, for example,
// would produce hexadecimal strings such as "0a","0f" and a 'kSpacePad5' value
// would produce hexadecimal strings such as "    a","    f".
enum PadSpec : uint8_t {
  kNoPad = 1,
  kZeroPad2,
  kZeroPad3,
  kZeroPad4,
  kZeroPad5,
  kZeroPad6,
  kZeroPad7,
  kZeroPad8,
  kZeroPad9,
  kZeroPad10,
  kZeroPad11,
  kZeroPad12,
  kZeroPad13,
  kZeroPad14,
  kZeroPad15,
  kZeroPad16,
  kZeroPad17,
  kZeroPad18,
  kZeroPad19,
  kZeroPad20,

  kSpacePad2 = kZeroPad2 + 64,
  kSpacePad3,
  kSpacePad4,
  kSpacePad5,
  kSpacePad6,
  kSpacePad7,
  kSpacePad8,
  kSpacePad9,
  kSpacePad10,
  kSpacePad11,
  kSpacePad12,
  kSpacePad13,
  kSpacePad14,
  kSpacePad15,
  kSpacePad16,
  kSpacePad17,
  kSpacePad18,
  kSpacePad19,
  kSpacePad20,
};

// -----------------------------------------------------------------------------
// Hex
// -----------------------------------------------------------------------------
//
// `Hex` stores a set of hexadecimal string conversion parameters for use
// within `AlphaNum` string conversions.
struct Hex {
  uint64_t value;
  uint8_t width;
  char fill;

  template <typename Int>
  explicit Hex(Int v, PadSpec spec = kNoPad,
               typename std::enable_if<sizeof(Int) == 1 && !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint8_t>(v)) {}
  template <typename Int>
  explicit Hex(Int v, PadSpec spec = kNoPad,
               typename std::enable_if<sizeof(Int) == 2 && !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint16_t>(v)) {}
  template <typename Int>
  explicit Hex(Int v, PadSpec spec = kNoPad,
               typename std::enable_if<sizeof(Int) == 4 && !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint32_t>(v)) {}
  template <typename Int>
  explicit Hex(Int v, PadSpec spec = kNoPad,
               typename std::enable_if<sizeof(Int) == 8 && !std::is_pointer<Int>::value>::type * = nullptr)
      : Hex(spec, static_cast<uint64_t>(v)) {}
  template <typename Pointee>
  explicit Hex(Pointee *v, PadSpec spec = kNoPad) : Hex(spec, reinterpret_cast<uintptr_t>(v)) {}

private:
  Hex(PadSpec spec, uint64_t v)
      : value(v), width(spec == kNoPad       ? 1
                        : spec >= kSpacePad2 ? spec - kSpacePad2 + 2
                                             : spec - kZeroPad2 + 2),
        fill(spec >= kSpacePad2 ? ' ' : '0') {}
};

// -----------------------------------------------------------------------------
// Dec
// -----------------------------------------------------------------------------
//
// `Dec` stores a set of decimal string conversion parameters for use
// within `AlphaNum` string conversions.  Dec is slower than the default
// integer conversion, so use it only if you need padding.
struct Dec {
  uint64_t value;
  uint8_t width;
  char fill;
  bool neg;

  template <typename Int>
  explicit Dec(Int v, PadSpec spec = kNoPad, typename std::enable_if<(sizeof(Int) <= 8)>::type * = nullptr)
      : value(v >= 0 ? static_cast<uint64_t>(v) : uint64_t{0} - static_cast<uint64_t>(v)),
        width(spec == kNoPad       ? 1
              : spec >= kSpacePad2 ? spec - kSpacePad2 + 2
                                   : spec - kZeroPad2 + 2),
        fill(spec >= kSpacePad2 ? ' ' : '0'), neg(v < 0) {}
};

constexpr size_t kFastToBufferSize = 32;

class AlphaNum {
public:
  AlphaNum(bool v) : piece_(v ? "true" : "false") {} // TRUE FALSE
  AlphaNum(short x) {
    auto ret = std::to_chars(digits_, digits_ + sizeof(digits_), x);
    piece_ = std::string_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(unsigned short x) {
    auto ret = std::to_chars(digits_, digits_ + sizeof(digits_), x);
    piece_ = std::string_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(int x) {
    auto ret = std::to_chars(digits_, digits_ + sizeof(digits_), x);
    piece_ = std::string_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(unsigned int x) {
    auto ret = std::to_chars(digits_, digits_ + sizeof(digits_), x);
    piece_ = std::string_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(long x) {
    auto ret = std::to_chars(digits_, digits_ + sizeof(digits_), x);
    piece_ = std::string_view(digits_, ret.ptr - digits_);
  }

  AlphaNum(unsigned long x) {
    auto ret = std::to_chars(digits_, digits_ + sizeof(digits_), x);
    piece_ = std::string_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(long long x) {
    auto ret = std::to_chars(digits_, digits_ + sizeof(digits_), x);
    piece_ = std::string_view(digits_, ret.ptr - digits_);
  }
  AlphaNum(unsigned long long x) {
    auto ret = std::to_chars(digits_, digits_ + sizeof(digits_), x);
    piece_ = std::string_view(digits_, ret.ptr - digits_);
  }

  AlphaNum(float f) // NOLINT(runtime/explicit)
  {
    auto l = snprintf(digits_, sizeof(digits_), "%g", f);
    piece_ = std::string_view(digits_, l);
  }
  AlphaNum(double f) // NOLINT(runtime/explicit)
  {
    auto l = snprintf(digits_, sizeof(digits_), "%g", f);
    piece_ = std::string_view(digits_, l);
  }
  // NOLINT(runtime/explicit)
  AlphaNum(Hex hex);
  AlphaNum(Dec dec);

  template <size_t size>
  AlphaNum( // NOLINT(runtime/explicit)
      const strings_internal::AlphaNumBuffer<size> &buf)
      : piece_(&buf.data[0], buf.size) {}

  AlphaNum(const char *cstr) : piece_(cstr) {}
  AlphaNum(std::string_view sv) : piece_(sv) {}
  template <typename Allocator>
  AlphaNum( // NOLINT(runtime/explicit)
      const std::basic_string<char, std::char_traits<char>, Allocator> &str)
      : piece_(str) {}
  AlphaNum(char c) = delete;
  AlphaNum(char32_t ch) : piece_(digits_, bela::encode_into_unchecked(ch, digits_)) {
    static_assert(sizeof(digits_) > bela::kMaxEncodedUTF8Size, "difits_ buffer is too small");
  }
  AlphaNum(const AlphaNum &) = delete;
  AlphaNum &operator=(const AlphaNum &) = delete;
  std::string_view::size_type size() const { return piece_.size(); }
  const char *data() const { return piece_.data(); }
  std::string_view Piece() const { return piece_; }

private:
  std::string_view piece_;
  char digits_[kFastToBufferSize];
};

namespace strings_internal {
// Do not call directly - this is not part of the public API.
std::string CatPieces(std::initializer_list<std::string_view> pieces);
void AppendPieces(std::string *dest, std::initializer_list<std::string_view> pieces);

} // namespace strings_internal

[[nodiscard]] inline std::string StringCat() { return std::string(); }

[[nodiscard]] inline std::string StringCat(const AlphaNum &a) { return std::string(a.data(), a.size()); }

[[nodiscard]] std::string StringCat(const AlphaNum &a, const AlphaNum &b);
[[nodiscard]] std::string StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c);
[[nodiscard]] std::string StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d);

// Support 5 or more arguments
template <typename... AV>
[[nodiscard]] inline std::string StringCat(const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d,
                                           const AlphaNum &e, const AV &...args) {
  return strings_internal::CatPieces(
      {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(), static_cast<const AlphaNum &>(args).Piece()...});
}

// -----------------------------------------------------------------------------
// StrAppend()
// -----------------------------------------------------------------------------
//
// Appends a string or set of strings to an existing string, in a similar
// fashion to `StringCat()`.
//
// WARNING: `StrAppend(&str, a, b, c, ...)` requires that none of the
// a, b, c, parameters be a reference into str. For speed, `StrAppend()` does
// not try to check each of its input arguments to be sure that they are not
// a subset of the string being appended to. That is, while this will work:
//
//   std::string s = "foo";
//   s += s;
//
// This output is undefined:
//
//   std::string s = "foo";
//   StrAppend(&s, s);
//
// This output is undefined as well, since `absl::string_view` does not own its
// data:
//
//   std::string s = "foobar";
//   absl::string_view p = s;
//   StrAppend(&s, p);

inline void StrAppend(std::string *) {}
void StrAppend(std::string *dest, const AlphaNum &a);
void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b);
void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c);
void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d);

// Support 5 or more arguments
template <typename... AV>
inline void StrAppend(std::string *dest, const AlphaNum &a, const AlphaNum &b, const AlphaNum &c, const AlphaNum &d,
                      const AlphaNum &e, const AV &...args) {
  strings_internal::AppendPieces(
      dest, {a.Piece(), b.Piece(), c.Piece(), d.Piece(), e.Piece(), static_cast<const AlphaNum &>(args).Piece()...});
}

} // namespace bela::narrow

#endif
