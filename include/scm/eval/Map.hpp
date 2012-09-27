#ifndef EVAL_MAP_H_INCLUDED
#define EVAL_MAP_H_INCLUDED

#include <scm/eval/Allocator.hpp>
#include <scm/eval/Data.hpp>
#include <scm/eval/CTypeUtil.hpp>
#include <map>

namespace scm {

template <typename Key> class Map;

template <typename Key>
    class MapIter : public Function
    {
    public:
        typedef std::map<Key, Value*> MapT;
        typedef typename MapT::iterator MapIterT;

        MapIter(MapIterT s, MapIterT e) : head_iter(s), last_iter(e) { }

        Value* Invoke(ConsPair* args, EnvironmentFrame* env)
        {
            if (args == 0 || args->tail() != 0) throw std::runtime_error("Iterator expects one argument.");

            Symbol* s = assert_type<Symbol>(Eval(args->head(), env));

            if (s->value() == "value")
                return head();
            else if (s->value() == "next")
                { next(); return 0; }
            else if (s->value() == "eof?")
                return eof() ? alloc_symbol(this->allocator(), "t") : 0;
            else
                throw std::runtime_error("Not an understood iterator command: " + s->value());
        }

        Value* head() const
        {
            assert_not_eof();

            typedef CToLisp<Key> CK;

            Allocator* a = this->allocator();
            return a->template allocate<ConsPair>(CK::alloc(a, head_iter->first), head_iter->second);
        }

        void next()
        {
            assert_not_eof();
            ++head_iter;
        }

        bool eof() const
        {
            return head_iter == last_iter;
        }
    private:
        friend class Map<Key>;
        MapIterT head_iter;
        MapIterT last_iter;

        void assert_not_eof() const
        {
            if (eof())
                throw std::runtime_error("Can't read at EOF.");
        }
    };

template <typename Key>
    class Map : public Function
    {
    public:
        typedef std::map<Key, Value*>         MapT;
        typedef typename MapT::iterator       MapIterT;
        typedef typename MapT::const_iterator MapCIterT;

        Value* Invoke(ConsPair* args, EnvironmentFrame* env)
        {
            gcguard<ConsPair> gargs(args);

	        if (args == 0) throw std::runtime_error("Map expects at least one argument.");
            Symbol* s = assert_type<Symbol>(Eval(args->head(), env));

            if (s->value() == "upsert")
            {
				gcguard<Value> kv = Eval(Nth(args, 1), env);
				gcguard<Value> vv = Eval(Nth(args, 2), env);

                upsert(UnboxValue<Key>(kv.ptr()), vv.ptr());
            }
            else if (s->value() == "size")
            {
                return size();
            }
            else if (s->value() == "clear")
            {
                this->map_data.clear();
            }
            else if (s->value() == "remove")
            {
                Value* iv = Eval(Nth(args, 1), env);
                if (MapIter<Key>* mik = expr_cast< MapIter<Key> >(iv))
                    remove(mik);
                else
                    remove(UnboxValue<Key>(iv));
            }
            else if (s->value() == "all")
            {
                return find_all();
            }
            else if (s->value() == "=")
            {
                return find_exact(UnboxValue<Key>(Eval(Nth(args, 1), env)));
            }
            else if (s->value() == "<")
            {
                return find_lt(UnboxValue<Key>(Eval(Nth(args, 1), env)));
            }
            else if (s->value() == "<=")
            {
                return find_lte(UnboxValue<Key>(Eval(Nth(args, 1), env)));
            }
            else if (s->value() == ">")
            {
                return find_gt(UnboxValue<Key>(Eval(Nth(args, 1), env)));
            }
            else if (s->value() == ">=")
            {
                return find_gte(UnboxValue<Key>(Eval(Nth(args, 1), env)));
            }
            else if (s->value() == "><")
            {
				gcguard<Value> kv = Eval(Nth(args, 1), env);
				gcguard<Value> vv = Eval(Nth(args, 2), env);

                return find_gt_lt(UnboxValue<Key>(kv.ptr()), UnboxValue<Key>(vv.ptr()));
            }
            else if (s->value() == ">=<")
            {
				gcguard<Value> kv = Eval(Nth(args, 1), env);
				gcguard<Value> vv = Eval(Nth(args, 2), env);

                return find_gt_lte(UnboxValue<Key>(kv.ptr()), UnboxValue<Key>(vv.ptr()));
            }
            else if (s->value() == ">=<=")
            {
				gcguard<Value> kv = Eval(Nth(args, 1), env);
				gcguard<Value> vv = Eval(Nth(args, 2), env);

                return find_gte_lte(UnboxValue<Key>(kv.ptr()), UnboxValue<Key>(vv.ptr()));
            }
            else if (s->value() == "><=")
            {
				gcguard<Value> kv = Eval(Nth(args, 1), env);
				gcguard<Value> vv = Eval(Nth(args, 2), env);

                return find_gt_lte(UnboxValue<Key>(kv.ptr()), UnboxValue<Key>(vv.ptr()));
            }
            else
                throw std::runtime_error("Not an understood map command: " + s->value());

            return 0;
	    }

