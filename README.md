# Memory Patcher (Cross-Platform)

This project is a cross-platform C++ utility to patch (overwrite) memory values of a running process by name and memory address.

## 🧠 Overview

This program allows you to:
- Get the PID of a target process by its name
- Write a new value to a specific memory address of that process
- Support for multiple data types (integers, floats, strings)

Supports:
- ✅ Windows
- ✅ macOS (requires SIP disabled and root)
- ✅ Linux (requires elevated permissions)

## ⚠️ Warnings

- On **macOS**, you must run as `sudo` and disable **SIP (System Integrity Protection)**.
- On **Linux**, ensure your system allows `process_vm_writev`.
- On **Windows**, run with admin privileges for access to other processes.

**DISCLAIMER:** This project is intended for educational purposes only. The user assumes full responsibility for how this tool is used and any consequences that may arise from its use.

---

## 🔧 Build Instructions (Using CMake)

### 1. Clone the repository

```bash
git clone https://github.com/paulocoutinhox/memory-patcher.git
cd memory-patcher
```

### 2. Create build folder and compile

```bash
cmake -B build
cmake --build build
```

### 3. Run the executable

```bash
# For Linux/macOS
sudo ./build/mempatcher

# For Windows
build\Debug\mempatcher.exe
```

---

## 🧪 Example Usage

```
Enter the process name (e.g., notepad.exe or light-hack-demo): myapp
Enter the memory address (hex format, e.g., 12345678): 0x14982AA00

Choose the data type to write:
1. 8-bit Integer
2. 16-bit Integer
3. 32-bit Integer
4. 64-bit Integer
5. Float
6. Double
7. 8-bit String
8. 16-bit String (UTF-16)
Enter your choice: 3

Enter int32 value: 123
Memory patched successfully!
```

### Supported Data Types

1. **8-bit Integer** - Range: -128 to 127
2. **16-bit Integer** - Range: -32,768 to 32,767
3. **32-bit Integer** - Range: -2,147,483,648 to 2,147,483,647
4. **64-bit Integer** - Range: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
5. **Float** - Single-precision floating-point
6. **Double** - Double-precision floating-point
7. **8-bit String** - ASCII string
8. **16-bit String** - UTF-16 string

## 📁 Files

- `main.cpp`: Main program logic
- `CMakeLists.txt`: CMake build file
- `README.md`: This documentation

---

## 📃 License

MIT License
