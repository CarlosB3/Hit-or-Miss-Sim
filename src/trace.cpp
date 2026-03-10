#include "trace.h"
#include <sstream>
#include <stdexcept>

TraceReader::TraceReader(const std::string& path) : in_(path) {
  if (!in_) throw std::runtime_error("Failed to open trace: " + path);
}

static uint64_t parse_u64(const std::string& s) {
  size_t idx = 0;
  uint64_t v = std::stoull(s, &idx, 0); // base 0 supports 0x...
  (void)idx;
  return v;
}

bool TraceReader::next(TraceEvent& ev) {
  while (std::getline(in_, line_)) {
    if (line_.empty()) continue;
    std::istringstream iss(line_);
    char t;
    iss >> t;
    if (!iss) continue;

    ev = TraceEvent{};
    if (t == 'I') {
      std::string pc; iss >> pc;
      ev.type = EvType::I; ev.pc = parse_u64(pc);
      return true;
    }
    if (t == 'R' || t == 'W') {
      std::string a; uint32_t sz = 0;
      iss >> a >> sz;
      ev.type = (t == 'R') ? EvType::R : EvType::W;
      ev.addr = parse_u64(a);
      ev.size = sz;
      return true;
    }
    if (t == 'B') {
      std::string pc, tgt; int taken = 0;
      iss >> pc >> tgt >> taken;
      ev.type = EvType::B;
      ev.pc = parse_u64(pc);
      ev.target = parse_u64(tgt);
      ev.taken = (taken != 0);
      return true;
    }
    // unknown: skip
  }
  return false;
}
