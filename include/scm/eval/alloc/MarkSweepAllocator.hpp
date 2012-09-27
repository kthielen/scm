#ifndef EVAL_MARKSWEEP_ALLOCATOR_H_INCLUDED
#define EVAL_MARKSWEEP_ALLOCATOR_H_INCLUDED

#include <typeinfo>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <iostream>

namespace scm {

class Allocator;
class AllocBase;

class AllocVisitor
{
public:
    void visit(AllocBase*& p);
};

class AllocBase
{
public:
    AllocBase();
    virtual ~AllocBase();

    Allocator* allocator() const;
private:
    friend class Allocator;
    friend class AllocVisitor;
    Allocator* alloc;
    int        def_marked;
protected:
    bool marked;
public:
    virtual void references(AllocVisitor& v);
    void inc_default_mark();
    void dec_default_mark();
};

template <typename T>
    class gcguard
    {
    public:
	gcguard() : p(0)                      { }
	gcguard(T* v) : p(v)                  { inc(); }
	gcguard(const gcguard<T>& v) : p(v.p) { inc(); }
	~gcguard()                            { dec(); }
	T& operator*()  const                 { return *p; }
	T* operator->() const                 { return p; }
	T* ptr() const                        { return p; }
	bool operator==(T* v) const           { return p == v; }
	void operator=(T* v)                  { dec(); p = v;   inc(); }
	void operator=(const gcguard<T>& v)   { dec(); p = v.p; inc(); }
    private:
	T* p;

	void inc() { if (p) p->inc_default_mark(); }
	void dec() { if (p) p->dec_default_mark(); }
    };

template <typename T>
    struct FreeDataVisit
    {
        static void visit(AllocVisitor& v, T& val) { v.visit((AllocBase*&)val); }
        static void mark_default(T& val) { val->inc_default_mark(); }
    };

template <>
    struct FreeDataVisit<bool> { static void visit(AllocVisitor& v, bool&) { } static void mark_default(bool&) { } };
template <>
    struct FreeDataVisit<int> { static void visit(AllocVisitor& v, int&) { } static void mark_default(int&) { } };

template <typename F, typename S>
    struct FreeDataVisit< std::pair<F, S> >
   {
      static void visit(AllocVisitor& v, std::pair<F, S>& p) { FreeDataVisit<F>::visit(v, p.first); FreeDataVisit<S>::visit(v, p.second); }
      static void mark_default(std::pair<F, S>& p)           { FreeDataVisit<F>::mark_default(p.first); FreeDataVisit<S>::mark_default(p.second); }
   };

template <typename T>
    struct FreeData : public AllocBase
    {
        FreeData(void*) : data() {  }
        T data;
        void references(AllocVisitor& v) { FreeDataVisit<T>::visit(v, data); }
    };

class Allocator
{
public:
    Allocator();
    ~Allocator();

    bool auto_collect() const;
    void auto_collect(bool f);

    template <typename T>
        T* allocate() { return Record(new T); }

    template <typename T, typename A1>
        T* allocate(A1 a1) { return Record(new T(a1)); }

    template <typename T, typename A1, typename A2>
        T* allocate(A1 a1, A2 a2) { return Record(new T(a1, a2)); }

    template <typename T, typename A1, typename A2, typename A3>
        T* allocate(A1 a1, A2 a2, A3 a3) { return Record(new T(a1, a2, a3)); }

    template <typename T, typename A1, typename A2, typename A3, typename A4>
        T* allocate(A1 a1, A2 a2, A3 a3, A4 a4) { return Record(new T(a1, a2, a3, a4)); }

    template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
        T* allocate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { return Record(new T(a1, a2, a3, a4, a5)); }

    template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
        T* allocate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) { return Record(new T(a1, a2, a3, a4, a5, a6)); }

    template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
        T* allocate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) { return Record(new T(a1, a2, a3, a4, a5, a6, a7)); }

    template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
        T* allocate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8) { return Record(new T(a1, a2, a3, a4, a5, a6, a7, a8)); }
	
    template <typename T, typename A1>
	T* flyweight_allocate(A1 a1)
	{
	    typedef std::map<A1, T*> FAllocT;
	    typedef typename FAllocT::const_iterator FAllocTIter;
	    
	    FAllocT*    fa  = flyweight_allocator<A1, T*>();
	    FAllocTIter fai = fa->find(a1);
	    if (fai != fa->end()) return fai->second;

            void* rdata = malloc(sizeof(T));
	    flyweight_data.push_back(rdata);
	    
	    T* ret = new (rdata) T(a1);
	    ret->def_marked = 1;
	    ret->alloc = this;
	    (*fa)[a1] = ret;
	    
	    return ret;
	}

    void collect(AllocBase* aux1 = 0, AllocBase* aux2 = 0);
    void force_collect(AllocBase* aux1 = 0, AllocBase* aux2 = 0);
    
    AllocBase* root() const;
    void root(AllocBase* v);
private:
    // flyweight allocation
    typedef std::pair<std::string, void*>    FlyweightAllocator;
    typedef std::list<FlyweightAllocator>    FlyweightAllocators;
    typedef std::list<void*>                 FlyweightAllocData;
    FlyweightAllocators flyweight_allocators;
    FlyweightAllocData  flyweight_data;

    template <typename Key, typename V>
	std::map<Key, V>* flyweight_allocator()
	{
	    std::string sti(typeid(std::map<Key, V>).name());
	    
	    for (FlyweightAllocators::const_iterator fai = this->flyweight_allocators.begin(); fai != this->flyweight_allocators.end(); ++fai)
		if (sti == fai->first)
		    return reinterpret_cast< std::map<Key, V>* >(fai->second);

	    void* mappy = malloc(sizeof(std::map<Key, V>) * 2);
	    std::map<Key, V>* fmap = new (mappy) std::map<Key, V>;
	    this->flyweight_allocators.push_back(FlyweightAllocator(sti, mappy));

	    return fmap;
	}

    // mark/sweep implementation
    typedef std::vector< AllocBase* > AllocSeq;
    AllocSeq   allocated_data;
    AllocBase* root_value;
    int        last_collect_size;
    bool       acollect;			// if false, collection only happens on force_collect()
    
    template <typename T>
        T* Record(T* v)
        {
	    if (this->root_value == 0) this->root_value = v;

            if (v == 0) { std::cout << "ERROR: Failed to allocate new memory." << std::endl; exit(0); }	    

            v->alloc = this;
            allocated_data.push_back(v);
            return v;
        }

    void FreeAll();
    void MarkSweepImpl(AllocBase* aux1, AllocBase* aux2);
    void FreeAllUnmarked();
};

}

#endif
