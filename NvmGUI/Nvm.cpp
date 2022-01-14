#include "Nvm.h"
#include <crtdbg.h>

Nvm::Nvm(HWND hwnd, HINSTANCE hInst)
    : m_hwnd(hwnd)
    , m_hIns(hInst)
{
}
Nvm::~Nvm()
{
}
void Nvm::parse_available_list(json::value& jsonobj)
{
    if (jsonobj.is_null())
        return;

    m_nodes.clear();
    json::array array = jsonobj.as_array();

    for (int i = 0; i < array.size(); ++i) {
        if (array[i][L"version"].is_null() || !array[i][L"version"].is_string())
            continue;

        if (array[i][L"npm"].is_null() || !array[i][L"npm"].is_string())
            continue;

        if (array[i][L"lts"].is_null() || !array[i][L"lts"].is_boolean())
            continue;

        if (array[i][L"security"].is_null() || !array[i][L"security"].is_boolean())
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
            Node* node = new Node(
                item[L"version"].as_string(),
                item[L"npm"].as_string(),
                item[L"lts"].as_bool(),
                item[L"security"].as_bool());
            m_nodes.push_back(node);
        }
    }
}
void Nvm::download_available_list(const std::wstring& url)
{
    http_client client(url);
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
            int nlen = get_nodes_len();
            OutputDebugString(L"\n");

        });
    try {
        resp.wait();
    } catch (const std::exception& e) {
        _RPTN(_CRT_WARN, "Error exception:%s\n", e.what());
    }
}
int Nvm::get_nodes_len()
{
    return m_nodes.size();
}
