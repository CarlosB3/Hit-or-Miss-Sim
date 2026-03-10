#include "cache.h"
#include <stdexcept>
#include <limits>

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

CacheAccessResult Cache::access(uint64_t addr, bool is_write) {
  stats_.accesses++;
  if (is_write) stats_.writes++; else stats_.reads++;

  tick_++;
  auto& set = sets_[set_index(addr)];
  uint64_t t = tag(addr);

  for (auto& line : set) {
    if (line.valid && line.tag == t) {
      stats_.hits++;
      line.last_use = tick_;
      if (is_write) line.dirty = true;
      return {true, cfg_.hit_lat};
    }
  }

  stats_.misses++;
  int victim = -1;
  uint64_t best = std::numeric_limits<uint64_t>::max();
  for (int i = 0; i < (int)set.size(); i++) {
    if (!set[i].valid) { victim = i; break; }
    if (set[i].last_use < best) { best = set[i].last_use; victim = i; }
  }

  set[victim].valid = true;
  set[victim].tag = t;
  set[victim].dirty = is_write;
  set[victim].last_use = tick_;

  return {false, cfg_.hit_lat};
}
