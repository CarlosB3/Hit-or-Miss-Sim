#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static void usage() {
  std::cerr
    << "Usage:\n"
    << "  ./gen --kind <stream|ptr|conflict> --out <file> [options]\n\n"
    << "stream/ptr:\n"
    << "  --wset_bytes W --passes P [--line B] [--seed S (ptr only)]\n\n"
    << "conflict:\n"
    << "  --cache_bytes C --lines K --reps R [--line B]\n\n"
    << "Examples:\n"
    << "  ./gen --kind stream --wset_bytes 32768 --passes 50 --out traces/stream_32KB_p50.trace\n"
    << "  ./gen --kind stream --wset_bytes 65536 --passes 30 --out traces/stream_64KB_p30.trace\n"
    << "  ./gen --kind conflict --cache_bytes 32768 --lines 16 --reps 2000 --out traces/conflict.trace\n";
}

static uint64_t parse_u64(const std::string& s) {
  size_t idx = 0;
  uint64_t v = std::stoull(s, &idx, 0);
  (void)idx;
  return v;
}

int main(int argc, char** argv) {
  try {
    std::string kind;
    std::string out_path;

    // stream/ptr params
    uint64_t wset_bytes = 0;
    uint64_t passes = 0;

    // conflict params
    uint64_t cache_bytes = 0;
    uint64_t lines = 0;
    uint64_t reps = 0;

    uint32_t line = 64;
    uint64_t seed = 1;

    for (int i = 1; i < argc; i++) {
      std::string a = argv[i];
      if (a == "--kind" && i + 1 < argc) kind = argv[++i];
      else if (a == "--out" && i + 1 < argc) out_path = argv[++i];
      else if (a == "--wset_bytes" && i + 1 < argc) wset_bytes = parse_u64(argv[++i]);
      else if (a == "--passes" && i + 1 < argc) passes = parse_u64(argv[++i]);
      else if (a == "--cache_bytes" && i + 1 < argc) cache_bytes = parse_u64(argv[++i]);
      else if (a == "--lines" && i + 1 < argc) lines = parse_u64(argv[++i]);
      else if (a == "--reps" && i + 1 < argc) reps = parse_u64(argv[++i]);
      else if (a == "--line" && i + 1 < argc) line = (uint32_t)parse_u64(argv[++i]);
      else if (a == "--seed" && i + 1 < argc) seed = parse_u64(argv[++i]);
      else { usage(); return 2; }
    }

    if (kind.empty() || out_path.empty()) { usage(); return 2; }
    if (line == 0) throw std::runtime_error("line must be > 0");

    std::ofstream out(out_path);
    if (!out) throw std::runtime_error("Failed to open output: " + out_path);

    auto emit_i = [&](uint64_t pc) { out << "I 0x" << std::hex << pc << std::dec << "\n"; };

    if (kind == "stream") {
      if (wset_bytes == 0 || passes == 0) { usage(); return 2; }
      wset_bytes = (wset_bytes / line) * line;
      uint64_t nlines = wset_bytes / line;

      uint64_t pc = 0x1000;
      emit_i(pc);
      uint64_t op = 0;
      for (uint64_t p = 0; p < passes; p++) {
        for (uint64_t i = 0; i < nlines; i++) {
          if ((op & 0x3FFu) == 0) emit_i(pc += 4);
          uint64_t addr = i * (uint64_t)line;
          out << "R 0x" << std::hex << addr << std::dec << " 8\n";
          op++;
        }
      }

      std::cerr << "Wrote " << out_path << " (stream wset=" << wset_bytes
                << ", passes=" << passes << ", line=" << line << ")\n";
      return 0;
    }

    if (kind == "ptr") {
      if (wset_bytes == 0 || passes == 0) { usage(); return 2; }
      wset_bytes = (wset_bytes / line) * line;
      uint64_t nlines = wset_bytes / line;

      std::vector<uint64_t> addrs;
      addrs.reserve((size_t)nlines);
      for (uint64_t i = 0; i < nlines; i++) addrs.push_back(i * (uint64_t)line);

      std::mt19937_64 rng(seed);
      uint64_t pc = 0x2000;
      emit_i(pc);
      uint64_t op = 0;

      for (uint64_t p = 0; p < passes; p++) {
        std::shuffle(addrs.begin(), addrs.end(), rng);
        for (uint64_t i = 0; i < nlines; i++) {
          if ((op & 0x3FFu) == 0) emit_i(pc += 4);
          out << "R 0x" << std::hex << addrs[(size_t)i] << std::dec << " 8\n";
          op++;
        }
      }

      std::cerr << "Wrote " << out_path << " (ptr wset=" << wset_bytes
                << ", passes=" << passes << ", line=" << line << ", seed=" << seed << ")\n";
      return 0;
    }

    if (kind == "conflict") {
      if (cache_bytes == 0 || lines == 0 || reps == 0) { usage(); return 2; }
      if (cache_bytes % line != 0) throw std::runtime_error("--cache_bytes must be multiple of --line");

      std::vector<uint64_t> addrs;
      addrs.reserve((size_t)lines);
      for (uint64_t i = 0; i < lines; i++) addrs.push_back(i * cache_bytes);

      uint64_t pc = 0x3000;
      emit_i(pc);
      uint64_t op = 0;

      for (uint64_t r = 0; r < reps; r++) {
        for (uint64_t i = 0; i < lines; i++) {
          if ((op & 0x3FFu) == 0) emit_i(pc += 4);
          out << "R 0x" << std::hex << addrs[(size_t)i] << std::dec << " 8\n";
          op++;
        }
      }

      std::cerr << "Wrote " << out_path << " (conflict cache_bytes=" << cache_bytes
                << ", lines=" << lines << ", reps=" << reps << ", line=" << line << ")\n";
      return 0;
    }

    throw std::runtime_error("Unknown --kind");
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
