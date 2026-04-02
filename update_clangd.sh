#!/usr/bin/env bash
# Auto-generate .clangd configuration with GCC include paths

GCC_INCLUDE=$(g++ -print-file-name=include)

cat > .clangd << EOF
CompileFlags:
  CompilationDatabase: build
  Add: [-isystem, $GCC_INCLUDE]
EOF

echo "Generated .clangd with GCC include path: $GCC_INCLUDE"

