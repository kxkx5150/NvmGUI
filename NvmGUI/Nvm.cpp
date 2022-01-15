#include "Nvm.h"
#include <commctrl.h>
#include <crtdbg.h>
#include <shlwapi.h>

Nvm::Nvm(HWND hwnd, HINSTANCE hInst)
    : m_hwnd(hwnd)
    , m_hIns(hInst)
{
    init();
}
Nvm::~Nvm()
{
    clear_nodes();
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
            create_listview_items();
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

        if (array[i][L"lts"].is_null() || !array[i][L"lts"].is_string())
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
            std::wstring ltsstr = item[L"lts"].as_string();
            Node* node = new Node(
                item[L"version"].as_string(),
                item[L"npm"].as_string(),
                ltsstr,
                item[L"security"].as_bool(),
                item[L"modules"].as_string());

            m_nodes.push_back(node);
            if (0 != ltsstr.find(L"false")) {
                m_lts_nodes.push_back(node);
            }
            if (item[L"security"].as_bool()) {
                m_sec_nodes.push_back(node);
            }
            if (0 != ltsstr.find(L"false") && item[L"security"].as_bool()) {
                m_lts_sec_nodes.push_back(node);
            }
        }
    }
}
int Nvm::get_nodes_len()
{
    return m_nodes.size();
}
HWND Nvm::create_listview(int nX, int nY, int nWidth, int nHeight, int id)
{
    HWND hwnd = CreateWindowEx(0,
        WC_LISTVIEW, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL,
        nX, nY, nWidth, nHeight,
        m_hwnd, (HMENU)id, m_hIns, NULL);

    LVCOLUMN colmod;
    colmod.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    colmod.fmt = LVCFMT_LEFT;
    colmod.cx = 70;
    colmod.pszText = (LPWSTR)L"Modules";
    ListView_InsertColumn(hwnd, 0, &colmod);

    LVCOLUMN colsec;
    colsec.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    colsec.fmt = LVCFMT_LEFT;
    colsec.cx = 70;
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
    colarch.cx = 50;
    colarch.pszText = (LPWSTR)L"Arch";
    ListView_InsertColumn(hwnd, 0, &colarch);

    LVCOLUMN colver;
    colver.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    colver.fmt = LVCFMT_LEFT;
    colver.cx = 200;
    colver.pszText = (LPWSTR)L"Version";
    ListView_InsertColumn(hwnd, 0, &colver);

    return hwnd;
}
void Nvm::create_listview_items()
{
    for (int i = 0; i < m_lts_nodes.size(); i++) {
        create_listview_item(m_dl_listview, m_lts_nodes[i]);
    }
}
void Nvm::create_listview_item(HWND lstvhwnd, Node* node)
{
    LVITEM item = { 0 };
    item.mask = LVIF_TEXT;
    for (int i = 0; i < 3; i++) {
        item.pszText = (LPWSTR)node->m_version.c_str();
        item.iItem = i;
        item.iSubItem = 0;
        ListView_InsertItem(lstvhwnd, &item);

        item.pszText = (LPWSTR)L"x64";
        item.iItem = i;
        item.iSubItem = 1;
        ListView_SetItem(lstvhwnd, &item);

        item.pszText = (LPWSTR)node->m_lts.c_str();
        item.iItem = i;
        item.iSubItem = 2;
        ListView_SetItem(lstvhwnd, &item);

        item.pszText = (LPWSTR)L"";
        item.iItem = i;
        item.iSubItem = 3;
        ListView_SetItem(lstvhwnd, &item);

        item.pszText = (LPWSTR)node->m_modules.c_str();
        item.iItem = i;
        item.iSubItem = 4;
        ListView_SetItem(lstvhwnd, &item);
    }
}
void Nvm::create_control()
{
    m_dl_listview = create_listview(0, 0, 700, 400, 555);

    //m_search_listhwnd = create_listbox(m_search_grouphwnd, 5, 44, 311, 155, IDC_SEARCH_LIST);
}
