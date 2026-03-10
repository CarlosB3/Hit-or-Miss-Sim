#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdint>

#include "trace.h"
#include "cache/cache.h"

struct LatCfg {
  uint32_t mem_lat = 120;
};

static void usage() {
  std::cerr << "Usage:\n"
            << "  ./sim --trace <file> --l1 <size,assoc,line> --lat <l1=HIT,mem=MEM>\n"
            << "Example:\n"
            << "  ./sim --trace traces/stream.trace --l1 32768,4,64 --lat l1=2,mem=120\n";
}

static uint32_t parse_u32(const std::string& s) {
  size_t idx = 0;
  uint32_t v = (uint32_t)std::stoul(s, &idx, 0);
  (void)idx;
  return v;
}

static CacheCfg parse_l1(const std::string& s) {
  CacheCfg c;
  size_t a = s.find(',');
  size_t b = (a == std::string::npos) ? std::string::npos : s.find(',', a + 1);
  if (a == std::string::npos || b == std::string::npos) throw std::runtime_error("Bad --l1 format");
  c.size_bytes = parse_u32(s.substr(0, a));
  c.assoc = parse_u32(s.substr(a + 1, b - (a + 1)));
  c.line_bytes = parse_u32(s.substr(b + 1));
  return c;
}

static void parse_lat(const std::string& s, CacheCfg& l1, LatCfg& lat) {
  size_t start = 0;
  while (start < s.size()) {
    size_t comma = s.find(',', start);
    std::string tok = s.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
    size_t eq = tok.find('=');
    if (eq == std::string::npos) throw std::runtime_error("Bad --lat token: " + tok);
    std::string key = tok.substr(0, eq);
    std::string val = tok.substr(eq + 1);
    if (key == "l1") l1.hit_lat = parse_u32(val);
    else if (key == "mem") lat.mem_lat = parse_u32(val);
    else throw std::runtime_error("Unknown --lat key: " + key);
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
}

int main(int argc, char** argv) {
  try {
    std::string trace_path;
    CacheCfg l1cfg;
    LatCfg lat;

    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "--trace" && i + 1 < argc) trace_path = argv[++i];
      else if (arg == "--l1" && i + 1 < argc) l1cfg = parse_l1(argv[++i]);
      else if (arg == "--lat" && i + 1 < argc) parse_lat(argv[++i], l1cfg, lat);
      else { usage(); return 2; }
    }
    if (trace_path.empty()) { usage(); return 2; }

    Cache l1(l1cfg);
    TraceReader tr(trace_path);

    uint64_t inst = 0;
    uint64_t cycles = 0;

    TraceEvent ev;
    while (tr.next(ev)) {
      inst++;
      cycles += 1; // base per event

      if (ev.type == EvType::R || ev.type == EvType::W) {
        bool is_write = (ev.type == EvType::W);
        auto res = l1.access(ev.addr, is_write);
        if (res.hit) cycles += res.cycles;
        else cycles += lat.mem_lat;
      }
    }

    const auto& st = l1.stats();
    double miss_rate = (st.accesses == 0) ? 0.0 : (double)st.misses / (double)st.accesses;
    double mpki = (inst == 0) ? 0.0 : (double)st.misses / ((double)inst / 1000.0);
    double cpi = (inst == 0) ? 0.0 : (double)cycles / (double)inst;

    std::cout << "=== SIM SUMMARY ===\n";
    std::cout << "Trace: " << trace_path << "\n";
    std::cout << "L1: size=" << l1cfg.size_bytes << "B assoc=" << l1cfg.assoc
              << " line=" << l1cfg.line_bytes << "B hit_lat=" << l1cfg.hit_lat << "\n";
    std::cout << "Mem lat: " << lat.mem_lat << "\n\n";

    std::cout << "Inst(events): " << inst << "\n";
    std::cout << "Cycles:       " << cycles << "\n";
    std::cout << "CPI proxy:    " << cpi << "\n\n";

    std::cout << "L1 accesses:  " << st.accesses << " (R " << st.reads << ", W " << st.writes << ")\n";
    std::cout << "L1 hits:      " << st.hits << "\n";
    std::cout << "L1 misses:    " << st.misses << "\n";
    std::cout << "Miss rate:    " << miss_rate << "\n";
    std::cout << "MPKI:         " << mpki << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
