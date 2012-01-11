#ifndef EVAL_DATA_H_INCLUDED
#define EVAL_DATA_H_INCLUDED

#include <scm/eval/Allocator.hpp>
#include <scm/str/Util.hpp>
#include <string>
#include <iostream>
#include <map>
#include <stdexcept>
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <time.h>

#define CTOR(n)
#define MEM(n)
#define VMEM(n)

namespace scm {

class Value : public AllocBase {
public:
    virtual bool operator==(const Value& rhs) const;
    virtual void format(std::ostream& out) const;
};

class ConsPair : public Value {
public:
    ConsPair(Value* h, Value* t);

    Value* head() const; void head(Value* h);
    Value* tail() const; void tail(Value* t);

    bool operator==(const Value& rhs) const;
    void format(std::ostream& out) const;
private:
    Value* headv;
    Value* tailv;
public:
    void references(AllocVisitor& v);
};

class Number : public Value {
public:
    Number(int v);
    Number(unsigned int uv);
    Number(const double& v);
    Number(long long lv);

    const double& value() const;
    bool operator==(const Value& rhs) const;
    void format(std::ostream& out) const;

    void update(const double& v);
private:
    double num;
};

class ValArray : public Value {
public:
	ValArray(unsigned int n, Allocator* a);
	~ValArray();

    bool operator==(const Value& rhs) const;
	void format(std::ostream& out) const;
	Value* get(unsigned int n);
	void set(unsigned int n, Value* v);
	Value* size() const;
private:
	Value**      data;
	unsigned int len;
	Value*       size_val;
public:
    void references(AllocVisitor& v);
};

class BitMatrix : public Value {
public:
	BitMatrix(unsigned int cols, unsigned int rows, Allocator* a);
	~BitMatrix();

    bool operator==(const Value& rhs) const;
	void format(std::ostream& out) const;
	bool get(unsigned int c, unsigned int r);
	void set(unsigned int c, unsigned int r, bool s);

	Value* column_size() const;
	Value* row_size() const;
private:
	unsigned char* bytes;
	unsigned int   columns;
	unsigned int   rows;
	unsigned int   tsize;
	Value*         csize_val;
	Value*         rsize_val;
public:
    void references(AllocVisitor& v);
};

class IntMatrix : public Value {
public:
	IntMatrix(unsigned int cols, unsigned int rows, Allocator* a);
	~IntMatrix();

    bool operator==(const Value& rhs) const;
	void format(std::ostream& out) const;
	Value* get(unsigned int c, unsigned int r);
	void set(unsigned int c, unsigned int r, int v);
	void remove_rows(unsigned int dr);
	IntMatrix* left_match_find_rows(ConsPair* colvals) const;
	IntMatrix* copy() const;
	void fill(int v);

	Value* column_size() const;
	Value* row_size() const;

    int cget(unsigned int c, unsigned int r) const;
    void cset(unsigned int c, unsigned int r, int v);

    unsigned int column_length() const;
    unsigned int row_length() const;
private:
	int*         data;
	unsigned int columns;
	unsigned int rows;
	unsigned int tsize;
	Number*       csize_val;
	Number*       rsize_val;
	Number*      data_val;
public:
    void references(AllocVisitor& v);
};

class String : public Value {
public:
    String(const std::string& v);

    const std::string& value() const;
    bool operator==(const Value& rhs) const;
    void format(std::ostream& out) const;
	void update(const std::string& v) { this->str = v; }
private:
    std::string str;
};

class DateTime : public Value {
public:
    DateTime(const time_t& v);

    time_t ctime() const;
    const time_t& value() const;
    bool operator==(const Value& rhs) const;
    void format(std::ostream& out) const;
private:
	time_t tmt;
};

class IStream : public Value {
public:
    IStream(std::istream* pis, bool auto_free);
    ~IStream();

    std::istream* value() const;
private:
    std::istream* stream;
    bool          auto_free;
};

class OStream : public Value {
public:
    OStream(std::ostream* pos, bool auto_free);
    ~OStream();

    std::ostream* value() const;
private:
    std::ostream* stream;
    bool          auto_free;
};

class IOStream : public Value {
public:
	IOStream(std::iostream* p, bool auto_free);
	~IOStream();

