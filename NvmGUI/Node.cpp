#include "Node.h"

Node::Node(std::wstring version, std::wstring npm, bool lts, bool security)
    : m_version(version)
    , m_npm(npm)
    , m_lts(lts)
    , m_security(security)
{

}
Node::~Node()
{

}
