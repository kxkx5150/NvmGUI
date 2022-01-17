#include "zip.h"
#include "unzip.h"
#include <Shellapi.h>
#include <shlobj.h>
#pragma comment(lib, "unzip/zlib.lib")

#define IsShiftJIS(x) ((BYTE)((x ^ 0x20) - 0xA1) <= 0x3B)

std::string WStringToString(std::wstring oWString)
{
    int iBufferSize = WideCharToMultiByte(CP_OEMCP, 0, oWString.c_str(), -1, (char*)NULL, 0, NULL, NULL);
    CHAR* cpMultiByte = new CHAR[iBufferSize];
    WideCharToMultiByte(CP_OEMCP, 0, oWString.c_str(), -1, cpMultiByte, iBufferSize, NULL, NULL);
    std::string oRet(cpMultiByte, cpMultiByte + iBufferSize - 1);
    delete[] cpMultiByte;
    return (oRet);
}
bool IsFileExist(const std::string& strFilename)
{
    return GetFileAttributesA(strFilename.c_str()) != 0xffffffff;
}
bool CreateDirectoryReflex(const std::string& strPath)
{
    const char* p = strPath.c_str();
    for (; *p; p += IsShiftJIS(*p) ? 2 : 1) {
        if (*p == '\\') {
            std::string strSubPath(strPath.c_str(), p);
            if (!IsFileExist(strSubPath.c_str())) {
                if (!CreateDirectoryA(strSubPath.c_str(), NULL))
                    return false;
            }
        }
    }

    return true;
}
bool UnzipZlib(const std::string& strZipFilename, const std::string& strTargetPath)
{
    unzFile hUnzip = unzOpen(strZipFilename.c_str());
    if (!hUnzip)
        return false;

    do {
        char szConFilename[512];
        unz_file_info fileInfo;
        if (unzGetCurrentFileInfo(hUnzip, &fileInfo, szConFilename, sizeof szConFilename, NULL, 0, NULL, 0) != UNZ_OK)
            break;

        int nLen = strlen(szConFilename);
        for (int i = 0; i < nLen; ++i) {
            if (szConFilename[i] == '/') {
                szConFilename[i] = '\\';
            }
        }

        if (nLen >= 2 && !IsShiftJIS(szConFilename[nLen - 2]) && szConFilename[nLen - 1] == '\\') {
            CreateDirectoryReflex(strTargetPath + szConFilename);
            continue;
        }

        if (unzOpenCurrentFile(hUnzip) != UNZ_OK)
            break;
        CreateDirectoryReflex(strTargetPath + szConFilename);
        HANDLE hFile = CreateFileA((strTargetPath + szConFilename).c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        BYTE szBuffer[8192];
        DWORD dwSizeRead;
        while ((dwSizeRead = unzReadCurrentFile(hUnzip, szBuffer, sizeof szBuffer)) > 0) {
            WriteFile(hFile, szBuffer, dwSizeRead, &dwSizeRead, NULL);
        }
        FlushFileBuffers(hFile);
        CloseHandle(hFile);
        unzCloseCurrentFile(hUnzip);

    } while (unzGoToNextFile(hUnzip) != UNZ_END_OF_LIST_OF_FILE);

    unzClose(hUnzip);
    return true;
}
bool ExtractZip(const TCHAR* ZipPath, const TCHAR* OutPath)
{
    IShellDispatch* pShellDisp;
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellDisp));
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }
    VARIANT vDtcDir;
    Folder* pOutDtc;

    VariantInit(&vDtcDir);
    vDtcDir.vt = VT_BSTR;
    vDtcDir.bstrVal = SysAllocString(OutPath);
    hr = pShellDisp->NameSpace(vDtcDir, &pOutDtc);
    VariantClear(&vDtcDir);
    if (hr != S_OK) {
        MessageBox(NULL, TEXT("error : Dist folder missing"), NULL, MB_ICONWARNING);
        return false;
    }

    VARIANT varZip;
    Folder* pZipFile;
    VariantInit(&varZip);
    varZip.vt = VT_BSTR;
    varZip.bstrVal = SysAllocString(ZipPath);
    hr = pShellDisp->NameSpace(varZip, &pZipFile);
    VariantClear(&varZip);
    if (hr != S_OK) {
        pOutDtc->Release();
        MessageBox(NULL, TEXT("error : zip file missing"), NULL, MB_ICONWARNING);
        return false;
    }

    FolderItems* pZipItems;
    hr = pZipFile->Items(&pZipItems);
    if (hr != S_OK) {
        pZipFile->Release();
        pOutDtc->Release();
        return false;
    }
    VARIANT vDisp, vOpt;
    VariantInit(&vDisp);
    vDisp.vt = VT_DISPATCH;
    vDisp.pdispVal = pZipItems;
    VariantInit(&vOpt);
    vOpt.vt = VT_I4;
    vOpt.lVal = 0;

    hr = pOutDtc->CopyHere(vDisp, vOpt);
    if (hr != S_OK) {
        pZipItems->Release();
        pZipFile->Release();
        pOutDtc->Release();
        MessageBox(NULL, TEXT("unzip error"), NULL, MB_ICONWARNING);
        return false;
    }

    pZipItems->Release();
    pZipFile->Release();
    pOutDtc->Release();
    CoUninitialize();

    return true;
}
bool Unzip(const std::wstring& strZipFilename, const std::wstring& strTargetPath, bool shellapi)
{
    if (shellapi) {
        return ExtractZip(strZipFilename.c_str(), strTargetPath.c_str());
    } else {
        std::string mpath = WStringToString(strZipFilename);
        std::string mrootdir = WStringToString(strTargetPath);
        return UnzipZlib(mpath, mrootdir);
    }
    return false;
}
bool Unzip(const std::string& strZipFilename, const std::string& strTargetPath)
{
    return UnzipZlib(strZipFilename, strTargetPath);
}
