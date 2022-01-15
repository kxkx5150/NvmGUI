﻿#include "Nvm.h"
#include "Resource.h"
#include <CommCtrl.h>
#include <commctrl.h>
#include <crtdbg.h>
#include <shlwapi.h>
#pragma comment(lib, "Comctl32.lib")

LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

Nvm* g_nvm = nullptr;
Nvm::Nvm(HWND hwnd, HINSTANCE hInst)
    : m_hwnd(hwnd)
    , m_hInst(hInst)
{
    g_nvm = this;
    init();
}
Nvm::~Nvm()
{
    clear_nodes();
    delete m_15Font;
    delete m_18Font;
}
void Nvm::init()
{
    TCHAR path[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, path);
    wsprintf(path, TEXT("%s\\nodes"), path);
    if (!PathFileExists(path)) {
        if (!CreateDirectory(path, NULL)) {
            OutputDebugString(L"error");
        }
    }
}
void Nvm::clear_nodes()
{
    for (int i = 0; i < m_nodes.size(); i++) {
        delete m_nodes[i];
    }
    m_nodes.clear();
    m_lts_nodes.clear();
    m_sec_nodes.clear();
    m_lts_sec_nodes.clear();
}
void Nvm::download_available_list()
{
    http_client client(m_json_url);
    pplx::task<web::http::http_response> resp = client.request(web::http::methods::GET);
    resp.then([=](pplx::task<web::http::http_response> task) {
            web::http::http_response response = task.get();
            if (response.status_code() != status_codes::OK) {
                throw std::runtime_error("Returned " + std::to_string(response.status_code()));
            }
            return response.extract_json();
        })
        .then([=](json::value jo) {
            parse_available_list(jo);
            //m_lts_nodes[0]->download_node();
        });
    try {
        resp.wait();
    } catch (const std::exception& e) {
        _RPTN(_CRT_WARN, "Error exception:%s\n", e.what());
    }
}
void Nvm::parse_available_list(json::value& jsonobj)
{
    clear_nodes();
    if (jsonobj.is_null())
        return;

    json::array array = jsonobj.as_array();

    for (int i = 0; i < array.size(); ++i) {
        if (array[i][L"version"].is_null() || !array[i][L"version"].is_string())
            continue;

        if (array[i][L"npm"].is_null() || !array[i][L"npm"].is_string())
            continue;

        if (array[i][L"lts"].is_null())
            continue;

        if (array[i][L"security"].is_null() || !array[i][L"security"].is_boolean())
            continue;

        if (array[i][L"modules"].is_null() || !array[i][L"modules"].is_string())
            continue;

        if (array[i][L"files"].is_null())
            continue;

        int chk = 0;
        std::wstring nodev = array[i].at(U("version")).as_string();
        web::json::array filetypes = array[i].at(U("files")).as_array();

        for (int j = 0; j < filetypes.size(); j++) {
            if (filetypes[j].is_null())
                continue;
            std::wstring fname = filetypes[j].as_string();
            if (0 == fname.find(L"win-x64-zip")) {
                chk++;
            } else if (0 == fname.find(L"win-x86-zip")) {
                chk++;
            }
        }

        if (chk == 2) {
            json::value item = array[i];
            std::wstring ltsstr = L"";
            std::wstring secstr = L"";

            if (item[L"lts"].is_string()) {
                ltsstr = item[L"lts"].as_string();
            } else {
                ltsstr = L"false";
            }

            if (item[L"security"].as_bool()) {
                secstr = L"true";
            } else {
                secstr = L"false";
            }

            Node* node = new Node(
                item[L"version"].as_string(),
                item[L"npm"].as_string(),
                ltsstr,
                secstr,
                item[L"modules"].as_string());

            m_nodes.push_back(node);
            if (0 != ltsstr.find(L"false")) {
                m_lts_nodes.push_back(node);
            }
            if (0 == secstr.find(L"true")) {
                m_sec_nodes.push_back(node);
            }
            if (0 != ltsstr.find(L"false") && 0 == secstr.find(L"true")) {
                m_lts_sec_nodes.push_back(node);
            }
        }
    }
}
int Nvm::get_nodes_len()
{
    return m_nodes.size();
}
void Nvm::click_dllist_btn()
{
    ListView_DeleteAllItems(m_dl_listview);
    int idx = SendMessage(m_dl_combobox, CB_GETCURSEL, 0, 0);
    if (idx == 1) {
        create_listview_items(m_nodes);
    } else if (idx == 2) {
        create_listview_items(m_lts_nodes);

    } else if (idx == 3) {
        create_listview_items(m_sec_nodes);

    } else if (idx == 4) {
        create_listview_items(m_lts_sec_nodes);
    }
}

