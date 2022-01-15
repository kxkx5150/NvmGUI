#include "Node.h"
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#include <Shellapi.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

Node::Node(std::wstring version, std::wstring npm, std::wstring lts, std::wstring security, std::wstring modules)
    : m_version(version)
    , m_npm(npm)
    , m_lts(lts)
    , m_security(security)
    , m_modules(modules)
{
    TCHAR extpath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, extpath);
    wsprintf(extpath, TEXT("%s\\nodes\\"), extpath);
    m_root_dir = extpath;
    m_x86_dir = m_root_dir + L"node-" + get_store_dirname(true);
    m_x64_dir = m_root_dir + L"node-" + get_store_dirname();
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
    fname += get_store_filename();
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
std::wstring Node::get_store_dirname(bool x86)
{
    std::wstring dname = m_version;
    if (x86) {
        dname += L"-win-x86";
    } else {
        dname += L"-win-x64";
    }
    return dname;
}
void Node::download_node(bool x86)
{
    BOOL ichk = installed_check(x86);
    if (ichk) {
        return;
    }
    downloaded_check(x86);

    std::wstring url = get_download_url();
    std::wstring path = get_store_path();
    HRESULT res = URLDownloadToFile(NULL, url.c_str(), path.c_str(), 0, NULL);

    if (res == S_OK) {
        OutputDebugString(L"Ok\n");
        ExtractZip(path.c_str(), m_root_dir.c_str());

    } else if (res == E_OUTOFMEMORY) {
        OutputDebugString(L"Buffer length invalid, or insufficient memory\n");
    } else if (res == INET_E_DOWNLOAD_FAILURE) {
        OutputDebugString(L"URL is invalid\n");
    } else {
        OutputDebugString(L"Other error:\n");
    }
}
BOOL Node::ExtractZip(const TCHAR* ZipPath, const TCHAR* OutPath)
{
    IShellDispatch* pShellDisp;
    CoInitializeEx(0, COINIT_MULTITHREADED);
    HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellDisp));
    if (FAILED(hr)) {
        CoUninitialize();
        return FALSE;
    }
    VARIANT vDtcDir;
    Folder* pOutDtc;
    // 展開先Folderオブジェクトを作成
    VariantInit(&vDtcDir);
    vDtcDir.vt = VT_BSTR;
    vDtcDir.bstrVal = SysAllocString(OutPath);
    hr = pShellDisp->NameSpace(vDtcDir, &pOutDtc);
    VariantClear(&vDtcDir);
    if (hr != S_OK) {
        MessageBox(NULL, TEXT("展開先フォルダーが見つかりませんでした。"), NULL, MB_ICONWARNING);
        return FALSE;
    }
    // ZIPファイルのFolderオブジェクトを作成
    VARIANT varZip;
    Folder* pZipFile;
    VariantInit(&varZip);
    varZip.vt = VT_BSTR;
    varZip.bstrVal = SysAllocString(ZipPath);
    hr = pShellDisp->NameSpace(varZip, &pZipFile);
    VariantClear(&varZip);
    if (hr != S_OK) {
        pOutDtc->Release();
        MessageBox(NULL, TEXT("ZIPファイルが見つかりませんでした。"), NULL, MB_ICONWARNING);
        return FALSE;
    }
    // ZIPファイルの中身を取得
    FolderItems* pZipItems;
    hr = pZipFile->Items(&pZipItems);
    if (hr != S_OK) {
        pZipFile->Release();
        pOutDtc->Release();
        return FALSE;
    }
    VARIANT vDisp, vOpt;
    VariantInit(&vDisp);
    vDisp.vt = VT_DISPATCH;
    vDisp.pdispVal = pZipItems;
    VariantInit(&vOpt);
    vOpt.vt = VT_I4;
    vOpt.lVal = 0; //FOF_SILENTを指定すると処理中の経過が表示されなくなります
    // ZIPファイルの中身を展開先フォルダーにコピー
    hr = pOutDtc->CopyHere(vDisp, vOpt);
    if (hr != S_OK) {
        pZipItems->Release();
        pZipFile->Release();
        pOutDtc->Release();
        MessageBox(NULL, TEXT("展開に失敗しました。"), NULL, MB_ICONWARNING);
        return FALSE;
    }

    pZipItems->Release();
    pZipFile->Release();
    pOutDtc->Release();
    CoUninitialize();

    return TRUE;
}
BOOL Node::Unzip2Folder(const TCHAR* ZipPath, const TCHAR* OutPath)
{
    BSTR lpZipFile = SysAllocString(ZipPath);
    BSTR lpFolder = SysAllocString(OutPath);

    IShellDispatch* pISD;

    Folder* pZippedFile = 0L;
    Folder* pDestination = 0L;

    long FilesCount = 0;
    IDispatch* pItem = 0L;
    FolderItems* pFilesInside = 0L;

    VARIANT Options, OutFolder, InZipFile, Item;
    CoInitialize(NULL);
    __try {
        if (CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pISD) != S_OK)
            return 1;

        InZipFile.vt = VT_BSTR;
        InZipFile.bstrVal = lpZipFile;
        pISD->NameSpace(InZipFile, &pZippedFile);
        if (!pZippedFile) {
            pISD->Release();
            return 1;
        }

        OutFolder.vt = VT_BSTR;
        OutFolder.bstrVal = lpFolder;
        pISD->NameSpace(OutFolder, &pDestination);
        if (!pDestination) {
            pZippedFile->Release();
            pISD->Release();
            return 1;
        }

        pZippedFile->Items(&pFilesInside);
        if (!pFilesInside) {
            pDestination->Release();
            pZippedFile->Release();
            pISD->Release();
            return 1;
        }

        pFilesInside->get_Count(&FilesCount);
        if (FilesCount < 1) {
            pFilesInside->Release();
            pDestination->Release();
            pZippedFile->Release();
            pISD->Release();
            return 0;
        }

        pFilesInside->QueryInterface(IID_IDispatch, (void**)&pItem);

        Item.vt = VT_DISPATCH;
        Item.pdispVal = pItem;

        Options.vt = VT_I4;
        Options.lVal = 1024 | 512 | 16 | 4; //http://msdn.microsoft.com/en-us/library/bb787866(VS.85).aspx

        bool retval = pDestination->CopyHere(Item, Options) == S_OK;

        pItem->Release();
        pItem = 0L;
        pFilesInside->Release();
        pFilesInside = 0L;
        pDestination->Release();
        pDestination = 0L;
        pZippedFile->Release();
        pZippedFile = 0L;
        pISD->Release();
        pISD = 0L;

        return retval;

    } __finally {
        CoUninitialize();
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
