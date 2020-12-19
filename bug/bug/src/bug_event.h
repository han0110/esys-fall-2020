#ifndef __BUG_EVENT_H__
#define __BUG_EVENT_H__

#include "mbed.h"
#include "variant"

template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

struct Luminosity {
  uint32_t from;
  uint32_t to;
};

struct XYZ {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

struct Position {
  XYZ from;
  XYZ to;
};

using BugEventKind = variant<Luminosity, Position>;

#define encode_little_endian_4(buf, offset, data)        \
  *(buf + offset * 4 + 3) = (data & 0xff000000UL) >> 24; \
  *(buf + offset * 4 + 2) = (data & 0x00ff0000UL) >> 16; \
  *(buf + offset * 4 + 1) = (data & 0x0000ff00) >> 8;    \
  *(buf + offset * 4) = (data & 0x000000ff);

class BugEventCodec {
 public:
  BugEventCodec(uint32_t secret) : _secret(secret) {}

  uint encode(uint8_t* buf, BugEventKind kind, time_t ts) {
    encode_little_endian_4(buf, 0, ts);
    encode_little_endian_4(buf, 1, _secret);

    auto visitors = overload{
        [&](Luminosity& luminosity) {
          encode_little_endian_4(buf, 2, 0);
          encode_little_endian_4(buf, 3, luminosity.from);
          encode_little_endian_4(buf, 4, luminosity.to);
          return 3;
        },
        [&](Position& position) {
          encode_little_endian_4(buf, 2, 1);
          encode_little_endian_4(buf, 3, position.from.x);
          encode_little_endian_4(buf, 4, position.from.y);
          encode_little_endian_4(buf, 5, position.from.z);
          encode_little_endian_4(buf, 6, position.to.x);
          encode_little_endian_4(buf, 7, position.to.y);
          encode_little_endian_4(buf, 8, position.to.z);
          return 7;
        },
    };

    uint kind_len = visit(visitors, kind);

    return (2 + kind_len) * 4;
  }

 private:
  uint32_t _secret;
};

#endif
