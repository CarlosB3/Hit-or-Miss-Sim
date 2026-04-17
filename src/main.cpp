#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "cache.h"
#include "trace.h"

using namespace std;

struct LatCfg {
  uint32_t mem_lat = 120;
};

static void usage() {
  cerr << "Usage:\n"
       << "  ./sim --trace <file> --l1 <size,assoc,line> --lat <l1=HIT,mem=MEM>\n"
       << "Example:\n"
       << "  ./sim --trace stream.trace --l1 32768,4,64 --lat l1=2,mem=120\n";
}

static uint32_t parse_u32(const string& s) {
  size_t idx = 0;
  uint32_t v = static_cast<uint32_t>(stoul(s, &idx, 0));
  (void)idx;
  return v;
}

static CacheCfg parse_l1(const string& s) {
  CacheCfg c;
  size_t a = s.find(',');
  size_t b = (a == string::npos) ? string::npos : s.find(',', a + 1);
  if (a == string::npos || b == string::npos) throw runtime_error("Bad --l1 format");
  c.size_bytes = parse_u32(s.substr(0, a));
  c.assoc = parse_u32(s.substr(a + 1, b - (a + 1)));
  c.line_bytes = parse_u32(s.substr(b + 1));
  return c;
}

static void parse_lat(const string& s, CacheCfg& l1, LatCfg& lat) {
  size_t start = 0;
  while (start < s.size()) {
    size_t comma = s.find(',', start);
    string tok = s.substr(start, comma == string::npos ? string::npos : comma - start);
    size_t eq = tok.find('=');
    if (eq == string::npos) throw runtime_error("Bad --lat token: " + tok);
    string key = tok.substr(0, eq);
    string val = tok.substr(eq + 1);
    if (key == "l1") l1.hit_lat = parse_u32(val);
    else if (key == "mem") lat.mem_lat = parse_u32(val);
    else throw runtime_error("Unknown --lat key: " + key);
    if (comma == string::npos) break;
    start = comma + 1;
  }
}

int main(int argc, char** argv) {
  try {
    string trace_path;
    CacheCfg l1cfg;
    LatCfg lat;

    for (int i = 1; i < argc; i++) {
      string arg = argv[i];
      if (arg == "--trace" && i + 1 < argc) trace_path = argv[++i];
      else if (arg == "--l1" && i + 1 < argc) l1cfg = parse_l1(argv[++i]);
      else if (arg == "--lat" && i + 1 < argc) parse_lat(argv[++i], l1cfg, lat);
      else {
        usage();
        return 2;
      }
    }
    if (trace_path.empty()) {
      usage();
      return 2;
    }

    Cache l1(l1cfg);
    TraceReader tr(trace_path);

    uint64_t inst = 0;
    uint64_t cycles = 0;

    TraceEvent ev;
    while (tr.next(ev)) {
      inst++;
      cycles += 1;

      CacheAccessResult res{};
      bool did_access = false;
      if (ev.type == EvType::I) {
        res = l1.access(ev.pc, AccessType::Ifetch);
        did_access = true;
      } else if (ev.type == EvType::R) {
        res = l1.access(ev.addr, AccessType::Read);
        did_access = true;
      } else if (ev.type == EvType::W) {
        res = l1.access(ev.addr, AccessType::Write);
        did_access = true;
      }

      if (did_access) {
        cycles += res.cycles;
        if (!res.hit) {
          cycles += lat.mem_lat;
          if (res.writeback) {
            cycles += lat.mem_lat;
          }
        }
      }
    }

    const auto& st = l1.stats();
    double miss_rate = (st.accesses == 0) ? 0.0 : static_cast<double>(st.misses) / static_cast<double>(st.accesses);
    double mpki = (inst == 0) ? 0.0 : static_cast<double>(st.misses) / (static_cast<double>(inst) / 1000.0);
    double cpi = (inst == 0) ? 0.0 : static_cast<double>(cycles) / static_cast<double>(inst);

    cout << "=== SIM SUMMARY ===\n";
    cout << "Trace: " << trace_path << "\n";
    cout << "L1: size=" << l1cfg.size_bytes << "B assoc=" << l1cfg.assoc
         << " line=" << l1cfg.line_bytes << "B hit_lat=" << l1cfg.hit_lat << "\n";
    cout << "Mem lat: " << lat.mem_lat << "\n\n";

    cout << "Inst(events): " << inst << "\n";
    cout << "Cycles:       " << cycles << "\n";
    cout << "CPI proxy:    " << cpi << "\n\n";

    cout << "L1 accesses:  " << st.accesses << " (I " << st.ifetches
         << ", R " << st.reads << ", W " << st.writes << ")\n";
    cout << "L1 hits:      " << st.hits << "\n";
    cout << "L1 misses:    " << st.misses << "\n";
    cout << "Writebacks:   " << st.writebacks << "\n";
    cout << "Miss rate:    " << miss_rate << "\n";
    cout << "MPKI:         " << mpki << "\n";
    return 0;
  } catch (const exception& e) {
    cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
