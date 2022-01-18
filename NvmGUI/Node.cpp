#include "Node.h"
#include "DownloadProgress.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include "unzip/zip.h"
#include <shlobj.h>


extern Nvm* g_nvm;
Node::Node(Nvm* nvm, std::wstring version, std::wstring npm, std::wstring lts,
    std::wstring security, std::wstring modules, HWND proghwnd)
    : m_version(version)
    , m_npm(npm)
    , m_lts(lts)
    , m_security(security)
    , m_modules(modules)
    , m_progresshwnd(proghwnd)
{
    TCHAR path[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, path);
    wsprintf(path, TEXT("%s\\nvm_gui\\"), path);
    m_root_dir = path;
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

void callback_progress(ULONG& number_entry, ULONG& cont, char* filename)
{
    g_nvm->set_progress_value(number_entry, cont, filename);
}
BOOL Node::download_node(bool x86)
{
    BOOL ichk = installed_check(x86);
    if (ichk)
        return FALSE;

    downloaded_check(x86);
    std::wstring url = get_download_url(x86);
    std::wstring path = get_store_path(x86);

    DownloadProgress progress;
    progress.SetHWND(m_progresshwnd);
    HRESULT res = URLDownloadToFile(NULL, url.c_str(), path.c_str(), 0,
        static_cast<IBindStatusCallback*>(&progress));

    if (res == S_OK) {
        UnzipCPP::Unzip(path, m_root_dir, callback_progress);
        DeleteFile(path.c_str());
        std::wstring verstr = this->m_version;
        std::wstring npmstr = this->m_npm;
        std::wstring ltsstr = this->m_lts;
        std::wstring secstr = this->m_security;
        std::wstring modstr = this->m_modules;
        g_nvm->add_installed_list(verstr, npmstr, ltsstr, secstr, modstr, x86);
        return TRUE;
    } else if (res == E_OUTOFMEMORY) {
        OutputDebugString(L"Buffer length invalid, or insufficient memory\n");
    } else if (res == INET_E_DOWNLOAD_FAILURE) {
        OutputDebugString(L"URL is invalid\n");
    } else {
        OutputDebugString(L"Other error:\n");
    }
    return FALSE;
}
BOOL Node::installed_check(bool x86)
{
    std::wstring rootpath = m_x64_dir;
    if (x86) {
        rootpath = m_x86_dir;
    }

    if (check_exists_dir(rootpath.c_str())) {
        //return TRUE;
        //std::wstring fname = rootpath + L"\\node.exe";
        //if (!check_exists_file(fname.c_str()))
        //    return remove_dir(x86);

        //fname = rootpath + L"\\npm";
        //if (!check_exists_file(fname.c_str()))
        //    return remove_dir(x86);

        //fname = rootpath + L"\\node_modules\\npm\\bin";
        //if (!check_exists_dir(fname.c_str()))
        //    return remove_dir(x86);
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
