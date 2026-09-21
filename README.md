# ROPsmith

<div align="center">
<pre>
:::::::..       ...   ::::::::::.                         `::   ::        
;;;;``;;;;   .;;;;;;;. `;;;```.;;;                      ;;,;;   ;;;       
 [[[,/[[['  ,[[     \[[,`]]nnn]]',cc[[[cc. [ccc, ,cccc, =[[[[[[.[[[[cc,,. 
 $$$$$$c    $$$,     $$$ $$$""   $$$____   $$$$$$$$"$$$ $$$$$   $$$"""$$$ 
 888b "88bo,"888,_ _,88P 888o     .     88,888 Y88" 888o88888,  888   "88o
 MMMM   "W"   "YMMMMMP"  YMMMb    "YUMMMMP"MMM  M'  "MMMMMMMMM  MMM    YMM
                                                                          
  <br><b>ROP gadget finder & chain generator</b>
</pre>
</div>

![Lines of code](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/wh0crypt/b1eda9ea61ce8d4bb2c35622c375dafa/raw/ropsmith-cloc.json?cacheSeconds=300)
[![build-dev](https://github.com/wh0crypt/ROPsmith/actions/workflows/build.yml/badge.svg)](https://github.com/wh0crypt/ROPsmith/actions/workflows/build.yml)
[![build-release](https://github.com/wh0crypt/ROPsmith/actions/workflows/release.yml/badge.svg)](https://github.com/wh0crypt/ROPsmith/actions/workflows/release.yml)
[![tests](https://github.com/wh0crypt/ROPsmith/actions/workflows/tests.yml/badge.svg)](https://github.com/wh0crypt/ROPsmith/actions/workflows/tests.yml)
![GitHub release](https://img.shields.io/github/v/release/wh0crypt/ROPsmith)
![License](https://img.shields.io/github/license/wh0crypt/ROPsmith)
![Status](https://img.shields.io/badge/status-active-success)
![Platforms](https://img.shields.io/badge/platform-linux%20%7C%20macos%20%7C%20windows-blue)

## What is ROPsmith?

ROPsmith is a lightweight toolkit focused on discovering ROP gadgets inside binaries (ELF/PE), presenting usable gadgets, and producing starter templates for ROP chains. The goal is educational and practical: help red-teamers, CTF players, and security researchers understand low-level exploit construction while keeping the tool modular and auditable.

> ⚠️ Ethics & usage: Only run ROPsmith against binaries and hosts you own or have explicit permission to test. This project is for research, education, and defensive testing.

## Status

- **Stage**: Work in progress (MVP: ELF `.text` scanner + gadget extraction)
- **Planned features**: Capstone disassembler integration, gadget ranking heuristics, chain templates, Windows PE support, JSON export, interactive CLI

## Installation

You can download pre-compiled binaries from the [releases page](https://github.com/wh0crypt/ropsmith/releases), or compile the project from source.

### Prerequisites

- **CMake** >=3.16
- **C++23 compatible compiler** (GCC 13+, Clang 17+, MSVC 19.36+)
- **ZLIB** development library

```bash
# Debian / Ubuntu
sudo apt update && sudo apt install -y build-essential cmake zlib1g-dev

# Arch Linux
sudo pacman -S base-devel cmake zlib

# macOS
brew install cmake zlib
```

### From Source

1. Clone the repository:

    ```bash
    git clone https://github.com/wh0crypt/ropsmith.git
    cd ropsmith
    ```

2. Configure and build using CMake:

    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ```

    The executable will be located at ./build/ropsmith.

## Testing

The test suite uses **Criterion** and is integrated via **CTest**:

```bash
# Install Criterion
# Ubuntu/Debian: sudo apt install libcriterion-dev
# Arch: sudo pacman -S criterion

# Build with tests enabled (Debug mode enables ASan & UBSan)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure
```

## Usage

- Display help and available options:

    ```bash
    ./build/ropsmith -h
    ```

- Scan a target binary for gadgets:

    ```bash
    ./build/ropsmith /path/to/binary
    ```

## Contributing

Contributions are welcome! Check out [`CONTRIBUTING.md`](.github/CONTRIBUTING.md) for build setup guidelines, code standards, and PR workflows.

## License

This project is licensed under the MIT License - see the [`LICENSE`](LICENSE) file for details.
