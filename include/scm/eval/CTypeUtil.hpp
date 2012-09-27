#ifndef EVAL_CTYPEUTIL_H_INCLUDED
#define EVAL_CTYPEUTIL_H_INCLUDED

#include <scm/eval/Allocator.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/str/Util.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <set>
#include <map>

#ifndef WIN32
#include <ext/hash_map>
#ifndef stdext
#define stdext __gnu_cxx
#endif
#endif

namespace scm {

#define DEFAULT_ARG_SET(T) \
	static void argSet(ConsPair* an, T p) { \
		an->head(alloc(an->allocator(), p)); \
	} \

#define DEFAULT_ARG_SET_DELAY(T) \
	static void argSet(ConsPair* an, T p) { \
		an->head(EnQuote(alloc(an->allocator(), p), an->allocator())); \
	} \

// the default binding method is to use opaque "boxes"
template <typename T> struct CToLisp {
	typedef Box<T> BoxType;
	static Value* alloc(Allocator* a, const T& v) { return a->template allocate< Box<T> >(v); }

	static void argSet(ConsPair* an, const T& v) {
		((Box<T>*)(an->head()))->value(v);
	}

	static std::string desc() {
		return str::short_type_name<T>();
	}
};

// allow identity transforms for types that should translate naturally to interpreter types
#define IDENTITY_SCM_TYPE_TRANSFORM(T) \
	template <> struct CToLisp< T* > { \
		typedef T* BoxType; \
		static Value* alloc(Allocator* a, T* v) { return v; } \
		static std::string desc() { return str::short_type_name<T>(); } \
	}; \
	template <> struct LispToC< T* > { \
		static T* unbox(Value* v) { return assert_type<T>(v); } \
		typedef gcguard<Value> PrimT; \
		static T* unpack(PrimT v) { return unbox(v.ptr()); } \
		static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return Eval(Nth(args, idx), env); } \
	}

// when defaulting to boxes, accept either "raw pointers" or boxes
template <typename T> struct LispToC {
    static T& unbox(Value* v) {
        if (T* rv = expr_cast<T>(v))
            return *rv;
        else
            return assert_type< Box<T> >(v)->value();
    }

    typedef gcguard< Value > PrimT;
    static T& unpack(PrimT v) {
        if (T* rv = expr_cast<T>(v.ptr()))
            return *rv;
        else
            return assert_type< Box<T> >(v.ptr())->value();
    }

    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) {
        return Eval(Nth(args, idx), env);
    }
};

// 'void' is the unit type, actually
template <>
	struct CToLisp<void> {
		static std::string desc() {
			return "()";
		}
	};

// the default binding method for pointers is to use boxed auto_ptrs
template <typename T> struct CToLisp<T*> {
	typedef Box< ptr<T> > BoxType;
	static Value* alloc(Allocator* a, T* v) { return a->template allocate< Box< ptr<T> > >(ptr<T>(v)); }

	static void argSet(ConsPair* an, T* v) {
		((Box< ptr<T> >*)(an->head()))->value(ptr<T>(v));
	}

	static std::string desc() {
		return str::short_type_name<T>() + "*";
	}
};

template <typename T> struct LispToC<T*> {
    static T* unbox(Value* v) { return assert_type< Box< ptr<T> > >(v)->value().value(); }

    typedef gcguard< Box< ptr<T> > > PrimT;
	static T* unpack(PrimT v) { return v->value().value(); }
    
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) {
        return assert_type< Box< ptr<T> > >(Eval(Nth(args, idx), env));
    }
};

// pass GC-guarded pointers through
template <typename T> struct CToLisp< gcguard<T> > {
	static Value* alloc(Allocator* a, const gcguard<T>& v) { return v.ptr(); }

	DEFAULT_ARG_SET_DELAY(const gcguard<T>&);

	static std::string desc() {
		return str::short_type_name<T>();
	}
};

template <typename T> struct ArgContrib { static const int V = 1; };

// remove the const attribute
template <typename T> struct CToLisp<const T>    : public CToLisp<T> { };
template <typename T> struct LispToC<const T>    : public LispToC<T> { };
template <typename T> struct ArgContrib<const T> : public ArgContrib<T> { };

// remove the reference attribute (copy / pass by value)
template <typename T> struct CToLisp<T&>    : public CToLisp<T> { };
template <typename T> struct LispToC<T&>    : public LispToC<T> { };
template <typename T> struct ArgContrib<T&> : public ArgContrib<T> { };

