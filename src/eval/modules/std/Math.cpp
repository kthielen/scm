
#include <scm/eval/Data.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/Util.hpp>
#include <math.h>

#include <iostream>
#include <iomanip>

namespace scm {

// arithmetic operations
double add     (double v1, double v2) { return v1 + v2; }
double subtract(double v1, double v2) { return v1 - v2; }
double multiply(double v1, double v2) { return v1 * v2; }
double divide  (double v1, double v2) { return v1 / v2; }

// basic predicates
bool gt (double v1, double v2) { return v1 > v2;  }
bool gte(double v1, double v2) { return v1 >= v2; }
bool lt (double v1, double v2) { return v1 < v2;  }
bool lte(double v1, double v2) { return v1 <= v2; }

bool eq(Value* v1, Value* v2, EnvironmentFrame* env)
{
    gcguard<Value> gv1 = Eval(v1, env);
    gcguard<Value> gv2 = Eval(v2, env);

    Value* ev1 = gv1.ptr();
    Value* ev2 = gv2.ptr();

    if (ev1 == ev2)
        return true;
    else if (ev1 == 0 || ev2 == 0)
        return false;
    else
        return *ev1 == *ev2;
}

bool neq(Value* v1, Value* v2, EnvironmentFrame* env)
{
    return !eq(v1, v2, env);
}

double fms_ceil(double v)           { return ceil(v); }
double fms_floor(double v)          { return floor(v); }
double fms_mod(double v, double r)  { return ((int)v) % ((int)r); }
double fms_log(double v)            { return log(v); }
double fms_pow(double b, double ex) { return pow(b, ex); }
double fms_sqrt(double v)           { return sqrt(v); }
double fms_abs(double v)            { return fabs(v); }

double fms_round(double v) {
	double fv = floor(v);
	double cv = ceil(v);

	double fd = fabs(fv) - fabs(v);
	double cd = fabs(cv) - fabs(v);

	if (fabs(fd) > fabs(cd)) {
		return cv;
	} else {
		return fv;
	}
}

#define DESTRUCTIVE_ARITH(FnName, OP) \
	Value* FnName(Value* nv, Value* iv, EnvironmentFrame* env) \
	{ \
		gcguard<Number> n1 = assert_type<Number>(Eval(nv, env)); \
		Number* n2 = assert_type<Number>(Eval(iv, env)); \
		n1->update(n1->value() OP n2->value()); \
		return n1.ptr(); \
	}

DESTRUCTIVE_ARITH(Incr, +);
DESTRUCTIVE_ARITH(Decr, -);
DESTRUCTIVE_ARITH(Mult, *);
DESTRUCTIVE_ARITH(SDiv, /);

// bitwise arithmetic
unsigned int L_bitor (unsigned int v1, unsigned int v2) { return v1 | v2; }
unsigned int L_bitand(unsigned int v1, unsigned int v2) { return v1 & v2; }
unsigned int L_bitxor(unsigned int v1, unsigned int v2) { return v1 ^ v2; }
unsigned int L_bitnot(unsigned int v)                   { return ~v; }
unsigned int L_bitshl(unsigned int v, unsigned int d)   { return v << d; }
unsigned int L_bitshr(unsigned int v, unsigned int d)   { return v >> d; }

// bit-matrix support
Value* MakeBitMatrix(unsigned int columns, unsigned int rows, Allocator* a)                      { return a->allocate<BitMatrix>(columns, rows, a); }
bool   ReadBitMatrix(Value* v, unsigned int col, unsigned int row, EnvironmentFrame* e)          { return assert_type<BitMatrix>(Eval(v, e))->get(col, row); }
void   WriteBitMatrix(Value* v, unsigned int col, unsigned int row, bool s, EnvironmentFrame* e) { assert_type<BitMatrix>(Eval(v, e))->set(col, row, s); }
Value* BitMatrixCols(Value* v, EnvironmentFrame* e)                                              { return assert_type<BitMatrix>(Eval(v, e))->column_size(); }
Value* BitMatrixRows(Value* v, EnvironmentFrame* e)                                              { return assert_type<BitMatrix>(Eval(v, e))->row_size(); }

// int-matrix support
Value* MakeIntMatrix(unsigned int columns, unsigned int rows, Allocator* a)                           { return a->allocate<IntMatrix>(columns, rows, a); }
Value* ReadIntMatrix(Value* v, unsigned int c, unsigned int r, EnvironmentFrame* e)                   { return assert_type<IntMatrix>(Eval(v, e))->get(c, r); }
void   WriteIntMatrix(Value* v, unsigned int c, unsigned int r, unsigned int nv, EnvironmentFrame* e) { assert_type<IntMatrix>(Eval(v, e))->set(c, r, nv); }
void   FillIntMatrix(Value* v, int fv, EnvironmentFrame* e)                                           { assert_type<IntMatrix>(Eval(v, e))->fill(fv); }
Value* IntMatrixCols(Value* v, EnvironmentFrame* e)                                                   { return assert_type<IntMatrix>(Eval(v, e))->column_size(); }
Value* IntMatrixRows(Value* v, EnvironmentFrame* e)                                                   { return assert_type<IntMatrix>(Eval(v, e))->row_size(); }
void   IntMatrixRemoveRows(Value* v, unsigned int r, EnvironmentFrame* e)                             { assert_type<IntMatrix>(Eval(v, e))->remove_rows(r); }
Value* IntMatrixLeftFindRows(Value* im, Value* cvs, EnvironmentFrame* e)                              { return assert_type<IntMatrix>(Eval(im, e))->left_match_find_rows(assert_type<ConsPair>(Eval(cvs, e))); }
Value* IntMatrixCopy(Value* im, EnvironmentFrame* e)                                                  { return assert_type<IntMatrix>(Eval(im, e))->copy(); }

std::string print_unsigned(unsigned int n)
{
    std::ostringstream ss; ss << n; return ss.str();
}

double fms_cos(double v) { return cos(v); }
double fms_sin(double v) { return sin(v); }
double fms_acos(double v) { return acos(v); }
double fms_asin(double v) { return asin(v); }

void InitMathStdEnv(EnvironmentFrame* env)
{
    Allocator* alloc = env->allocator();

    // allocate arithmetic functions
    env->Define("dy+", bindfn(alloc, &add));
    env->Define("dy-", bindfn(alloc, &subtract));
    env->Define("dy*", bindfn(alloc, &multiply));
    env->Define("dy/", bindfn(alloc, &divide));

	// import trig functions	
	env->Define("cos",  bindfn(alloc, &fms_cos));
	env->Define("sin",  bindfn(alloc, &fms_sin));
	env->Define("acos", bindfn(alloc, &fms_acos));
	env->Define("asin", bindfn(alloc, &fms_asin));

    // import some of the standard C math functions
    env->Define("ceil",  bindfn(alloc, &fms_ceil));
    env->Define("floor", bindfn(alloc, &fms_floor));
	env->Define("round", bindfn(alloc, &fms_round));
    env->Define("mod",   bindfn(alloc, &fms_mod));
    env->Define("ln",    bindfn(alloc, &fms_log));
    env->Define("pow",   bindfn(alloc, &fms_pow));
	env->Define("sqrt",  bindfn(alloc, &fms_sqrt));
	env->Define("abs",   bindfn(alloc, &fms_abs));

	// add destructive-update functions for numeric values
	env->Define("incr", bindfn(alloc, &Incr));
	env->Define("decr", bindfn(alloc, &Decr));
	env->Define("mult", bindfn(alloc, &Mult));
	env->Define("sdiv", bindfn(alloc, &SDiv));

	// bitwise arithmetic
	env->Define("bit/or",  bindfn(alloc, &L_bitor));
	env->Define("bit/and", bindfn(alloc, &L_bitand));
	env->Define("bit/xor", bindfn(alloc, &L_bitxor));
	env->Define("bit/not", bindfn(alloc, &L_bitnot));
	env->Define("bit/shl", bindfn(alloc, &L_bitshl));
	env->Define("bit/shr", bindfn(alloc, &L_bitshr));

	// bit-matrix support
	env->Define("make-bit-matrix",    bindfn(alloc, &MakeBitMatrix));
	env->Define("bit-matrix-get",     bindfn(alloc, &ReadBitMatrix));
	env->Define("bit-matrix-set",     bindfn(alloc, &WriteBitMatrix));
	env->Define("bit-matrix-columns", bindfn(alloc, &BitMatrixCols));
	env->Define("bit-matrix-rows",    bindfn(alloc, &BitMatrixRows));

	// int-matrix support
	env->Define("make-int-matrix",           bindfn(alloc, &MakeIntMatrix));
	env->Define("int-matrix-get",            bindfn(alloc, &ReadIntMatrix));
	env->Define("int-matrix-set",            bindfn(alloc, &WriteIntMatrix));
	env->Define("int-matrix-fill",           bindfn(alloc, &FillIntMatrix));
	env->Define("int-matrix-columns",        bindfn(alloc, &IntMatrixCols));
	env->Define("int-matrix-rows",           bindfn(alloc, &IntMatrixRows));
	env->Define("int-matrix-left-find-rows", bindfn(alloc, &IntMatrixLeftFindRows));
	env->Define("int-matrix-remove-rows",    bindfn(alloc, &IntMatrixRemoveRows));
	env->Define("int-matrix-copy",           bindfn(alloc, &IntMatrixCopy));

    // allocate basic predicates
    env->Define("=",   bindfn(alloc, &eq));
    env->Define("<>",  bindfn(alloc, &neq));
    env->Define(">",   bindfn(alloc, &gt));
    env->Define(">=",  bindfn(alloc, &gte));
    env->Define("<",   bindfn(alloc, &lt));
    env->Define("<=",  bindfn(alloc, &lte));

    // random number generation
    env->Define("srand",    bindfn(alloc, &srand));
    env->Define("rand",     bindfn(alloc, &rand));
    env->Define("rand-max", alloc->allocate<Number>(RAND_MAX));

    // simple utilities
    env->Define("print-unsigned", bindfn(alloc, &print_unsigned));
}

}
