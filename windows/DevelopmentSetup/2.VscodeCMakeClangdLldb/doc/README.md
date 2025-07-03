# VSCode CMake with clangd and LLDB

- **Base project:** CMakeHelloWorld
- Ensure `Ninja` and `clangd` is already installed on your system

---

## Install Extensins

- clangd
- CMake Tools
- CodeLLDB

![extensions](images/extensions.png)

---

## How to Create a launch.json File

1. Open the Run and Debug panel (`Ctrl + Shift + D`)

2. Click **"create a launch.json file"**

   ![createALaunchJsonFile](images/createALaunchJsonFile.png)

3. Select **CMake Debugger**

   ![cmakeDebugger](images/cmakeDebuger.png)

4. Add **CodeLLDB Launch**

   ![codeLldbLaunch](images/codeLldebLaunch.png)

5. Modify the `program` path:

   From:

   ```json
   "program": "${workspaceFolder}/<program>"
   ```

   To:

   ```json
   "program": "${command:cmake.launchTargetPath}"
   ```

6. Ensure the project is built before debugging by adding:

   ```json
   "preLaunchTask": "CMake: build"
   ```

7. Start debugging with `F5`

8. In `.vscode/settings.json`, add the following:

   ```json
   "cmake.generator": "Ninja"
   ```

9. In `CMakeLists.txt`, add the following line **before** the `project(...)` line:

   ```cmake
   set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
   ```

10. Confirm that `compile_commands.json` is generated in your build folder.
