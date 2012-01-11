#include <scm/eval/Allocator.hpp>
#include <scm/eval/modules/std/DateTime.hpp>
#include <typeinfo>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <iostream>

namespace scm {

void AllocVisitor::visit(AllocBase*& p) { if (p != 0 && !p->marked) { p->marked = true; p->references(*this); } }

AllocBase::AllocBase() : def_marked(0) { }
AllocBase::~AllocBase() { }
Allocator* AllocBase::allocator() const { return this->alloc; }
void AllocBase::references(AllocVisitor& v) { }
void AllocBase::inc_default_mark() { ++this->def_marked; }
void AllocBase::dec_default_mark() { --this->def_marked; }

Allocator::Allocator() : root_value(0), last_collect_size(1000), acollect(true) { this->allocated_data.reserve(last_collect_size); }
Allocator::~Allocator() { FreeAll(); }

bool Allocator::auto_collect() const { return this->acollect; }
void Allocator::auto_collect(bool f) { this->acollect = f; }

void Allocator::collect(AllocBase* aux1, AllocBase* aux2)
{
    if (this->acollect && this->allocated_data.size() > this->last_collect_size)
    {
        force_collect(aux1, aux2);
        this->last_collect_size = this->allocated_data.size() * 3;
    }
}

void Allocator::force_collect(AllocBase* aux1, AllocBase* aux2) {
    int          tinit  = TickCount();
    unsigned int before = this->allocated_data.size();
	
    MarkSweepImpl(aux1, aux2);
}

AllocBase* Allocator::root() const { return this->root_value; }
void Allocator::root(AllocBase* v) { this->root_value = v; }

void* Allocator::alloc(unsigned int sz) {
	return malloc(sz);
}

void Allocator::FreeAll() {
    for (AllocSeq::const_iterator asi = this->allocated_data.begin(); asi != this->allocated_data.end(); ++asi) {
		((AllocBase*)*asi)->~AllocBase();
		free(*asi);
	}

    this->allocated_data.clear();

    for (FlyweightAllocators::const_iterator fai = this->flyweight_allocators.begin(); fai != this->flyweight_allocators.end(); ++fai)
	free(fai->second);

    for (FlyweightAllocData::const_iterator fdi = this->flyweight_data.begin(); fdi != this->flyweight_data.end(); ++fdi)
	free(*fdi);

    this->flyweight_allocators.clear();
    this->flyweight_data.clear();
}

void Allocator::MarkSweepImpl(AllocBase* aux1, AllocBase* aux2)
{
    // initially unmark all blocks not exempted
    //    (the exemption allows alternate memory management schemes to operate on top of this basic GC)
    for (AllocSeq::const_iterator asi = this->allocated_data.begin(); asi != this->allocated_data.end(); ++asi)
	(*asi)->marked = false;

    AllocVisitor v;
#   define VMARK(p) if ((p) != 0) { (p)->marked = true; (p)->references(v); }

    // mark the root and the aux values (if supplied)
    VMARK(this->root_value);
    VMARK(aux1);
    VMARK(aux2);

    // mark default-marked objects
    for (AllocSeq::const_iterator asi = this->allocated_data.begin(); asi != this->allocated_data.end(); ++asi)
	if ((*asi)->def_marked > 0)
	    VMARK(*asi);

    // sweep
    FreeAllUnmarked();
}

void Allocator::FreeAllUnmarked()
{
    AllocSeq compacted_data = this->allocated_data;

    this->allocated_data.clear();
    this->allocated_data.reserve(compacted_data.size());
	
    for (AllocSeq::iterator asi = compacted_data.begin(); asi != compacted_data.end(); ++asi)
    {
	if (!(*asi)->marked)
	    delete *asi;
	else
	    this->allocated_data.push_back(*asi);
    }
}

}