// Primitive binding (forces evaluation of arguments)
#define NUMBER_CONVERSION(T) \
    template <> struct CToLisp< T > { \
		typedef Number BoxType; \
		static Value* alloc(Allocator* a, T v) { return a->allocate<Number>((double)v); } \
        static void argSet(ConsPair* an, T v) { \
            ((Number*)(an->head()))->update(v); \
        } \
		static std::string desc() { \
			return #T; \
		} \
	}; \
    template <> struct LispToC< T > { \
        static T unbox(Value* v) {  return (T)(assert_type<Number>(v)->value());  } \
        \
        typedef gcguard<Number> PrimT; \
        static T unpack(PrimT v) { return (T)(v->value()); } \
        static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return assert_type<Number>(Eval(Nth(args, idx), env)); } \
    }

NUMBER_CONVERSION(char);
NUMBER_CONVERSION(unsigned char);
NUMBER_CONVERSION(short);
NUMBER_CONVERSION(unsigned short);
NUMBER_CONVERSION(int);
NUMBER_CONVERSION(unsigned int);
NUMBER_CONVERSION(long);
NUMBER_CONVERSION(unsigned long);
NUMBER_CONVERSION(long long);
NUMBER_CONVERSION(unsigned long long);
NUMBER_CONVERSION(float);
NUMBER_CONVERSION(double);

template <> struct CToLisp<std::string> {
	typedef String BoxType;
	static Value* alloc(Allocator* a, const std::string& v) { return a->allocate<String>(v); }
	
	static void argSet(ConsPair* an, const std::string& v) {
		((String*)(an->head()))->update(v);
	}

	static std::string desc() {
		return "string";
	}
};
template <> struct LispToC<std::string> {
    static std::string unbox(Value* v) { return assert_type<String>(v)->value(); }

    typedef Value* PrimT;
    static std::string unpack(PrimT v) { return unbox(v); }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return Eval(Nth(args, idx), env); }
};

template <> struct CToLisp<const char*> {
	typedef String BoxType;
	static Value* alloc(Allocator* a, const char* v) { return a->allocate<String>(std::string(v)); }

	static void argSet(ConsPair* an, const char* v) {
		((String*)(an->head()))->update(std::string(v ? v : ""));
	}

	static std::string desc() {
		return "string";
	}
};
template <> struct LispToC<const char*> {
    static const char* unbox(Value* v) { return assert_type<String>(v)->value().c_str(); }

    typedef gcguard<String> PrimT;
    static const char* unpack(PrimT v) { return v->value().c_str(); }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return assert_type<String>(Eval(Nth(args, idx), env)); }
};

template <> struct CToLisp<char*> {
	typedef String BoxType;
	static Value* alloc(Allocator* a, char* v) { return a->allocate<String>(std::string(v)); }

	static void argSet(ConsPair* an, char* v) {
		((String*)(an->head()))->update(std::string(v ? v : ""));
	}

	static std::string desc() {
		return "string";
	}
};
template <> struct LispToC<char*> {
    static char* unbox(Value* v) { return const_cast<char*>(assert_type<String>(v)->value().c_str()); }

    typedef gcguard<String> PrimT;
    static char* unpack(PrimT v) { return const_cast<char*>(v->value().c_str()); }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return assert_type<String>(Eval(Nth(args, idx), env)); }
};

template <> struct CToLisp<bool> {
	typedef Value* BoxType;
	static Value* alloc(Allocator* a, bool v) { return v ? a->allocate<Number>(1.0) : 0; }

	static void argSet(ConsPair* an, bool v) {
		if (v) {
			an->head(alloc_symbol(an->allocator(), "t"));
		} else {
			an->head(0);
		}
	}

	static std::string desc() {
		return "bool";
	}
};
template <> struct LispToC<bool> {
    static bool unbox(Value* v) { return v != 0; }

    typedef bool PrimT;
    static bool unpack(PrimT v) { return v; }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return Eval(Nth(args, idx), env) != 0; }
};

template <> struct CToLisp<std::istream*> {
	typedef IStream BoxType;
	static Value* alloc(Allocator* a, std::istream* v) { return a->allocate<IStream>(v, true); }
	DEFAULT_ARG_SET(std::istream*);
	static std::string desc() {
		return "istream";
	}
};
template <> struct LispToC<std::istream*> {
    static std::istream* unbox(Value* v) {
        static std::istringstream ss;
	
        if (String* vs = expr_cast<String>(v)) {
            ss.str(vs->value());
            ss.clear();
            return &ss;
        } else if (IOStream* io = expr_cast<IOStream>(v)) {
			return io->value();
		} else {
            return assert_type<IStream>(v)->value();
        }
    }

	typedef gcguard<Value> PrimT;
	static std::istream* unpack(PrimT v) { return unbox(v.ptr()); }
	static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return Eval(Nth(args, idx), env); }
};

