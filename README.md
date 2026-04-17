# ECE 622 Cache Simulator

## Overview

This project implements a configurable L1 cache simulator in C++. The simulator models cache behavior using memory access traces and evaluates performance based on different cache configurations and access patterns.

The goal of this project is to analyze how cache parameters such as size, associativity, and line size impact performance metrics like miss rate and CPI.

---

## Features

* Set-associative cache model
* LRU (Least Recently Used) replacement policy
* Configurable cache parameters:

  * Cache size
  * Associativity
  * Cache line size
  * Latency values
* Trace-driven simulation
* Synthetic trace generation:

  * Streaming access
  * Pointer-based access
  * Conflict-heavy patterns
* Performance metrics:

  * Cache hits and misses
  * Miss rate
  * MPKI (Misses per 1000 Instructions)
  * CPI (Cycles Per Instruction) proxy

---

## File Structure

```
main.cpp           # Simulator driver
cache.cpp/h        # Cache implementation
trace.cpp/h        # Trace reader
gen_traces.cpp     # Trace generator
Makefile           # Build configuration
*.trace            # Input trace files
```

---

## Build Instructions

Compile the project using:

```
make
```

This generates:

```
sim   # cache simulator
gen   # trace generator
```

---

## Usage

### Run Simulator

```
./sim --trace <trace_file> --l1 <size,assoc,line> --lat l1=<hit>,mem=<mem>
```

Example:

```
./sim --trace stream.trace --l1 32768,4,64 --lat l1=2,mem=120
```

---

### Generate Traces

#### Streaming Pattern

```
./gen --kind stream --wset_bytes 32768 --passes 50 --out stream.trace
```

#### Pointer-Based Pattern

```
./gen --kind ptr --wset_bytes 32768 --passes 50 --out ptr.trace
```

#### Conflict Pattern

```
./gen --kind conflict --cache_bytes 32768 --lines 16 --reps 2000 --out conflict.trace
```

---

## Output

The simulator prints a summary including:

* Total instructions/events
* Total cycles
* CPI (proxy)
* Cache accesses
* Hits and misses
* Miss rate
* MPKI

---

## Example Output

```
=== SIM SUMMARY ===
Inst(events): 100000
Cycles:       350000
CPI proxy:    3.5

L1 accesses:  50000
L1 hits:      45000
L1 misses:    5000
Miss rate:    0.10
MPKI:         50
```

---

## Concepts Demonstrated

* Cache organization (set-associative)
* Memory hierarchy behavior
* Spatial and temporal locality
* Cache replacement policies (LRU)
* Performance analysis (CPI, miss rate)

---

## Notes

* This simulator models a single-level cache (L1)
* CPI is an approximation based on latency penalties
* Results depend on the memory access pattern of the trace

---

## Author

Carlos Bautista

