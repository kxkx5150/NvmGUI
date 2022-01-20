#pragma once
#include "Node.h"
#include <Windows.h>
#include <cpprest/http_client.h>
#include <string>
#include <vector>

using namespace web;
using namespace web::http;
using namespace web::http::client;

class Node;

class Nvm {

    HINSTANCE m_hInst = nullptr;
    HWND m_hwnd = nullptr;

    HWND m_dl_listview = nullptr;
    HWND m_dl_combobox = nullptr;
    HWND m_dl_get_btn = nullptr;
    HWND m_dl_progress = nullptr;
    HWND m_installed_combobox = nullptr;
    HWND m_installed_usebtn = nullptr;
    HWND m_installed_deletebtn = nullptr;

    std::vector<Node*> m_nodes;
    std::vector<Node*> m_lts_nodes;
    std::vector<Node*> m_sec_nodes;
    std::vector<Node*> m_lts_sec_nodes;
    std::vector<Node*> m_current_dllist;
    std::vector<Node*> m_installed_list;

    std::wstring m_json_url = L"https://nodejs.org/dist/index.json";
    std::wstring m_node_path = L"C:\\Program Files\\nodejs";

    std::wstring m_active_version = L"";

    bool m_dl_ok = false;

public:
    std::wstring m_app_path = L"";
    HWND m_dl_install_btn = nullptr;

    HFONT m_headFont = nullptr;
    HFONT m_20Font = nullptr;
    HFONT m_15Font = nullptr;
    HFONT m_18Font = nullptr;

public:
    Nvm(HWND hwnd, HINSTANCE hInst);
    ~Nvm();
    void init();
    void get_node_install_path();
    void create_control();

    void download_available_list();
    void parse_available_list(json::value& jsonobj);

    void clear_nodes();
    void clear_install_nodes();

    void click_dllist_btn();
    void click_dlinstall_btn();
    void click_delete_installed_btn();
    void click_use_btn();

    void add_installed_list(std::wstring verstr, std::wstring npmstr, std::wstring ltsstr,
        std::wstring secstr, std::wstring modstr, bool x86, bool actflg = false);

    void set_progress_value(ULONG& number_entry, ULONG& cont, char* filename);
    void write_setting_csv();
    void toggle_disabled(HWND hwnd, bool noSave = false);

private:
    void set_font();
    HFONT create_font(int fontsize);

    HWND create_listview(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id);
    void create_dllistview_items(std::vector<Node*> nodes);
    void create_listview_item(HWND lstvhwnd, Node* node, int idx, const TCHAR* arch);
    HWND create_combobox(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id);
    HWND create_button(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id, const TCHAR* txt);
    HWND create_progress(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id);
    HWND create_statictxt(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id, const TCHAR* txt);

    void check_node_path_type(Node* instnode);
    bool move_original_node();
    bool revert_original_node();
    void create_symlink(Node* instnode);
    void revert_symlink();

    bool check_env_path();
    std::wstring get_regval(HKEY hKey, std::wstring prntkey, std::wstring keystr);
    bool set_regval(HKEY hKey, std::wstring prntkey, std::wstring keystr, std::wstring valstr);
    bool add_regval(HKEY hKey, std::wstring prntkey, std::wstring keystr, std::wstring valstr);
    bool set_regkey(HKEY hKey, std::wstring keystr);
    void send_change_reg_msg();

    int write_file(TCHAR* filename, TCHAR* args);
    void read_setting_csv();
    TCHAR* read_file(const TCHAR* filename);
    std::vector<std::wstring> split(std::wstring& input, TCHAR delimiter);
    void exe_directory_path(TCHAR* path);
};
