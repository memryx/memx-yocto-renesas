# MemryX Benchmark Tool (memx_bench)

A comprehensive benchmarking utility for MemryX accelerator inference performance testing. This tool supports both single-run benchmarks and continuous stress testing with real-time performance monitoring by directly interfacing with the device's drivers.

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
  - [Basic Usage](#basic-usage)
  - [Single Run Mode](#single-run-mode)
  - [Continuous Mode](#continuous-mode)
  - [Command-Line Options](#command-line-options)
- [Examples](#examples)
- [Output](#output)
- [Notes](#notes)

## Features

- **Two Operation Modes:**
  - **Single Run Mode**: Execute inference with a fixed number of frames
  - **Continuous Mode**: Run inference continuously for a specified duration (hours)

- **Performance Metrics:**
  - Frames per second (FPS)
  - Total frames processed
  - Duration tracking (HH:MM:SS)
  - Real-time progress monitoring (continuous mode)

- **Hardware Configuration:**
  - Configurable frequency (MHz)
  - Configurable voltage (mV)

- **Flexible Input:**
  - Single DFP model file
  - Directory scanning for multiple DFP models

### Directory Structure Requirements

When using the `-d` (directory) option, your folder structure **must** follow this format:

```
parent_directory/
├── model1/
│   └── model.dfp
├── model2/
│   └── model.dfp
├── model3/
│   └── model.dfp
└── ...
```

**Key Requirements:**
- Specify the path to the **parent directory** (e.g., `./models`)
- Each subdirectory represents one model
- Each subdirectory **must** contain a file named exactly `model.dfp`
- Subdirectory names will be used in the output for model name identification
- The tool will automatically scan all subdirectories and benchmark each model

**Example:**
```bash
# Directory structure:
# YOLOV8/
# ├── yolov8n/
# │   └── model.dfp
# ├── yolov8s/
# │   └── model.dfp
# └── yolov8m/
#     └── model.dfp

./memx_bench -d ./YOLOV8 -f 100
```

This will benchmark all three models sequentially (yolov8n, yolov8s, yolov8m).


## Requirements

- MemryX SDK/drivers and runtime libraries
- C++11 compatible compiler
- CMake 3.10 or higher
- MemryX accelerator hardware

## Building

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

The compiled binary will be located at `build/memx_bench`.

## Usage

### Basic Usage

```bash
./memx_bench [options]
```

**Note:** Either `-m` (model) or `-d` (directory) must be specified, but not both.

### Single Run Mode

Run inference with a fixed number of frames (default: 100):

```bash
./memx_bench -m /path/to/model.dfp -f 100
```

Or scan a directory for DFP models. Each model found will be run for 500 frames:

```bash
./memx_bench -d /path/to/models -f 500
```

### Continuous Mode

Run inference continuously for a specified duration (in hours):

```bash
./memx_bench -m /path/to/model.dfp -t 2
```

This will run continuous inference for 2 hours with real-time monitoring every 30 seconds.

### Command-Line Options

| Option | Long Form | Description | Default |
|--------|-----------|-------------|---------|
| `-h` | `--help` | Print help message | - |
| `-m` | `--model` | DFP file path (required if `-d` not used) | - |
| `-d` | `--dir` | Directory to scan for DFP files (required if `-m` not used) | - |
| `-g` | `--group` | Accelerator group ID (device ID) | 0 |
| `-f` | `--frames` | Number of frames for single run | 100 |
| `-t` | `--hours` | Duration for continuous inference (hours) | - |
| `-q` | `--freq` | Set frequency in MHz | 200 |
| `-v` | `--voltage` | Set voltage in millivolts (mV) | 670 |

**Important:** `-f` and `-t` are mutually exclusive. Use one or the other.

## Examples

### Example 1: Quick Single Run

Test a model with default settings:

```bash
./memx_bench -m yolov8/model.dfp
```

### Example 2: Single Run with Custom Settings

Run 1000 frames at 400 MHz and 750 mV:

```bash
./memx_bench -m yolov8/model.dfp -f 1000 -q 400 -v 685
```

### Example 3: Continuous Burn-In Test

Run continuous inference for 24 hours:

```bash
./memx_bench -m yolov8/model.dfp -t 24 -q 200 -v 670
```

### Example 4: Batch Testing Multiple Models

Test all models in a directory:

```bash
./memx_bench -d ./YOLOV8 -f 500 -q 300 -v 675
```

## Output

### Single Run Mode Output

```
╔════════════════════════════════════════════════════╗
║                 BENCHMARK COMPLETE                 ║
╠════════════════════════════════════════════════════╣
║  Model: yolov8                                     ║
║  Status: PASS                                      ║
║  Frames: 100                                       ║
║  FPS: 125.34                                       ║
║  Frequency: 200 MHz                                ║
║  Voltage: 670 mV                                   ║
╚════════════════════════════════════════════════════╝
```

### Continuous Mode Output

**During Execution:**
```
╔════════════════════════════════════════════════════╗
║           CONTINUOUS INFERENCE MODE                ║
╠════════════════════════════════════════════════════╣
║  Target Duration: 2 hour(s)                        ║
╚════════════════════════════════════════════════════╝

  [00:00:30] Frames: 3750 | FPS: 125.00
  [00:01:00] Frames: 7500 | FPS: 125.00
  [00:01:30] Frames: 11250 | FPS: 125.00
  ...
```

**Completion:**
```
╔════════════════════════════════════════════════════╗
║                 BENCHMARK COMPLETE                 ║
╠════════════════════════════════════════════════════╣
║  Model: yolov8                                     ║
║  Status: PASS                                      ║
║  Duration: 02:00:00                                ║
║  Total Frames: 900000                              ║
║  Average FPS: 125.00                               ║
║  Frequency: 200 MHz                                ║
║  Voltage: 670 mV                                   ║
╚════════════════════════════════════════════════════╝
```

## Notes

### Frequency and Voltage Limits

- **Frequency Range:** 200 - 1000 MHz
- **Voltage Range:** 670 - 850 mV

When specifying frequency, you must also specify the accompanying voltage in millivolts according to the folowing table:

| Frequency (MHz) | Voltage (mV) |
|-----------------|--------------|
| 200 225 250 275 | 670          |
| 300 325         | 675          |
| 350 375         | 680          |
| 400 425 450     | 685          |
| 475 500 525     | 690          |
| 550 575         | 695          |
| 600             | 700          |
| 625             | 705          |
| 650             | 710          |
| 675             | 720          |
| 700             | 725          |
| 725             | 740          |
| 750             | 745          |
| 775             | 750          |
| 800             | 760          |
| 825             | 770          |
| 850             | 780          |
| 875             | 790          |
| 900             | 800          |
| 925             | 815          |
| 950             | 820          |
| 975             | 835          |
| 1000            | 850          |

### Interrupt Handling

Press `Ctrl+C` to gracefully stop inference at any time. The tool will clean up resources and exit.

### Directory Scanning

When using `-d` (directory mode), the tool will:
1. Scan all subdirectories
2. Look for `model.dfp` files - the DFP MUST be named `model.dfp`!
3. Run benchmarks on each found model sequentially for the specified number of frames or hours

### Performance Tips

- For accurate FPS measurements in single-run mode, use at least 100 frames
- For burn-in testing, continuous mode provides more stable long-term metrics
- Use appropriate cooling for extended continuous runs
- 

### Logging

- Standard output (`stdout`): Normal operation messages and results
- Standard error (`stderr`): Error messages and warnings
- Debug messages are only shown in debug builds

## Troubleshooting

**Problem:** "Error: No dfp file or directory specified"  
**Solution:** Provide either `-m` for a single model or `-d` for a directory with multiple models.

**Problem:** "-f and -t are mutually exclusive"  
**Solution:** Use either `-f` for single run with a number of frames OR `-t` for continuous mode, not both.

**Problem:** "invalid group X"  
**Solution:** Group ID must be match the device ID for the intended module, e.g. `0` for `/sys/memx0`. Check available devices.