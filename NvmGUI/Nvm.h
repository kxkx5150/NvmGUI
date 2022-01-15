#pragma once
#include "Node.h"
#include <Windows.h>
#include <cpprest/http_client.h>
#include <string>
#include <vector>

using namespace web;
using namespace web::http;
using namespace web::http::client;

class Nvm {

    HINSTANCE m_hInst = nullptr;
    HWND m_hwnd = nullptr;

    HWND m_dl_listview = nullptr;
    HWND m_dl_combobox = nullptr;
    HWND m_dl_get_btn = nullptr;
    HWND m_dl_install_btn = nullptr;


    HFONT m_20Font = nullptr;
    HFONT m_18Font = nullptr;
    HFONT m_15Font = nullptr;

    std::vector<Node*> m_nodes;
    std::vector<Node*> m_lts_nodes;
    std::vector<Node*> m_sec_nodes;
    std::vector<Node*> m_lts_sec_nodes;

    std::wstring m_json_url = L"https://nodejs.org/dist/index.json";

public:
    void parse_available_list(json::value& jsonobj);

public:
    Nvm(HWND hwnd, HINSTANCE hInst);
    ~Nvm();

    void init();
    void download_available_list();

    void create_control();

    int get_nodes_len();
    void clear_nodes();

    void click_dllist_btn();

private:
    HFONT create_font(int fontsize);
    void set_font();
    HWND create_listview(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id);

    void create_listview_items(std::vector<Node*> nodes);
    void create_listview_item(HWND lstvhwnd, Node* node, int idx, const TCHAR* arch);
    HWND create_combobox(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id);
    HWND create_button(HWND hParent, int nX, int nY, int nWidth, int nHeight, int id, const TCHAR* txt);
};
