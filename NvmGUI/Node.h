#pragma once
#include <Windows.h>
#include <string>

class Node {

    std::wstring m_version = nullptr;
    std::wstring m_npm = nullptr;
    std::wstring m_lts = nullptr;
    bool m_security = false;
    std::wstring m_modules = nullptr;
    std::wstring m_x86 = L"win-x86.zip";
    std::wstring m_x64 = L"win-x64.zip";

    std::wstring m_root_dir = L"";
    std::wstring m_x86_dir = L"";
    std::wstring m_x64_dir = L"";

public:
    Node(std::wstring version, std::wstring npm, std::wstring lts, bool security, std::wstring modules);
    ~Node();

    std::wstring get_download_url(bool x86 = false);
    std::wstring get_store_path(bool x86 = false);
    std::wstring get_store_filename(bool x86 = false);
    std::wstring get_store_dirname(bool x86 = false);
    void download_node(bool x86 = false);

private:
    BOOL installed_check(bool x86);
    void downloaded_check(bool x86);
    BOOL check_exists_dir(const TCHAR* path);
    BOOL check_exists_file(const TCHAR* path);
    BOOL remove_dir(bool x86 = false);

    BOOL ExtractZip(const TCHAR* ZipPath, const TCHAR* OutPath);
    BOOL Unzip2Folder(const TCHAR* ZipPath, const TCHAR* OutPath);
};
