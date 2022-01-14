#pragma once
#include "Node.h"
#include <cpprest/http_client.h>
#include <Windows.h>
#include <vector>
#include <string>

using namespace web;
using namespace web::http;
using namespace web::http::client;

class Nvm {

    HWND g_hwnd = nullptr;
    HINSTANCE g_hIns = nullptr;
    std::vector<Node> m_nodes;

public:
public:
    Nvm(HWND hwnd, HINSTANCE hInst);
    ~Nvm();

    void download_available_list(const std::wstring& url);
    void parse_available_list(json::value& jsonobj);



private:
};
