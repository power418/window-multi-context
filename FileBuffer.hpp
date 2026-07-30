#ifndef __FILEBUFFER_HPP__
#define __FILEBUFFER_HPP__

#include <iostream>
#include <string>
#include <fstream>
#include <vector>

class FileBuffer
{
public:
    FileBuffer() = default;

    FileBuffer(const std::string& path)
    {
        load(path);
    }

    bool load(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            return false;

        m_path = path;
        m_size = file.tellg();
        file.seekg(0, std::ios::beg);

        m_data.resize(m_size);
        file.read(m_data.data(), m_size);
        file.close();
        return true;
    }

    const char* data() const { return m_data.data(); }
    std::size_t size() const { return m_size; }
    const std::string& path() const { return m_path; }
    bool empty() const { return m_size == 0; }

    std::string toString() const
    {
        return std::string(m_data.data(), m_size);
    }

    void ds_info() const
    {
        std::cout << "file: " << m_path
                  << ", size: " << m_size << " bytes\n";
    }

private:
    std::string m_path;
    std::vector<char> m_data;
    std::size_t m_size = 0;
};

#endif // __FILEBUFFER_HPP__
