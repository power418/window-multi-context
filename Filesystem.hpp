#ifndef __FILESYSTEM_HPP__
#define __FILESYSTEM_HPP__

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "FileBuffer.hpp"

namespace fs = std::filesystem;

class Filesystem
{
public:
    Filesystem(const std::string& pattern = "")
        : m_pattern(pattern)
    {
        if (!pattern.empty())
            scan(".");
    }

    void scan(const std::string& directory)
    {
        m_files.clear();
        if (!fs::exists(directory) || !fs::is_directory(directory))
            return;

        for (const auto& entry : fs::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                auto ext = entry.path().extension().string();
                if (ext == m_pattern)
                    m_files.push_back(entry.path().string());
            }
        }
    }

    const std::vector<std::string>& files() const { return m_files; }
    std::size_t count() const { return m_files.size(); }

    FileBuffer load(std::size_t index) const
    {
        if (index >= m_files.size())
            return FileBuffer();
        return FileBuffer(m_files[index]);
    }

    FileBuffer load(const std::string& path) const
    {
        return FileBuffer(fs::exists(path) ? path : "");
    }

    void ds_info() const
    {
        std::cout << "pattern: \"" << m_pattern << "\", files found: " << m_files.size() << "\n";
        for (std::size_t i = 0; i < m_files.size(); i++)
            std::cout << "  [" << i << "] " << m_files[i] << "\n";
    }

    static inline bool exists(const std::string& path) { return fs::exists(path); }
    static inline bool isFile(const std::string& path) { return fs::is_regular_file(path); }
    static inline bool isDirectory(const std::string& path) { return fs::is_directory(path); }
    static inline std::uintmax_t fileSize(const std::string& path) { return fs::file_size(path); }
    static inline std::string extension(const std::string& path) { return fs::path(path).extension().string(); }
    static inline std::string filename(const std::string& path) { return fs::path(path).filename().string(); }
    static inline std::string stem(const std::string& path) { return fs::path(path).stem().string(); }

private:
    std::string m_pattern;
    std::vector<std::string> m_files;
};

#endif // __FILESYSTEM_HPP__
