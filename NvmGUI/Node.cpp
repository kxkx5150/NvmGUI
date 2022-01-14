#include "Node.h"

Node::Node(std::wstring version, std::wstring npm, std::wstring lts, bool security)
    : m_version(version)
    , m_npm(npm)
    , m_lts(lts)
    , m_security(security)
{
}
Node::~Node()
{
}
std::wstring Node::get_download_url(bool x86)
{
    std::wstring url = m_node_baseurl;
    url += m_version;
    url += L"/node-";
    url += m_version;
    url += L"-";
    if (x86) {
        url += m_x86;
    } else {
        url += m_x64;
    }
    return url;
}
std::wstring Node::get_store_filename(bool x86)
{
    std::wstring fname = m_version;
    if (x86) {
        fname += L"-win-x86.zip";
    } else {
        fname += L"-win-x64.zip";
    }
    return fname;
}
