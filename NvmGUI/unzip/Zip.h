#pragma once
#include <string>
#include <windows.h>

bool Unzip(const std::string& strZipFilename, const std::string& strTargetPath);
bool Unzip(const std::wstring& strZipFilename, const std::wstring& strTargetPath, bool shellapi=false);

