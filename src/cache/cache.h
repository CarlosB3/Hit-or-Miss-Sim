#pragma once
#include <cstdint>
#include <vector>

struct CacheCfg {
  uint32_t size_bytes = 32 * 1024;
  uint32_t assoc = 4;
  uint32_t line_bytes = 64;
  uint32_t hit_lat = 2;
};

struct CacheStats {
  uint64_t accesses = 0;
  uint64_t hits = 0;
  uint64_t misses = 0;
  uint64_t reads = 0;
  uint64_t writes = 0;
};

struct CacheAccessResult {
  bool hit = false;
  uint32_t cycles = 0; // hit latency; miss penalty handled by caller
};

class Cache {
public:
  explicit Cache(const CacheCfg& cfg);
  CacheAccessResult access(uint64_t addr, bool is_write);
  const CacheStats& stats() const { return stats_; }

private:
  struct Line {
    bool valid = false;
    bool dirty = false;
    uint64_t tag = 0;
    uint64_t last_use = 0;
  };

  CacheCfg cfg_;
  uint32_t num_sets_ = 0;
  uint64_t tick_ = 0;
  std::vector<std::vector<Line>> sets_;
  CacheStats stats_;

  uint32_t set_index(uint64_t addr) const;
  uint64_t tag(uint64_t addr) const;
};
