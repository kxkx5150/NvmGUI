#include "Node.h"
#include "DownloadProgress.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include "unzip/zip.h"

Node::Node(Nvm* nvm, std::wstring version, std::wstring npm, std::wstring lts,
    std::wstring security, std::wstring modules, HWND proghwnd)
    : m_nvm(nvm)
    , m_version(version)
    , m_npm(npm)
    , m_lts(lts)
    , m_security(security)
    , m_modules(modules)
    , m_progresshwnd(proghwnd)
{
    TCHAR extpath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, extpath);
    wsprintf(extpath, TEXT("%s\\nodes\\"), extpath);
    m_root_dir = extpath;
    m_x86_dir = m_root_dir + L"node-" + m_version + L"-win-x86";
    m_x64_dir = m_root_dir + L"node-" + m_version + L"-win-x64";
    
}
Node::~Node()
{
}
std::wstring Node::get_download_url(bool x86)
{
    std::wstring url = L"https://nodejs.org/dist/";
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
std::wstring Node::get_store_path(bool x86)
{
    std::wstring fname = m_root_dir;
    fname += get_store_filename(x86);
    return fname;
}
std::wstring Node::get_store_filename(bool x86)
{
    std::wstring fname = L"node-";
    fname += m_version;
    if (x86) {
        fname += L"-win-x86.zip";
    } else {
        fname += L"-win-x64.zip";
    }
    return fname;
}
void Node::download_node(bool x86)
{
    BOOL ichk = installed_check(x86);
    if (ichk) {
        return;
    }
    downloaded_check(x86);

    std::wstring url = get_download_url();
    std::wstring path = get_store_path(x86);

    OutputDebugString(L"start-----------");
    DownloadProgress progress;
    progress.SetHWND(m_progresshwnd);

    HRESULT res = URLDownloadToFile(NULL, url.c_str(), path.c_str(), 0,
        static_cast<IBindStatusCallback*>(&progress));

    if (res == S_OK) {
        OutputDebugString(L"Ok\n");

        Unzip(path, m_root_dir);
        DeleteFile(path.c_str());
        m_nvm->add_installed_list(this, x86);
        OutputDebugString(L"end-----------\n");

    } else if (res == E_OUTOFMEMORY) {
        OutputDebugString(L"Buffer length invalid, or insufficient memory\n");
    } else if (res == INET_E_DOWNLOAD_FAILURE) {
        OutputDebugString(L"URL is invalid\n");
    } else {
        OutputDebugString(L"Other error:\n");
    }
}
BOOL Node::installed_check(bool x86)
{
    std::wstring rootpath = m_x64_dir;
    if (x86) {
        rootpath = m_x86_dir;
    }

    if (check_exists_dir(rootpath.c_str())) {

        std::wstring fname = rootpath + L"\\node.exe";
        if (!check_exists_file(fname.c_str()))
            return remove_dir(x86);

        fname = rootpath + L"\\npm";
        if (!check_exists_file(fname.c_str()))
            return remove_dir(x86);

        fname = rootpath + L"\\node_modules\\npm\\bin";
        if (!check_exists_dir(fname.c_str()))
            return remove_dir(x86);

        return TRUE;
    }
    return FALSE;
}
BOOL Node::remove_dir(bool x86)
{
    std::wstring wzDirectory = m_x64_dir;
    if (x86) {
        wzDirectory = m_x86_dir;
    }

    WCHAR szDir[MAX_PATH + 1];
    SHFILEOPSTRUCTW fos = { 0 };
    wcscpy_s(szDir, MAX_PATH, wzDirectory.c_str());
    int len = lstrlenW(szDir);
    szDir[len + 1] = 0;

    fos.wFunc = FO_DELETE;
    fos.pFrom = szDir;
    fos.fFlags = FOF_NO_UI;
    SHFileOperation(&fos);
    return FALSE;
}
BOOL Node::check_exists_dir(const TCHAR* path)
{
    if (PathFileExists(path) && PathIsDirectory(path)) {
        return TRUE;
    }
    return FALSE;
}
BOOL Node::check_exists_file(const TCHAR* path)
{
    if (PathFileExists(path) && !PathIsDirectory(path)) {
        return TRUE;
    }
    return FALSE;
}
void Node::downloaded_check(bool x86)
{
    std::wstring path = get_store_path();
    if (check_exists_file(path.c_str())) {
        DeleteFile(path.c_str());
    }
}
