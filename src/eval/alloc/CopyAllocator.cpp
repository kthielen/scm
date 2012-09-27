#include <scm/eval/Allocator.hpp>
#include <stdexcept>
#include <sstream>

namespace scm {

AllocBase::AllocBase()                         { }
AllocBase::~AllocBase()                        { }
Allocator* AllocBase::allocator() const        { return alloc; }
void AllocBase::references(AllocCells& acells) { }

AllocCells& AllocVisitor::refs()        { return this->cells; }
void AllocVisitor::visit(AllocBase*& p) { this->cells.push_back(&p); }

Allocator::Allocator(unsigned int dps) : root_value(0), last_collect_size(50000000), acollect(true), def_page_size(dps)
{
    this->used_heap_size = 0;
    this->root_active    = alloc_page(0);
    this->cur_active     = this->root_active;
    this->root_free      = alloc_page(0);
}

Allocator::~Allocator()              { free_all_memory();     }
bool Allocator::auto_collect() const { return this->acollect; }
void Allocator::auto_collect(bool f) { this->acollect = f;    }

void Allocator::collect(AllocBase* aux1, AllocBase* aux2)
{
    if (this->acollect && this->used_heap_size > this->last_collect_size)
    {
	this->last_collect_size = this->used_heap_size;
	force_collect(aux1, aux2);
    }
}

void Allocator::force_collect(AllocBase* aux1, AllocBase* aux2)
{
    // swap free for active
    this->cur_active     = this->root_free;
    this->root_free      = this->root_active;
    this->root_active    = this->cur_active;
    this->used_heap_size = 0;
    
    MemPage* p = this->root_active;
    while (p != 0) { p->usize = 0; p = p->nextp; }

    // copy reachable cells
    AllocMap amap;

    if (this->root_value != 0) copy_cell(this->root_value, amap);
    if (aux1 != 0)             copy_cell(aux1, amap);
    if (aux2 != 0)             copy_cell(aux2, amap);
}

AllocBase* Allocator::root() const { return this->root_value; }
void Allocator::root(AllocBase* v) { this->root_value = v; }

MemPage* Allocator::alloc_page(MemPage* prev)
{
    MemPage* pd  = (MemPage*)malloc(sizeof(MemPage) + this->def_page_size);
    MemPage* ppd = 0;
    if (prev != 0) { ppd = prev->nextp;	prev->nextp = pd; }
    
    pd->psize = this->def_page_size;
    pd->usize = 0;
    pd->nextp = ppd;
    pd->data  = (void*)((char*)(pd) + sizeof(MemPage));

    return pd;
}

void* Allocator::mem_alloc(unsigned int len)
{
    if (len >= this->def_page_size)
    {
	std::ostringstream ss;
	ss << "Can't allocate " << len << "B block in " << this->def_page_size << "B region.";
	throw std::runtime_error(ss.str());
    }
    else if (len > (this->def_page_size - this->cur_active->usize))
    {
	this->cur_active = alloc_page(this->cur_active);
    }

    void* r = (char*)(this->cur_active->data) + this->cur_active->usize;
    this->cur_active->usize += len;
    this->used_heap_size    += len;
    return r;
}

void Allocator::release_page(MemPage* p)
{
    while (p != 0)
    {
	unsigned int idx = 0;
	
	while (idx < p->usize)
	{
	    AllocBase* ab = (AllocBase*)((char*)(p->data) + idx);
	    idx += ab->tsize;
	    ab->~AllocBase();
	}
    
	p = p->nextp;
    }
}

AllocBase* Allocator::copy_cell(AllocBase* c, AllocMap& am)
{
    if (c == 0) return 0;
    
    AllocMap::const_iterator ami = am.find(c);
    if (ami != am.end()) return ami->second;
    
    AllocVisitor vkids;
    c->references(vkids);

    for (AllocCells::iterator ki = vkids.refs().begin(); ki != vkids.refs().end(); ++ki)
	**ki = copy_cell(**ki, am);
    
    void* tp = mem_alloc(c->tsize);
    memcpy(tp, c, c->tsize);
    am[c] = (AllocBase*)tp;
    return (AllocBase*)tp;
}

void Allocator::free_all_memory()
{
    for (FlyweightAllocators::const_iterator fai = this->flyweight_allocators.begin(); fai != this->flyweight_allocators.end(); ++fai)
	free(fai->second);

    for (FlyweightAllocData::const_iterator fdi = this->flyweight_data.begin(); fdi != this->flyweight_data.end(); ++fdi)
	free(*fdi);

    this->flyweight_allocators.clear();
    this->flyweight_data.clear();

    release_page(this->root_active);
    release_page(this->root_free);
}

}
