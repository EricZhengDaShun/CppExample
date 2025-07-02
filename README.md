# CppExample

This repository contains several C++ example projects for learning and demonstration purposes.  
Each project is organized by platform (e.g., Windows), and contains its own README and build instructions.


## Projects

### `windows/CMakeHelloWorld`

A minimal CMake-based C++ project.  
Demonstrates basic project structure and platform-independent build setup.

### `windows/VscodeCMakeClangdLldb`

Extends `CMakeHelloWorld` to demonstrate debugging in VSCode.  
Includes configuration for `clangd`, `CMake Tools`, and `CodeLLDB`.

### `windows/VcpkgExample`

Extends `VscodeCMakeClangdLldb` by integrating **vcpkg** as the package manager.  
Demonstrates how to manage dependencies with `vcpkg.json`, configure version locking, and set up CMake toolchain integration.

---

## Folder Structure

```
CppExample/
├── windows/
│   ├── CMakeHelloWorld/           # Basic CMake project
│   ├── VscodeCMakeClangdLldb/     # VSCode + LLDB debug setup
│   └── VcpkgExample/              # vcpkg integration example
└── README.md                      # Project overview (this file)
```

---

## How to Use

1. Navigate to the `windows/` directory.
2. Enter a specific project folder to view its documentation.
3. Follow the provided instructions in each `README.md` to build and run the example.
4. All projects are self-contained and can be used independently.

---