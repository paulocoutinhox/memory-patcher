#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#if defined(_WIN32)

#include <tlhelp32.h>
#include <windows.h>

#elif defined(__APPLE__)

#include <cstring>
#include <libproc.h>
#include <sys/sysctl.h>

extern "C"
{
#include <mach/mach.h>
#include <mach/vm_map.h>
}

#elif defined(__linux__)

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sys/uio.h>
#include <sys/types.h>
#include <unistd.h>

#endif

/**
 * Finds a process ID by its name
 * @param processName The name of the process to find
 * @return The process ID (PID) if found, 0 otherwise
 */
pid_t getPidByName(const std::string &processName)
{
#if defined(_WIN32)
    // Windows implementation using ToolHelp API
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &entry))
    {
        do
        {
            if (_stricmp(entry.szExeFile, processName.c_str()) == 0)
            {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;

#elif defined(__APPLE__)
    // MacOS implementation using sysctl to get process information
    // Get the number of processes
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size;
    if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0)
    {
        return 0;
    }

    struct kinfo_proc *procs = (struct kinfo_proc *)malloc(size);
    if (!procs)
    {
        return 0;
    }

    if (sysctl(mib, 4, procs, &size, NULL, 0) < 0)
    {
        free(procs);
        return 0;
    }

    int count = size / sizeof(struct kinfo_proc);

    for (int i = 0; i < count; i++)
    {
        pid_t pid = procs[i].kp_proc.p_pid;

        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
        int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));

        if (ret > 0)
        {
            // Extract just the process name from the full path
            std::string fullPath(pathbuf);
            size_t lastSlash = fullPath.find_last_of("/");
            std::string name = (lastSlash != std::string::npos) ? fullPath.substr(lastSlash + 1) : fullPath;

            if (name == processName)
            {
                free(procs);
                return pid;
            }
        }
    }

    free(procs);
    return 0;
#elif defined(__linux__)
    // Linux implementation using /proc filesystem
    DIR *dir = opendir("/proc");
    if (!dir)
        return 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_type != DT_DIR)
            continue;
        std::string pidStr = entry->d_name;
        if (!std::all_of(pidStr.begin(), pidStr.end(), ::isdigit))
            continue;

        std::string cmdPath = "/proc/" + pidStr + "/comm";
        std::ifstream cmdFile(cmdPath);
        if (!cmdFile.is_open())
            continue;

        std::string name;
        std::getline(cmdFile, name);
        if (name == processName)
        {
            closedir(dir);
            return std::stoi(pidStr);
        }
    }

    closedir(dir);
    return 0;
#else
#error Platform not supported
#endif
}

/**
 * Writes a value to a specific memory address in a target process
 * @param pid Process ID of the target
 * @param address Memory address to write to
 * @param value Pointer to the value to write
 * @param size Size of the value in bytes
 * @return true if successful, false otherwise
 */