HFONT Nvm::create_font(int fontsize)
{
    return CreateFont(fontsize, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, 0,
        SHIFTJIS_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
}
void Nvm::set_font()
{
    SendMessage(m_dl_combobox, WM_SETFONT, (WPARAM)m_18Font, MAKELPARAM(FALSE, 0));
    SendMessage(m_dl_get_btn, WM_SETFONT, (WPARAM)m_18Font, MAKELPARAM(FALSE, 0));
    SendMessage(m_dl_listview, WM_SETFONT, (WPARAM)m_15Font, MAKELPARAM(FALSE, 0));
    SendMessage(m_dl_install_btn, WM_SETFONT, (WPARAM)m_20Font, MAKELPARAM(FALSE, 0));
}
HWND Nvm::create_listview(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id)
{
    HWND hwnd = CreateWindowEx(0,
        WC_LISTVIEW, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL | WS_BORDER,
        nX, nY, nWidth, nHeight,
        m_hwnd, (HMENU)id, m_hInst, NULL);

    LVCOLUMN colmod;
    colmod.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    colmod.fmt = LVCFMT_LEFT;
    colmod.cx = 60;
    colmod.pszText = (LPWSTR)L"Modules";
    ListView_InsertColumn(hwnd, 0, &colmod);

    LVCOLUMN colsec;
    colsec.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    colsec.fmt = LVCFMT_LEFT;
    colsec.cx = 60;
    colsec.pszText = (LPWSTR)L"Security";
    ListView_InsertColumn(hwnd, 0, &colsec);

    LVCOLUMN collts;
    collts.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    collts.fmt = LVCFMT_LEFT;
    collts.cx = 100;
    collts.pszText = (LPWSTR)L"LTS";
    ListView_InsertColumn(hwnd, 0, &collts);

    LVCOLUMN colarch;
    colarch.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    colarch.fmt = LVCFMT_LEFT;
    colarch.cx = 40;
    colarch.pszText = (LPWSTR)L"Arch";
    ListView_InsertColumn(hwnd, 0, &colarch);

    LVCOLUMN colver;
    colver.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    colver.fmt = LVCFMT_LEFT;
    colver.cx = 120;
    colver.pszText = (LPWSTR)L"Version";
    ListView_InsertColumn(hwnd, 0, &colver);

    return hwnd;
}
void Nvm::create_listview_items(std::vector<Node*> nodes)
{
    for (int i = 0; i < nodes.size(); i++) {
        create_listview_item(m_dl_listview, nodes[i], i * 2, L"x64");
        create_listview_item(m_dl_listview, nodes[i], i * 2 + 1, L"x86");
    }
}
void Nvm::create_listview_item(HWND lstvhwnd, Node* node, int idx, const TCHAR* arch)
{
    LVITEM item = { 0 };
    item.mask = LVIF_TEXT;
    item.pszText = (LPWSTR)node->m_version.c_str();
    item.iItem = idx;
    item.iSubItem = 0;
    ListView_InsertItem(lstvhwnd, &item);

    item.pszText = (LPWSTR)arch;
    item.iItem = idx;
    item.iSubItem = 1;
    ListView_SetItem(lstvhwnd, &item);

    item.pszText = (LPWSTR)node->m_lts.c_str();
    item.iItem = idx;
    item.iSubItem = 2;
    ListView_SetItem(lstvhwnd, &item);

    item.pszText = (LPWSTR)node->m_security.c_str();
    item.iItem = idx;
    item.iSubItem = 3;
    ListView_SetItem(lstvhwnd, &item);

    item.pszText = (LPWSTR)node->m_modules.c_str();
    item.iItem = idx;
    item.iSubItem = 4;
    ListView_SetItem(lstvhwnd, &item);
}
HWND Nvm::create_combobox(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id)
{
    return CreateWindow(
        L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | WS_CLIPSIBLINGS | WS_VSCROLL,
        nX, nY, nWidth, nHeight,
        hParent, (HMENU)id, m_hInst, NULL);
}
HWND Nvm::create_button(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id, const TCHAR* txt)
{
    return CreateWindow(
        L"BUTTON", txt,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        nX, nY, nWidth, nHeight,
        hParent, (HMENU)id, m_hInst, NULL);
}
void Nvm::create_control()
{
    m_20Font = create_font(20);
    m_18Font = create_font(18);
    m_15Font = create_font(15);

    m_dl_combobox = create_combobox(m_hwnd, 2, 8, 200, 200, IDC_DL_COMBOBOX);
    m_dl_get_btn = create_button(m_hwnd, 206, 7, 80, 24, IDC_DL_BUTTON, L"Get List");
    m_dl_listview = create_listview(m_hwnd, 2, 38, 400, 200, IDC_DL_LISTVIEW);
    m_dl_install_btn = create_button(m_hwnd, 98, 246, 200, 40, IDC_DL_BUTTON, L"Install");
    SetWindowSubclass(m_hwnd, &SubclassWindowProc, 0, 0);

    set_font();

    SendMessage(m_dl_combobox, CB_ADDSTRING, 0, (LPARAM)L"");
    SendMessage(m_dl_combobox, CB_ADDSTRING, 1, (LPARAM)L"All");
    SendMessage(m_dl_combobox, CB_ADDSTRING, 2, (LPARAM)L"LTS");
    SendMessage(m_dl_combobox, CB_ADDSTRING, 3, (LPARAM)L"Security");
    SendMessage(m_dl_combobox, CB_ADDSTRING, 4, (LPARAM)L"LTS & Security");
    SendMessage(m_dl_combobox, CB_SETCURSEL, 0, 0);
}
LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg) {
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {

        case IDC_DL_BUTTON: {
            g_nvm->click_dllist_btn();
        } break;
        }
    } break;

    case WM_NCDESTROY:
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
