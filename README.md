# Trace-Driven Superscalar RISC Pipeline Timing Simulator

A cycle-accurate, trace-driven microarchitectural timing simulator written in C++ modeling an configurable $N$-wide superscalar execution pipeline. The simulator incorporates microarchitectural mechanisms including intra-bundle and cross-stage RAW dependency tracking, multi-stage operand forwarding networks with load-use stall logic, condition code status tracking, and dynamic branch prediction architectures (Always-Taken and 12-bit Gshare).

---

## Architectural Highlights

- **Configurable Superscalar Issue Width ($N$-wide):** Scalable architecture supporting single-issue scalar ($N=1$) through multi-issue superscalar configurations ($N \ge 2$).
- **Dual-Domain Hazard Resolution:** 
  - Tracks Read-After-Write (RAW) data hazards across pipeline stages (`EX`, `MEM`, `WB`).
  - Resolves intra-bundle dependencies occurring between co-dispatched instructions within the same Decode (`ID`) packet.
- **Operand Forwarding & Bypass Network:**
  - Bypasses intermediate execution results from `EX` and `MEM` directly to consuming instructions in `ID`.
  - Distinguishes execute latency versus memory access latency, enforcing 1-cycle pipeline stalls for load-to-use hazards.
- **Condition Code Register Tracking:** Dedicated dependency and forwarding paths for status flags (`cc_read` / `cc_write`), handling conditional branch flags produced by ALU and Load operations.
- **Dynamic Branch Prediction Subsystem:**
  - Integrated with an idealized Branch Target Buffer (BTB) identifying branches and targets at fetch time.
  - Direction prediction modeled using a baseline **Always-Taken** scheme and a two-level adaptive **Gshare** predictor ($12$-bit GHR, 4096-entry PHT with 2-bit saturating counters).
  - Accurate misprediction penalty modeling: freezes fetch and flushes pipeline until branch resolution at the `MEM` stage.

---

## Pipeline Microarchitecture

The simulator models a 5-stage classic RISC execution pipeline:

```
+-----------------------------------------------------------------------+
|  [IF] Instruction Fetch                                              |
|       - Fetches up to N instructions per cycle from trace stream      |
|       - Consults BTB & Branch Predictor (Always-Taken / Gshare)       |
|       - Stalls on branch misprediction until MEM stage resolution      |
+-----------------------------------------------------------------------+
                                   |
+-----------------------------------------------------------------------+
|  [ID] Instruction Decode & Hazard Detection                          |
|       - Decodes operands (src1_reg, src2_reg, dest_reg, cc_read/write)|
|       - Intra-bundle RAW dependency evaluation: slot i vs slot j (j<i)|
|       - Cross-stage RAW dependency evaluation vs in-flight latches     |
|       - Operand forwarding multiplexing (EX -> ID, MEM -> ID)         |
|       - Detects load-to-use dependencies and injects EX stalls        |
+-----------------------------------------------------------------------+
                                   |
+-----------------------------------------------------------------------+
|  [EX] Execute                                                         |
|       - ALU operations and effective address generation               |
|       - Acts as forwarding source for ALU results and status flags    |
+-----------------------------------------------------------------------+
                                   |
+-----------------------------------------------------------------------+
|  [MEM] Memory Access                                                  |
|       - Handles Load / Store operations                              |
|       - Forwarding source for load operands and load-generated cc    |
|       - Resolves conditional branch outcomes and trains branch pred   |
|       - Clears fetch stalls on mispredictions                        |
+-----------------------------------------------------------------------+
                                   |
+-----------------------------------------------------------------------+
|  [WB] Writeback                                                       |
|       - Register file update on clock falling edge                    |
|       - Eliminates forwarding requirement from WB to ID               |
+-----------------------------------------------------------------------+
```

---

## Technical Specifications & Implementation Details

### 1. Register & Status Dependency Tracking
- **Trace Fields:** Operands are validated through trace masks (`src1_needed`, `src2_needed`, `dest_needed`).
- **Condition Codes:** Condition flag dependencies (`cc_read`, `cc_write`) are tracked concurrently with general-purpose registers (GPRs) to support flag-dependent conditional branches.
- **Falling-Edge Writeback:** The register file commits on the falling clock edge (written first half of the cycle, read second half), guaranteeing that results in `WB` are safely available in `ID` without bypass circuitry.

### 2. Superscalar Intra-Bundle Checks ($N=2$)
In superscalar mode, younger instructions in slot $i$ of the decode bundle can depend on older instructions in slot $j$ ($j < i$) within the same cycle. The hazard detection logic checks:
1. Cross-stage dependencies against instructions in downstream latches (`EX_latch`, `MEM_latch`).
2. Intra-cycle dependencies where instruction $i$ reads from a register written by instruction $j$ ($j < i$) currently in `ID_latch`. If an intra-bundle hazard is detected, subsequent instructions in the bundle are stalled while older instructions advance.

