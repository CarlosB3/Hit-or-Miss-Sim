#include "cache.h"
#include <limits>
#include <stdexcept>

static bool is_pow2(uint32_t x) { return x && ((x & (x - 1u)) == 0u); }

Cache::Cache(const CacheCfg& cfg) : cfg_(cfg) {
  if (cfg_.assoc == 0 || cfg_.line_bytes == 0 || cfg_.size_bytes == 0)
    throw std::runtime_error("Invalid cache config (zeros).");
  if (!is_pow2(cfg_.line_bytes))
    throw std::runtime_error("line_bytes must be power of 2.");
  if (cfg_.size_bytes % (cfg_.assoc * cfg_.line_bytes) != 0)
    throw std::runtime_error("size must be multiple of (assoc*line).");

  num_sets_ = cfg_.size_bytes / (cfg_.assoc * cfg_.line_bytes);
  if (num_sets_ == 0) throw std::runtime_error("num_sets computed as 0.");

  sets_.assign(num_sets_, std::vector<Line>(cfg_.assoc));
}

uint32_t Cache::set_index(uint64_t addr) const {
  uint64_t line_addr = addr / cfg_.line_bytes;
  return static_cast<uint32_t>(line_addr % num_sets_);
}

uint64_t Cache::tag(uint64_t addr) const {
  uint64_t line_addr = addr / cfg_.line_bytes;
  return line_addr / num_sets_;
}

CacheAccessResult Cache::access(uint64_t addr, AccessType type) {
  stats_.accesses++;
  switch (type) {
    case AccessType::Read: stats_.reads++; break;
    case AccessType::Write: stats_.writes++; break;
    case AccessType::Ifetch: stats_.ifetches++; break;
  }

  tick_++;
  auto& set = sets_[set_index(addr)];
  uint64_t t = tag(addr);
  const bool is_write = (type == AccessType::Write);

  for (auto& line : set) {
    if (line.valid && line.tag == t) {
      stats_.hits++;
      line.last_use = tick_;
      if (is_write) line.dirty = true;
      return {true, false, cfg_.hit_lat};
    }
  }

  stats_.misses++;
  int victim = -1;
  uint64_t best = std::numeric_limits<uint64_t>::max();
  for (int i = 0; i < static_cast<int>(set.size()); i++) {
    if (!set[i].valid) {
      victim = i;
      break;
    }
    if (set[i].last_use < best) {
      best = set[i].last_use;
      victim = i;
    }
  }

  bool needs_writeback = set[victim].valid && set[victim].dirty;
  if (needs_writeback) {
    stats_.writebacks++;
  }

  set[victim].valid = true;
  set[victim].tag = t;
  set[victim].dirty = is_write;
  set[victim].last_use = tick_;

  return {false, needs_writeback, cfg_.hit_lat};
}
