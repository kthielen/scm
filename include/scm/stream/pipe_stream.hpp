#ifndef PIPESTREAM_H_INCLUDED
#define PIPESTREAM_H_INCLUDED

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>

namespace stream
{

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class pipe_stream_buffer : public std::basic_streambuf<Char, Traits>
    {
    public:
        typedef typename std::basic_streambuf<Char, Traits>::int_type  int_type;
        typedef typename std::basic_streambuf<Char, Traits>::char_type char_type;
        typedef typename std::basic_streambuf<Char, Traits> BaseT;

        pipe_stream_buffer(int rh, int wh) : readh(rh), writeh(wh)
        {
		    setg(0, 0, 0);
        }

        ~pipe_stream_buffer()
        {
		    if (this->readh)  close(this->readh);
			if (this->writeh) close(this->writeh);
        }

		int read_handle()  const { return readh;  }
		int write_handle() const { return writeh; }
    private:
        typedef typename std::vector<Char> BufferT;

        int     readh;
		int     writeh;
        BufferT input;
        BufferT output;

        int_type underflow()
        {
            if (BaseT::gptr() != BaseT::egptr())
                return Traits::to_int_type(*BaseT::gptr());

            input.clear();
            input.resize(2048);

            int rret = 0;

            do
            {
                rret = read(this->readh, &(*(input.begin())), input.size());
                input.resize(rret);

                if (rret == 0)
				{
					close(this->readh);
					this->readh = 0;
                    return Traits::eof();
				}
            }
            while (input.size() == 0);

            setg(&(*(input.begin())), &(*(input.begin())), &(*(input.begin() + input.size())));
            return Traits::to_int_type(*(input.begin()));
        }

        int_type overflow(int_type c)
        {
            if (c != Traits::eof())
                output.push_back(Traits::to_char_type(c));
            else
                sync();

            return Traits::not_eof(c);
        }

        int sync()
        {
            int sr = 0, bs = 0;

            while (bs != output.size())
            {
			    sr = write(this->writeh, &(*(output.begin() + bs)), output.size() - bs);

                if (sr != -1)
                    bs += sr;
                else
                    throw std::runtime_error("Error sending data on pipe.");
            }

            output.clear();
            return 0;
        }

        pipe_stream_buffer();
        pipe_stream_buffer(const pipe_stream_buffer<Char, Traits>& rhs);
        void operator=(const pipe_stream_buffer<Char, Traits>& rhs);
    };

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class pipe_stream : public std::basic_iostream<Char, Traits>
    {
    public:
        pipe_stream(int readh, int writeh) : std::basic_iostream<Char, Traits>(&buffer), buffer(readh, writeh)
        {
        }

		int read_handle()  const { return pipe_buffer.read_handle(); }
		int write_handle() const { return pipe_buffer.write_handle(); }
    private:
        typedef pipe_stream_buffer<Char, Traits> pipe_buffer;
        pipe_buffer buffer;

        pipe_stream();
        pipe_stream(const pipe_stream<Char, Traits>& rhs);
        void operator=(const pipe_stream<Char, Traits>& rhs);
    };

}

#endif