template <> struct CToLisp<std::ostream*> {
	typedef OStream BoxType;
	static Value* alloc(Allocator* a, std::ostream* v) { return a->allocate<OStream>(v, true); }
	DEFAULT_ARG_SET(std::ostream*);
	static std::string desc() {
		return "ostream";
	}
};
template <> struct LispToC<std::ostream*> {
    static std::ostream* unbox(Value* v) {
		if (IOStream* io = expr_cast<IOStream>(v))
			return io->value();
		else
			return assert_type<OStream>(v)->value();
	}

	typedef gcguard<Value> PrimT;
	static std::ostream* unpack(PrimT v) { return unbox(v.ptr()); }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return Eval(Nth(args, idx), env); }
};

template <> struct CToLisp<std::iostream*> {
	typedef IOStream BoxType;
	static Value* alloc(Allocator* a, std::iostream* v) { return a->allocate<IOStream>(v, true); }
	DEFAULT_ARG_SET(std::iostream*);
	static std::string desc() {
		return "iostream";
	}
};
template <> struct LispToC<std::iostream*> {
    static std::iostream* unbox(Value* v) { return assert_type<IOStream>(v)->value(); }

	typedef gcguard<IOStream> PrimT;
	static std::iostream* unpack(PrimT v) { return v->value(); }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return assert_type<IOStream>(Eval(Nth(args, idx), env)); }
};

template <typename U, typename V> struct CToLisp< std::pair<U, V> > {
	typedef ConsPair BoxType;

    static ConsPair* alloc(Allocator* a, const std::pair<U, V>& p) {
        typedef CToLisp<U> CU;
        typedef CToLisp<V> CV;

        return a->template allocate<ConsPair>(CU::alloc(a, p.first), CV::alloc(a, p.second));
    }

	typedef std::pair<U, V> XPair;
	DEFAULT_ARG_SET_DELAY(const XPair&);

	static std::string desc() {
		return "(" + CToLisp<U>::desc() + ", " + CToLisp<V>::desc() + ")";
	}
};
template <typename U, typename V> struct LispToC< std::pair<U, V> > {
    static std::pair<U, V> unbox(Value* v) {
        ConsPair* p = assert_type<ConsPair>(v);

        typedef LispToC<U> ULispToC;
        typedef LispToC<V> VLispToC;

        return std::pair<U, V>(ULispToC::unbox(p->head()), VLispToC::unbox(p->tail()));
    }

	typedef gcguard<ConsPair> PrimT;
	static std::pair<U, V> unpack(PrimT v) { return unbox(v.ptr()); }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(assert_type<ConsPair>(Eval(Nth(args, idx), env))); }
};

template <typename K, typename V> struct CToLisp< std::map<K, V> > {
	static ConsPair* alloc(Allocator* a, const std::map<K, V>& v) { return LiftContainer(v, a); }
	typedef std::map<K, V> XMap;
	DEFAULT_ARG_SET_DELAY(const XMap&);
	static std::string desc() {
		return "{" + CToLisp<K>::desc() + " => " + CToLisp<V>::desc() + "}";
	}
};

#ifndef WIN32
template <typename K, typename V> struct CToLisp< stdext::hash_map<K, V> > {
	static ConsPair* alloc(Allocator* a, const stdext::hash_map<K, V>& v) { return LiftContainer(v, a); }
	typedef stdext::hash_map<K, V> XMap;
	DEFAULT_ARG_SET_DELAY(const XMap&);
	static std::string desc() {
		return "{" + CToLisp<K>::desc() + " =#=> " + CToLisp<V>::desc() + "}";
	}
};
#endif

template <typename T> struct CToLisp< std::list<T> > {
	typedef ConsPair BoxType;
    static ConsPair* alloc(Allocator* a, const std::list<T>& v) { return LiftContainer(v, a); }
	DEFAULT_ARG_SET_DELAY(const std::list<T>&);
	static std::string desc() {
		return "[:" + CToLisp<T>::desc() + ":]";
	}
};
template <typename T> struct LispToC< std::list<T> > {
	typedef gcguard<ConsPair> PrimT;

	static std::list<T> unpack(PrimT p) {
		typedef LispToC<T> TLispToC;
		typedef typename TLispToC::PrimT TBox;

		std::list<T> result;

		while (p.ptr() != 0) {
			result.push_back(TLispToC::unbox(p->head()));
			p = assert_type<ConsPair>(p->tail());
		}

		return result;
	}

    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) {
        return assert_type<ConsPair>(Eval(Nth(args, idx), env));
    }
};

