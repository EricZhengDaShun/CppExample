# CMakeHelloWorld

## **How To Build**

1. Crete a build folder:

   ```bash
   mkdir build
   ```

2. Enter the build folder

   ```bash
   cd build
   ```

3. Configure with CMake:

  - Debug mode:

     ```bash
     cmake .. -DCMAKE_BUILD_TYPE=Debug
     ```

  - Release mode:

     ```bash
     cmake .. -DCMAKE_BUILD_TYPE=Release
     ```

4. Buid the project:

   ```bash
   cmake --build .
   ```
