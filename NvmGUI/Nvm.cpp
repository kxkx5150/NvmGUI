//#define _CRT_SECURE_NO_WARNINGS

#include "Nvm.h"
#include "Resource.h"
#include <CommCtrl.h>
#include <crtdbg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>
#pragma comment(lib, "Comctl32.lib")
#include "PointInfo.h"

LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

extern Nvm* g_nvm;
Nvm::Nvm(HWND hwnd, HINSTANCE hInst)
    : m_hwnd(hwnd)
    , m_hInst(hInst)
{
    set_regkey(HKEY_CURRENT_USER, L"SOFTWARE\\NVM_GUI_kxkx5150");
    init();
}
Nvm::~Nvm()
{
    RemoveWindowSubclass(m_hwnd, SubclassWindowProc, 0);
    write_setting_csv();
    clear_nodes();
    clear_install_nodes();
}
void Nvm::init()
{
    TCHAR path[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, path);
    wsprintf(path, TEXT("%s\\nvm_gui\\"), path);
    if (!PathFileExists(path)) {
        if (!CreateDirectory(path, NULL)) {
            OutputDebugString(L"error");
            return;
        }
    }

    ULONG qReparseTag = 0;
    std::wstring strLinkPath = L"";
    std::wstring strDispName = L"";
    GetReparsePointInfo(m_node_path.c_str(), qReparseTag, strLinkPath, strDispName);

    std::wstring rootpath = path;
    std::transform(strDispName.begin(), strDispName.end(), strDispName.begin(), std::tolower);
    std::transform(rootpath.begin(), rootpath.end(), rootpath.begin(), std::tolower);
    m_app_path = rootpath;

    if (0 == strDispName.find(rootpath)) {
        if (strDispName.length() == rootpath.length())
            return;

        std::wstring fname = strDispName.substr(rootpath.length());
        if (PathFileExists(strDispName.c_str()) && PathIsDirectory(strDispName.c_str())) {
            m_active_version = fname;
        } else {
            return;
        }
    }
}
void Nvm::clear_nodes()
{
    for (size_t i = 0; i < m_nodes.size(); i++) {
        delete m_nodes[i];
    }
    m_nodes.clear();
    m_lts_nodes.clear();
    m_sec_nodes.clear();
    m_lts_sec_nodes.clear();
}
void Nvm::clear_install_nodes()
{
    for (size_t i = 0; i < m_installed_list.size(); i++) {
        delete m_installed_list[i];
    }
    m_installed_list.clear();
}
void Nvm::download_available_list()
{
    if (m_dl_ok) {
        click_dllist_btn();
    } else {
        m_dl_ok = true;
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
                click_dllist_btn();
            });
        try {
            resp.wait();
        } catch (const std::exception& e) {
            _RPTN(_CRT_WARN, "Error exception:%s\n", e.what());
        }
    }
}
void Nvm::parse_available_list(json::value& jsonobj)
{
    clear_nodes();
    if (jsonobj.is_null())
        return;

    json::array array = jsonobj.as_array();

    for (size_t i = 0; i < array.size(); i++) {
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

        for (size_t j = 0; j < filetypes.size(); j++) {
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
                this,
                item[L"version"].as_string(),
                item[L"npm"].as_string(),
                ltsstr,
                secstr,
                item[L"modules"].as_string(),
                m_dl_progress);

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
void Nvm::set_progress_value(ULONG& number_entry, ULONG& cont, char* filename)
{
    int val = cont * 100 / number_entry;
    SendMessage(m_dl_progress, PBM_SETPOS, (WPARAM)val, 0);
}
void Nvm::add_installed_list(std::wstring verstr, std::wstring npmstr, std::wstring ltsstr,
    std::wstring secstr, std::wstring modstr, bool x86, bool actflg)
{
    Node* node = new Node(
        this,
        verstr,
        npmstr,
        ltsstr,
        secstr,
        modstr,
        m_dl_progress);

    std::wstring lbl = node->m_version + L"   ";
    if (x86) {
        lbl += L"x86";
        node->m_install_arch = L"x86";
        if (!node->check_exists_dir(node->m_x86_dir.c_str())) {
            delete node;
            return;
        }
    } else {
        lbl += L"x64";
        node->m_install_arch = L"x64";
        if (!node->check_exists_dir(node->m_x64_dir.c_str())) {
            delete node;
            return;
        }
    }

    m_installed_list.push_back(node);
    SendMessage(m_installed_combobox, CB_ADDSTRING, 1, (LPARAM)lbl.c_str());
    EnableWindow(m_dl_install_btn, TRUE);

    if (actflg) {
        //SendMessage(m_installed_combobox, CB_SETCURSEL, (WPARAM)1, (LPARAM)0);
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
        create_dllistview_items(m_nodes);
    } else if (idx == 2) {
        create_dllistview_items(m_lts_nodes);

    } else if (idx == 3) {
        create_dllistview_items(m_sec_nodes);

    } else if (idx == 4) {
        create_dllistview_items(m_lts_sec_nodes);
    }
}
void Nvm::click_dlinstall_btn()
{
    int idx = ListView_GetNextItem(m_dl_listview, -1, LVNI_SELECTED);
    if (idx != -1) {
        ListView_SetItemState(m_dl_listview, -1, 0, LVIS_SELECTED);
        int itemidx = idx / 2;
        Node* node = m_current_dllist[itemidx];
        bool x86 = idx % 2;
        bool instcheck = false;
        for (size_t i = 0; i < m_installed_list.size(); i++) {
            Node* instnode = m_installed_list[i];
            if (x86) {
                instcheck = 0 == node->m_x86_dir.find(instnode->m_x86_dir) ? true : false;
            } else {
                instcheck = 0 == node->m_x64_dir.find(instnode->m_x64_dir) ? true : false;
            }

            if (instcheck) {
                SendMessage(m_installed_combobox, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
                return;
            }
        }

        EnableWindow(m_dl_install_btn, FALSE);
        ULONG max = 1;
        ULONG cnt = 0;
        set_progress_value(max, cnt, NULL);
        BOOL rval = node->download_node(x86);
        if (!rval) {
            EnableWindow(m_dl_install_btn, TRUE);
        }
    }
}
void Nvm::click_delete_installed_btn()
{
    int idx = SendMessage(m_installed_combobox, CB_GETCURSEL, 0, 0);
    if (idx != -1) {
        Node* node = m_installed_list[idx];
        std::wstring archstr = node->m_install_arch;
        bool x86 = 0 == archstr.find(L"x86") ? true : false;
        SendMessage(m_installed_combobox, CB_SETCURSEL, -1, 0);
        SendMessage(m_installed_combobox, CB_DELETESTRING, idx, 0);
        m_installed_list.erase(m_installed_list.begin() + idx);
        node->remove_dir(x86);
        delete node;
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
    SendMessage(m_installed_combobox, WM_SETFONT, (WPARAM)m_headFont, MAKELPARAM(FALSE, 0));
    SendMessage(m_installed_usebtn, WM_SETFONT, (WPARAM)m_20Font, MAKELPARAM(FALSE, 0));
    SendMessage(m_installed_deletebtn, WM_SETFONT, (WPARAM)m_20Font, MAKELPARAM(FALSE, 0));
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
void Nvm::create_dllistview_items(std::vector<Node*> nodes)
{
    m_current_dllist.clear();
    for (size_t i = 0; i < nodes.size(); i++) {
        create_listview_item(m_dl_listview, nodes[i], i * 2, L"x64");
        create_listview_item(m_dl_listview, nodes[i], i * 2 + 1, L"x86");
        m_current_dllist.push_back(nodes[i]);
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
HWND Nvm::create_progress(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id)
{
    return CreateWindowEx(0,
        PROGRESS_CLASS, TEXT(""),
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        nX, nY, nWidth, nHeight,
        hParent, (HMENU)id, m_hInst, NULL);
}
HWND Nvm::create_statictxt(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id, const TCHAR* txt)
{
    return CreateWindow(
        TEXT("STATIC"), txt,
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_CENTER,
        nX, nY, nWidth, nHeight,
        hParent, (HMENU)id, m_hInst, NULL);
}
void Nvm::create_control()
{
    InitCommonControls();

    m_headFont = create_font(23);
    m_20Font = create_font(20);
    m_18Font = create_font(18);
    m_15Font = create_font(15);

    m_dl_combobox = create_combobox(m_hwnd, 2, 7, 200, 200, IDC_DL_COMBOBOX);
    m_dl_get_btn = create_button(m_hwnd, 206, 6, 80, 24, IDC_DL_BUTTON, L"Get List");
    m_dl_listview = create_listview(m_hwnd, 2, 38, 400, 180, IDC_DL_LISTVIEW);
    m_dl_install_btn = create_button(m_hwnd, 102, 222, 200, 36, IDC_DL_INSTALL, L"Install");
    m_dl_progress = create_progress(m_hwnd, 103, 270, 200, 16, IDC_DL_PROGRESS);

    HWND insttxt = create_statictxt(m_hwnd, 0, 320, 404, 28, IDC_INST_TXT, L"Installed List");
    SendMessage(insttxt, WM_SETFONT, (WPARAM)m_headFont, MAKELPARAM(FALSE, 0));
    m_installed_combobox = create_combobox(m_hwnd, 65, 360, 272, 200, IDC_INST_LIST);
    m_installed_usebtn = create_button(m_hwnd, 62, 416, 120, 36, IDC_INST_USE, L"Use");
    m_installed_deletebtn = create_button(m_hwnd, 216, 416, 120, 36, IDC_INST_DELETE, L"Delete");

    SetWindowSubclass(m_hwnd, &SubclassWindowProc, 0, 0);
    set_font();

    SendMessage(m_dl_combobox, CB_ADDSTRING, 0, (LPARAM)L"");
    SendMessage(m_dl_combobox, CB_ADDSTRING, 1, (LPARAM)L"All");
    SendMessage(m_dl_combobox, CB_ADDSTRING, 2, (LPARAM)L"LTS");
    SendMessage(m_dl_combobox, CB_ADDSTRING, 3, (LPARAM)L"Security");
    SendMessage(m_dl_combobox, CB_ADDSTRING, 4, (LPARAM)L"LTS & Security");
    SendMessage(m_dl_combobox, CB_SETCURSEL, 0, 0);

    std::wstring disablestr = get_regval(HKEY_CURRENT_USER, L"SOFTWARE\\NVM_GUI_kxkx5150", L"disabled");
    if (0 != disablestr.find(L"false") && 0 < disablestr.length()) {
        toggle_disabled(m_hwnd, true);
        m_active_version = disablestr;
    }
    read_setting_csv();
}
void Nvm::click_use_btn()
{
    ULONG max = 10;
    ULONG cnt = 0;
    set_progress_value(max, cnt, NULL);

    int idx = SendMessage(m_installed_combobox, CB_GETCURSEL, 0, 0);
    if (idx != -1) {
        EnableWindow(m_installed_usebtn, FALSE);
        Node* instnode = m_installed_list[idx];
        Sleep(50);
        cnt = 2;
        set_progress_value(max, cnt, NULL);

        if (check_env_path()) {
            Sleep(50);
            cnt = 4;
            set_progress_value(max, cnt, NULL);

            check_node_path_type(instnode);

            Sleep(50);
            cnt = 7;
            set_progress_value(max, cnt, NULL);
            Sleep(50);
            cnt = 10;
            set_progress_value(max, cnt, NULL);
        }
    }
    EnableWindow(m_installed_usebtn, TRUE);
}
void Nvm::check_node_path_type(Node* instnode)
{
    ULONG qReparseTag = 0;
    std::wstring strLinkPath = L"";
    std::wstring strDispName = L"";
    GetReparsePointInfo(m_node_path.c_str(), qReparseTag, strLinkPath, strDispName);

    if (IO_REPARSE_TAG_SYMLINK == qReparseTag) {
        RemoveDirectory(m_node_path.c_str());
        create_symlink(instnode);
    } else if (PathFileExists(m_node_path.c_str()) && PathIsDirectory(m_node_path.c_str())) {
        if (move_original_node()) {
            create_symlink(instnode);
        }
    } else {
        create_symlink(instnode);
    }
}

bool Nvm::move_original_node()
{
    std::wstring prefix = L"";
    std::wstring distpath = L"";
    std::wstring fname = L"";

    int idx = m_node_path.find_last_of(L"\\");
    idx++;
    fname = m_node_path.substr(idx);
    distpath = m_node_path.substr(0, idx);

    for (size_t i = 0; i < 100; i++) {
        prefix += L"_";
        std::wstring path = distpath + fname + prefix + L"original";
        if (PathFileExists(path.c_str()) && PathIsDirectory(path.c_str())) {
            continue;
        } else {
            MoveFile(m_node_path.c_str(), path.c_str());
            return true;
        }
    }
    return false;
}
void Nvm::create_symlink(Node* instnode)
{
    std::wstring nodename = L"node-" + instnode->m_version + L"-win-";
    if (instnode->m_install_arch.find(L"x86") == 0) {
        int rt = CreateSymbolicLink(m_node_path.c_str(),
            instnode->m_x86_dir.c_str(), 1);
        nodename += L"x86";
    } else {
        int rt = CreateSymbolicLink(m_node_path.c_str(),
            instnode->m_x64_dir.c_str(), 1);
        nodename += L"x64";
    }

    m_active_version = nodename;
}
void Nvm::revert_symlink()
{
    if (m_active_version.length() < 4)
        return;

    bool x86 = true;
    if (std::wstring::npos != m_active_version.find(L"x64")) {
        x86 = false;
    }

    std::wstring actpath = m_app_path + m_active_version;
    std::transform(actpath.begin(), actpath.end(), actpath.begin(), std::tolower);

    for (size_t i = 0; i < m_installed_list.size(); i++) {
        Node* instnode = m_installed_list[i];
        std::wstring instpath = instnode->m_x86_dir;
        if (!x86) {
            instpath = instnode->m_x64_dir;
        }
        std::transform(instpath.begin(), instpath.end(), instpath.begin(), std::tolower);
        if (std::wstring::npos != actpath.find(instpath)) {
            check_node_path_type(instnode);
            return;
        }
    }
}
bool Nvm::revert_original_node()
{
    ULONG qReparseTag = 0;
    std::wstring strLinkPath = L"";
    std::wstring strDispName = L"";
    GetReparsePointInfo(m_node_path.c_str(), qReparseTag, strLinkPath, strDispName);
    std::transform(strDispName.begin(), strDispName.end(), strDispName.begin(), std::tolower);

    std::wstring orgnodepath = m_node_path + L"_original";

    if (IO_REPARSE_TAG_SYMLINK == qReparseTag) {
        if (0 == strDispName.find(m_app_path)) {
            RemoveDirectory(m_node_path.c_str());
        }
    } else if (PathFileExists(m_node_path.c_str()) && PathIsDirectory(m_node_path.c_str())) {
        return false;
    }

    if (PathFileExists(orgnodepath.c_str()) && PathIsDirectory(orgnodepath.c_str())) {
        MoveFile(orgnodepath.c_str(), m_node_path.c_str());
        return true;
    }
    return false;
}
bool Nvm::check_env_path()
{
    TCHAR* buf = nullptr;
    size_t sz = 0;
    if (_wdupenv_s(&buf, &sz, L"PATH") == 0 && buf != nullptr) {
        std::wstring pathstr = buf;
        std::wstring nodpath = m_node_path;
        std::transform(pathstr.begin(), pathstr.end(), pathstr.begin(), std::toupper);
        std::transform(nodpath.begin(), nodpath.end(), nodpath.begin(), std::toupper);

        if (std::wstring::npos == pathstr.find(nodpath) && std::wstring::npos == pathstr.find(L"\%NVM_GUI_SYMLINK\%")) {
            free(buf);
            if (false) {
                ShellExecute(NULL, L"runas", L"cmd.exe", L"/C setreg.bat", NULL, SW_HIDE);
            } else {
                std::wstring envpath = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
                set_regval(HKEY_LOCAL_MACHINE, envpath, L"NVM_GUI_SYMLINK", m_node_path);
                add_regval(HKEY_LOCAL_MACHINE, envpath, L"PATH", L"\%NVM_GUI_SYMLINK\%;");
            }

            MessageBox(NULL, TEXT("Please restart Windows"), TEXT("Nvm GUI"), MB_ICONINFORMATION);
            send_change_reg_msg();
            return true;
        } else {
            free(buf);
            return true;
        }
    }

    return false;
}
std::wstring Nvm::get_regval(HKEY hKey, std::wstring prntkey, std::wstring keystr)
{
    std::wstring strvalue = L"false";
    HKEY newValue;
    if (RegOpenKey(hKey, prntkey.c_str(), &newValue) != ERROR_SUCCESS)
        return strvalue;

    DWORD dwType = REG_SZ;
    TCHAR szBuffer[2048] = {};
    DWORD dwBufferSize = sizeof(szBuffer);
    if (ERROR_SUCCESS == RegQueryValueEx(newValue, keystr.c_str(), 0, &dwType, (LPBYTE)szBuffer, &dwBufferSize)) {
        strvalue = szBuffer;
    }
    RegCloseKey(hKey);
    return strvalue;
}
bool Nvm::set_regval(HKEY hKey, std::wstring prntkey, std::wstring keystr, std::wstring valstr)
{
    HKEY newValue;
    BOOL rflg = FALSE;
    if (RegOpenKey(hKey, prntkey.c_str(), &newValue) != ERROR_SUCCESS)
        return rflg;

    DWORD valbytes = valstr.size() * sizeof(wchar_t);
    if (RegSetValueEx(newValue, keystr.c_str(), 0, REG_SZ, (LPBYTE)valstr.c_str(), valbytes) == ERROR_SUCCESS) {
        rflg = TRUE;
        BOOL bRet = SetEnvironmentVariable(keystr.c_str(), valstr.c_str());
    }
    RegCloseKey(newValue);
    return rflg;
}
bool Nvm::add_regval(HKEY hKey, std::wstring prntkey, std::wstring keystr, std::wstring valstr)
{
    HKEY newValue;
    BOOL rflg = FALSE;
    if (RegOpenKey(hKey, prntkey.c_str(), &newValue) != ERROR_SUCCESS)
        return rflg;

    DWORD dwType = REG_SZ;
    TCHAR szBuffer[2048] = {};
    DWORD dwBufferSize = sizeof(szBuffer);
    if (RegQueryValueEx(newValue, keystr.c_str(), 0, &dwType, (LPBYTE)szBuffer, &dwBufferSize) == ERROR_SUCCESS) {
        std::wstring bufstr = szBuffer;
        if (std::wstring::npos != bufstr.find(valstr)) {
            RegCloseKey(newValue);
            return rflg;
        }
        valstr += bufstr;
        DWORD regvalbytes = valstr.size() * sizeof(wchar_t);
        if (RegSetValueEx(newValue, keystr.c_str(), 0, REG_SZ, (LPBYTE)valstr.c_str(), regvalbytes) == ERROR_SUCCESS) {
            rflg = TRUE;
            BOOL bRet = SetEnvironmentVariable(keystr.c_str(), valstr.c_str());
        }
    }
    RegCloseKey(newValue);
    return rflg;
}
bool Nvm::set_regkey(HKEY hKey, std::wstring keystr)
{
    HKEY usrhkey;
    DWORD dwDisposition;
    if (RegOpenKey(hKey, keystr.c_str(), &usrhkey) != ERROR_SUCCESS) {
        DWORD Ret = RegCreateKeyEx(hKey, keystr.c_str(), 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
        if (Ret != ERROR_SUCCESS)
            return false;
    }
    RegCloseKey(usrhkey);
    return true;
}
void Nvm::send_change_reg_msg()
{
    DWORD_PTR sndmsgrval;
    LRESULT Ret = SendMessageTimeout(
        HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)TEXT("Environment"),
        SMTO_ABORTIFHUNG, 5000, &sndmsgrval);
}
void Nvm::toggle_disabled(HWND hwnd, bool noSave)
{
    HMENU hmenu = GetMenu(hwnd);
    UINT uState = GetMenuState(hmenu, ID_MENU_DISABLED, MF_BYCOMMAND);
    std::wstring valstr = m_node_path;
    std::wstring regval = L"false";

    if (uState) {
        EnableWindow(m_dl_combobox, TRUE);
        EnableWindow(m_dl_get_btn, TRUE);
        EnableWindow(m_dl_install_btn, TRUE);
        //EnableWindow(m_installed_combobox, TRUE);
        EnableWindow(m_installed_usebtn, TRUE);
        EnableWindow(m_installed_deletebtn, TRUE);
        move_original_node();
        revert_symlink();
        CheckMenuItem(hmenu, ID_MENU_DISABLED, MF_BYCOMMAND | MFS_UNCHECKED);
    } else {
        EnableWindow(m_dl_combobox, FALSE);
        EnableWindow(m_dl_get_btn, FALSE);
        EnableWindow(m_dl_install_btn, FALSE);
        //EnableWindow(m_installed_combobox, FALSE);
        EnableWindow(m_installed_usebtn, FALSE);
        EnableWindow(m_installed_deletebtn, FALSE);
        valstr = L"C:\\Program Files\\nodejs_disabled";
        CheckMenuItem(hmenu, ID_MENU_DISABLED, MF_BYCOMMAND | MFS_CHECKED);
        revert_original_node();
        regval = m_active_version;
    }
    if (noSave)
        return;

    std::wstring envpath = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    set_regval(HKEY_LOCAL_MACHINE, envpath, L"NVM_GUI_SYMLINK", valstr);
    set_regval(HKEY_CURRENT_USER, L"SOFTWARE\\NVM_GUI_kxkx5150", L"disabled", regval);
    send_change_reg_msg();
}
void Nvm::exe_directory_path(TCHAR* path)
{
    GetModuleFileName(NULL, path, MAX_PATH);
    TCHAR* ptmp = _tcsrchr(path, _T('\\'));
    if (ptmp != NULL) {
        ptmp = _tcsinc(ptmp);
        *ptmp = '\0';
    }
}
void Nvm::write_setting_csv()
{
    std::wstring argstr = L"";
    for (auto&& node : m_installed_list) {
        argstr += node->m_version;
        argstr += L",";
        argstr += node->m_npm;
        argstr += L",";
        argstr += node->m_lts;
        argstr += L",";
        argstr += node->m_security;
        argstr += L",";
        argstr += node->m_modules;
        argstr += L",";
        argstr += node->m_install_arch;
        argstr += L"\n";
    }

    int strlen = argstr.length() + 10;
    TCHAR* filename = new TCHAR[MAX_PATH];
    TCHAR* buftmp = new TCHAR[strlen];
    wcscpy_s(filename, MAX_PATH, L"settings.csv");
    wcscpy_s(buftmp, strlen, argstr.c_str());
    write_file(filename, buftmp);
    delete[] filename;
    delete[] buftmp;
}
int Nvm::write_file(TCHAR* filename, TCHAR* args)
{
    TCHAR path[MAX_PATH] = { '\0' };
    exe_directory_path(path);
    wcscat_s(path, MAX_PATH, filename);
    wcscpy_s(filename, MAX_PATH, path);

    HANDLE hFile = CreateFile(path,
        GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;
    CloseHandle(hFile);

    hFile = CreateFile(path,
        GENERIC_WRITE, 0, NULL,
        TRUNCATE_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    DWORD written;
    WriteFile(hFile, args, _tcslen(args) * sizeof(TCHAR), &written, NULL);
    CloseHandle(hFile);
    return 0;
}
TCHAR* Nvm::read_file(const TCHAR* filename)
{
    TCHAR m_Path[MAX_PATH] = { '\0' };
    exe_directory_path(m_Path);
    wcscat_s(m_Path, MAX_PATH, filename);

    HANDLE hFile = CreateFile(m_Path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    DWORD dfsize = ::GetFileSize(hFile, NULL);
    TCHAR* argstmp = new TCHAR[dfsize + 10];
    DWORD wReadSize;
    BOOL ret = ReadFile(hFile, argstmp, dfsize, &wReadSize, NULL);
    CloseHandle(hFile);
    if (ret)
        return argstmp;
    else {
        return nullptr;
    }
}
void Nvm::read_setting_csv()
{
    TCHAR* stradd = read_file(L"settings.csv");
    if (!stradd)
        return;

    std::wstring txt = stradd;
    std::vector<std::wstring> str = split(txt, '\n');
    size_t len = str.size();
    int selidx = -1;
    int itemcnt = -1;

    for (size_t i = 0; i < len; i++) {
        std::vector<std::wstring> strvec = split(str.at(i), ',');
        int csvlen = strvec.size();
        if (csvlen < 6)
            continue;
        itemcnt++;
        std::wstring verstr = strvec.at(0);
        std::wstring npmstr = strvec.at(1);
        std::wstring ltsstr = strvec.at(2);
        std::wstring secstr = strvec.at(3);
        std::wstring modstr = strvec.at(4);
        std::wstring archstr = strvec.at(5);
        bool x86 = 0 == archstr.find(L"x86") ? true : false;

        bool actflg = false;
        std::wstring nodename = L"node-" + verstr + L"-win-";
        nodename += x86 ? L"x86" : L"x64";
        if (0 == m_active_version.find(nodename)) {
            selidx = itemcnt;
            actflg = true;
        }
        add_installed_list(verstr, npmstr, ltsstr, secstr, modstr, x86, actflg);
    }
    if (-1 < selidx) {
        SendMessage(m_installed_combobox, CB_SETCURSEL, (WPARAM)selidx, (LPARAM)0);
    }

    delete[] stradd;
}
std::vector<std::wstring> Nvm::split(std::wstring& input, TCHAR delimiter)
{
    std::wistringstream stream(input);
    std::wstring field;
    std::vector<std::wstring> result;
    while (getline(stream, field, delimiter)) {
        result.push_back(field);
    }
    return result;
}
LRESULT CALLBACK SubclassWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg) {
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {

        case IDC_DL_BUTTON: {
            g_nvm->download_available_list();
        } break;

        case IDC_DL_INSTALL: {
            g_nvm->click_dlinstall_btn();
        } break;

        case IDC_INST_DELETE: {
            g_nvm->click_delete_installed_btn();
        } break;

        case IDC_INST_USE: {
            g_nvm->click_use_btn();
        } break;
        }
    } break;

    case WM_CTLCOLORSTATIC: {
        DWORD CtrlID = GetDlgCtrlID((HWND)lParam);
        if (CtrlID == IDC_INST_TXT) {
            static HBRUSH hBrush = CreateSolidBrush(RGB(0, 230, 120));
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(0, 0, 0));
            SetBkColor(hdcStatic, RGB(0, 230, 120));
            return (INT_PTR)hBrush;
        }
    } break;

    case WM_NCCREATE: {
        break;
    }

    case WM_NCDESTROY: {
        DeleteObject(g_nvm->m_headFont);
        DeleteObject(g_nvm->m_15Font);
        DeleteObject(g_nvm->m_18Font);
        DeleteObject(g_nvm->m_20Font);
        delete g_nvm;
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
