#pragma once
#include <string>

class Node {

    std::wstring m_version = nullptr;
    std::wstring m_npm = nullptr;
    std::wstring m_lts = nullptr;
    bool m_security = false;
    std::wstring m_x86 = L"win-x86.zip";
    std::wstring m_x64 = L"win-x64.zip";

    const std::wstring m_node_baseurl = L"https://nodejs.org/dist/";

public:
    Node(std::wstring version, std::wstring npm, std::wstring lts, bool security);
    ~Node();

    std::wstring get_download_url(bool x86 = false);
    std::wstring get_store_filename(bool x86 = false);

private:
};
