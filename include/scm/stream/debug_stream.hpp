#ifndef DEBUGSTREAM_H_INCLUDED
#define DEBUGSTREAM_H_INCLUDED

#include <iostream>
#include <vector>
#include <string>

namespace stream
{

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class debug_stream_buffer : public std::basic_streambuf<Char, Traits>
    {
    public:
        typedef std::basic_istream<Char, Traits> IStream;
        typedef typename std::basic_streambuf<Char, Traits>::int_type  int_type;
        typedef typename std::basic_streambuf<Char, Traits>::char_type char_type;
	typedef typename std::basic_streambuf<Char, Traits> BaseT;

        debug_stream_buffer(IStream& st) : child(st)
        {
            BaseT::setg(0, 0, 0);
        }
    private:
        typedef typename std::vector<Char> BufferT;

        IStream& child;
        Char     input;

        int_type underflow()
        {
            if (BaseT::gptr() != BaseT::egptr())
                return Traits::to_int_type(*BaseT::gptr());

            child.get(input);

            if (child.gcount() > 0)
            {
                if (input == 0)
                    std::cout << "\\0";
                else
                    std::cout << input;

                BaseT::setg(&input, &input, (&input) + 1);
                return Traits::to_int_type(input);
            }
            else
            {
                BaseT::setg(0, 0, 0);
                return Traits::eof();
            }
        }

        debug_stream_buffer();
        debug_stream_buffer(const debug_stream_buffer<Char, Traits>& rhs);
        void operator=(const debug_stream_buffer<Char, Traits>& rhs);
    };

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class debug_stream : public std::basic_istream<Char, Traits>
    {
    public:
        debug_stream(std::basic_istream<Char, Traits>& child) : std::basic_istream<Char, Traits>(&buffer), buffer(child)
        {
            std::cout << "begin debug stream" << std::endl;
        }

        ~debug_stream()
        {
            std::cout << "end debug stream" << std::endl;
        }
    private:
        typedef debug_stream_buffer<Char, Traits> stream_buffer;
        stream_buffer buffer;

        debug_stream();
        debug_stream(const debug_stream<Char, Traits>& rhs);
        void operator=(const debug_stream<Char, Traits>& rhs);
    };

}

#endif
