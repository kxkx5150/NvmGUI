#pragma once
#include "Nvm.h"
#include <Windows.h>
#include <string>

class Nvm;

class Node {
public:
    std::wstring m_version = nullptr;
    std::wstring m_npm = nullptr;
    std::wstring m_lts = nullptr;
    std::wstring m_security = nullptr;
    std::wstring m_modules = nullptr;
    std::wstring m_install_arch = L"";

    std::wstring m_x86_dir = L"";
    std::wstring m_x64_dir = L"";

private:
    HWND m_progresshwnd = nullptr;

    std::wstring m_x86 = L"win-x86.zip";
    std::wstring m_x64 = L"win-x64.zip";

    std::wstring m_root_dir = L"";

public:
    Node(Nvm* nvm, std::wstring version, std::wstring npm, std::wstring lts,
        std::wstring security, std::wstring modules, HWND proghwnd);
    ~Node();

    std::wstring get_download_url(bool x86 = false);
    std::wstring get_store_path(bool x86 = false);
    std::wstring get_store_filename(bool x86 = false);
    BOOL download_node(bool x86 = false);
    BOOL remove_dir(bool x86 = false);

    BOOL check_exists_dir(const TCHAR* path);

private:
    BOOL installed_check(bool x86);
    void downloaded_check(bool x86);
    BOOL check_exists_file(const TCHAR* path);
};
