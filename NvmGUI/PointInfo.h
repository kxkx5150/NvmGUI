#pragma once
#include <Shlobj.h>
#include <iostream>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#ifndef REPARSE_DATA_BUFFER
typedef struct _REPARSE_DATA_BUFFER {
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#endif

HRESULT GetReparsePointInfo(
    LPCWSTR pFilePath,
    ULONG& qReparseTag,
    std::wstring& strLinkPath,
    std::wstring& strDispName)
{
    BOOL bRet = 0;
    HRESULT hResult = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwBufferSize = 1024 * 64;
    BYTE* bpInfoBuffer = NULL;
    BOOL bReparsePoint = FALSE;
    qReparseTag = 0;
    DWORD dwFileAttr = ::GetFileAttributes(pFilePath);
    if (-1 == dwFileAttr) {
        hResult = ::HRESULT_FROM_WIN32(::GetLastError());
        goto err;
    }
    if (0 == (FILE_ATTRIBUTE_REPARSE_POINT & dwFileAttr)) {
        return (S_OK);
    }

    bpInfoBuffer = (BYTE*)malloc(dwBufferSize);
    if (NULL == bpInfoBuffer) {
        hResult = E_OUTOFMEMORY;
        goto err;
    }

    hFile = CreateFile(
        pFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        hResult = ::HRESULT_FROM_WIN32(::GetLastError());
        goto err;
    }
    {
        DWORD dwReadSize = 0;
        bRet = DeviceIoControl(
            hFile, FSCTL_GET_REPARSE_POINT, NULL, 0, bpInfoBuffer, dwBufferSize, &dwReadSize, NULL);
        REPARSE_DATA_BUFFER* tpReparseDataBuffer = (_REPARSE_DATA_BUFFER*)bpInfoBuffer;
        DWORD dwIsMSTag = IsReparseTagMicrosoft(tpReparseDataBuffer->ReparseTag);
        if (dwIsMSTag != 0) {
            qReparseTag = tpReparseDataBuffer->ReparseTag;
            switch (qReparseTag) {
            case IO_REPARSE_TAG_MOUNT_POINT: {
                WCHAR* wpPathBuffer = (WCHAR*)tpReparseDataBuffer->MountPointReparseBuffer.PathBuffer;
                strLinkPath = std::wstring(
                    &wpPathBuffer[tpReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset / 2], &wpPathBuffer[(tpReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset + tpReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength) / 2]);
                strDispName = std::wstring(
                    &wpPathBuffer[tpReparseDataBuffer->MountPointReparseBuffer.PrintNameOffset / 2], &wpPathBuffer[(tpReparseDataBuffer->MountPointReparseBuffer.PrintNameOffset + tpReparseDataBuffer->MountPointReparseBuffer.PrintNameLength) / 2]);
                qReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
            } break;

            case IO_REPARSE_TAG_SYMLINK: {
                WCHAR* wpPathBuffer = (WCHAR*)tpReparseDataBuffer->SymbolicLinkReparseBuffer.PathBuffer;
                strLinkPath = std::wstring(
                    &wpPathBuffer[tpReparseDataBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset / 2], &wpPathBuffer[(tpReparseDataBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset + tpReparseDataBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength) / 2]);
                strDispName = std::wstring(
                    &wpPathBuffer[tpReparseDataBuffer->SymbolicLinkReparseBuffer.PrintNameOffset / 2], &wpPathBuffer[(tpReparseDataBuffer->SymbolicLinkReparseBuffer.PrintNameOffset + tpReparseDataBuffer->SymbolicLinkReparseBuffer.PrintNameLength) / 2]);
                qReparseTag = IO_REPARSE_TAG_SYMLINK;
            } break;

            default:
                qReparseTag = 0;
                break;
            }
        }
    }

err:
    if (NULL != bpInfoBuffer) {
        ::free(bpInfoBuffer);
    }
    if (INVALID_HANDLE_VALUE != hFile) {
        ::CloseHandle(hFile);
    }
    return (hResult);
}
