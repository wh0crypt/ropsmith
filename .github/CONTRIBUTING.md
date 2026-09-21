# Contributing to ROPsmith

First off, thank you for considering contributing to **ROPsmith**! Your input helps improve and refine this project for everyone in the security and reverse engineering community.

## How to Contribute

### 1. Fork & Clone

```bash
git clone https://github.com/<your-username>/ropsmith.git
cd ropsmith
```

### 2. Create a Feature Branch

```bash
git checkout -b feat/your-feature-name
```

### 3. Code Standards

* Write **clean, modern C++ (C++23)** code.
* Follow the existing **project structure** and naming conventions.
* Prefer zero-copy non-owning views (`std::span`, `std::string_view`) over unnecessary allocations.
* Handle errors idiomatically using `std::expected` / `core::Result` instead of raw exceptions where appropriate.
* Ensure code is **portable** across major platforms (Linux, Windows, and macOS).
* Document complex logic, especially around binary offsets, disassembly, and gadget chain heuristics.

### 4. Testing

* Run all existing tests before submitting a PR.
* Add new tests for any added parser logic, gadget scanners, or file format handlers.
* Verify that your code does **not break existing gadget detection logic**.

The test suite uses [Criterion](https://github.com/Snaipe/Criterion) and is integrated with **CMake** / **CTest**.

#### Prerequisites

Make sure Criterion and ZLIB development libraries are installed:

```bash
# Debian / Ubuntu
sudo apt install libcriterion-dev zlib1g-dev

# Arch Linux
sudo pacman -S criterion zlib

# macOS
brew install zlib
```

#### Building the Tests

By default, tests are enabled (`BUILD_TESTING=ON`). Configure the project in `Debug` mode to enable runtime sanitizers (ASan, UBSan, LeakSanitizer):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
```

#### Running the Tests

You can run the tests using **CTest**:

```bash
ctest --test-dir build --output-on-failure
```

Or execute the test runner binary directly for full Criterion output (supports filtering and verbose logs):

```bash
# run all tests
./build/test_binary --verbose

# run a specific suite or test
./build/test_binary --suite TestBinary
./build/test_binary --filter "TestBinary/creates_empty_bin"
```

### 5. Commit Guidelines

Use clear, concise commit messages using the imperative:

```text
feat: add new ROP chain generator logic
fix: correct bounds check in section header parser
rev: migrate scanner to std::span
test: add test cases for corrupted ELF headers
ci: fix dependency error in test workflow
```

Before committing, make sure all the files are formatted correctly using `clang-format`.

> **Note:** We use `pre-commit` hooks to enforce code style. Please ensure you have it installed and configured.

Set up the git hook scripts:

```bash
pre-commit install
```

To manually run the pre-commit checks on all tracked files:

```bash
pre-commit run --all-files
```

### 6. Pull Request (PR)

* Open a PR to the `main` branch.
* Include a **detailed description** of what your changes do and how they were tested.
* Reference any related issues using GitHub keywords (e.g., `Fixes #12`).

Once reviewed and CI passes, your PR will be merged.

## Adding New Features

When developing new features, please ensure they align with the overall goals of ROPsmith: modularity, educational value, zero-copy performance, and practical utility for ROP gadget discovery and chain generation.

## Adding New Tests

All test source files live in the `tests/` directory and are discovered automatically by CMake (`file(GLOB_RECURSE TEST_SOURCES ...)`).

* **In-Memory Fixtures**: Whenever possible, avoid relying on pre-compiled binary files on disk. Prefer constructing raw byte buffers (`std::array<uint8_t, N>`) or minimal ELF structures directly in C++ code.
* **Temporary Files**: If your test must test filesystem paths, use isolated temporary fixtures (e.g., creating temporary files under `std::filesystem::temp_directory_path()` with unique names) and clean them up automatically using RAII.
* `Deterministic Behavior`: Ensure tests run independently without race conditions, as Criterion executes test cases concurrently using worker processes.

## Dev Requirements

* **CMake** >= 3.16
* **C++23 compliant compiler** (GCC 13+, Clang 17+, MSVC 19.36+)
* **ZLIB** (`zlib1g-dev` / `zlib`)
* **Criterion** (`libcriterion-dev` / `criterion`) for running tests
* **Git** for version control
* `pre-commit` & `clang-format` for style checks

## Security Contributions

If you find a vulnerability, parser bug, or crash hazard in ROPsmith (e.g., out-of-bounds reads during binary parsing):

1. **Open an issue** on GitHub using the [Security Issue / Bug Report template](https://github.com/wh0crypt/ropsmith/issues).
2. Include detailed steps to reproduce the issue, your environment details (OS, architecture, compiler), and a minimal PoC binary or byte sequence if possible.
3. Add any relevant sanitiser logs (ASan, UBSan) or stack traces to help diagnose the problem quickly.

We welcome reports that help make ROPsmith more robust and secure!

---

### Contact

For contribution-related questions or vulnerability reports:

* **Maintainer:** [wh0crypt](mailto:wh0crypt@proton.me)

Thank you for helping make **ROPsmith** better and safer for everyone!
