#ifndef EMBEDDEDSCRIPTSTREAM_H_INCLUDED
#define EMBEDDEDSCRIPTSTREAM_H_INCLUDED

#include <iostream>
#include <vector>
#include <string>

namespace stream
{

template < typename Char = char, typename Traits = std::char_traits<Char> >
    struct script_substitutions {
		typedef typename std::basic_string<Char, Traits> StringT;
	
		script_substitutions(const StringT& ptp = StringT(), const StringT& pts = StringT(), const StringT& icp = StringT(), const StringT& ics = StringT()) :
		    pass_through_prefix(ptp), pass_through_suffix(pts), inline_code_prefix(icp), inline_code_suffix(ics)
		{
		}
	
		StringT pass_through_prefix;
		StringT pass_through_suffix;
		StringT inline_code_prefix;
		StringT inline_code_suffix;
    };

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class embedded_script_stream_buffer : public std::basic_streambuf<Char, Traits>
    {
    public:
        typedef std::basic_istream<Char, Traits> IStream;
        typedef typename std::basic_streambuf<Char, Traits>::int_type  int_type;
        typedef typename std::basic_streambuf<Char, Traits>::char_type char_type;
		typedef typename std::basic_streambuf<Char, Traits> BaseT;

        embedded_script_stream_buffer(IStream& st, const script_substitutions<Char, Traits>& ss) :
		    child(st), in_pass_through(true), switch_key(false), code_inline(0), eof_reached(false)
        {
	    	this->pass_through_prefix = ss.pass_through_prefix; // "(send \"";
	    	this->pass_through_suffix = ss.pass_through_suffix; // "\")";
	    	this->inline_code_prefix  = ss.inline_code_prefix;  // "(send ";
	    	this->inline_code_suffix  = ss.inline_code_suffix;  // ")";
	    	
	    	begin_pass_through();
	    	commit_buffer();
        }
    private:
        typedef typename std::vector<Char>               BufferT;
		typedef typename std::basic_string<Char, Traits> StringT;

		StringT  pass_through_prefix;
		StringT  pass_through_suffix;
		StringT  inline_code_prefix;
		StringT  inline_code_suffix;

        IStream& child;
        BufferT  input;
		bool     in_pass_through; // pass through text directly or read text as script code
		bool     switch_key;      // position in reading '<%' or '%>'
		bool     eof_reached;     // true when the last possible byte has already been returned
		int      code_inline;     // 0 if not determined, 1 if in '<%= x %>', 2 if in '<% x %>'

		void clear_buffer() {
		    this->input.clear();
		}

		void begin_pass_through() {
		    append_buffer(this->pass_through_prefix);
		}

		void end_pass_through() {
		    append_buffer(this->pass_through_suffix);
		}

		void begin_inline_code() {
		    append_buffer(this->inline_code_prefix);
		}

		void end_inline_code() {
		    append_buffer(this->inline_code_suffix);
		}

		void begin_code() {
		}

		void end_code() {
		}

		void append_buffer(const Char& c) {
		    this->input.push_back(c);
		}

		void append_buffer(const StringT& data) {
		    append_buffer(data.begin(), data.end());
		}

		template < typename IterT >
		    void append_buffer(IterT begin, IterT end) {
				this->input.insert(this->input.end(), begin, end);
		    }

		int_type commit_buffer() {
			if (this->input.size() > 0) {
				BaseT::setg(&(*(this->input.begin())), &(*(this->input.begin())), &(*(this->input.begin())) + this->input.size());
				return Traits::to_int_type(*(input.begin()));
			} else if (!this->eof_reached) {
				if (!this->in_pass_through)
				    throw std::runtime_error("Unterminated <% or <%= in input.");

				end_pass_through();
				this->eof_reached = true;
				return commit_buffer();
			} else {
				BaseT::setg(0, 0, 0);
				return Traits::eof();
			}
		}

		int_type underflow() {
		    if (BaseT::gptr() != BaseT::egptr()) {
		        return Traits::to_int_type(*BaseT::gptr());
			} else if (!child) {
				clear_buffer();
				return commit_buffer();
			}

		    clear_buffer();

			BufferT stream_segment;
			stream_segment.resize(4096);
			child.getline(&(*(stream_segment.begin())), stream_segment.size());
			stream_segment.resize(child.gcount());
			stream_segment.push_back((Char)'\n');

		    for (typename BufferT::const_iterator bi = stream_segment.begin(); bi != stream_segment.end(); ++bi) {
		        if (*bi == '\0') continue;

				if (this->in_pass_through) {
				    if (!this->switch_key && *bi == '<') {
						this->switch_key = true;
				    } else if (this->switch_key && *bi == '%') {
						this->in_pass_through = false;
						this->switch_key      = false;
						this->code_inline     = 0;
						end_pass_through();
				    } else if (this->switch_key) {
						append_buffer('<');
						append_buffer(*bi);
						this->switch_key = false;
				    } else {
						if (*bi == '\\' || *bi == '\"')
					    	append_buffer('\\');
					    
						append_buffer(*bi);
				    }
				} else if (this->code_inline == 0 && *bi == '=') {
				    this->code_inline = 1;
				    begin_inline_code();
				} else {
				    if (this->code_inline == 0) {
						this->code_inline = 2;
						begin_code();
				    }
	
				    if (!this->switch_key && *bi == '%') {
						this->switch_key = true;
				    } else if (this->switch_key && *bi == '>') {
						this->in_pass_through = true;
						this->switch_key      = false;
						
						if (this->code_inline == 1)
						    end_inline_code();
						else
						    end_code();
			
						begin_pass_through();
				    } else if (this->switch_key) {
						append_buffer('%');
						append_buffer(*bi);
						this->switch_key = false;
				    } else {
						append_buffer(*bi);
				    }
				}
			}
	
			return commit_buffer();
		}

		embedded_script_stream_buffer();
		embedded_script_stream_buffer(const embedded_script_stream_buffer<Char, Traits>& rhs);
		void operator=(const embedded_script_stream_buffer<Char, Traits>& rhs);
    };

template < typename Char = char, typename Traits = std::char_traits<Char> >
    class embedded_script_stream : public std::basic_istream<Char, Traits>
    {
    public:
        embedded_script_stream(std::basic_istream<Char, Traits>& child, const script_substitutions<Char, Traits>& ss) :
			std::basic_istream<Char, Traits>(&buffer), buffer(child, ss)
        {
        }
    private:
        typedef embedded_script_stream_buffer<Char, Traits> stream_buffer;
        stream_buffer buffer;

        embedded_script_stream();
        embedded_script_stream(const embedded_script_stream<Char, Traits>& rhs);
        void operator=(const embedded_script_stream<Char, Traits>& rhs);
    };

}

#endif
