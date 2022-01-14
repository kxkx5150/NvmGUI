#include "Nvm.h"
#include <crtdbg.h>

Nvm::Nvm(HWND hwnd, HINSTANCE hInst)
    : g_hwnd(hwnd)
    , g_hIns(hInst)
{
}
Nvm::~Nvm()
{
}
void Nvm::parse_available_list(json::value& jsonobj)
{
    auto aaa = jsonobj.serialize();
    OutputDebugString(L"");
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
        });
    try {
        resp.wait();
    } catch (const std::exception& e) {
        _RPTN(_CRT_WARN, "Error exception:%s\n", e.what());
    }
}
