#ifndef SOCKETSTREAM_H_INCLUDED
#define SOCKETSTREAM_H_INCLUDED

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define closesocket(x) shutdown(x, SHUT_RDWR), close(x)
#endif

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

namespace stream
{

int connect_socket(const std::string& addrport);
int connect_socket(const std::string& addr, unsigned int port);

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class socket_stream_buffer : public std::basic_streambuf<Char, Traits>
    {
    public:
        typedef typename std::basic_streambuf<Char, Traits>::int_type  int_type;
        typedef typename std::basic_streambuf<Char, Traits>::char_type char_type;
	typedef typename std::basic_streambuf<Char, Traits> BaseT;

        socket_stream_buffer(int h, bool interact = false) : handle(h), interactive(interact)
        {
            setg((Char*)0, (Char*)0, (Char*)0);
        }

        ~socket_stream_buffer()
        {
            closesocket(this->handle);
        }
    private:
        typedef typename std::vector<Char> BufferT;

        int     handle;
        BufferT input;
        BufferT output;
        bool    interactive;

        int_type underflow()
        {
            if (BaseT::gptr() != BaseT::egptr())
                return Traits::to_int_type(*BaseT::gptr());

            input.clear();
            input.resize(2048);

            int rret = 0;

            do
            {
                rret = recv(this->handle, &(*(input.begin())), input.size(), 0);

                if (rret == 0)
                    return Traits::eof();
                else if (rret == -1)
                    throw std::runtime_error("Error reading socket.");
                else
                    input.resize(rret);
            }
            while (input.size() == 0);

            setg(&(*(input.begin())), &(*(input.begin())), &(*(input.begin())) + input.size());//&(*(input.end())));
            return Traits::to_int_type(*(input.begin()));
        }

        int_type overflow(int_type c)
        {
            if (c != Traits::eof())
            {
		if (this->interactive && c == static_cast<Char>('\n'))
		    output.push_back(static_cast<Char>('\r'));
		    
                output.push_back(Traits::to_char_type(c));

		if (this->interactive)
		    sync();
            }
            else
            {
                sync();
            }

            return Traits::not_eof(c);
        }

        int sync()
        {
            int sr = 0, bs = 0;

            while (bs != output.size())
            {
                sr = send(this->handle, &(*(output.begin() + bs)), output.size() - bs, 0);

                if (sr != -1)
                    bs += sr;
                else
                    throw std::runtime_error("Error sending data on socket.");
            }

            output.clear();
            return 0;
        }

        socket_stream_buffer();
        socket_stream_buffer(const socket_stream_buffer<Char, Traits>& rhs);
        void operator=(const socket_stream_buffer<Char, Traits>& rhs);
    };

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class socket_stream : public std::basic_iostream<Char, Traits>
    {
    public:
        socket_stream(int handle, bool interactive = false) : std::basic_iostream<Char, Traits>(&buffer), buffer(handle, interactive)
        {
        }

        socket_stream(const std::string& s) : std::basic_iostream<Char, Traits>(&buffer), buffer(connect_socket(s))
        {
        }

        socket_stream(const std::string& s, unsigned int port) : std::basic_iostream<Char, Traits>(&buffer), buffer(connect_socket(s, port))
        {
        }
    private:
        typedef socket_stream_buffer<Char, Traits> socket_buffer;
        socket_buffer buffer;

        socket_stream();
        socket_stream(const socket_stream<Char, Traits>& rhs);
        void operator=(const socket_stream<Char, Traits>& rhs);
    };

}

#endif
