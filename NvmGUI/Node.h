#pragma once
#include <string>

class Node {

    std::wstring m_version = nullptr;
    std::wstring m_npm = nullptr;
    bool m_lts = false;
    bool m_security = false;
    std::wstring m_x86 = L"win-x86-zip";
    std::wstring m_x64 = L"win-x64-zip";


public:
    Node(std::wstring version, std::wstring npm, bool lts, bool security);
    ~Node();

private:




};

