#ifndef PROCESSSTREAM_H_INCLUDED
#define PROCESSSTREAM_H_INCLUDED

#include <iostream>
#include <vector>
#include <string>
#include <windows.h>

namespace stream
{

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class process_stream_buffer : public std::basic_streambuf<Char, Traits>
    {
    public:
        typedef typename std::basic_streambuf<Char, Traits>::int_type  int_type;
        typedef typename std::basic_streambuf<Char, Traits>::char_type char_type;
        typedef typename std::basic_streambuf<Char, Traits> BaseT;

		process_stream_buffer(const std::string& exp) :
			pstdin_read(INVALID_HANDLE_VALUE), pstdin_write(INVALID_HANDLE_VALUE),
			pstdout_read(INVALID_HANDLE_VALUE), pstdout_write(INVALID_HANDLE_VALUE),
			process(INVALID_HANDLE_VALUE)
        {
			SECURITY_ATTRIBUTES sa;
			memset(&sa, 0, sizeof(sa));
			sa.nLength              = sizeof(sa);
			sa.bInheritHandle       = TRUE;
			sa.lpSecurityDescriptor = NULL;

			try
			{
				if (!CreatePipe(&this->pstderr_read, &this->pstderr_write, &sa, 0))
					throw std::runtime_error("Couldn't create standard err for process: " + exp);
				if (!SetHandleInformation(this->pstderr_read, HANDLE_FLAG_INHERIT, 0) )
					throw std::runtime_error("Couldn't create standard err for process: " + exp);

				if (!CreatePipe(&this->pstdout_read, &this->pstdout_write, &sa, 0))
					throw std::runtime_error("Couldn't create standard out for process: " + exp);
				if (!SetHandleInformation(this->pstdout_read, HANDLE_FLAG_INHERIT, 0) )
					throw std::runtime_error("Couldn't create standard out for process: " + exp);

				if (!CreatePipe(&this->pstdin_read, &this->pstdin_write, &sa, 0))
					throw std::runtime_error("Couldn't create standard in for process: " + exp);
				if (!SetHandleInformation(this->pstdin_write, HANDLE_FLAG_INHERIT, 0) )
					throw std::runtime_error("Couldn't create standard in for process: " + exp);

				PROCESS_INFORMATION pi;
				STARTUPINFO         si;
				BOOL                bs = FALSE; 
				memset(&pi, 0, sizeof(pi));
				memset(&si, 0, sizeof(si));

				si.cb = sizeof(si); 
				si.hStdError  = this->pstderr_write;
				si.hStdOutput = this->pstdout_write;
				si.hStdInput  = this->pstdin_read;
				si.dwFlags   |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
				si.wShowWindow = SW_HIDE;

				if (!CreateProcess(NULL, const_cast<LPSTR>(exp.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
					throw std::runtime_error("Unable to create process: " + exp);

				process = pi.hProcess;
				CloseHandle(pi.hThread);
			}
			catch (...)
			{
				cleanup();
				throw;
			}

		    setg(0, 0, 0);
        }

        ~process_stream_buffer() { cleanup(); }

		std::string err() 
		{
			std::string buffer;
			char buff[2048];

			DWORD dwr = 0;
			CloseHandle(this->pstderr_write);
			this->pstderr_write = INVALID_HANDLE_VALUE;

            do
            {
				BOOL b = ReadFile(this->pstderr_read, buff, sizeof(buff), &dwr, 0);
				if (dwr) buffer.insert(buffer.end(), buff, buff + dwr);

                if (!b || dwr == 0)
				{
					CloseHandle(this->pstderr_read);
					this->pstderr_read = INVALID_HANDLE_VALUE;
				}
            }
            while (dwr != 0);

			return buffer;
		}
    private:
		HANDLE pstderr_read;
		HANDLE pstderr_write;
		HANDLE pstdin_read;
		HANDLE pstdin_write;
		HANDLE pstdout_read;
		HANDLE pstdout_write;
		HANDLE process;

		void cleanup()
		{
			cleanup_handle(pstdin_read);
			cleanup_handle(pstdin_write);
			cleanup_handle(pstdout_read);
			cleanup_handle(pstdout_write);
			cleanup_handle(pstderr_read);
			cleanup_handle(pstderr_write);
			cleanup_handle(process);
		}
		
		void cleanup_handle(HANDLE h)
		{
			if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
		}

        typedef typename std::vector<Char> BufferT;

        BufferT input;
        BufferT output;

        int_type underflow()
        {
            if (BaseT::gptr() != BaseT::egptr())
                return Traits::to_int_type(*BaseT::gptr());
			else if (this->process == INVALID_HANDLE_VALUE)
				return Traits::eof();

			// disable writing to the child process
			if (this->pstdin_write != INVALID_HANDLE_VALUE)
			{
				CloseHandle(this->pstdin_write);
				this->pstdin_write = INVALID_HANDLE_VALUE;
				bool waited = WAIT_OBJECT_0 == WaitForSingleObject(this->process, 500);

				CloseHandle(this->process);
				CloseHandle(this->pstdout_write);
				this->pstdout_write = INVALID_HANDLE_VALUE;
				this->process       = INVALID_HANDLE_VALUE;
			}

            input.clear();
			char buff[2048];

			DWORD dwr = 0;

            do
            {
				BOOL b = ReadFile(this->pstdout_read, buff, sizeof(buff), &dwr, 0);
				if (dwr) input.insert(input.end(), buff, buff + dwr);

                if (!b || dwr == 0)
				{
					CloseHandle(this->pstdout_read);
					this->pstdout_read = INVALID_HANDLE_VALUE;
				}
            }
            while (dwr != 0);

			if (input.size() == 0)
				return Traits::eof();

            setg(&(*(input.begin())), &(*(input.begin())), &(*(input.begin())) + input.size());
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
			DWORD sr = 0, bs = 0;

			if (this->pstdin_write != INVALID_HANDLE_VALUE)
			{
				while (bs != output.size())
				{
					if (!WriteFile(this->pstdin_write, &(*(output.begin() + bs)), output.size() - bs, &sr, 0))
						throw std::runtime_error("Error sending data to external process.");

					bs += sr;
				}
			}

            output.clear();
            return 0;
        }

        process_stream_buffer();
        process_stream_buffer(const process_stream_buffer<Char, Traits>& rhs);
        void operator=(const process_stream_buffer<Char, Traits>& rhs);
    };

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class process_stream : public std::basic_iostream<Char, Traits>
    {
    public:
		process_stream(const std::string& exp) :
		  std::basic_iostream<Char, Traits>(&buffer), buffer(exp)
        {
		}

		~process_stream() { }

		std::string err() { return buffer.err(); }
    private:
        typedef process_stream_buffer<Char, Traits> process_buffer;
        process_buffer buffer;

        process_stream();
        process_stream(const process_stream<Char, Traits>& rhs);
        void operator=(const process_stream<Char, Traits>& rhs);
    };

}

#endif
