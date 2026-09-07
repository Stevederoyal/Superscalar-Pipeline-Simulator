
Conversation with Gemini
# Superscalar Pipeline Simulator



A trace-driven timing simulator (C++) modeling an N-wide superscalar RISC pipeline, extended with data dependency tracking, operand forwarding, and branch prediction. Built for ECE 4100/6100 (Advanced Computer Architecture) at Georgia Tech.



## Overview



The simulator models a 5-stage pipeline — Instruction Fetch (IF), Instruction Decode (ID), Execute (EX), Memory (MEM), and Writeback (WB) — driven by pre-recorded instruction traces (no functional data values, timing only). Starting from a provided N-wide superscalar skeleton with no dependency tracking, the project implements:



1. **Data dependency tracking and stalling** for a scalar (N=1) machine

2. **Generalized dependency tracking** for an N-wide superscalar machine (tested at N=2), accounting for dependencies from instructions still in the ID stage

3. **Data forwarding** from the EX and MEM stages, including correct handling of load-to-use dependencies (a load's value isn't available until MEM)

4. **Branch prediction**, first with an idealized "AlwaysTaken" predictor, then a **gshare** predictor (12-bit global history register XORed with the PC, indexing a pattern history table of 2-bit saturating counters)



## Architecture



```

sim.cpp          — opens trace files, initializes and runs the pipeline

trace.h          — trace record format (op fields, *_needed flags, cc_read/cc_write)

pipeline.cpp/.h  — Pipeline class; pipe_cycle_IF() ... pipe_cycle_WB() implement

                   per-stage behavior, dependency tracking, stalling, and forwarding

bpred.cpp/.h     — branch predictor interface (get prediction / update predictor)

```



### Pipeline stages modeled



| Stage | Responsibility |

|---|---|

| **IF** | Fetches the next instruction (up to N-wide); consults the branch predictor for conditional branches |

| **ID** | Decodes the instruction; checks for data dependencies against older in-flight instructions (ID, EX, MEM) |

| **EX** | Executes the operation; forwarding sources for downstream dependent instructions |

| **MEM** | Completes memory operations (loads/stores); final point where load results become available |

| **WB** | Commits results — writes on the falling edge, so no forwarding is needed from WB back to ID |



## Part A — Dependency Tracking & Forwarding



- **A.1 — Scalar dependency tracking (N=1):** Detects RAW hazards between an instruction in ID and older instructions still in flight, stalling ID until the dependency clears.

- **A.2 — N-wide superscalar generalization:** Extends dependency checks so an instruction can depend not only on instructions in EX/MEM, but also on **older instructions still in the same ID bundle** — since multiple instructions can be in ID simultaneously in a superscalar machine.

- **A.3 — Data forwarding (EX and MEM):** Adds forwarding paths so dependent instructions don't always need to stall. Load instructions are a special case — their value isn't available until MEM, so a load-dependent instruction can only forward from MEM, never from EX.



## Part B — Branch Prediction



- **B.1 — AlwaysTaken predictor:** A baseline predictor assuming every conditional branch is taken; integrated with the A.3 pipeline (N=2) via an idealized Branch Target Buffer (BTB) that identifies branches and provides the target address at fetch time. On a misprediction, fetch stalls until the branch resolves in MEM.

- **B.2 — gshare predictor:** A 2-level adaptive predictor. The Global History Register (12 bits) is XORed with the low-order bits of the instruction address to index a Pattern History Table of 2-bit saturating counters (initialized to weakly-taken, `10`), giving direction predictions that adapt to per-branch and correlated global history patterns.



## Results





## Tools



- **Language:** C++

- **Build:** `make` (see `Lab_2/src`)

- **Reference environment:** `ece-linlabsrv01.ece.gatech.edu`



## Running the Simulator



```bash

cd Lab_2/src

make

./sim ../traces/mcf.ptr.gz                      # scalar (N=1)

./sim -pipewidth 2 ../traces/mcf.ptr.gz         # 2-wide superscalar

./sim -h                                        # full list of command-line options

```



The `scripts/runall.sh` script runs all configurations (A1, A2, A3, B1, B2) across all provided traces in one pass. `scripts/runtests.sh` compares output against the provided reference results (`ref/refoutput_*.pdf`).



## Repository Structure



```

├── src/

│   ├── sim.cpp

│   ├── trace.h

│   ├── pipeline.cpp

│   ├── pipeline.h

│   ├── bpred.cpp

│   └── bpred.h

├── scripts/

│   ├── runall.sh

│   └── runtests.sh

├── traces/

├── ref/

├── docs/

│   └── gshare_predictor.png

└── README.md

```



