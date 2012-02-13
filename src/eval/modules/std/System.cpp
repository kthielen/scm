#include <scm/eval/Data.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/str/Util.hpp>

#include <unistd.h>
#include <dlfcn.h>

#include <list>

namespace scm {

std::string SSystem(const std::string& cmd) {
    int ostdo = dup(STDOUT_FILENO);
    int pio[2]; // 0 = read, 1 = write

    if (pipe(pio) != 0)
        throw std::runtime_error("Unable to allocate pipe to capture spawned process output.");

    if (dup2(pio[1], STDOUT_FILENO) == -1)
        throw std::runtime_error("Unable to bind local pipe to STDOUT.");

    system(cmd.c_str());

    dup2(ostdo, STDOUT_FILENO);
    close(pio[1]);

    std::string ret;
    char buf[256];
    int  n;
    while (n = read(pio[0], buf, sizeof(buf)))
        ret.append(buf, n);

    close(pio[0]);
    return ret;
}

void DLLoad(const std::string& fname, EnvironmentFrame* env) {
    void* h = dlopen(fname.c_str(), RTLD_NOW);
    if (h == 0) throw std::runtime_error("Unable to load module: " + fname);

    typedef void (*PINITPROC)(EnvironmentFrame*);
    PINITPROC initproc = (PINITPROC)dlsym(h, "init");
    if (!initproc) throw std::runtime_error("Invalid module format: " + fname);

    initproc(env);
}

int LSystem(const std::string& cmd) {
    return system(cmd.c_str());
}

std::string LEnv(const std::string& evar) {
    const char* ev = getenv(evar.c_str());
    return ev != 0 ? std::string(ev) : std::string();
}

void DLLoadEnv(const std::string& fname, Value* eframe, EnvironmentFrame* env) {
    gcguard<EnvironmentFrame> e = assert_type<EnvironmentFrame>(Eval(eframe, env));
    DLLoad(fname, e.ptr());
}

void DLImport(Value* msym, EnvironmentFrame* env) {
	Symbol*           smsym = assert_type<Symbol>(msym);
	Allocator*        alloc = smsym->allocator();
	EnvironmentFrame* menv  = alloc->allocate<EnvironmentFrame>((EnvironmentFrame*)0);

	DLLoad("lib" + smsym->value() + ".so", menv);
	env->Define(smsym->value(), menv);
}

void InitSystemStdEnv(EnvironmentFrame* env) {
    Allocator* alloc = env->allocator();

    // allocate system functions
    env->Define("system",   bindfn(alloc, &LSystem));
    env->Define("env-var",  bindfn(alloc, &LEnv));
    env->Define("load",     bindfn(alloc, &DLLoad));
    env->Define("load/env", bindfn(alloc, &DLLoadEnv));
	env->Define("import",   bindfn(alloc, &DLImport));

    env->Define("pid", bindfn(alloc, &getpid));
    env->Define("ssystem", bindfn(alloc, &SSystem));
}

}