template <typename T> struct CToLisp< std::vector<T> > {
	typedef ConsPair BoxType;
    static ConsPair* alloc(Allocator* a, const std::vector<T>& v) { return LiftContainer(v, a); }
	DEFAULT_ARG_SET_DELAY(const std::vector<T>&);
	static std::string desc() {
		return "[" + CToLisp<T>::desc() + "]";
	}
};
template <typename T> struct LispToC< std::vector<T> > {
	typedef gcguard<Value> PrimT;

	static std::vector<T> unbox(Value* ep) {
		typedef LispToC<T> TLispToC;
		typedef typename TLispToC::PrimT TBox;

		std::vector<T> result;

		if (ValArray* va = expr_cast<ValArray>(ep)) {
			const ValArray::Values& vs = va->value();
			result.reserve(vs.size());
			for (ValArray::Values::const_iterator vi = vs.begin(); vi != vs.end(); ++vi) {
				result.push_back(TLispToC::unbox(*vi));
			}
			return result;
		} else {
			ConsPair* p = assert_type<ConsPair>(ep);
	
			while (p != 0) {
				result.push_back(TLispToC::unbox(p->head()));
				p = assert_type<ConsPair>(p->tail());
			}
	
			return result;
		}
	}

	static std::vector<T> unpack(PrimT p) {
		return unbox(p.ptr());
	}

    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) {
        return Eval(Nth(args, idx), env);
    }
};

template <typename T> struct CToLisp< std::set<T> > {
	typedef ConsPair BoxType;
    static ConsPair* alloc(Allocator* a, const std::set<T>& v) { return LiftContainer(v, a); }
	DEFAULT_ARG_SET_DELAY(const std::vector<T>&);
	static std::string desc() {
		return "{" + CToLisp<T>::desc() + "}";
	}
};
template <typename T> struct LispToC< std::set<T> > {
	typedef gcguard<ConsPair> PrimT;

	static std::set<T> unbox(Value* ep) {
		typedef LispToC<T> TLispToC;
		typedef typename TLispToC::PrimT TBox;

		std::set<T> result;
		ConsPair* p = assert_type<ConsPair>(ep);

		while (p != 0) {
			result.insert(TLispToC::unbox(p->head()));
			p = assert_type<ConsPair>(p->tail());
		}

		return result;
	}

	static std::set<T> unpack(PrimT p) {
		return unbox(p.ptr());
	}

    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) {
        return assert_type<ConsPair>(Eval(Nth(args, idx), env));
    }
};

// Script value bindings
//   script value types are not evaluated -- the callee must force evaluation
template <> struct CToLisp<Symbol*> {
	typedef Symbol* BoxType;
	static Value* alloc(Allocator* a, Symbol* v) { return v; }
	DEFAULT_ARG_SET(Symbol*);
	static std::string desc() { return "symbol"; }
};
template <> struct LispToC<Symbol*> {
    static Symbol* unbox(Value* v) { return assert_type<Symbol>(v); }
	typedef Symbol* PrimT;
	static Symbol* unpack(PrimT v) { return v; }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return unbox(Nth(args, idx)); }
};

template <> struct CToLisp<Value*> {
	typedef Value* BoxType;
	static Value* alloc(Allocator* a, Value* v) { return v; }
	DEFAULT_ARG_SET_DELAY(Value*);
	static std::string desc() { return "value"; }
};
template <> struct LispToC<Value*> {
    static Value* unbox(Value* v) { return v; }

	typedef Value* PrimT;
	static Value* unpack(PrimT v) { return v; }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return Nth(args, idx); }
};

template <> struct CToLisp<EnvironmentFrame*> {
	typedef EnvironmentFrame* BoxType;
	static Value* alloc(Allocator* a, EnvironmentFrame* v) { return v; }
	DEFAULT_ARG_SET(EnvironmentFrame*);
	static std::string desc() { return ""; }
};
template <> struct LispToC<EnvironmentFrame*> {
    static EnvironmentFrame* unbox(Value* v) { return assert_type<EnvironmentFrame>(v); }

	typedef EnvironmentFrame* PrimT;
	static EnvironmentFrame* unpack(PrimT v) { return v; }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return env; }
};
template <> struct ArgContrib<EnvironmentFrame*> { static const int V = 0; };

