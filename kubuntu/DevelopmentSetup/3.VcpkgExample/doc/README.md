# Vcpkg Example

Ensure that **vcpkg** is installed on your system.

## Create a vcpkg Installation List

### 1. Initialize the vcpkg environment

```bash
vcpkg new --application
```

### 2. Add a library (example: fmt)

```bash
vcpkg add port fmt
```

You can verify the result in the `vcpkg.json` file:

```json
{
  "dependencies": [
    "fmt"
  ]
}
```

### 3. Lock library version (optional)

- **Update baseline**

  ```bash
  vcpkg x-update-baseline --add-initial-baseline
  ```

  This will add the `builtin-baseline` field in your `vcpkg.json`.

- **Manually specify a minimum version**

  ```json
  {
    "dependencies": [
      {
        "name": "fmt",
        "version>=": "10.2.1"
      }
    ],
    "builtin-baseline": "f9b54c1c539dda8d61c3001bb30eb9f0c5032086"
  }
  ```

### 4. Force a specific version (optional)

Use the `overrides` field to enforce a specific version:

```json
{
  "dependencies": [
    {
      "name": "fmt",
      "version>=": "10.2.1"
    }
  ],
  "builtin-baseline": "f9b54c1c539dda8d61c3001bb30eb9f0c5032086",
  "overrides": [
    { "name": "fmt", "version": "11.0.2" }
  ]
}
```

### 5. Install packages

```bash
vcpkg install
```

### 6. Configure your CMake project

Add the toolchain configuration **before** the `project(...)` line in `CMakeLists.txt`:

```cmake
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
```