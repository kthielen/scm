#ifndef EVAL_COPY_ALLOCATOR_H_INCLUDED
#define EVAL_COPY_ALLOCATOR_H_INCLUDED

#include <typeinfo>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <iostream>

namespace scm {

class Allocator;
class AllocBase;

typedef std::vector<AllocBase**>         AllocCells; // the set of cells referenced by another cell
typedef std::map<AllocBase*, AllocBase*> AllocMap;   // indicates where copied cells went

class AllocVisitor
{
public:
    AllocCells& refs();
    void visit(AllocBase*& p);
private:
    AllocCells cells;
};

class AllocBase
{
public:
    AllocBase();
    virtual ~AllocBase();

    Allocator* allocator() const;
private:
    friend class Allocator;
    Allocator*   alloc;
    unsigned int tsize;
protected:
    virtual void references(AllocVisitor& v);
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

	void inc() { } //if (p) p->inc_default_mark(); }
	void dec() { } //if (p) p->dec_default_mark(); }
    };

struct MemPage
{
    int      psize; // the size of this page (in bytes)
    int      usize; // the number of used bytes in the page
    MemPage* nextp; // the next memory page in a chain
    void*    data;  // the page's memory
};

class Allocator
{
public:
    Allocator(unsigned int dpage_size = 1000000 /* default page size of 1MB */);
    ~Allocator();

    bool auto_collect() const;
    void auto_collect(bool f);

#   define EVAL_ALLOC_BODY(args) \
	unsigned int sz = sizeof(T);        \
	void* p         = mem_alloc(sz);    \
	T* r            = new (p) T args;   \
	r->alloc        = this;             \
	r->tsize        = sz;               \
	return r
	
    template <typename T>
	T* allocate()                                  { EVAL_ALLOC_BODY(()); }
    template <typename T, typename A1>
	T* allocate(A1 a1)                             { EVAL_ALLOC_BODY((a1)); }
    template <typename T, typename A1, typename A2>
	T* allocate(A1 a1, A2 a2)                      { EVAL_ALLOC_BODY((a1, a2)); }
    template <typename T, typename A1, typename A2, typename A3>
	T* allocate(A1 a1, A2 a2, A3 a3)               { EVAL_ALLOC_BODY((a1, a2, a3)); }
    template <typename T, typename A1, typename A2, typename A3, typename A4>
	T* allocate(A1 a1, A2 a2, A3 a3, A4 a4)        { EVAL_ALLOC_BODY((a1, a2, a3, a4)); }
    template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
	T* allocate(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) { EVAL_ALLOC_BODY((a1, a2, a3, a4, a5)); }
	
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

    // gc heap implementation
    unsigned int def_page_size;
    unsigned int used_heap_size;
    MemPage*     root_active;
    MemPage*     cur_active;
    MemPage*     root_free;
    
    MemPage* alloc_page(MemPage* prev);
    void*    mem_alloc(unsigned int len);
    void     release_page(MemPage* p);
    
    AllocBase* root_value;
    int        last_collect_size;
    bool       acollect;			// if false, collection only happens on force_collect()
    
    AllocBase* copy_cell(AllocBase* ab, AllocMap& am);
    void       free_all_memory();
};

}

#endif