        void upsert(const Key& k, Value* v)
        {
            MapIterT mi = this->map_data.find(k);

            if (mi != this->map_data.end())
                mi->second = v;
            else
                this->map_data[k] = v;
        }

        void remove(const Key& k)
        {
            MapIterT mi = this->map_data.find(k);

            if (mi != this->map_data.end())
                this->map_data.erase(mi);
        }

        void remove(MapIter<Key>* mi)
        {
            this->map_data.erase(mi->head_iter, mi->last_iter);
        }
	
        Value* find_all()
        {
            return allocator()->template allocate< MapIter<Key> >(this->map_data.begin(), this->map_data.end());
        }
	
        Value* find_exact(const Key& k)
        {
            MapIterT mi = this->map_data.find(k);
            return (mi != this->map_data.end()) ? mi->second : 0;
        }

        Value* find_lt(const Key& k)  { return iter_alloc(this->map_data.begin(), this->map_data.lower_bound(k)); }
        Value* find_lte(const Key& k) { return iter_alloc(this->map_data.begin(), this->map_data.upper_bound(k)); }
        Value* find_gt(const Key& k)  { return iter_alloc(this->map_data.upper_bound(k), this->map_data.end());   }
        Value* find_gte(const Key& k) { return iter_alloc(this->map_data.lower_bound(k), this->map_data.end());   }
	
        Value* find_gt_lt(const Key& gtk, const Key& ltk)
        {
            if (gtk >= ltk)
                return nulliter_alloc();
            else
                return iter_alloc(this->map_data.upper_bound(gtk), this->map_data.lower_bound(ltk));
        }

        Value* find_gte_lt(const Key& gtek, const Key& ltk)
        {
            if (gtek >= ltk)
                return nulliter_alloc();
            else
                return iter_alloc(this->map_data.lower_bound(gtek), this->map_data.lower_bound(ltk));
        }

        Value* find_gte_lte(const Key& gtek, const Key& ltek)
        {
            if (gtek > ltek)
                return nulliter_alloc();
            else
                return iter_alloc(this->map_data.lower_bound(gtek), this->map_data.upper_bound(ltek));
        }

        Value* find_gt_lte(const Key& gtk, const Key& ltek)
        {
            if (gtk > ltek)
                return nulliter_alloc();
            else
                return iter_alloc(this->map_data.upper_bound(gtk), this->map_data.upper_bound(ltek));
        }

        Value* size() const
        {
            return allocator()->template allocate< Number >((long long)map_data.size());
        }

        void format(std::ostream& out) const
        {
            out << "{ ";
            for (MapCIterT mi = this->map_data.begin(); mi != this->map_data.end(); ++mi)
            {
                out << std::string(mi == this->map_data.begin() ? "" : ", ") << mi->first << " -> ";
                PrintExpression(mi->second, out);
            }
            out << " }";
        }

        const MapT& value() const { return this->map_data; }

        void references(AllocVisitor& v)
        {
            for (MapIterT mi = this->map_data.begin(); mi != this->map_data.end(); ++mi)
                v.visit((AllocBase*&)mi->second);
        }
    private:
        MapT map_data;

        Value* iter_alloc(MapIterT begin, MapIterT end)
        {
            return allocator()->template allocate< MapIter<Key> >(begin, end);
        }

        Value* nulliter_alloc()
        {
            return iter_alloc(this->map_data.end(), this->map_data.end());
        }
    };

}

#endif