	std::iostream* value() const;
private:
	std::iostream* stream;
	bool           auto_free;
};

class Symbol : public Value {
public:
    Symbol(const std::string& v);

    const std::string& value() const;
    void value(const std::string& v);
    bool operator==(const Value& rhs) const;
    void format(std::ostream& out) const;

    bool query_var() const;
    const Symbol* non_query_var() const;
private:
    std::string sym;
};

class EnvironmentFrame : public Value {
public:
    typedef std::vector< std::pair<Symbol*, Value*> > VarFrame;
    
    EnvironmentFrame(EnvironmentFrame* parent = 0);

    void Define(const std::string& sym, Value* v);
    void Define(const Symbol* sym, Value* v);
    void Set(const std::string& sym, Value* v);
    void Set(const Symbol* sym, Value* v);

    Value* Lookup(const std::string& sym) const;
    Value* Lookup(const Symbol* sym) const;

    // used for pattern-matching support
    bool   HasImmediateValue(const Symbol* sym) const;
    Value* LookupImmediateValue(const Symbol* sym) const;
    void   Clear();

    void format(std::ostream& out) const;
    const VarFrame& value() const;
    VarFrame::iterator find(const Symbol* s);
    VarFrame::const_iterator find(const Symbol* s) const;
    EnvironmentFrame* Parent() const;
private:
    VarFrame          frame;
    EnvironmentFrame* parent;
public:
    void references(AllocVisitor& v);
};

class Function : public Value {
public:
    virtual Value* Invoke(ConsPair* arguments, EnvironmentFrame* env) = 0;
    void format(std::ostream& out) const;
};

class LambdaFunction : public Function {
public:
    LambdaFunction(Value* argl, ConsPair* body, EnvironmentFrame* lex_env);

    Value* Invoke(ConsPair* args, EnvironmentFrame* env);
private:
    Value*            argl;
    ConsPair*         body;
    EnvironmentFrame* lex_env;
public:
    void references(AllocVisitor& v);

    ConsPair* CurryBind(Value* arg_names, ConsPair* arg_vals, EnvironmentFrame* eval_env, EnvironmentFrame* fill_env) const;
    static Value* prep_argl(Value* argl);
    static Value* prep_arg(Value* arg);
    static bool   thunk_arg(Value* v);
    static Value* thunkref(Value* v);

    static bool thunksym(Symbol* s);
    static Symbol* unthunksym(Symbol* s);

    Value* Delay(Value* v, EnvironmentFrame* env) const;
};

// a simple tool for 'boxing' arbitrary C++ types
template <typename T>
    class Box : public Value {
    public:
        Box(const T& cv) : v(cv) { }

        T& value() { return v; }
        const T& value() const { return v; }
		void value(const T& x) { this->v = x; }

        void format(std::ostream& out) const {
			out << this->v;
        }

		bool operator==(const Box<T>& rhs) const {
			return v == rhs.v;
		}
    private:
        T v;
    };

// an auto_ptr-style "smart pointer" for use with Box<T>
template <typename T>
	class ptr {
    public:
		ptr()                : p(0)   { }
		ptr(T* v)            : p(v)   { }
		ptr(const ptr<T>& c) : p(c.p) { const_cast< ptr<T>& >(c).p = 0; }
		~ptr()                        { delete p; }

        ptr<T>& operator=(const ptr<T>& c) { if (c.p != p) delete p; p = c.p; const_cast< ptr<T>& >(c).p = 0; return *this; }

        const T& operator*() const { return *p; }
        T& operator*() { return *p; }

        const T* operator->() const { return p; }
        T* operator->() { return p; }

		const T* value() const { return p; }
        T* value() { return p; }
    private:
        T* p;
    };

template <typename T>
    T* expr_cast(Value* v) {
        return dynamic_cast<T*>(v);
    }

template <typename T>
	const T* expr_cast(const Value* v) {
        return dynamic_cast<const T*>(v);
    }

// format from a pointer to a value
template <typename Char, typename T>
	std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& os, const ptr<T>& v) {
		if (const T* vp = v.value()) {
			os << *vp;
		} else {
			os << "()";
		}

		return os;
	}

template <typename Char>
    std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& os, Value* v) {
        if (v != 0) {
            v->format(os);
        } else {
            os << "()";
		}

        return os;
    };

}

#endif
