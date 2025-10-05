# DACO (Delay-Aware COoperative caching framework)

- NOTE: COVERED is a legacy alias of DACO in implementation.

## Table of Content

1. [Overview](#1-overview)
2. [Directories](#2-directories)
3. [Requirements](#3-requirements)
    1. [Configurations](#31-configurations)
    2. [Prerequisites](#32-prerequisites)
    3. [Libraries](#33-libraries)
    4. [Code Compilation](#34-code-compilation)
    5. [Download Traces](#35-download-traces)
4. [Evaluation in Local Testbed](#4-evaluation-in-local-testbed)
    1. [Preprocess Traces](#41-preprocess-traces)
    2. [Dataset Loading](#42-dataset-loading)
    3. [Multi-node Prototype](#43-multi-node-prototype)
    4. [Single-node Prototype](#44-single-node-prototype)
    5. [Single-node Simulation](#45-single-node-simulation)
5. [Evaluation in AliCloud](#5-evaluation-in-alicloud)
6. [Others](#6-others)

## 1. Overview

- See details in [overview.md](docs/readme/overview.md).

## 2. Directories

- See directory structure in [directory.md](docs/readme/directory.md).

## 3. Requirements

### 3.1 Configurations

- Update `config.json`, which will be used by scripts and main programs.
    - Update `physical_machines` based on your own testbed.
    - If you do NOT have sufficient disk space to install third-party libraries into the default directory `lib` under COVERED project, update `library_path` in `config.json` accordingly.
    - **NOTE: config.json in all physical machines MUST be consistent, otherwise the project may NOT work** (e.g., inconsistent is_track_event may cause wrong packet formats and hence clients will timeout due to NO valid repsonses).
<br />

- Run `python3 -m scripts.tools.configure_ssh` in **one of your physical machines** to build passfree SSH connections between each pair of machines in `config.json`.
    - NOTE: **ONLY need to run once** in one of your physical machines; NO need to run in other physical machines.

<!--
- If you do NOT have sufficient disk space to install third-party libraries into the default directory `lib` under COVERED project:
    - Update `lib_dirpath` in `scripts/common.py` accordingly.
    - Run `python3 -m scripts.tools.replace_dir` to update involved files.
    - After all experiments, you can use `python3 -m scripts.tools.restore_dir` to restore involved files.
-->

### 3.2 Prerequisites

- Linux: Ubuntu 20.04 focal (required by cachelib for Ubuntu 18.04 bionic or higher, yet except Ubuntu 22.04 fammy due to unmatched liburing; lower version does NOT work (e.g., Ubuntu 16.04 xenial)).
    - Terminal: bash 5.0.17.
    - LOC counting tool: cloc 1.82.
<br />

- Run `sudo python3 -m scripts.tools.install_prerequisite` in **each physical machine** to install the following dependencies.
    - **NOTE: set `is_update_swapspace_size` as True for local testbed, yet set it as False for AliCloud testbed due to limited disk space.**
        - NOTE: we need to allocate large enough swap memory (e.g., 200G) to fit the requirements of some learning-based baselines.
    - Python: python >= 3.6.9 with the following libraries
        - colorama (0.4.6).
    - Compiler: g++ 9.4.0 for C++-17.
    - CMake: cmake 3.25.2 (required by cachelib for CMake 3.12 or higher).
<br />

- Possible errors when installing prerequisites.
    - For compiler:
        - If with an error of **failed to add apt repo for gcc/g++**:
            - If add-apt-repository is not found, `sudo apt install software-properties-common`
            - If python3 cannot import apt_pkg module, `sudo ln -s /usr/lib/python3/dist-packages/apt_pkg.cpython-36m-x86_64-linux-gnu.so /usr/lib/python3/dist-packages/apt_pkg.so`
            - If python3 cannot import _gi module, `sudo ln -s /usr/lib/python3/dist-packages/gi/_gi.cpython-36m-x86_64-linux-gnu.so /usr/lib/python3/dist-packages/gi/_gi.so`
    - NOTE: re-run `sudo python3 -m scripts.tools.install_prerequisite` after fixing errors.

<!--
- Possible errors when installing prerequisites.
    - For python3:
        - If with an error of **AttributeError: module 'pip.__main__' has no attribute 'main'** after `pip3`:
            - `sudo vim $(which pip3)`, change `from pip import main` into `from pip import __main__`, and change `sys.exit(main())` into `sys.exit(__main__._main())`
        - If with an error of **subprocess.CalledProcessError: Command '('lsb_release', '-a')' returned non-zero exit status 1** or **ModuleNotFoundError: No module named 'lsb_release'**:
            - `sudo vim $(which lsb_release)`, change `#!/usr/bin/python3 -Es` as `#!/usr/bin/python3.6m -Es` (replace `python3.6m` based on your own python3 version before installing python 3.7.5)
-->

### 3.3 Libraries

- Run `sudo python3 -m scripts.tools.install_lib :$LD_LIBRARY_PATH` in **each physical machine** to install the following C/C++ libraries.
    - Boost 1.81.0.
    - CacheLib with CacheBench (commit ID: 7886d6d).
        - You can run `cd lib/Cachelib; ./opt/cachelib/bin/cachebench --json_test_config cachelib/cachebench/test_configs/hit_ratio/cdn/config.json` to try if cachelib/cachebench is installed correctly.
    - LA-Cache (commit ID: 2dcb806).
    - LRU cache (commit ID: de1c4a0).
    - LFU cache and FIFO cache (commit ID: 0f65db1).
    - RocksDB 8.1.1.
    - SMHasher (commit ID: 61a0530).
    - SegCache (commit ID: 0abdfee).
    - WebCacheSim (including GDSF and AdaptSize) (commit ID: 8818442).
    - TommyDS (commit ID: 97ff743).
    - LHD (commit ID: 806ef46).
    - S3FIFO, SIEVE, ARC, TinyLFU, and SLRU (commit ID: 79fde46).
    - GLCache (commit ID: fbb8240).
    - LRB (commit ID: 9e8b442).
    - FrozenHot (commit ID: eabb2b9).
- Run `source ~/.bashrc` as hinted by scripts/install_lib.py to set system variables.
<br />

- Possible errors when installing libraries.
    - For cachelib:
        - If with an unknown error of any sub-library (e.g., libfolly):
            - Use `cd lib/Cachelib; bash contrib/build.sh -v -S` to dump more details without pulling git repos again.
        - If with an error of **redefinition of 'struct extent_hooks_s'** for libfolly:
            - Set the macro `JEMALLOC_VERSION_MAJOR` as 5 in your jemalloc.h (e.g., /usr/local/include/jemalloc/jemalloc.h).
    - If you want to change cmake parameters of libs (e.g., segcache/glcache/lrb), remember to clear cmake caches and generated cmake files (see `scripts/tools/clearlib.sh`).
    - NOTE: re-run `sudo python3 -m scripts.install_lib` after fixing errors.

<!--
- Set `CMAKE_VERBOSE_MAKEFILE:BOOL` as TRUE in lib/CacheLib/build-folly/CMakeCache.txt for more details.
- If with errors like **undefined reference to 'boost::re_detail_106800'** for libfolly, it means that libfolly is compiled with libboost 1.68.0 yet linked with another libboost (e.g., 1.71.0):
    - Use `echo | gcc -xc++ -E -v -` or `echo | gcc -xc -E -v -` to check default include path and link paths. For example:
        - Default include path: `/usr/local/include` (BOOST_VERSION = 106800 in boost/version.hpp).
        - Default link path: `/usr/lib/x86_64_-linux-gnu` (BOOST_VERSION = 107100 in boost/version.hpp).
    - Make the included libboost has the same version as the linked libboost.
        - `sudo mv /usr/local/include/boost /usr/local/include/boost_bak` to rename the unmatched included libboost temporarily.
        - `sudo rm -r lib/CacheLib/build-folly/CMakeFiles/; sudo rm -r lib/CacheLib/build-folly/Makefile` to delete previously compiled files.
- `sudo mv /usr/local/include/boost_bak /usr/local/include/boost` to retrieve the included libboost if changed before.
-->

### 3.4 Code Compilation

- Compile source code in **each physical machine**.
    - `make all`
        - If with an error of **undefined reference to `folly::f14::detail::F14LinkCheck<(folly::f14::detail::F14IntrinsicsMode)2>::check()**:
            - Reason: the macro FOLLY_F14_VECTOR_INTRINSICS_AVAILABLE differs when compiling cachelib (maybe system libboost) and compiling our code (lib/boost-1.81.0).
            - Solution: use boost-1.81.0 to re-compile cachelib.

### 3.5 Download Traces

- Run `python3 -m scripts.tools.download_traces` in **each client machine** to download the following replayed traces:
    - [Wikipedia image and text CDN traces](https://analytics.wikimedia.org/published/datasets/caching/2019/).
<br />

- Generate your own `scripts/tools/autogen_download_tencent_traces.sh` for Tencent photo caching traces in `http://iotta.snia.org/traces/parallel`; then do the followings in **each client machine**:
    - Run `mkdir -p data/tencent/; cd data/tencent/; bash ../../scripts/tools/autogen_download_tencent_traces.sh; cd ../../` to download Tencent photo caching trace files.
    - Run `python3 -m scripts.tools.decompress_tencent_traces` to decompress Tencent photo caching trace files into `data/tencent/dataset1` and `data/tencent/dataset2`.
<br />

<!-- - Run `python3 -m scripts.tools.gen_akamai_traces` in **each client machine** to generate Akamai's CDN traces.
<br /> -->

## 4. Evaluation in Local Testbed

### 4.1 Characterize Traces

- Run `python3 -m scripts.workload.characterize_zipf_traces` in **each client machine** to characterize replayed traces (curvefit by Zipfian distribution for Zipfian constant and key/value size distributions) and dump characteristics files (synced to cloud machine).

<!-- - Run `python3 -m scripts.workload.characterize_zeta_traces` in **each client machine** to characterize replayed traces (curvefit by Zipfian distribution for Zipfian constant and key/value size distributions) and dump characteristics files (synced to cloud machine). -->

<!-- OBSOLETE due to incorrect trace partition
### 4.1 Preprocess Traces

- Run `python3 -m scripts.exps.preprocess_traces` in **each client machine** to preprocess replayed traces (randomly sample 1M requests) and dump dataset & workload files (synced to cloud machine).
-->

### 4.2 Dataset Loading

- Run `python3 -m scripts.exps.load_dataset` in **the cloud machine** to perform loading phase for datasets used in our evaluation.

### 4.3 Multi-node Prototype

- Run the following experiment scripts in **the evaluator machine**:
    - For Exp#1 (performance against existing methods), run `python3 -m scripts.exps.exp_performance_existing`
    - For Exp#2 (performance against extended methods), run `python3 -m scripts.exps.exp_performance_extended`
    - For Exp#3 (performance against different workloads), run `python3 -m scripts.exps.exp_performance_workloads`
    - For Exp#4 (performance on skewness), run `python3 -m scripts.exps.exp_performance_skewness`
    - For Exp#5 (performance on stable evaluation time), run `python3 -m scripts.exps.exp_parameter_stresstest_time`
    - For Exp#6 (performance on cache space), run `python3 -m scripts.exps.exp_parameter_memory`
    - For Exp#7 (performance on dataset size (# of unique objects)), run `python3 -m scripts.exps.exp_parameter_datasetsize`
    - For Exp#8 (perfomrance on latency settings), run `python3 -m scripts.exps.exp_parameter_latency`
    - For Exp#9 (perfomrance on covered settings), run `python3 -m scripts.exps.exp_parameter_covered`

<!-- - For Exp#7 (performance on cache scale (# of cache nodes)), run `python3 -m scripts.exps.exp_parameter_cachescale` -->

### 4.4. Single-node Prototype

- Run `./single_node_prototype` to launch a multi-thread single-node prototype in **a single machine**.
    - NOTE: clients/edges/cloud/evaluator MUST be in the same physical machine in `config.json`.
    - See usage hints by `./single_node_prototype -h`

### 4.5. Single-node Simulation

- Run the following experiment scripts in **a single machine**:
    - NOTE: use `ulimit -a` to check max # of open files -> use `ulimit -n 1048576` if necessary (e.g., get an error of cannot create UDP socket with an errno of 24 when using single_node_simulator).
    - NOTE: the single machine should have large enough memory (e.g., 128GB memory in our testbed) to support large-scale simulation (e.g., 1024 nodes).
    - For Exp#10 (performance on cache scale (# of cache nodes)), run `python3 -m scripts.exps.exp_simulation_cachescale`

## 5. Evaluation in AliCloud

- See details in [alicloud.md](docs/readme/ailcloud.md).

## 6. Others

- Kill all related threads in testbed: `python3 -m scripts.tools.cleanup_testbed`.
<br />

- Re-dump existing results under the given settings: `./total_statistics_loader`
    - See usage hints by `./total_statistics_loader -h`
<br />

- Get LOC statistics.
    - `python3 -m scripts.tools.cloc`
<br />

- Note that we already set correct dataset size (trace_xxx_keycnt) in `config.json` for each replayed trace used in our evaluation.
    - If you want to add another replayed trace, you need to:
        - (i) Provide a corresponding workload wrapper in src/workload/ to load the trace files;
        - (ii) Update relevant modules (e.g., Util, Config, and Cli), relevant scripts (e.g., download_traces.py and proprocess_traces.py), and config.json (leave dataset size as 0 temporarily) to support the trace;
        - (iii) Run `python3 -m scripts.tools.download_traces` in each client machine to download the trace.
        - (iv) Run `python3 -m scripts.tools.preprocess_traces` in each client machine to preprocess traces for dataset size, dump dataset and workload files, and copy dataset file to cloud machine if necessary;
        - (v) Update config.json to set correct dataset size before evaluation.
<br />

- Note that we already set correct relative filepaths in `config.json` for each trace-based workload (replayed trace or Zeta-based reproduced trace) used in our evaluation.
    - If you add a new workload with trace files, you need to:
        - (i) Download and decompress trace files into a sub-folder under `data/`.
        - (ii) Update `config.json` with relative filepaths of the workload (generated by `scripts/tools/walk_filepaths_for_config.py`).
