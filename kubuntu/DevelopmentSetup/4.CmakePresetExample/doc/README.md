# CmakePresetExample

This project demonstrates how to set up and use `CMakePresets.json` for a modern, structured CMake workflow.

---

## 1. Create a Basic CMake Preset File

Start by creating a `CMakePresets.json` file with the minimal required structure:

```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 25,
        "patch": 0
    }
}
```

---

## 2. Add a Base Preset

Define a hidden base preset with:

- Generator set to **Ninja**
- Output directory set to `${sourceDir}/build`
- Compiler set to `gcc`
- Toolchain path from `VCPKG_ROOT`

```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 25,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "linux-base",
            "hidden": true,
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": {
                "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
                "CMAKE_C_COMPILER": "gcc",
                "CMAKE_CXX_COMPILER": "g++"
            }
        }
    ]
}
```

---

## 3. Add Debug and Release Configure Presets

Extend the base preset to create build-type specific configurations:

```json
{
    "configurePresets": [
        {
            "name": "linux-debug",
            "inherits": "linux-base",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "linux-release",
            "inherits": "linux-base",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        }
    ]
}
```

---

## 4. Define Build Presets

Add named build presets referencing the above configure presets:

```json
{
    "buildPresets": [
        {
            "name": "linux-build-debug",
            "configurePreset": "linux-debug"
        },
        {
            "name": "linux-build-release",
            "configurePreset": "linux-release"
        }
    ]
}
```

---

## 5. Build and Run the Project

### Steps

1. **Configure the project**

    ```bash
    cmake --preset linux-debug
    ```

2. **Build the project**

    ```bash
    cmake --build --preset linux-build-debug
    ```
