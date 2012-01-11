#ifndef LINEBUFFEREDSTREAM_H_INCLUDED
#define LINEBUFFEREDSTREAM_H_INCLUDED

#include <iostream>
#include <vector>
#include <string>

namespace stream
{

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class line_buffered_stream_buffer : public std::basic_streambuf<Char, Traits>
    {
    public:
        typedef std::basic_istream<Char, Traits> IStream;
        typedef typename std::basic_streambuf<Char, Traits>::int_type  int_type;
        typedef typename std::basic_streambuf<Char, Traits>::char_type char_type;
	typedef typename std::basic_streambuf<Char, Traits> BaseT;

        line_buffered_stream_buffer(IStream& st) : child(st)
        {
            BaseT::setg(0, 0, 0);
        }
    private:
        typedef typename std::vector<Char> BufferT;

        IStream& child;
        BufferT  input;

        int_type underflow()
        {
            if (BaseT::gptr() != BaseT::egptr())
                return Traits::to_int_type(*BaseT::gptr());
            else if (!child)
                return Traits::eof();

            input.clear();
            input.resize(4096);
            child.getline(&(*(input.begin())), input.size());
            input.resize(child.gcount());
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

        line_buffered_stream_buffer();
        line_buffered_stream_buffer(const line_buffered_stream_buffer<Char, Traits>& rhs);
        void operator=(const line_buffered_stream_buffer<Char, Traits>& rhs);
    };

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class line_buffered_stream : public std::basic_istream<Char, Traits>
    {
    public:
        line_buffered_stream(std::basic_istream<Char, Traits>& child) : std::basic_istream<Char, Traits>(&buffer), buffer(child)
        {
        }
    private:
        typedef line_buffered_stream_buffer<Char, Traits> stream_buffer;
        stream_buffer buffer;

        line_buffered_stream();
        line_buffered_stream(const line_buffered_stream<Char, Traits>& rhs);
        void operator=(const line_buffered_stream<Char, Traits>& rhs);
    };

}

#endif