template <> struct CToLisp<ConsPair*> {
	typedef ConsPair* BoxType;
	static Value* alloc(Allocator* a, ConsPair* v) { return v; }
	DEFAULT_ARG_SET_DELAY(ConsPair*);
	static std::string desc() { return "cons"; }
};
template <> struct LispToC<ConsPair*> {
    static ConsPair* unbox(Value* v) { return assert_type<ConsPair>(v); }

	typedef ConsPair* PrimT;
	static ConsPair* unpack(PrimT v) { return v; }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return NthPair(args, idx); }
};
template <> struct ArgContrib<ConsPair*> { static const int V = -1000; };

template <> struct CToLisp<Allocator*> {
	typedef Allocator* BoxType;
	static std::string desc() { return ""; }
};
template <> struct LispToC<Allocator*> {
	typedef Allocator* PrimT;
	static Allocator* unpack(PrimT v) { return v; }
    static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return env->allocator(); }
};
template <> struct ArgContrib<Allocator*> { static const int V = 0; };

template <typename ContT>
    ConsPair* LiftContainer(const ContT& container, Allocator* alloc) {
        typedef typename ContT::const_iterator   CIter;
        typedef typename ContT::value_type       VT;
        typedef CToLisp<VT>                      CVT;

        ConsPair* r   = 0;
        ConsPair* ret = 0;

        for (CIter i = container.begin(); i != container.end(); ++i) {
            if (r == 0) {
                r = alloc->template allocate<ConsPair>((Value*)0, (Value*)0);
                ret = r;
            } else {
                ConsPair* nr = alloc->template allocate<ConsPair>((Value*)0, (Value*)0);
                r->tail(nr);
                r = nr;
            }

            r->head(CVT::alloc(alloc, *i));
        }

        return ret;
    }

template <typename K, typename V>
    ConsPair* LiftContainer(const std::map<K, V>& cont, Allocator* alloc) {
        typedef typename std::map<K, V>        ContT;
        typedef typename ContT::const_iterator CIter;
        typedef CToLisp<K>                     CK;
        typedef CToLisp<V>                     CV;

        ConsPair* r   = 0;
        ConsPair* ret = 0;

        for (CIter i = cont.begin(); i != cont.end(); ++i) {
            if (r == 0) {
                r = alloc->template allocate<ConsPair>((Value*)0, (Value*)0);
                ret = r;
            } else {
                ConsPair* nr = alloc->template allocate<ConsPair>((Value*)0, (Value*)0);
                r->tail(nr);
                r = nr;
            }

            ConsPair* ent = alloc->template allocate<ConsPair>(CK::alloc(alloc, i->first), CV::alloc(alloc, i->second));

            r->head(ent);
        }

        return ret;
    }

#ifndef WIN32
template <typename K, typename V>
    ConsPair* LiftContainer(const stdext::hash_map<K, V>& cont, Allocator* alloc) {
        typedef typename stdext::hash_map<K, V> ContT;
        typedef typename ContT::const_iterator  CIter;
        typedef CToLisp<K>                      CK;
        typedef CToLisp<V>                      CV;

        ConsPair* r   = 0;
        ConsPair* ret = 0;

        for (CIter i = cont.begin(); i != cont.end(); ++i) {
            if (r == 0) {
                r = alloc->template allocate<ConsPair>((Value*)0, (Value*)0);
                ret = r;
            } else {
                ConsPair* nr = alloc->template allocate<ConsPair>((Value*)0, (Value*)0);
                r->tail(nr);
                r = nr;
            }

            ConsPair* ent = alloc->template allocate<ConsPair>(CK::alloc(alloc, i->first), CV::alloc(alloc, i->second));

            r->head(ent);
        }

        return ret;
    }
#endif

template <typename T>
    Value* LiftValue(const T& v, Allocator* alloc) {
        typedef CToLisp<T> CT;
        return CT::alloc(alloc, v);
    }

template <typename ContT>
    ContT UnboxContainer(ConsPair* cp) {
        typedef typename ContT::value_type VT;
        typedef LispToC<VT> L2VC;

        ContT ret;
        while (cp != 0) {
            ret.push_back(L2VC::unbox(cp->head()));
            cp = expr_cast<ConsPair>(cp->tail());
        }

        return ret;
    }

template <typename T>
    T UnboxValue(Value* v) {
        typedef LispToC<T> L2VC;
        return L2VC::unbox(v);
    }

}

#include <scm/eval/CTupleUtil.hpp>

#endif
