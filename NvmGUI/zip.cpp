#include "zip.h"
#include "unzip.h"
#pragma comment(lib, "zlib.lib")
#define IsShiftJIS(x) ((BYTE)((x ^ 0x20) - 0xA1) <= 0x3B)

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
bool Unzip(const std::string& strZipFilename, const std::string& strTargetPath)
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
