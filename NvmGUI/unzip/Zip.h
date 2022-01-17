#pragma once
#include <string>
#include <windows.h>

class UnzipCPP {
#define IsShiftJIS(x) ((BYTE)((x ^ 0x20) - 0xA1) <= 0x3B)

public:
    static bool m_shellapi;

public:
    UnzipCPP();
    ~UnzipCPP();

    static bool Unzip(const std::wstring& strZipFilename, const std::wstring& strTargetPath,
        void (*callback)(ULONG&, ULONG&, char*));

    static bool Unzip(const std::string& strZipFilename, const std::string& strTargetPath,
        void (*callback)(ULONG&, ULONG&, char*));

private:
    static std::string WStringToString(std::wstring oWString);
    static bool IsFileExist(const std::string& strFilename);
    static bool CreateDirectoryReflex(const std::string& strPath);
    static bool UnzipZlib(const std::string& strZipFilename, const std::string& strTargetPath,
        void (*callback)(ULONG&, ULONG&, char*));
    static bool ExtractZip(const TCHAR* ZipPath, const TCHAR* OutPath);
};