bool patchMemoryValue(pid_t pid, uintptr_t address, const void *value, size_t size)
{
#if defined(_WIN32)
    // Windows implementation using WriteProcessMemory
    HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    if (!hProcess)
    {
        std::cerr << "Unable to open target process. Please make sure you have sufficient permissions." << std::endl;
        return false;
    }

    SIZE_T written;
    if (!WriteProcessMemory(hProcess, (LPVOID)address, value, size, &written))
    {
        std::cerr << "Failed to write to target process memory." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    CloseHandle(hProcess);
    return true;

#elif defined(__APPLE__)
    // MacOS implementation using Mach VM API
    mach_port_t task;
    if (task_for_pid(mach_task_self(), pid, &task) != KERN_SUCCESS)
    {
        std::cerr << "Unable to access target process. Try running as root and ensure SIP is disabled." << std::endl;
        return false;
    }

    kern_return_t kr = vm_write(task,
                                (vm_address_t)address,
                                (vm_offset_t)value,
                                (mach_msg_type_number_t)size);

    if (kr != KERN_SUCCESS)
    {
        std::cerr << "Failed to write to target process memory." << std::endl;
        return false;
    }

    return true;

#elif defined(__linux__)
    // Linux implementation using process_vm_writev
    struct iovec local_iov = {
        .iov_base = const_cast<void *>(value),
        .iov_len = size};
    struct iovec remote_iov = {
        .iov_base = (void *)address,
        .iov_len = size};

    ssize_t nwritten = process_vm_writev(pid, &local_iov, 1, &remote_iov, 1, 0);
    if (nwritten != (ssize_t)size)
    {
        std::cerr << "Failed to write to target process memory." << std::endl;
        return false;
    }

    return true;

#else
#error Unsupported platform
#endif
}

/**
 * Main function - handles user interaction and memory patching operations
 */
int main()
{
    // Get target process name from user
    std::string processName;
    std::cout << "Enter the process name (e.g., notepad.exe or light-hack-demo): ";
    std::cin >> processName;

    // Get target memory address in hexadecimal format
    uintptr_t address;
    std::cout << "Enter the memory address (hex format, e.g., 0x12345678):";
    std::cin >> std::hex >> address;
    std::cin.clear();

    // Display menu for data type selection
    std::cout << "Choose the data type to write:" << std::endl;
    std::cout << "1. 8-bit Integer" << std::endl;
    std::cout << "2. 16-bit Integer" << std::endl;
    std::cout << "3. 32-bit Integer" << std::endl;
    std::cout << "4. 64-bit Integer" << std::endl;
    std::cout << "5. Float" << std::endl;
    std::cout << "6. Double" << std::endl;
    std::cout << "7. 8-bit String" << std::endl;
    std::cout << "8. 16-bit String (UTF-16)" << std::endl;
    std::cout << "Enter your choice: ";

    int choice;
    std::cin >> choice;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Find the process by name
    pid_t pid = getPidByName(processName);
    if (pid == 0)
    {
        std::cerr << "Process not found. Please check the name and try again." << std::endl;
        return 1;
    }

    // Handle different data types based on user selection
    bool success = false;
    switch (choice)
    {
    case 1:
    {
        // 8-bit integer
        int8_t value;
        std::cout << "Enter int8 value: ";
        std::cin >> std::dec >> value;
        success = patchMemoryValue(pid, address, &value, sizeof(value));
        break;
    }
    case 2:
    {
        // 16-bit integer
        int16_t value;
        std::cout << "Enter int16 value: ";
        std::cin >> std::dec >> value;
        success = patchMemoryValue(pid, address, &value, sizeof(value));
        break;
    }
    case 3:
    {
        // 32-bit integer
        int32_t value;
        std::cout << "Enter int32 value: ";
        std::cin >> std::dec >> value;
        success = patchMemoryValue(pid, address, &value, sizeof(value));
        break;
    }
    case 4:
    {
        // 64-bit integer
        int64_t value;
        std::cout << "Enter int64 value: ";
        std::cin >> std::dec >> value;
        success = patchMemoryValue(pid, address, &value, sizeof(value));
        break;
    }
    case 5:
    {
        // Float
        float value;
        std::cout << "Enter float value: ";
        std::cin >> value;
        success = patchMemoryValue(pid, address, &value, sizeof(value));
        break;
    }
    case 6:
    {
        // Double
        double value;
        std::cout << "Enter double value: ";
        std::cin >> value;
        success = patchMemoryValue(pid, address, &value, sizeof(value));
        break;
    }
    case 7:
    {
        // ASCII string (null-terminated)
        std::string value;
        std::cout << "Enter ASCII string: ";
        std::cin.ignore();
        std::getline(std::cin, value);
        success = patchMemoryValue(pid, address, value.c_str(), value.size() + 1);
        break;
    }
    case 8:
    {
        // UTF-16 string (simple conversion from UTF-8)
        std::u16string value;
        std::string input;
        std::cout << "Enter UTF-8 string to convert to UTF-16: ";
        std::cin.ignore();
        std::getline(std::cin, input);
        value.assign(input.begin(), input.end());
        success = patchMemoryValue(pid, address, value.c_str(), (value.size() + 1) * 2);
        break;
    }
    default:
        std::cerr << "Invalid option selected." << std::endl;
        return 1;
    }

    // Report operation result
    if (success)
    {
        std::cout << "Memory patched successfully!" << std::endl;
    }
    else
    {
        std::cerr << "Could not patch memory." << std::endl;
    }

    return 0;
}
