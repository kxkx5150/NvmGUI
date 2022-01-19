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

    bool m_dl_ok = false;

public:
    HWND m_dl_install_btn = nullptr;

    HFONT m_headFont = nullptr;
    HFONT m_20Font = nullptr;
    HFONT m_15Font = nullptr;
    HFONT m_18Font = nullptr;

public:
    void parse_available_list(json::value& jsonobj);

    Nvm(HWND hwnd, HINSTANCE hInst);
    ~Nvm();

    void init();
    void download_available_list();

    void create_control();

    int get_nodes_len();
    void clear_nodes();
    void clear_install_nodes();

    void click_dllist_btn();
    void click_dlinstall_btn();
    void click_delete_installed_btn();
    void click_use_btn();


    void add_installed_list(std::wstring verstr, std::wstring npmstr, std::wstring ltsstr,
        std::wstring secstr, std::wstring modstr, bool x86);

    void set_progress_value(ULONG& number_entry, ULONG& cont, char* filename);
    void write_setting_csv();

private:
    HFONT create_font(int fontsize);
    void set_font();
    HWND create_listview(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id);

    void create_dllistview_items(std::vector<Node*> nodes);
    void create_listview_item(HWND lstvhwnd, Node* node, int idx, const TCHAR* arch);
    HWND create_combobox(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id);
    HWND create_button(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id, const TCHAR* txt);
    HWND create_progress(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id);
    HWND create_statictxt(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id, const TCHAR* txt);
    int write_file(TCHAR* filename, TCHAR* args);
    void exe_directory_path(TCHAR* path);
    void read_setting_csv();
    TCHAR* read_file(const TCHAR* filename);
    std::vector<std::wstring> split(std::wstring& input, TCHAR delimiter);

    void create_symbolic_link(Node* instnode);

    bool check_env_path();
    bool create_reg_symkey();
    bool append_env_path();
    bool move_original_node();


};

