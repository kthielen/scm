#include <scm/eval/Data.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/eval/Util.hpp>
#include <scm/str/Util.hpp>
#include <algorithm>
#include <stdexcept>
#include <math.h>

#include <iostream>
#include <ios>
#include <iomanip>

using namespace str;
//using namespace boost::gregorian;
//using namespace boost::posix_time;

namespace scm {

bool Value::operator==(const Value& rhs) const       { return this == &rhs; }
void Value::format(std::ostream& out) const          { out << "#<UNKNOWN-TYPE>"; }

Symbol::Symbol(const std::string& v) : sym(lcase(v)) { }
const std::string& Symbol::value() const             { return this->sym; }
void Symbol::value(const std::string& v)             { this->sym = v; }
bool Symbol::operator==(const Value& rhs) const      { return expr_cast<Symbol>(&rhs) != 0 && this->sym == expr_cast<Symbol>(&rhs)->sym; } // { return this == &rhs; }
void Symbol::format(std::ostream& out) const         { out << this->sym; }
bool Symbol::query_var() const                       { return this->sym.at(0) == '?'; }

const Symbol* Symbol::non_query_var() const
{
    if (!query_var())
	return this;
    else
	return alloc_symbol(this->allocator(), std::string(this->sym.begin() + 1, this->sym.end()));
}

String::String(const std::string& v) : str(v)        { }
const std::string& String::value() const             { return this->str; }
void String::format(std::ostream& out) const         { out << "\"" << escape(this->str) << "\""; }
bool String::operator==(const Value& rhs) const      { return expr_cast<String>(&rhs) != 0 && this->str == expr_cast<String>(&rhs)->str; }

// a basic real/integer number
Number::Number(int v) : num(v)                       { }
Number::Number(unsigned int uv) : num(uv)            { }
Number::Number(const double& v) : num(v)             { }
Number::Number(long long v) : num(v)                 { }
const double& Number::value() const                  { return this->num; }
void Number::update(const double& v)                 { this->num = v; }
bool Number::operator==(const Value& rhs) const      { return expr_cast<Number>(&rhs) != 0 && this->num == expr_cast<Number>(&rhs)->num;  }
void Number::format(std::ostream& out) const
{
    if (this->num == floor(this->num))
        out << static_cast<int>(this->num);
    else
        out << std::fixed << std::setprecision(8) << this->num;
}

// a 2D matrix of bits
BitMatrix::BitMatrix(unsigned int c, unsigned int r, Allocator* a) : columns(c), rows(r)
{
	this->tsize = static_cast<int>(ceil(static_cast<float>(c * r) / 8.0f));
	this->bytes = new unsigned char[this->tsize];
	memset(this->bytes, 0, this->tsize);

	this->csize_val = a->allocate<Number>(this->columns);
	this->rsize_val = a->allocate<Number>(this->rows);
}

BitMatrix::~BitMatrix()
{
	delete[] this->bytes;
}

bool BitMatrix::operator==(const Value& rhs) const
{
	const BitMatrix* brhs = expr_cast<BitMatrix>(&rhs);

	if (brhs == 0)
		return false;
	else if (this->columns != brhs->columns || this->rows != brhs->rows)
		return false;
	else
		return memcmp(this->bytes, brhs->bytes, this->tsize) == 0;
}

bool BitMatrix::get(unsigned int c, unsigned int r)
{
	unsigned int bidx  = (r * this->columns) + c;
	unsigned int byidx = bidx >> 3;
	if (byidx >= this->tsize) throw std::runtime_error("Invalid bit-matrix index (" + to_string(c) + ", " + to_string(r) + ") into (" + to_string(this->columns) + ", " + to_string(this->rows) + ")");

	// bitidx = floor(bidx / 8) + remainder(bidx / 8)
	return (this->bytes[byidx] & (1 << (bidx & 7))) != 0;
}

void BitMatrix::set(unsigned int c, unsigned int r, bool s)
{
	unsigned int bidx  = (r * this->columns) + c;
	unsigned int byidx = bidx >> 3;
	if (byidx >= this->tsize) throw std::runtime_error("Invalid bit-matrix index (" + to_string(c) + ", " + to_string(r) + ") into (" + to_string(this->columns) + ", " + to_string(this->rows) + ")");

	// bitidx = floor(bidx / 8) + remainder(bidx / 8)
	if (s)
		this->bytes[byidx] |= (1 << (bidx & 7));
	else
		this->bytes[byidx] &= ~(1 << (bidx & 7));
}

Value* BitMatrix::column_size() const             { return this->csize_val; }
Value* BitMatrix::row_size() const                { return this->rsize_val; }
void   BitMatrix::format(std::ostream& out) const { out << "#<BIT-MATRIX>"; }

void BitMatrix::references(AllocVisitor& v)
{
	v.visit((AllocBase*&)this->csize_val);
	v.visit((AllocBase*&)this->rsize_val);
}

// a 2D matrix of 32-bit integers
IntMatrix::IntMatrix(unsigned int c, unsigned int r, Allocator* a) : columns(c), rows(r)
{
	this->tsize = c * r;
	this->data  = new int[this->tsize];
	memset(this->data, 0, c * r * sizeof(int));

	this->csize_val = a->allocate<Number>(this->columns);
	this->rsize_val = a->allocate<Number>(this->rows);
	this->data_val  = a->allocate<Number>(0);
}

bool IntMatrix::operator==(const Value& rhs) const
{
	const IntMatrix* mrhs = expr_cast<IntMatrix>(&rhs);

	if (mrhs == 0)
		return false;
	else if (this->columns != mrhs->columns || this->rows != mrhs->rows)
		return false;
	else
		return memcmp(this->data, mrhs->data, this->tsize * sizeof(unsigned int)) == 0;
}

Value* IntMatrix::get(unsigned int c, unsigned int r)
{
	unsigned int idx = (r * this->columns) + c;
	if (idx >= this->tsize) throw std::runtime_error("Invalid int-matrix index (" + to_string(c) + ", " + to_string(r) + ") into (" + to_string(this->columns) + ", " + to_string(this->rows) + ")");

	this->data_val->update(this->data[idx]);
	return this->data_val;
}

void IntMatrix::set(unsigned int c, unsigned int r, int v)
{
	unsigned int idx = (r * this->columns) + c;
	if (idx >= this->tsize) throw std::runtime_error("Invalid int-matrix index (" + to_string(c) + ", " + to_string(r) + ") into (" + to_string(this->columns) + ", " + to_string(this->rows) + ")");

	this->data[idx] = v;
}

int IntMatrix::cget(unsigned int c, unsigned int r) const
{
    return this->data[(r * this->columns) + c];
}

void IntMatrix::cset(unsigned int c, unsigned int r, int v)
{
    this->data[(r * this->columns) + c] = v;
}

unsigned int IntMatrix::column_length() const { return this->columns; }
unsigned int IntMatrix::row_length()    const { return this->rows;    }

void IntMatrix::remove_rows(unsigned int dr)
{
	if (dr == 0)
	{
		return;
	}
	else if (dr > this->rows)
	{
		throw std::runtime_error("Int-matrix asked to remove " + to_string(dr) + " rows but only " + to_string(this->rows) + " rows are defined.");
	}
	else if (dr == this->rows)
	{
		this->rows = 0;
		delete[] this->data;
		this->data = 0;
		this->tsize = 0;
	}
	else
	{
		unsigned int nrows = this->rows - dr;
		unsigned int nsize = this->columns * nrows;

		if (nsize > (2 * this->tsize))
		{
			int* ndata = new int[nsize];
			memcpy(ndata, this->data, nsize * sizeof(int));
			delete[] this->data;
			this->data = ndata;
		}

		this->tsize = nsize;
		this->rows  = nrows;
	}

	this->rsize_val->update(this->rows);
}

void IntMatrix::fill(int v)
{
	for (int i = 0; i < this->tsize; ++i)
		this->data[i] = v;
}

IntMatrix* IntMatrix::left_match_find_rows(ConsPair* colvals) const
{
	// convert input column values
	std::vector<int> col_vals;
	while (colvals != 0) { col_vals.push_back((int)(assert_type<Number>(colvals->head())->value())); colvals = assert_type<ConsPair>(colvals->tail()); }
	if (col_vals.size() > this->columns) return this->allocator()->allocate<IntMatrix>(0, 0, this->allocator());

	// find all rows with a left-partial-match to the input values
	IntMatrix* ret = this->allocator()->allocate<IntMatrix>(1, this->rows, this->allocator());
	unsigned int r = 0;

	for (int sr = 0; sr < this->rows; ++sr)
	{
		bool rmatch = true;

		for (unsigned int cvi = 0; cvi < col_vals.size(); ++cvi)
		{
			if (col_vals[cvi] != this->data[(sr * this->columns) + cvi])
			{
				rmatch = false;
				break;
			}
		}

		if (rmatch)
			ret->set(0, r++, sr);
	}

	ret->remove_rows(ret->rows - r);
	return ret;
}

IntMatrix* IntMatrix::copy() const
{
	IntMatrix* ret = this->allocator()->allocate<IntMatrix>(this->columns, this->rows, this->allocator());
	memcpy(ret->data, this->data, this->tsize * sizeof(int));
	return ret;
}

IntMatrix::~IntMatrix()                           { delete[] this->data; }
Value* IntMatrix::column_size() const             { return this->csize_val; }
Value* IntMatrix::row_size() const                { return this->rsize_val; }
void   IntMatrix::format(std::ostream& out) const { out << "#<INT-MATRIX>"; }

void IntMatrix::references(AllocVisitor& v)
{
	v.visit((AllocBase*&)this->csize_val);
	v.visit((AllocBase*&)this->rsize_val);
	v.visit((AllocBase*&)this->data_val);
}

// an array data type
ValArray::ValArray(unsigned int n, Allocator* a) : len(n)  { this->data = new Value*[n]; memset(this->data, 0, n * sizeof(unsigned int)); this->size_val = a->allocate<Number>(n); }
ValArray::~ValArray()                                      { delete[] this->data; }
Value* ValArray::get(unsigned int n)                       { return n >= this->len ? 0 : this->data[n]; }
void ValArray::set(unsigned int n, Value* v)               { if (n < this->len) this->data[n] = v; }
Value* ValArray::size() const                              { return this->size_val; }
void ValArray::references(AllocVisitor& v)                 { v.visit((AllocBase*&)this->size_val); for (int i = 0; i < this->len; ++i) v.visit((AllocBase*&)this->data[i]); }
void ValArray::format(std::ostream& out) const             { out << "#<ARRAY>"; }

bool ValArray::operator==(const Value& rhs) const
{
	const ValArray* arhs = expr_cast<ValArray>(&rhs);
	
	if (arhs == 0)
		return false;
	else if (this->len != arhs->len)
		return false;
	else
		for (int i = 0; i < this->len; ++i)
			if (this->data[i] == 0 && arhs->data[i] != 0)
				return false;
			else if (arhs->data[i] == 0)
				return false;
			else if (!(*(this->data[i]) == *(arhs->data[i])))
				return false;

	return true;
}

/*
DateTime::DateTime(const time_t& v) : dt(from_time_t(v)) { }
DateTime::DateTime(const ptime& v) : dt(v)               { }
time_t DateTime::ctime() const                           { tm tmv = to_tm(this->dt); return mktime(&tmv); }
const ptime& DateTime::value() const                     { return this->dt; }
bool DateTime::operator==(const Value& rhs) const        { return expr_cast<DateTime>(&rhs) != 0 && this->dt == expr_cast<DateTime>(&rhs)->dt; }
void DateTime::format(std::ostream& out) const           { out << "#" << trim<char>(to_simple_string(this->dt)) << "#"; }
*/
DateTime::DateTime(const time_t& v) : tmt(v)             { }
time_t DateTime::ctime() const                           { return this->tmt; }
bool DateTime::operator==(const Value& rhs) const        { return expr_cast<DateTime>(&rhs) != 0 && this->tmt == expr_cast<DateTime>(&rhs)->tmt; }
void DateTime::format(std::ostream& out) const           { out << "#" << tmt << "#"; }

IStream::IStream(std::istream* pis, bool af) : stream(pis), auto_free(af) { }
IStream::~IStream()                                                       { if (auto_free) delete this->stream; }
std::istream* IStream::value() const                                      { return this->stream; }
    
OStream::OStream(std::ostream* pos, bool af) : stream(pos), auto_free(af) { }
OStream::~OStream()                                                       { if (auto_free) delete this->stream; }
std::ostream* OStream::value() const                                      { return this->stream; }
    
IOStream::IOStream(std::iostream* pos, bool af) : stream(pos), auto_free(af) { }
IOStream::~IOStream()                                                        { if (auto_free) delete this->stream; }
std::iostream* IOStream::value() const                                       { return this->stream; }

// a cons pair
ConsPair::ConsPair(Value* h, Value* t) : headv(h), tailv(t) { }

Value* ConsPair::head() const { return this->headv; } void ConsPair::head(Value* h) { this->headv = h; }
Value* ConsPair::tail() const { return this->tailv; } void ConsPair::tail(Value* t) { this->tailv = t; }

bool ConsPair::operator==(const Value& rhs) const
{
    const ConsPair* r = expr_cast<ConsPair>(&rhs);
    if (this == r) return true;
    if (r == 0)    return false;

    const ConsPair* tv = this;

#   define PVAL_CMP(p1, p2) \
	if (((p1) == 0 && (p2) != 0) || ((p2) == 0 && (p1) != 0)) \
	    return false; \
	else if ((p1) != (p2) && !(*(p1) == *(p2))) \
	    return false

    while (r != 0 && tv != 0)
    {
        // compare the heads
        PVAL_CMP(tv->headv, r->headv);

        // compare the tail
        const ConsPair* rt  = expr_cast<ConsPair>(r->tailv);
        const ConsPair* tvt = expr_cast<ConsPair>(tv->tailv);

        if ((rt == 0 && r->tailv != 0) || (tvt == 0 && tv->tailv != 0))
        {
            PVAL_CMP(tv->tailv, r->tailv);
        }
        
        r  = rt;
        tv = tvt;
    }

    // if we get here, every element is equal between the sequences
    // provided that most sequence markers are at nil
    return r == 0 && tv == 0;
}

void ConsPair::format(std::ostream& out) const
{
    out << "(";

    const ConsPair* tv = this;

    while (tv != 0)
    {
        if (tv->headv != 0)
            tv->headv->format(out);
        else
            out << "()";

        if (tv->tailv != 0)
        {
            out << " ";

            if (expr_cast<ConsPair>(tv->tailv) == 0)
            {
                out << ". ";
                tv->tailv->format(out);
            }
        }

        tv = expr_cast<ConsPair>(tv->tailv);
    }

    out << ")";
}

void ConsPair::references(AllocVisitor& v)
{
    ConsPair* n = this;

    while (n != 0)
    {
        v.visit((AllocBase*&)n->headv);
        
        ConsPair* nn = expr_cast<ConsPair>(n->tailv);
        if (nn == 0)
            v.visit((AllocBase*&)n->tailv);
        else
            nn->marked = true;

        n = nn;
    }
}

// an environment frame
EnvironmentFrame::EnvironmentFrame(EnvironmentFrame* p) : parent(p) { }
void EnvironmentFrame::Define(const std::string& sym, Value* v)     { Define(alloc_symbol(this->allocator(), sym), v); }
void EnvironmentFrame::Set(const std::string& sym, Value* v)        { Set(alloc_symbol(this->allocator(), sym), v); }
Value* EnvironmentFrame::Lookup(const std::string& sym) const       { return Lookup(alloc_symbol(this->allocator(), sym)); }

void EnvironmentFrame::Define(const Symbol* sym, Value* v)
{
    VarFrame::const_iterator vfi = find(sym);
    if (vfi != this->frame.end())
        throw std::runtime_error("The variable '" + PrintExpression(sym) + "' is already defined in this context.");

    this->frame.push_back(std::pair<Symbol*, Value*>(const_cast<Symbol*>(sym), v));
}

void EnvironmentFrame::Set(const Symbol* sym, Value* v)
{
    VarFrame::iterator vfi = find(sym);
    
    if (vfi != this->frame.end())
        vfi->second = v;
    else if (this->parent != 0)
        this->parent->Set(sym, v);
    else
        throw std::runtime_error("The variable '" + PrintExpression(sym) + "' is not defined in this context, and can't be updated.");
}

Value* EnvironmentFrame::Lookup(const Symbol* sym) const
{
    VarFrame::const_iterator vfi = find(sym);

    if (vfi != this->frame.end())
        return vfi->second;
    else if (this->parent != 0)
        return this->parent->Lookup(sym);
    else
        throw std::runtime_error("The variable '" + PrintExpression(sym) + "' is not defined.");
}

EnvironmentFrame* EnvironmentFrame::Parent() const
{
    return this->parent;
}

void EnvironmentFrame::format(std::ostream& out) const
{
    out << "{";

    for (VarFrame::const_iterator vfi = this->frame.begin(); vfi != this->frame.end(); ++vfi)
    {
        if (vfi != this->frame.begin()) out << ", ";

	PrintExpression(vfi->first, out);
        out << " = ";
        PrintExpression(vfi->second, out);
    }

    out << "}:";

    if (this->parent == 0)
        out << "()";
    else
        this->parent->format(out);
}

bool EnvironmentFrame::HasImmediateValue(const Symbol* sym) const
{
    return find(sym) != this->frame.end();
}

Value* EnvironmentFrame::LookupImmediateValue(const Symbol* sym) const
{
    VarFrame::const_iterator vfi = find(sym);

    if (vfi == this->frame.end())
        throw std::runtime_error("The variable '" + PrintExpression(sym) + "' is not defined in the immediate environment frame.");
    else
        return vfi->second;
}

const EnvironmentFrame::VarFrame& EnvironmentFrame::value() const
{
    return this->frame;
}

EnvironmentFrame::VarFrame::iterator EnvironmentFrame::find(const Symbol* sym)
{
    for (VarFrame::iterator vfi = this->frame.begin(); vfi != this->frame.end(); ++vfi)
	if (vfi->first == sym)
	    return vfi;

    return this->frame.end();
}

EnvironmentFrame::VarFrame::const_iterator EnvironmentFrame::find(const Symbol* sym) const
{
    for (VarFrame::const_iterator vfi = this->frame.begin(); vfi != this->frame.end(); ++vfi)
	if (vfi->first == sym)
	    return vfi;

    return this->frame.end();
}

void EnvironmentFrame::Clear()
{
    this->frame.clear();
}

void EnvironmentFrame::references(AllocVisitor& v)
{
    v.visit((AllocBase*&)parent);

    for (VarFrame::iterator vfi = this->frame.begin(); vfi != this->frame.end(); ++vfi)
    {
	v.visit((AllocBase*&)vfi->first);
	v.visit((AllocBase*&)vfi->second);
    }
}

// the function formatter
void Function::format(std::ostream& out) const { out << "#<FUNCTION>"; }

// a lambda function
LambdaFunction::LambdaFunction(Value* al, ConsPair* body_exprs, EnvironmentFrame* env) : argl(prep_argl(al)), body(body_exprs), lex_env(env)
{
}

Value* LambdaFunction::Invoke(ConsPair* args, EnvironmentFrame* env)
{
    // start a new call-frame
    EnvironmentFrame* call_frame = allocator()->allocate<EnvironmentFrame>(this->lex_env);
    gcguard<EnvironmentFrame> cfguard(call_frame);

    // try to evaluate all necessary arguments, and curry if a subset was provided
    if (ConsPair* rest_args = CurryBind(this->argl, args, env, call_frame))
        return allocator()->allocate<LambdaFunction>(rest_args, this->body, call_frame);
    else
        return EvalForEach(this->body, call_frame);
}

ConsPair* LambdaFunction::CurryBind(Value* arg_names, ConsPair* arg_vals, EnvironmentFrame* eval_env, EnvironmentFrame* fill_env) const
{
    if (arg_names == 0)
    {
        if (arg_vals != 0)
            throw std::runtime_error("Too many arguments [" + PrintExpression(arg_vals) + "] for the parameter list nil.");
        else
            return 0;
    }
    else if (Symbol* varargl = expr_cast<Symbol>(arg_names))
    {
        if (thunksym(varargl))
            fill_env->Define(unthunksym(varargl), Delay(EnList(arg_vals, allocator()), eval_env));
        else
            fill_env->Define(varargl, EvalMap(arg_vals, eval_env));

        return 0;
    }
    else if (ConsPair* pparams = expr_cast<ConsPair>(arg_names))
    {
        ConsPair* cargs = arg_vals;

        while (pparams != 0 && arg_vals != 0)
        {
            Symbol* sym = assert_type<Symbol>(pparams->head());

            if (thunksym(sym))
                fill_env->Define(unthunksym(sym), Delay(arg_vals->head(), eval_env));
            else
                fill_env->Define(sym, Eval(arg_vals->head(), eval_env));

            if (Symbol* rsym = expr_cast<Symbol>(pparams->tail()))
            {
                if (thunksym(rsym))
                    fill_env->Define(rsym, Delay(EnList(arg_vals->tail(), allocator()), eval_env));
                else
                    fill_env->Define(rsym, EvalMap(expr_cast<ConsPair>(arg_vals->tail()), eval_env));

                pparams  = 0;
                arg_vals = 0;
                break;
            }

            pparams  = expr_cast<ConsPair>(pparams->tail());
            arg_vals = expr_cast<ConsPair>(arg_vals->tail());
        }

        if (arg_vals != 0)
            throw std::runtime_error("Too many arguments [" + PrintExpression(cargs) + "] for the parameter list [" + PrintExpression(arg_names) + "].");
        else
            return pparams;
    }
    else
    {
        throw std::runtime_error("Lambda parameter list expected, but received '" + PrintExpression(arg_names) + "' instead.");
    }
}

Value* LambdaFunction::prep_argl(Value* argl)
{
    if (thunk_arg(argl))
        return thunkref(argl);
    else if (expr_cast<ConsPair>(argl) == 0)
        return argl;

    ConsPair* pargl = (ConsPair*)argl;
    pargl->head(prep_arg(pargl->head()));
    pargl->tail(prep_argl(pargl->tail()));
    return argl;
}

Value* LambdaFunction::prep_arg(Value* arg)
{
    if (thunk_arg(arg))
        return thunkref(arg);
    else
        return arg;
}

bool LambdaFunction::thunk_arg(Value* v)
{
    return expr_cast<ConsPair>(v) != 0 && expr_cast<Symbol>(((ConsPair*)v)->head()) != 0 && ((Symbol*)(((ConsPair*)v)->head()))->value() == "lambda" &&
           expr_cast<ConsPair>(((ConsPair*)v)->tail()) != 0 && expr_cast<ConsPair>(((ConsPair*)((ConsPair*)v)->tail())->tail()) != 0 &&
           expr_cast<Symbol>(((ConsPair*)((ConsPair*)((ConsPair*)v)->tail())->tail())->head()) != 0;
}

Value* LambdaFunction::thunkref(Value* v)
{
    return alloc_symbol(v->allocator(), "~" + ((Symbol*)((ConsPair*)((ConsPair*)((ConsPair*)v)->tail())->tail())->head())->value());
}

bool LambdaFunction::thunksym(Symbol* s)
{
    return s->value().size() > 1 && s->value()[0] == '~';
}

Symbol* LambdaFunction::unthunksym(Symbol* s)
{
    return alloc_symbol(s->allocator(), s->value().substr(1, s->value().size()));
}

Value* LambdaFunction::Delay(Value* v, EnvironmentFrame* env) const
{
    return allocator()->allocate<LambdaFunction>((Value*)0, allocator()->allocate<ConsPair>(v, (Value*)0), env);
}

void LambdaFunction::references(AllocVisitor& v)
{
    v.visit((AllocBase*&)argl);
    v.visit((AllocBase*&)body);
    v.visit((AllocBase*&)lex_env);
}

}