### 3. Operand Forwarding & Load-to-Use Hazards
Forwarding paths route results from the outputs of `EX` and `MEM` directly back to the inputs of `ID`:
- **ALU Results:** Forwarded from either `EX_latch` or `MEM_latch` with zero stall cycles.
- **Load Hazards:** Since load data is only retrieved at the end of the `MEM` stage, an instruction in `ID` requiring a value produced by a `LOAD` currently in `EX` cannot be forwarded immediately. The hazard detection unit forces a 1-cycle pipeline bubble.
- **Condition Code Forwarding:** Condition flags written by ALU ops forward from `EX`, whereas condition flags produced by Load instructions forward only from `MEM`.

### 4. Gshare Branch Predictor Microarchitecture
- **Global History Register (GHR):** 12-bit shift register tracking historical conditional branch outcomes ($1 = \text{Taken}, 0 = \text{Not-Taken}$).
- **Pattern History Table (PHT):** $2^{12} = 4096$ entries composed of 2-bit saturating up/down counters.
  - Counter States: `00` (Strongly Not Taken), `01` (Weakly Not Taken), `10` (Weakly Taken), `11` (Strongly Taken).
  - Default initialization: `10` (Weakly Taken).
- **Index Function:** Computed via bitwise XOR of the GHR and the low-order address bits of the branch PC:
  $$\text{Index} = \text{GHR}_{11:0} \oplus \text{PC}_{13:2}$$
- **Training & Resolution:** Lookups occur during `IF`. Predictor updates and GHR shifting execute when the branch reaches `MEM`. On a misprediction, instruction fetch stalls until the branch resolves, resuming fetch in the subsequent cycle.

---

## Architectural Configurations Evaluated

| Config ID | Issue Width | Forwarding Paths | Branch Predictor | Hazard Handling Mechanics |
|---|:---:|:---:|:---:|---|
| **A.1** | $N=1$ | None | Perfect | RAW stalls across `EX` and `MEM` stages |
| **A.2** | $N=2$ | None | Perfect | Intra-bundle + cross-stage RAW stalls |
| **A.3** | $N=2$ | `EX` & `MEM` Enabled | Perfect | Bypassing enabled; 1-cycle stall on Load-to-Use |
| **B.1** | $N=2$ | `EX` & `MEM` Enabled | Always-Taken | Fixed taken prediction; fetch stall to `MEM` on mispredict |
| **B.2** | $N=2$ | `EX` & `MEM` Enabled | 12-bit Gshare | Correlated branch history table; dynamic mispredict recovery |

---

## Repository Structure

```
├── src/
│   ├── sim.cpp           # Top-level simulator loop, timing clock, CLI parser
│   ├── pipeline.cpp      # Cycle execution logic (pipe_cycle_IF() through WB())
│   ├── pipeline.h        # Pipeline latches, stage structs, and pipeline classes
│   ├── bpred.cpp         # Branch predictor interfaces and implementations
│   ├── bpred.h           # Predictor class declarations (AlwaysTaken, Gshare)
│   ├── trace.h           # Trace record definition, opcodes, operand masks
│   └── Makefile          # Compiler rules and optimization flags
├── scripts/
│   ├── runall.sh         # Automated batch runner for all traces and configurations
│   └── runtests.sh       # Automated regression test suite against reference outputs
├── traces/               # Compressed benchmark instruction traces (.ptr.gz)
│   ├── gcc.ptr.gz
│   ├── mcf.ptr.gz
│   └── sml.ptr.gz
├── ref/                  # Gold reference simulation logs and cycle counts
└── README.md
```

---

## Getting Started

### Prerequisites
- GCC / G++ (C++11 compatible)
- GNU Make
- `zlib` development headers (for compressed trace decompression)

### Compilation
```bash
cd src
make clean
make
```

### Running Simulations

Execute trace simulations using CLI options to configure issue width and branch prediction policies:

```bash
# Run scalar pipeline (N=1)
./sim ../traces/mcf.ptr.gz

# Run 2-wide superscalar pipeline
./sim -pipewidth 2 ../traces/mcf.ptr.gz

# Run 2-wide superscalar with Always-Taken branch prediction
./sim -pipewidth 2 -bpredpolicy 1 ../traces/mcf.ptr.gz

# Run 2-wide superscalar with 12-bit Gshare branch prediction
./sim -pipewidth 2 -bpredpolicy 2 ../traces/mcf.ptr.gz

# View all CLI flags and simulation parameters
./sim -h
```

### Automated Testing & Verification
Run the regression scripts across all benchmark traces (`gcc`, `mcf`, `sml`):

```bash
cd scripts
chmod +x runall.sh runtests.sh

# Run complete benchmark matrix
./runall.sh

# Validate against reference outputs
./runtests.sh
```

---

## Author

**Stephen Tagoe**  
*Computer Architecture & Embedded Systems Specialist*  
[GitHub](https://github.com/Stevederoyal) • [LinkedIn](https://linkedin.com/in/stephen-tagoe-7588a61b7/)