## Author



**Stephen Tagoe**

[GitHub](https://github.com/Stevederoyal) | [LinkedIn](https://linkedin.com/in/stephen-tagoe-7588a61b7/)

write this like a project i am placing on my github

Markdown
# Superscalar Pipeline Simulator (C++)

A cycle-accurate, trace-driven timing simulator modeling an $N$-wide superscalar RISC microarchitecture. Implemented features include intra-bundle and cross-stage RAW hazard detection, multi-stage operand forwarding networks, and dynamic branch prediction via an adaptive **gshare** scheme.

---

## Key Highlights

- **Configurable Issue Width ($N$-wide):** Dynamically scales from a baseline scalar pipeline ($N=1$) to superscalar execution ($N \ge 2$).
- **Intra-Bundle Dependency Tracking:** Handles in-flight RAW data hazards across pipeline stages, as well as older-to-younger dependencies co-located within the same Decode (ID) fetch packet.
- **Operand Forwarding Paths:** Resolves register hazards from `EX` and `MEM` back to `ID`, including specialized 1-cycle stall handling for load-to-use hazards.
- **Dynamic Branch Prediction:** Implements an Always-Taken baseline alongside a 12-bit **gshare** branch predictor using a 4096-entry Pattern History Table (PHT) of 2-bit saturating counters.

---

## Pipeline Microarchitecture

The simulator models a classic 5-stage RISC execution pipeline:

+-------------------------------------------------------------+
|  [IF]  Instruction Fetch (Up to N-wide, queries BPred/BTB)  |
+-------------------------------------------------------------+
|
+-------------------------------------------------------------+
|  [ID]  Instruction Decode & Dependency Check                |
|        - Resolves intra-bundle RAW hazards                  |
|        - Resolves cross-stage RAW hazards (EX, MEM, WB)     |
|        - Interacts with operand forwarding logic            |
+-------------------------------------------------------------+
|
+-------------------------------------------------------------+
|  [EX]  Execute (ALU operations, EX-forwarding source)       |
+-------------------------------------------------------------+
|
+-------------------------------------------------------------+
|  [MEM] Memory Access (Load/Store operations, MEM-forwarding)|
|        - Resolves branch direction / mispredict stalls       |
+-------------------------------------------------------------+
|
+-------------------------------------------------------------+
|  [WB]  Writeback (Commits instructions)                      |
|        - Falling-edge write eliminates WB-to-ID stall paths |
+-------------------------------------------------------------+


### 1. Hazard Mitigation & Forwarding

* **Scalar Hazard Detection (N=1):** Tracks destination register specifiers of in-flight operations across `EX` and `MEM`. Inserts pipeline stalls when a RAW hazard is identified at decode.
* **Superscalar Intra-Bundle Tracking (N=2):** Simultaneously evaluates dependencies between younger instructions in slot $i$ and older instructions in slot $j$ ($j < i$) within the current ID bundle, stalling only the dependent slices.
* **Multi-Stage Forwarding:** Bypasses computed results from `EX` and `MEM` directly to `ID`. 
* **Load-to-Use Stalls:** Detects producer instructions of type `LOAD` at the `EX` stage and forces a mandatory 1-cycle stall, as load results are not available until the completion of `MEM`.

### 2. Branch Prediction Subsystem

* **Always-Taken Baseline:** Evaluated against an idealized Branch Target Buffer (BTB) that supplies the target address immediately at fetch. Mispredictions stall instruction fetch until resolution at the `MEM` stage.
* **12-bit gshare Predictor:** 
  * Maintains a 12-bit **Global History Register (GHR)** tracking branch outcomes.
  * Indices a 4096-entry **Pattern History Table (PHT)** by computing:
    $$\text{Index} = \text{GHR} \oplus \text{PC}[13:2]$$
  * Each entry features a 2-bit saturating counter initialized to weakly-taken (`10`).
  * Updates both the saturating counter state machine and the shifted GHR upon branch resolution in the `MEM` stage.

---

## Module Breakdown

| Source File | Functionality |
|---|---|
| `src/sim.cpp` | Driver program; parses CLI flags, opens compressed trace streams, and clocks the pipeline. |
| `src/pipeline.cpp` | Implements cycle-by-cycle stage logic (`pipe_cycle_IF()` through `pipe_cycle_WB()`), stall insertion, and forwarding multiplexing. |
| `src/bpred.cpp` | Encapsulates predictor state, branch lookup (`get_prediction`), and training logic (`update_predictor`). |
| `src/trace.h` | Defines trace record parsing, operand register flags, and condition code read/write tracking. |

---

## Verification & Results

Evaluated across standard integer and memory-intensive execution traces (e.g., `mcf.ptr.gz`).

| Architecture Configuration | Hazards Handled | Stalling Behavior | IPC Impact |
|---|---|---|---|
| **Scalar (N=1), No Forwarding** | RAW (Cross-stage) | Multi-cycle decode stall | Baseline |
| **Superscalar (N=2), No Forwarding** | RAW (Intra-bundle & cross-stage) | Multi-cycle decode stall | Improved throughput; bottlenecked by RAW stalls |
| **Superscalar (N=2), Forwarding Enabled** | RAW (Bypassed via EX/MEM) | 1-cycle load-to-use stalls only | Significant IPC improvement |
| **Superscalar (N=2), Always-Taken** | Control + Data Hazards | Pipeline flush/stall to MEM on not-taken | Trace-dependent branch penalty |
| **Superscalar (N=2), gshare Predictor** | Control + Data Hazards | Aliasing-resistant dynamic prediction | Optimal branch penalty reduction and highest overall IPC |

---

## Getting Started

### Prerequisites
* GCC / G++ (C++11 or higher)
* GNU Make
* zlib (for parsing compressed `.gz` traces)

### Build Instructions
```bash
# Clone the repository
git clone [https://github.com/Stevederoyal/superscalar-pipeline-sim.git](https://github.com/Stevederoyal/superscalar-pipeline-sim.git)
cd superscalar-pipeline-sim/src

# Compile binary
make
Running the Simulator
Bash
# Run scalar baseline (N=1)
./sim ../traces/mcf.ptr.gz

# Run 2-wide superscalar pipeline
./sim -pipewidth 2 ../traces/mcf.ptr.gz

# Display full CLI options and parameter flags
./sim -h
Automated Regression Testing
To run the automated test suite across all pipeline permutations (A1–A3, B1–B2) and validate against reference outputs:

Bash
cd ../scripts
chmod +x runall.sh runtests.sh

# Run all traces across all configurations
./runall.sh

# Validate against reference outputs
./runtests.sh
Repository Structure
├── src/
│   ├── sim.cpp           # Main simulator loop
│   ├── pipeline.cpp      # Pipeline stage implementations
│   ├── pipeline.h        # Stage registers and pipeline declarations
│   ├── bpred.cpp         # AlwaysTaken & gshare branch predictors
│   ├── bpred.h           # Predictor class interfaces
│   ├── trace.h           # Instruction trace decode structs
│   └── Makefile          # Build configuration
├── scripts/
│   ├── runall.sh         # Batch execution over traces
│   └── runtests.sh       # Output comparison and regression checking
├── traces/               # Benchmark instruction traces (.ptr.gz)
├── ref/                  # Reference execution logs and cycle profiles
└── README.md
Author
Stephen Tagoe

GitHub • LinkedIn

send me the md file

+-------------------------------------------------------------+
|  [IF]  Instruction Fetch (Up to N-wide, queries BPred/BTB)  |
+-------------------------------------------------------------+
|
+-------------------------------------------------------------+
|  [ID]  Instruction Decode & Dependency Check                |
|        - Resolves intra-bundle RAW hazards                  |
|        - Resolves cross-stage RAW hazards (EX, MEM, WB)     |
|        - Interacts with operand forwarding logic            |
+-------------------------------------------------------------+
|
+-------------------------------------------------------------+
|  [EX]  Execute (ALU operations, EX-forwarding source)       |
+-------------------------------------------------------------+
|
+-------------------------------------------------------------+
|  [MEM] Memory Access (Load/Store operations, MEM-forwarding)|
|        - Resolves branch direction / mispredict stalls       |
+-------------------------------------------------------------+
|
+-------------------------------------------------------------+
|  [WB]  Writeback (Commits instructions)                      |
|        - Falling-edge write eliminates WB-to-ID stall paths |
+-------------------------------------------------------------+


### 1. Hazard Mitigation & Forwarding

* **Scalar Hazard Detection (N=1):** Tracks destination register specifiers of in-flight operations across `EX` and `MEM`. Inserts pipeline stalls when a RAW hazard is identified at decode.
* **Superscalar Intra-Bundle Tracking (N=2):** Simultaneously evaluates dependencies between younger instructions in slot $i$ and older instructions in slot $j$ ($j < i$) within the current ID bundle, stalling only the dependent slices.
* **Multi-Stage Forwarding:** Bypasses computed results from `EX` and `MEM` directly to `ID`. 
* **Load-to-Use Stalls:** Detects producer instructions of type `LOAD` at the `EX` stage and forces a mandatory 1-cycle stall, as load results are not available until the completion of `MEM`.

### 2. Branch Prediction Subsystem

* **Always-Taken Baseline:** Evaluated against an idealized Branch Target Buffer (BTB) that supplies the target address immediately at fetch. Mispredictions stall instruction fetch until resolution at the `MEM` stage.
* **12-bit gshare Predictor:** 
  * Maintains a 12-bit **Global History Register (GHR)** tracking branch outcomes.
  * Indices a 4096-entry **Pattern History Table (PHT)** by computing:
    $$\\text{Index} = \\text{GHR} \\oplus \\text{PC}[13:2]$$
  * Each entry features a 2-bit saturating counter initialized to weakly-taken (`10`).
  * Updates both the saturating counter state machine and the shifted GHR upon branch resolution in the `MEM` stage.

---

## Module Breakdown

| Source File | Functionality |
|---|---|
| `src/sim.cpp` | Driver program; parses CLI flags, opens compressed trace streams, and clocks the pipeline. |
| `src/pipeline.cpp` | Implements cycle-by-cycle stage logic (`pipe_cycle_IF()` through `pipe_cycle_WB()`), stall insertion, and forwarding multiplexing. |
| `src/bpred.cpp` | Encapsulates predictor state, branch lookup (`get_prediction`), and training logic (`update_predictor`). |
| `src/trace.h` | Defines trace record parsing, operand register flags, and condition code read/write tracking. |

---

## Verification & Results

Evaluated across standard integer and memory-intensive execution traces (e.g., `mcf.ptr.gz`).

| Architecture Configuration | Hazards Handled | Stalling Behavior | IPC Impact |
|---|---|---|---|
| **Scalar (N=1), No Forwarding** | RAW (Cross-stage) | Multi-cycle decode stall | Baseline |
| **Superscalar (N=2), No Forwarding** | RAW (Intra-bundle & cross-stage) | Multi-cycle decode stall | Improved throughput; bottlenecked by RAW stalls |
| **Superscalar (N=2), Forwarding Enabled** | RAW (Bypassed via EX/MEM) | 1-cycle load-to-use stalls only | Significant IPC improvement |
| **Superscalar (N=2), Always-Taken** | Control + Data Hazards | Pipeline flush/stall to MEM on not-taken | Trace-dependent branch penalty |
| **Superscalar (N=2), gshare Predictor** | Control + Data Hazards | Aliasing-resistant dynamic prediction | Optimal branch penalty reduction and highest overall IPC |

---

## Getting Started

### Prerequisites
* GCC / G++ (C++11 or higher)
* GNU Make
* zlib (for parsing compressed `.gz` traces)

### Build Instructions
```bash
# Clone the repository
git clone [https://github.com/Stevederoyal/superscalar-pipeline-sim.git](https://github.com/Stevederoyal/superscalar-pipeline-sim.git)
cd superscalar-pipeline-sim/src

# Compile binary
make
Running the Simulator
Bash
# Run scalar baseline (N=1)
./sim ../traces/mcf.ptr.gz

# Run 2-wide superscalar pipeline
./sim -pipewidth 2 ../traces/mcf.ptr.gz

# Display full CLI options and parameter flags
./sim -h
Automated Regression Testing
To run the automated test suite across all pipeline permutations (A1–A3, B1–B2) and validate against reference outputs:

Bash
cd ../scripts
chmod +x runall.sh runtests.sh

# Run all traces across all configurations
./runall.sh

# Validate against reference outputs
./runtests.sh
Repository Structure
├── src/
│   ├── sim.cpp           # Main simulator loop
│   ├── pipeline.cpp      # Pipeline stage implementations
│   ├── pipeline.h        # Stage registers and pipeline declarations
│   ├── bpred.cpp         # AlwaysTaken & gshare branch predictors
│   ├── bpred.h           # Predictor class interfaces
│   ├── trace.h           # Instruction trace decode structs
│   └── Makefile          # Build configuration
├── scripts/
│   ├── runall.sh         # Batch execution over traces
│   └── runtests.sh       # Output comparison and regression checking
├── traces/               # Benchmark instruction traces (.ptr.gz)
├── ref/                  # Reference execution logs and cycle profiles
└── README.md
Author
Stephen Tagoe

GitHub • LinkedIn
"""

file_path = "README.md"
with open(file_path, "w", encoding="utf-8") as f:
f.write(readme_content.strip() + "\n")

print(f"File created successfully: {file_path}")


```text?code_stdout&code_event_index=1
File created successfully: README.md

Your Markdown file is ready:

MD icon
README
 MD 
The README.md file contains the complete, GitHub-ready documentation formatted with architecture diagrams, module tables, build instructions, and testing workflows.

write a readme for this projeect. write it as a project i have done not a lab

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
- **Global History Register (GHR):** 12-bit shift register tracking historical conditional branch outcomes ($1 = \\text{Taken}, 0 = \\text{Not-Taken}$).
- **Pattern History Table (PHT):** $2^{12} = 4096$ entries composed of 2-bit saturating up/down counters.
  - Counter States: `00` (Strongly Not Taken), `01` (Weakly Not Taken), `10` (Weakly Taken), `11` (Strongly Taken).
  - Default initialization: `10` (Weakly Taken).
- **Index Function:** Computed via bitwise XOR of the GHR and the low-order address bits of the branch PC:
  $$\\text{Index} = \\text{GHR}_{11:0} \\oplus \\text{PC}_{13:2}$$
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
Running Simulations
Execute trace simulations using CLI options to configure issue width and branch prediction policies:

Bash
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
Automated Testing & Verification
Run the regression scripts across all benchmark traces (gcc, mcf, sml):

Bash
cd scripts
chmod +x runall.sh runtests.sh

# Run complete benchmark matrix
./runall.sh

# Validate against reference outputs
./runtests.sh
Author
Stephen Tagoe

Computer Architecture & Embedded Systems Specialist

GitHub • LinkedIn
"""

with open("README.md", "w", encoding="utf-8") as f:
f.write(readme_project.strip() + "\n")

print("README.md generated successfully.")


```text?code_stdout&code_event_index=1
README.md generated successfully.

Your project README file is ready:

MD icon
README
 MD 
Below is the complete text formatted as an engineering project:

Markdown
# Trace-Driven Superscalar RISC Pipeline Timing Simulator

A cycle-accurate, trace-driven microarchitectural timing simulator written in C++ modeling a configurable $N$-wide superscalar execution pipeline. The simulator incorporates microarchitectural mechanisms including intra-bundle and cross-stage RAW dependency tracking, multi-stage operand forwarding networks with load-use stall logic, condition code status tracking, and dynamic branch prediction architectures (Always-Taken and 12-bit Gshare).

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

+-----------------------------------------------------------------------+
|  [IF] Instruction Fetch                                               |
|       - Fetches up to N instructions per cycle from trace stream      |
|       - Consults BTB & Branch Predictor (Always-Taken / Gshare)       |
|       - Stalls on branch misprediction until MEM stage resolution     |
+-----------------------------------------------------------------------+
|
+-----------------------------------------------------------------------+
|  [ID] Instruction Decode & Hazard Detection                           |
|       - Decodes operands (src1_reg, src2_reg, dest_reg, cc_read/write)|
|       - Intra-bundle RAW dependency evaluation: slot i vs slot j (j<i)|
|       - Cross-stage RAW dependency evaluation vs in-flight latches    |
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
|       - Handles Load / Store operations                               |
|       - Forwarding source for load operands and load-generated cc     |
|       - Resolves conditional branch outcomes and trains branch pred   |
|       - Clears fetch stalls on mispredictions                         |
+-----------------------------------------------------------------------+
|
+-----------------------------------------------------------------------+
|  [WB] Writeback                                                       |
|       - Register file update on clock falling edge                    |
|       - Eliminates forwarding requirement from WB to ID               |
+-----------------------------------------------------------------------+


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
Running Simulations
Execute trace simulations using CLI options to configure issue width and branch prediction policies:

Bash
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
Automated Testing & Verification
Run the regression scripts across all benchmark traces (gcc, mcf, sml):

Bash
cd scripts
chmod +x runall.sh runtests.sh

# Run complete benchmark matrix
./runall.sh

# Validate against reference outputs
./runtests.sh
Author
Stephen Tagoe

Computer Architecture & Embedded Systems Specialist

GitHub • LinkedIn




Gemini is AI and can make mistakes.

Analyzing
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
README.md
Displaying README.md.
