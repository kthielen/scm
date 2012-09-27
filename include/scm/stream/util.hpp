#ifndef STREAM_UTIL_H_INCLUDED
#define STREAM_UTIL_H_INCLUDED

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <sstream>

namespace stream
{

template <typename T>
    std::string to_string(const T& v)
    {
        std::ostringstream ss; ss << v; return ss.str();
    }

template <typename T>
	T from_string(const std::string& s)
	{
		T                  result;
		std::istringstream ss(s);
		
		ss >> result;
		return result;
	}

template <typename Char>
    std::basic_string<Char> slurp(std::basic_istream<Char>& input) {
        std::basic_string<Char> result;
        Char                    buff[4096];

        while (input) {
            input.read(buff, sizeof(buff));
            result += std::basic_string<Char>(buff, buff + input.gcount());
        }

        return result;
    }

template <typename Char>
	std::vector<Char> slurpvec(std::basic_istream<Char>& input) {
		std::vector<Char> result;
		Char buff[4096];

		while (input) {
			input.read(buff, sizeof(buff));
			result.insert(result.end(), buff, buff + input.gcount());
		}

		return result;
	}

template <typename Char>
    std::vector< std::basic_string<Char> > read_lines(std::basic_istream<Char>& input)
    {
        typedef typename std::basic_string<Char> Str;
        typedef typename std::vector< Str > StrVec;
        StrVec result;
        for (std::string line ; std::getline(input, line) ; ) {
	  result.push_back(line);
	}
        return result;
    }

template <typename Char>
    std::vector< std::basic_string<Char> > skipws_read_lines(std::basic_istream<Char>& input)
    {
        typedef typename std::basic_string<Char> Str;
        typedef typename std::vector< Str > StrVec;
        StrVec result;

        for (std::string line ; std::getline(input, line) ; ) {
	  if ( !line.empty() ) {
	    result.push_back(line);
	  }
	}
        return result;
    }

template <typename Char>
    std::vector< std::basic_string<Char> > read_lines_from_file(const std::basic_string<Char>& filename)
    {
        typedef typename std::basic_ifstream<Char> FStream;
        FStream file(filename.c_str());

        if (!file.is_open())
            throw std::runtime_error("Unable to open '" + filename + "' for reading.");
        else
            return read_lines(file);
    }

template <typename Char>
    std::vector< std::basic_string<Char> > skipws_read_lines_from_file(const std::basic_string<Char>& filename)
    {
        typedef typename std::basic_ifstream<Char> FStream;
        FStream file(filename.c_str());

        if (!file.is_open())
            throw std::runtime_error("Unable to open '" + filename + "' for reading.");
        else
            return skipws_read_lines(file);
    }

}

#endif
