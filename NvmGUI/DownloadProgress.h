#pragma once
#include <CommCtrl.h>
#include <Windows.h>

class DownloadProgress : public IBindStatusCallback {
    HWND m_progress = nullptr;

public:
    HRESULT __stdcall QueryInterface(const IID&, void**)
    {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release(void)
    {
        return 1;
    }
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding* pib)
    {
        return E_NOTIMPL;
    }
    ULONG STDMETHODCALLTYPE SetHWND(HWND hwnd)
    {
        m_progress = hwnd;
        return 1;
    }
    virtual HRESULT STDMETHODCALLTYPE GetPriority(LONG* pnPriority)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved)
    {
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown* punk)
    {
        return E_NOTIMPL;
    }
    virtual HRESULT __stdcall OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
    {
        if (0 < ulProgressMax) {
            int val = ulProgress * 100 / ulProgressMax;
            SendMessage(m_progress, PBM_SETPOS, (WPARAM)val, 0);
        }
        return S_OK;
    }
};
