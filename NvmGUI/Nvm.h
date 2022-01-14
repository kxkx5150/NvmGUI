#pragma once
#include <cpprest/http_client.h>
#include "Node.h"
#include <Windows.h>
#include <vector>
#include <string>

using namespace web;
using namespace web::http;
using namespace web::http::client;

class Nvm {

    HWND m_hwnd = nullptr;
    HINSTANCE m_hIns = nullptr;
    std::vector<Node*> m_nodes;
    std::vector<Node*> m_lts_nodes;
    std::vector<Node*> m_sec_nodes;
    std::vector<Node*> m_lts_sec_nodes;



public:


    void parse_available_list(json::value& jsonobj);


public:
    Nvm(HWND hwnd, HINSTANCE hInst);
    ~Nvm();

    void download_available_list(const std::wstring& url);
    int  get_nodes_len();
    void download_node(Node* node);
    void clear_nodes();


private:
};
 