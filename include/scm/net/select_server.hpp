#ifndef SELECTSERVER_H_INCLUDED
#define SELECTSERVER_H_INCLUDED

#if !defined(WIN32) && !defined(_WIN32_WCE)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef closesocket
#define closesocket close
#endif
#else
#include <winsock2.h>
#include <Lmcons.h>
#endif

#include <map>
#include <vector>

namespace net
{

enum SegmentResult { Reset=0, Continue, Disconnect };

template < typename SCtx, typename ConnCtxT /* ConnCtxT <: { run : InBuffer -> OutBuffer -> (Reset | Continue | Disconnect) } */ >
class select_server
{
public:
	select_server(SCtx ctx, int p, const std::string& sname = "Network Server") : context(ctx), server(alloc_server(sname, p)), port(p), server_name(sname) { }

	~select_server()
    {
        closesocket(server);
        for (typename ConnMap::const_iterator i = connections.begin(); i != connections.end(); ++i)
        {
            closesocket(i->first);
            delete i->second.context;
        }
    }

	void step(int delay = 10)
	{
		int    max_fd = server;
		fd_set rd;
		FD_ZERO(&rd);

		FD_SET(server, &rd);
		for (typename ConnMap::const_iterator i = connections.begin(); i != connections.end(); ++i)
		{
			FD_SET(i->first, &rd);
			max_fd = std::max<int>(max_fd, i->first);
		}

        timeval tv; memset(&tv, 0, sizeof(tv));
        tv.tv_usec = delay;
        timeval* p = (delay >= 0) ? &tv : 0;

		if (select(max_fd + 1, &rd, 0, 0, p) == -1)
			throw std::runtime_error(server_name + ": Select failed on local server.");
		
		for (typename ConnMap::iterator i = connections.begin(); i != connections.end();)
		{
			int          csck = i->first;
			SConnection& conn = i->second;
			bool         cset = FD_ISSET(csck, &rd);

			if (!cset)
				++i;
			else
			{
			    static char buf[256];
			    int n = 0, m = 0;
			    do
			    {
			        m  = recv(csck, buf, sizeof(buf), 0);
			        if (m > 0) {
						n += m;
						conn.buffer.insert(conn.buffer.end(), buf, buf + m);
					}
			    }
			    while (m > 0);

				std::vector<char> out;
				SegmentResult r = (n > 0) ? conn.context->run(conn.buffer, out) : Disconnect;

				int sr = 0, bs = 0;
				while (bs != out.size() && sr >= 0)
				{
					sr =  send(csck, (char*)(&(*(out.begin() + bs))), out.size() - bs, 0);
					bs += sr;
				}

				switch (r)
				{
				default:
					++i;
					break;
				case Reset:
					++i;
					conn.buffer.clear();
					break;
				case Disconnect:
					delete conn.context;
					closesocket(csck);
					connections.erase(i++);
					break;
				}
			}
		}

        if (FD_ISSET(this->server, &rd))
        {
            int c = accept(this->server, 0, 0);
            set_non_blocking(c);
            connections[c].context = new ConnCtxT(context);
        }
	}

	void run()
	{
		while (true)
			step(-1);
	}
private:
	typedef std::vector<char> CBuffer;
	struct SConnection
	{
		CBuffer   buffer;
		ConnCtxT* context;
	};
	typedef std::map<int, SConnection> ConnMap;

	SCtx        context;
	int         server;
	int         port;
	ConnMap     connections;
	std::string server_name;

	static int alloc_server(const std::string& server_name, int port)
	{
		// allocate the socket
	    int sh = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP /* 0 */);
    	if (sh == -1)
        	throw std::runtime_error(server_name + ": Unable to create local server socket.");

	    // make sure that we can quickly restart the server
    	int ra = 1;
	    setsockopt(sh, SOL_SOCKET, SO_REUSEADDR, (char*)&ra, sizeof(ra));

    	// bind the new socket to a local address
	    sockaddr_in laddr;
    	memset(&laddr, 0, sizeof(laddr));

	    laddr.sin_family      = AF_INET;
    	laddr.sin_port        = htons(port);
	    laddr.sin_addr.s_addr = INADDR_ANY;

    	if (bind(sh, (sockaddr*)&laddr, sizeof(laddr)) == -1)
        	throw std::runtime_error(server_name + ": Unable to bind server socket to local address.");

	    // start to listen
    	if (listen(sh, SOMAXCONN) == -1)
        	throw std::runtime_error(server_name + ": Unable to listen on local address.");

		return sh;
	}

	static void set_non_blocking(int s)
	{
#	ifndef WIN32
    	int opts = fcntl(s, F_GETFL);
	    if (opts < 0)
    	    { std::cout << "fcntl(F_GETFL)" << std::endl; return; }

	    opts = (opts | O_NONBLOCK);
    	if (fcntl(s, F_SETFL,opts) < 0)
        	{ std::cout << "fcntl(F_SETFL)" << std::endl; return; }
#	else
    	u_long b = 1;
	    ioctlsocket(s, FIONBIO, &b);
#	endif
	}
};

}

#endif
