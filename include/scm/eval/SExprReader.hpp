#ifndef EVAL_H_INCLUDED
#define EVAL_H_INCLUDED

#include <scm/eval/Data.hpp>
#include <iostream>
#include <string>

namespace scm {

Value* ReadSExpression(std::istream& input, Allocator& alloc);
Value* ReadSExpression(const std::string& input, Allocator& alloc);

void PrintExpression(const Value* v, std::ostream& output);
std::string PrintExpression(const Value* v);

}

#endif
