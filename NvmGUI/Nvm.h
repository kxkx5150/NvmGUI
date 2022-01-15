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

    HINSTANCE m_hIns = nullptr;
    HWND m_hwnd = nullptr;
    HWND m_dl_listview = nullptr;

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

private:
    HWND create_listview(int nX, int nY, int nWidth, int nHeight, int id);

    void create_listview_items();
    void create_listview_item(HWND lstvhwnd, Node* node);
};
