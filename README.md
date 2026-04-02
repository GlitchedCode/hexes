# Hexes

A C++20 project using FTXUI for terminal UI, Sol2 for Lua scripting, Glaze for JSON serialization, and yaml-cpp for YAML serialization.

## Prerequisites

- CMake 3.21 or higher
- C++20 compatible compiler (GCC 10+, Clang 12+, or MSVC 2019+)
- Lua 5.4 development libraries
- Internet connection (for initial dependency download via CPM)

### Installing Lua 5.4 on Linux

**Debian/Ubuntu:**

```bash
sudo apt-get install lua5.4 liblua5.4-dev
```

**Arch Linux:**

```bash
sudo pacman -S lua54
```

**Fedora:**

```bash
sudo dnf install lua lua-devel
```

## Building the Project

### Basic Build

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .
```

### Build with Specific Generator

```bash
# Using Ninja (faster builds)
cmake -G Ninja -B build
cmake --build build

# Using Unix Makefiles
cmake -G "Unix Makefiles" -B build
cmake --build build
```

### Build Types

```bash
# Debug build (default)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Release with debug info
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

### Parallel Builds

Speed up compilation by using multiple cores:

```bash
cmake --build build -j$(nproc)
```

### CPM Source Cache (Optional)

To avoid re-downloading dependencies on clean builds, set the CPM_SOURCE_CACHE environment variable:

```bash
export CPM_SOURCE_CACHE=$HOME/.cache/CPM
cmake -B build
cmake --build build
```

## Project Structure

```
hexes/
├── app/              # Main TUI application
├── examples/         # Example programs
│   └── basic/        # Basic usage example
├── include/          # Public headers
├── src/              # Library source code
│   ├── fs/           # Filesystem utilities
│   ├── lua/          # Lua integration
│   └── serialization.cpp
├── scripts/          # Lua scripts (copied to build output)
└── CMakeLists.txt    # Build configuration
```

## Build Targets

- **hexes_lib**: Core library (`libhexes_lib.a`)
- **hexes_app**: Main TUI application (executable)
- **example_basic**: Basic usage example (executable)

### Building Specific Targets

```bash
# Build only the library
cmake --build build --target hexes_lib

# Build only the main application
cmake --build build --target hexes_app

# Build only the example
cmake --build build --target example_basic
```

## Running

After building, executables are located in the build directory:

```bash
# Run main application
./build/hexes_app

# Run basic example
./build/example_basic
```

The Lua scripts from `scripts/` are automatically copied to the executable directories during build.

## Dependencies

All dependencies are automatically downloaded and built via CPM (CMake Package Manager):

- **FTXUI** v6.1.9 - Terminal UI library
- **Sol2** v3.5.0 - Lua binding library
- **Glaze** v2.9.5 - JSON serialization (C++20)
- **yaml-cpp** v0.8.0 - YAML serialization

## Known Build Issues and Solutions

### Issue 1: Glaze In-Source Build Error

**Error Message:**

```
CMake Error: In-source builds are not supported.
```

**Cause:** CMake cache files (`CMakeCache.txt`, `CMakeFiles/`) exist in the project root directory.

**Solution:**

```bash
# Clean the root directory
rm -f CMakeCache.txt
rm -rf CMakeFiles/ _deps/

# Build using out-of-source build directory
cmake -B build
cmake --build build
```

**Prevention:** Always use `cmake -B build` instead of running `cmake .` in the project root.

### Issue 2: Warning in examples/basic/main.cpp

**Warning Message:**

```
warning: ignoring return value of 'bool hexes::from_json(T&, std::string_view)',
declared with attribute 'nodiscard' [-Wunused-result]
```

**Status:** Non-critical warning. Build completes successfully. The warning indicates that the return value of `from_json()` (which indicates success/failure) should be checked.

**Impact:** Does not prevent compilation or execution.

## Troubleshooting

### Lua 5.4 Not Found

If CMake cannot find Lua 5.4, you may need to adjust the paths in `CMakeLists.txt` or install the development package:

```bash
# Check if Lua 5.4 is installed
lua5.4 -v

# Find Lua library location
ldconfig -p | grep lua5.4
```

### Clean Build

If you encounter build issues, try a clean rebuild:

```bash
rm -rf build
cmake -B build
cmake --build build
```

### Compiler Errors

Ensure your compiler supports C++20:

```bash
# GCC version
g++ --version  # Should be 10 or higher

# Clang version
clang++ --version  # Should be 12 or higher
```

## Development

### IDE/LSP Support with clangd

The project is configured for clangd language server support. CMake automatically generates `compile_commands.json` in the build directory.

#### Setup Instructions

1. **Build the project first** (this generates `compile_commands.json`):

   ```bash
   cmake -B build
   cmake --build build
   ```

2. **Verify clangd configuration**:
   The project includes a `.clangd` file that points clangd to the build directory:

   ```yaml
   CompileFlags:
     CompilationDatabase: build
   ```

3. **Editor-specific setup**:

   **VSCode:**
   - Install the [clangd extension](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)
   - Disable the C/C++ extension's IntelliSense to avoid conflicts:
     ```json
     {
       "C_Cpp.intelliSenseEngine": "disabled"
     }
     ```
   - clangd will automatically find the `.clangd` configuration

   **Neovim/Vim:**
   - Using nvim-lspconfig:
     ```lua
     require('lspconfig').clangd.setup{}
     ```
   - Using coc.nvim, install coc-clangd:
     ```
     :CocInstall coc-clangd
     ```

   **Emacs:**
   - Using lsp-mode:
     ```elisp
     (use-package lsp-mode
       :hook (c++-mode . lsp))
     ```

   **Other editors:**
   - clangd will automatically detect the `.clangd` file in the project root

4. **Verify clangd is working**:
   - Open any source file (e.g., `app/main.cpp`)
   - You should see code completion, diagnostics, and go-to-definition working
   - Check your editor's LSP logs if issues occur

#### Troubleshooting clangd

**Issue: "stddef.h not found" or missing system headers**

This occurs when clangd can't find GCC's compiler intrinsic headers.

**Solution:** Add GCC include directory to `.clangd`:

```bash
# Find your GCC version
GCC_VERSION=$(g++ -dumpversion | cut -d. -f1-2)

# Find GCC include directory
GCC_INCLUDE=$(g++ -print-file-name=include)

# Update .clangd
cat > .clangd << EOF
CompileFlags:
  CompilationDatabase: build
  Add: [-isystem, $GCC_INCLUDE]
EOF
```

**Manual alternative:**
```bash
# Find the path
ls /usr/lib/gcc/x86_64-*/*/include/stddef.h

# Update .clangd with the path (without /stddef.h)
# Example:
CompileFlags:
  CompilationDatabase: build
  Add: [-isystem, /usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include]
```

**Issue: "compile_commands.json not found"**

```bash
# Ensure you've run CMake
cmake -B build
```

**Issue: "Outdated diagnostics after editing CMakeLists.txt"**

```bash
# Regenerate compile database
cmake -B build
# Restart your editor's LSP server
```

**Debug clangd:**

```bash
# Test clangd on a specific file
clangd --check=app/main.cpp
```

## License

See LICENSE file for details.
