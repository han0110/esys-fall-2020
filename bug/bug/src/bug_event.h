#ifndef __BUG_EVENT_H__
#define __BUG_EVENT_H__

#include "mbed.h"
#include "variant"

struct TimeRange {
  time_t started_at;
  time_t ended_at;
  TimeRange() : started_at(0), ended_at(0) {}
};

template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

struct Luminosity {
  uint32_t from;
  uint32_t to;
  Luminosity() : from(0), to(0) {}
};

struct XYZ {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  XYZ() : x(0), y(0), z(0) {}
};

struct Position {
  XYZ from;
  XYZ to;
  Position() : from(XYZ()), to(XYZ()) {}
  Position(XYZ f, XYZ t) : from(f), to(t) {}
};

using BugEventKind = variant<Luminosity*, Position*>;

#define encode_little_endian_4(buf, offset, data)        \
  *(buf + offset * 4 + 3) = (data & 0xff000000UL) >> 24; \
  *(buf + offset * 4 + 2) = (data & 0x00ff0000UL) >> 16; \
  *(buf + offset * 4 + 1) = (data & 0x0000ff00) >> 8;    \
  *(buf + offset * 4) = (data & 0x000000ff);

class BugEventCodec {
 public:
  BugEventCodec(uint32_t secret) : _secret(secret) {}

  uint encode(uint8_t* buf, BugEventKind kind, TimeRange& tr) {
    encode_little_endian_4(buf, 0, tr.started_at);
    encode_little_endian_4(buf, 1, tr.ended_at);
    encode_little_endian_4(buf, 2, _secret);
    uint len = 3;

    auto visitors = overload{
        [&](Luminosity*& luminosity) {
          encode_little_endian_4(buf, len, 0);
          encode_little_endian_4(buf, len + 1, luminosity->from);
          encode_little_endian_4(buf, len + 2, luminosity->to);
          return 3;
        },
        [&](Position*& position) {
          encode_little_endian_4(buf, len, 1);
          encode_little_endian_4(buf, len + 1, position->from.x);
          encode_little_endian_4(buf, len + 2, position->from.y);
          encode_little_endian_4(buf, len + 3, position->from.z);
          encode_little_endian_4(buf, len + 4, position->to.x);
          encode_little_endian_4(buf, len + 5, position->to.y);
          encode_little_endian_4(buf, len + 6, position->to.z);
          return 7;
        },
    };

    len += visit(visitors, kind);

    return len * 4;
  }

 private:
  uint32_t _secret;
};

#endif
