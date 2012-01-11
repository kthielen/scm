#include <scm/str/Util.hpp>
#include <scm/stream/socket_stream.hpp>
#include <stdexcept>

using namespace str;

namespace stream
{

inline bool is_numeric(char c) { return c >= '0' && c <= '9'; }

typedef std::pair<std::string, std::string> string_pair;

string_pair split(const std::string& input, const std::string& delim)
{
    std::string::size_type pos = input.find(delim);

    if (pos == std::string::npos)
        return string_pair(input, "");
    else
        return string_pair(input.substr(0, pos), input.substr(pos + delim.length()));
}

int connect_socket(const std::string& addrport)
{
    string_pair    ps   = split(addrport, ":");
    unsigned int   port = from_string<unsigned int>(ps.second);
    if (port == 0) port = 8080;

    return connect_socket(ps.first, port);
}

int connect_socket(hostent* hs, unsigned int port)
{
    int r = socket(AF_INET, SOCK_STREAM, 0);
    if (r == -1)
        throw std::runtime_error("Error creating socket.");

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr   = *((in_addr*)(hs->h_addr_list[0]));
    addr.sin_port   = htons(port);

    if (connect(r, (sockaddr*)&addr, sizeof(addr)) == -1)
    {
        closesocket(r);
        throw std::runtime_error("Error connecting socket to its destination address.");
    }

    // wait for writeability
    fd_set wd;
    FD_ZERO(&wd);
    FD_SET(r, &wd);

    if (select(r + 1, 0, &wd, 0, 0) == -1)
        throw std::runtime_error("Error waiting for socket writeability.");

    return r;
}

int connect_socket(const std::string& addr, unsigned int port)
{
    if (is_numeric(*(addr.begin())))
        return connect_socket(gethostbyaddr(addr.c_str(), addr.size(), AF_INET), port);
    else
        return connect_socket(gethostbyname(addr.c_str()), port);
}

}
