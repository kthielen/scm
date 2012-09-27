#ifndef RLSTREAM_H_INCLUDED
#define RLSTREAM_H_INCLUDED

#include <iostream>
#include <vector>
#include <string>

#include <readline/readline.h>
#include <readline/history.h>

namespace stream
{

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class rl_stream_buffer : public std::basic_streambuf<Char, Traits>
    {
    public:
        typedef typename std::basic_streambuf<Char, Traits>::int_type  int_type;
        typedef typename std::basic_streambuf<Char, Traits>::char_type char_type;
        typedef typename std::basic_streambuf<Char, Traits> BaseT;

        rl_stream_buffer()
        {
            BaseT::setg(0, 0, 0);
        }
    private:
        typedef typename std::vector<Char> BufferT;
        BufferT  input;

        int_type underflow()
        {
            if (BaseT::gptr() != BaseT::egptr())
                return Traits::to_int_type(*BaseT::gptr());

            char*       l = readline("");
	        std::string r(l);
		    if (l && *l) add_history(l);
			free(l);

			input = BufferT(r.begin(), r.end());
            input.push_back((Char)'\n');

            if (input.size() > 0)
            {
                BaseT::setg(&(*(input.begin())), &(*(input.begin())), &(*(input.end())));
                return Traits::to_int_type(*(input.begin()));
            }
            else
            {
                BaseT::setg(0, 0, 0);
                return Traits::eof();
            }
        }

        rl_stream_buffer(const rl_stream_buffer<Char, Traits>& rhs);
        void operator=(const rl_stream_buffer<Char, Traits>& rhs);
    };

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class rl_stream : public std::basic_istream<Char, Traits>
    {
    public:
        rl_stream() : std::basic_istream<Char, Traits>(&buffer)
        {
        }
    private:
        typedef rl_stream_buffer<Char, Traits> stream_buffer;
        stream_buffer buffer;

        rl_stream(const rl_stream<Char, Traits>& rhs);
        void operator=(const rl_stream<Char, Traits>& rhs);
    };

}

#endif
