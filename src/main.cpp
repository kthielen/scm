#include <scm/eval/SExprReader.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/CTypeUtil.hpp>

#include <scm/eval/modules/std/StdEnv.hpp>

#include <scm/str/Util.hpp>
#include <scm/stream/line_buffered_stream.hpp>
//#include <scm/stream/rl_stream.hpp>
#include <scm/stream/http_reader.hpp>
#include <scm/stream/debug_stream.hpp>
#include <scm/stream/util.hpp>

// for easily importing scripts
#include <scm/eval/modules/std/FileIO.hpp>

// for the alternate incremental network server
#include <scm/net/repl.hpp>

#include <sstream>
#include <fstream>

using namespace str;

namespace scm {

typedef std::vector<std::string> StringSeq;

struct ReplEnv {
    ReplEnv() {
        InitRootFrame(allocator.allocate<EnvironmentFrame>((EnvironmentFrame*)0), std::vector<std::string>());
    }

    ReplEnv(const std::vector<std::string>& argl) {
        InitRootFrame(allocator.allocate<EnvironmentFrame>((EnvironmentFrame*)0), argl);
    }

    EnvironmentFrame* root;
    Allocator         allocator;
private:
    void InitRootFrame(EnvironmentFrame* rf, const std::vector<std::string>& argl) {
        this->root = rf;
        allocator.root(this->root);

        root->Define("arguments", LiftContainer(argl, &allocator));
        InitStdModule  (this->root);
    }
};

/*
void run_local_repl(const StringSeq& sargs, bool interact, const std::string& inc_str) {
	while (true) {
		try {
			Allocator a;
			std::cout << PrintExpression(ReadSExpression(std::cin, a)) << std::endl;
		} catch (std::exception& e) {
			std::cout << "*\n** " << e.what() << "\n*" << std::endl;
		}
	}
}
*/

void run_local_repl(const StringSeq& sargs, bool interact, const std::string& inc_str) {
    try {
        std::istringstream ss(inc_str);
        ReplEnv env(sargs);

        while (ss) {
			Eval(ReadSExpression(ss, env.allocator), env.root);
		}

        if (interact) {
            std::cout << "Scheme console:"                   << std::endl
                      << " type 'exit' to leave the console" << std::endl
                      << std::endl;

//			stream::rl_stream<char> input;
			std::istream& input = std::cin;
            REPL(input, std::cout, env.root);
        }
    } catch (std::runtime_error& e) {
        std::cout << "*" << std::endl << "** Fatal Error: " << e.what() << std::endl << "*" << std::endl;
    }
}

void print_help() {
    std::cout << "scm: The Scheme interpreter" << std::endl << std::endl
              << "Usage: scm [-?] [-i] [-s <port>] [-r <arguments>] [-h <port>] [-d <port>] <files>" << std::endl
              << std::endl
              << "Switch:  Meaning:" << std::endl
              << "-i       If present, runs in interactive mode." << std::endl
              << "-s       If present, runs a REPL server on port <port>." << std::endl
              << "-r       If present, passes <arguments> to scripts as the 'arguments' variable." << std::endl
              << "-h       If present, run an HTTP service on port <port>." << std::endl
              << "-d       If present, run a generic network service." << std::endl
              << "-?       Print this message." << std::endl
              << std::endl
              << "If <files> are provided, each will be interpreted (left to right)." << std::endl
              << "If no <files> are provided, -i is assumed." << std::endl << std::endl;
}

inline std::string inccmd(const char* arg) {
	return "(include \"" + str::escape<char>(std::string(arg)) + "\") ";
}

}

int main(int argc, char** argv) {
	using namespace scm;

    std::string inc_str;
    bool        rec_args    = false;
    bool        interact    = false;
    bool        run_server  = false;
	bool        run_sserver = false;
    bool        run_httpd   = false;
    bool        run_daemon  = false;
    int         port        = 8080;

    std::vector<std::string> sargs;
 
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        std::string next_arg = i + 1 < argc ? argv[i + 1] : "";

		if (rec_args)
			sargs.push_back(arg);
        else if (arg == "-i")
            interact = true;
        else if (arg == "-s")
            { run_server = true; if (!next_arg.empty()) { port = str::from_string<int>(next_arg); ++i; } }
		else if (arg == "-ss")
            { run_sserver = true; if (!next_arg.empty()) { port = str::from_string<int>(next_arg); ++i; } }
        else if (arg == "-h")
            { run_httpd = true; if (!next_arg.empty()) { port = str::from_string<int>(next_arg); ++i; } }
        else if (arg == "-d")
            { run_daemon = true; if (!next_arg.empty()) { port = str::from_string<int>(next_arg); ++i; } }
        else if (arg == "-?" || arg == "--help" || arg == "-help")
            { print_help(); return 0; }
		else if (arg == "-x")
			{ rec_args = true; inc_str += inccmd(next_arg.c_str()); ++i; }
		else if (arg == "-e")
			{ inc_str += next_arg; ++i; }
	    else if (arg == "-r")
	        rec_args = true;
	    else
			inc_str += inccmd(argv[i]);
    }

/*
    if (run_server)
        run_server_repl(sargs, port, inc_str);
	if (run_sserver)
		run_select_server_repl(sargs, port, inc_str);
    else if (run_httpd)
        run_httpd_repl(sargs, port, inc_str);
    else if (run_daemon)
        run_daemon_repl(sargs, port, inc_str);
    else
*/
        run_local_repl(sargs, interact || inc_str.empty(), inc_str);

    return 0;
}
