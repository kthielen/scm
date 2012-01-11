#ifndef EVAL_STDMODULE_FILEIO_H_INCLUDED
#define EVAL_STDMODULE_FILEIO_H_INCLUDED

#include <scm/eval/Data.hpp>
#include <vector>
#include <list>
#include <fstream>
#include <string>

namespace scm {

void InitFileIOStdEnv(EnvironmentFrame* env);

void Include(const std::string& fname, EnvironmentFrame* env);
std::list<std::string> Ls(const std::string& dirname);
void make_directory(const std::string& dirname);
bool IsDirectoryP(const std::string& dirname);

}

#endif
