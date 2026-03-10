#pragma once
#include <cstdint>
#include <string>
#include <fstream>

enum class EvType { I, R, W, B };

struct TraceEvent {
  EvType type;
  uint64_t pc = 0;
  uint64_t addr = 0;
  uint32_t size = 0;
  uint64_t target = 0;
  bool taken = false;
};

class TraceReader {
public:
  explicit TraceReader(const std::string& path);
  bool next(TraceEvent& ev);

private:
  std::ifstream in_;
  std::string line_;
};
