
#ifndef NET_REPL_H_INCLUDED
#define NET_REPL_H_INCLUDED

#include <scm/eval/Data.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/eval/Util.hpp>
#include <scm/net/select_server.hpp>
#include <scm/str/Util.hpp>
#include <vector>
#include <stdexcept>
#include <sstream>

namespace net
{

class Repl
{
public:
	Repl(scm::EnvironmentFrame* r)
	{
		root = r->allocator()->allocate<scm::EnvironmentFrame>(r);
	}

	SegmentResult run(const std::vector<char>& in, std::vector<char>& out)
	{
		if (in.size() == 0)           return Continue;
		char lc = in[in.size() - 1];
		if (lc != '\r' && lc != '\n') return Continue;
		std::istringstream ss(std::string(in.begin(), in.end()));

		while (ss) {
			std::string r;

			try
			{
				scm::Value* v = scm::ReadSExpression(ss, *root->allocator());

				if (v)
					r = "(quote " + scm::PrintExpression(scm::Eval(v, root.ptr())) + ")\n";
			}
			catch (std::runtime_error& e)
			{
				std::string r = "(raise \"" + str::escape<char>(e.what()) + "\")\n";
			}

			out.insert(out.end(), r.begin(), r.end());
		}

		return Reset;
	}
private:
	scm::gcguard<scm::EnvironmentFrame> root;
};

typedef select_server<scm::EnvironmentFrame*, Repl> ReplServer;

}

#endif
