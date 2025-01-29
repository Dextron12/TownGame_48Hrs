#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <string>
#include <filesystem>
#include <iostream>
#include <unordered_map>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <libgen.h>
#endif

class FileSystem {
public:
    // Constructor that automatically initializes the executable path
    FileSystem() {
        // Directly initialize the executable path in the constructor
#ifdef _WIN32
        char path[MAX_PATH];  // Allocate enough space for the path
        DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
        if (length == 0) {
            std::cout << "Failed to get the operating path" << std::endl;
            return;
        }

        // Print the full path to verify
        //std::cout << "Full executable path: " << path << std::endl;

        // Remove the executable name from the path by finding the last backslash
        char* lastSlash = strrchr(path, '\\');  // Use strrchr to find the last backslash
        if (lastSlash != nullptr) {
            *lastSlash = '\0';  // Terminate the string at the last backslash
        }

        // Now, the path should point to the directory of the executable
        m_paths.insert_or_assign("execPath", path);
#else
        // Unix-based systems (Linux/macOS) implementation using readlink
        char path[1024];
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len == -1) {
            std::cout << "Failed to determine executable path" << std::endl;
            return;
        }
        path[len] = '\0';  // Null-terminate the string

        // Use filesystem library to handle paths
        std::filesystem::path fsPath(path);
        fsPath = fsPath.parent_path();
        m_paths.insert_or_assign("execPath", path);
#endif
    }

    // Getter for the executable path  
    std::string getExecPath() const {
        auto it = m_paths.find("execPath");
        if (it != m_paths.end()) {
            return it->second;
        }
        else {
            std::cout << "Cannot return execPath" << std::endl;
            return "";
        }

        //return m_paths[(std::string)"execPath"];
    }

    // Set a custom path in the Paths vector (if it exists)
    bool setPath(const std::string& ID, const std::string& path) {
        if (std::filesystem::exists(path)) {
            m_paths.insert_or_assign(ID, path);
            return true;
        }
        else {
            std::printf("The path: %s does not exist and has NOT been created.\n", path.c_str());
        }
        return false;
    }

    // Get a path by index from the Paths vector
    std::string getPath(const std::string& ID) const {
        auto it = m_paths.find(ID);
        if (it != m_paths.end()) {
            return it->second;
        }
        else {
            std::printf("The path ID(%s) does not exist", ID.c_str());
            return "";
        }
    }

    // Join two paths and return the resulting string (execPath and relative file)
    std::string joinPaths(const std::string& basePath, const std::string& filePath) {
        std::filesystem::path fullPath(basePath);
        fullPath /= filePath;
        return fullPath.string();
    }

    std::string joinToExecDir(const std::string& filePath) {
        std::filesystem::path fullPath(m_paths["execPath"]);
        fullPath /= filePath;
        return fullPath.string();
    }

private:
    std::string m_execPath;   // Stores the executable path (without the exec name)
    std::unordered_map<std::string, std::string> m_paths;
};

#endif // FILESYSTEM_HPP
