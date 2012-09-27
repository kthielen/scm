#include <scm/eval/Data.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/modules/std/FileIO.hpp>
#include <scm/str/Util.hpp>

#include <unistd.h>
#include <dlfcn.h>
#include <fstream>

#include <list>
#include <stack>

namespace scm {

std::string SSystem(const std::string& cmd) {
    int ostdo = dup(STDOUT_FILENO);
    int pio[2]; // 0 = read, 1 = write

    if (pipe(pio) != 0) {
        throw std::runtime_error("Unable to allocate pipe to capture spawned process output.");
	}

    if (dup2(pio[1], STDOUT_FILENO) == -1) {
        throw std::runtime_error("Unable to bind local pipe to STDOUT.");
	}

    system(cmd.c_str());

    dup2(ostdo, STDOUT_FILENO);
    close(pio[1]);

    std::string ret;
    char buf[256];
    int  n;
    while (n = read(pio[0], buf, sizeof(buf))) {
        ret.append(buf, n);
	}

    close(pio[0]);
    return ret;
}

int LSystem(const std::string& cmd) {
    return system(cmd.c_str());
}

std::string LEnv(const std::string& evar) {
    const char* ev = getenv(evar.c_str());
    return ev != 0 ? std::string(ev) : std::string();
}

void DLLoad(const std::string& fname, EnvironmentFrame* env) {
    void* h = dlopen(fname.c_str(), RTLD_NOW);
    if (h == 0) throw std::runtime_error("Unable to load module: " + fname);

    typedef void (*PINITPROC)(EnvironmentFrame*);
    PINITPROC initproc = (PINITPROC)dlsym(h, "init");
    if (!initproc) throw std::runtime_error("Invalid module format: " + fname);

    initproc(env);
}

void DLLoadEnv(const std::string& fname, Value* eframe, EnvironmentFrame* env) {
    gcguard<EnvironmentFrame> e = assert_type<EnvironmentFrame>(Eval(eframe, env));
    DLLoad(fname, e.ptr());
}

bool importScriptFile(const std::string& fname, EnvironmentFrame* env) {
	std::ifstream f(fname.c_str());
	if (!f.is_open()) {
		return false;
	} else {
		f.close();
		Include(fname, env);
		return true;
	}
}

void ModuleImportAs(std::string mpath, const std::string& mname, EnvironmentFrame* env) {
	Allocator*      a    = env->allocator();
	gcguard<Symbol> msym = alloc_symbol(a, mname);

	// end the import if it's already happened (or a value with the same name is defined in any case)
	if (env->HasImmediateValue(msym.ptr())) {
		return;
	}

	gcguard<EnvironmentFrame> menv = a->allocate<EnvironmentFrame>(env);
	if (mpath.size() > 0 && mpath[0] != '/') {
		mpath = "./" + mpath;
	}

	// allocate a fresh environment and import definitions from <name> into it by one of the following methods:
	//  * evaluate the script in <name>/init.scm
	//  * load lib<name>.so
	if (!importScriptFile(mpath + "/init.scm", menv.ptr())) {
		if (!importScriptFile(LEnv("SCM_MODULE_BASE") + "/" + mpath + "/init.scm", menv.ptr())) {
			throw std::runtime_error("Unable to find module '" + mpath + "' to load.");
		}
	}

	env->Define(msym.ptr(), menv.ptr());
}

void MImport(Value* mname, EnvironmentFrame* env) {
	Symbol* s = assert_type<Symbol>(mname);
	ModuleImportAs(s->value(), s->value(), env);
}

void MImportAs(const std::string& fpath, Value* mname, EnvironmentFrame* env) {
	Symbol* s = assert_type<Symbol>(mname);
	ModuleImportAs(fpath, s->value(), env);
}

std::string getPWD() {
	char pwd[4096]; pwd[0] = '\0';
	getcwd(pwd, sizeof(pwd));
	return std::string(pwd);
}

typedef std::stack<std::string> Dirs;
Dirs prevDirs;

void pushD(const std::string& dir) {
	prevDirs.push(getPWD());
	chdir(dir.c_str());
}

void popD() {
	if (!prevDirs.empty()) {
		chdir(prevDirs.top().c_str());
		prevDirs.pop();
	}
}

void InitSystemStdEnv(EnvironmentFrame* env) {
    Allocator* alloc = env->allocator();

    // allocate system functions
    env->Define("system",    bindfn(alloc, &LSystem));
    env->Define("env-var",   bindfn(alloc, &LEnv));
    env->Define("load",      bindfn(alloc, &DLLoad));
    env->Define("load/env",  bindfn(alloc, &DLLoadEnv));
	env->Define("import",    bindfn(alloc, &MImport));
	env->Define("import-as", bindfn(alloc, &MImportAs));

    env->Define("pid",     bindfn(alloc, &getpid));
	env->Define("pwd",     bindfn(alloc, &getPWD));
    env->Define("ssystem", bindfn(alloc, &SSystem));
	env->Define("pushd",   bindfn(alloc, &pushD));
	env->Define("popd",    bindfn(alloc, &popD));
}

}
