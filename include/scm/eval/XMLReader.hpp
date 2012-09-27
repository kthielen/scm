#ifndef EVAL_XMLREADER_H_INCLUDED
#define EVAL_XMLREADER_H_INCLUDED

#include <scm/eval/Data.hpp>
#include <iostream>
#include <string>

namespace scm {

Value* ReadXML(std::istream* input, Allocator* alloc);
Value* ReadXML(const std::string& input, Allocator* alloc);

}

#endif
