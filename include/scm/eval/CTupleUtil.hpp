

#ifndef SCM_CTUPLEUTIL_H_INCLUDED
#define SCM_CTUPLEUTIL_H_INCLUDED

#include <scm/eval/CTypeUtil.hpp>

namespace scm
{

    template < typename A0 >
	    struct tuple1
		{
			tuple1() { }
			tuple1(A0 a0) : v0(a0) { }
         
		    A0 v0;
		};

	template < typename A0 >
	    struct CToLisp< tuple1< A0 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple1< A0 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + ")";
			}
		};
	
	template < typename A0 >
	    struct LispToC< tuple1< A0 > >
		{
		    static tuple1< A0 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple1< A0 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple1< A0 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
    template < typename A0, typename A1 >
	    struct tuple2
		{
			tuple2() { }
			tuple2(A0 a0,A1 a1) : v0(a0),v1(a1) { }
         
		    A0 v0;
		    A1 v1;
		};

	template < typename A0, typename A1 >
	    struct CToLisp< tuple2< A0, A1 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple2< A0, A1 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A1 >::alloc(a, v.v1), r);
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + CToLisp< A1 >::desc() + ")";
			}
		};
	
	template < typename A0, typename A1 >
	    struct LispToC< tuple2< A0, A1 > >
		{
		    static tuple2< A0, A1 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple2< A0, A1 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
			    result.v1 = LispToC<A1>::unbox(Nth(p, 1));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple2< A0, A1 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
    template < typename A0, typename A1, typename A2 >
	    struct tuple3
		{
			tuple3() { }
			tuple3(A0 a0,A1 a1,A2 a2) : v0(a0),v1(a1),v2(a2) { }
         
		    A0 v0;
		    A1 v1;
		    A2 v2;
		};

	template < typename A0, typename A1, typename A2 >
	    struct CToLisp< tuple3< A0, A1, A2 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple3< A0, A1, A2 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A2 >::alloc(a, v.v2), r);
				r = a->allocate<ConsPair>(CToLisp< A1 >::alloc(a, v.v1), r);
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + CToLisp< A1 >::desc() + CToLisp< A2 >::desc() + ")";
			}
		};
	
	template < typename A0, typename A1, typename A2 >
	    struct LispToC< tuple3< A0, A1, A2 > >
		{
		    static tuple3< A0, A1, A2 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple3< A0, A1, A2 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
			    result.v1 = LispToC<A1>::unbox(Nth(p, 1));
			    result.v2 = LispToC<A2>::unbox(Nth(p, 2));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple3< A0, A1, A2 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
    template < typename A0, typename A1, typename A2, typename A3 >
	    struct tuple4
		{
			tuple4() { }
			tuple4(A0 a0,A1 a1,A2 a2,A3 a3) : v0(a0),v1(a1),v2(a2),v3(a3) { }
         
		    A0 v0;
		    A1 v1;
		    A2 v2;
		    A3 v3;
		};

	template < typename A0, typename A1, typename A2, typename A3 >
	    struct CToLisp< tuple4< A0, A1, A2, A3 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple4< A0, A1, A2, A3 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A3 >::alloc(a, v.v3), r);
				r = a->allocate<ConsPair>(CToLisp< A2 >::alloc(a, v.v2), r);
				r = a->allocate<ConsPair>(CToLisp< A1 >::alloc(a, v.v1), r);
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + CToLisp< A1 >::desc() + CToLisp< A2 >::desc() + CToLisp< A3 >::desc() + ")";
			}
		};
	
	template < typename A0, typename A1, typename A2, typename A3 >
	    struct LispToC< tuple4< A0, A1, A2, A3 > >
		{
		    static tuple4< A0, A1, A2, A3 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple4< A0, A1, A2, A3 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
			    result.v1 = LispToC<A1>::unbox(Nth(p, 1));
			    result.v2 = LispToC<A2>::unbox(Nth(p, 2));
			    result.v3 = LispToC<A3>::unbox(Nth(p, 3));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple4< A0, A1, A2, A3 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
    template < typename A0, typename A1, typename A2, typename A3, typename A4 >
	    struct tuple5
		{
			tuple5() { }
			tuple5(A0 a0,A1 a1,A2 a2,A3 a3,A4 a4) : v0(a0),v1(a1),v2(a2),v3(a3),v4(a4) { }
         
		    A0 v0;
		    A1 v1;
		    A2 v2;
		    A3 v3;
		    A4 v4;
		};

	template < typename A0, typename A1, typename A2, typename A3, typename A4 >
	    struct CToLisp< tuple5< A0, A1, A2, A3, A4 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple5< A0, A1, A2, A3, A4 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A4 >::alloc(a, v.v4), r);
				r = a->allocate<ConsPair>(CToLisp< A3 >::alloc(a, v.v3), r);
				r = a->allocate<ConsPair>(CToLisp< A2 >::alloc(a, v.v2), r);
				r = a->allocate<ConsPair>(CToLisp< A1 >::alloc(a, v.v1), r);
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + CToLisp< A1 >::desc() + CToLisp< A2 >::desc() + CToLisp< A3 >::desc() + CToLisp< A4 >::desc() + ")";
			}
		};
	
	template < typename A0, typename A1, typename A2, typename A3, typename A4 >
	    struct LispToC< tuple5< A0, A1, A2, A3, A4 > >
		{
		    static tuple5< A0, A1, A2, A3, A4 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple5< A0, A1, A2, A3, A4 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
			    result.v1 = LispToC<A1>::unbox(Nth(p, 1));
			    result.v2 = LispToC<A2>::unbox(Nth(p, 2));
			    result.v3 = LispToC<A3>::unbox(Nth(p, 3));
			    result.v4 = LispToC<A4>::unbox(Nth(p, 4));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple5< A0, A1, A2, A3, A4 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
    template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5 >
	    struct tuple6
		{
			tuple6() { }
			tuple6(A0 a0,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5) : v0(a0),v1(a1),v2(a2),v3(a3),v4(a4),v5(a5) { }
         
		    A0 v0;
		    A1 v1;
		    A2 v2;
		    A3 v3;
		    A4 v4;
		    A5 v5;
		};

	template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5 >
	    struct CToLisp< tuple6< A0, A1, A2, A3, A4, A5 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple6< A0, A1, A2, A3, A4, A5 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A5 >::alloc(a, v.v5), r);
				r = a->allocate<ConsPair>(CToLisp< A4 >::alloc(a, v.v4), r);
				r = a->allocate<ConsPair>(CToLisp< A3 >::alloc(a, v.v3), r);
				r = a->allocate<ConsPair>(CToLisp< A2 >::alloc(a, v.v2), r);
				r = a->allocate<ConsPair>(CToLisp< A1 >::alloc(a, v.v1), r);
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + CToLisp< A1 >::desc() + CToLisp< A2 >::desc() + CToLisp< A3 >::desc() + CToLisp< A4 >::desc() + CToLisp< A5 >::desc() + ")";
			}
		};
	
	template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5 >
	    struct LispToC< tuple6< A0, A1, A2, A3, A4, A5 > >
		{
		    static tuple6< A0, A1, A2, A3, A4, A5 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple6< A0, A1, A2, A3, A4, A5 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
			    result.v1 = LispToC<A1>::unbox(Nth(p, 1));
			    result.v2 = LispToC<A2>::unbox(Nth(p, 2));
			    result.v3 = LispToC<A3>::unbox(Nth(p, 3));
			    result.v4 = LispToC<A4>::unbox(Nth(p, 4));
			    result.v5 = LispToC<A5>::unbox(Nth(p, 5));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple6< A0, A1, A2, A3, A4, A5 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
    template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6 >
	    struct tuple7
		{
			tuple7() { }
			tuple7(A0 a0,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6) : v0(a0),v1(a1),v2(a2),v3(a3),v4(a4),v5(a5),v6(a6) { }
         
		    A0 v0;
		    A1 v1;
		    A2 v2;
		    A3 v3;
		    A4 v4;
		    A5 v5;
		    A6 v6;
		};

	template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6 >
	    struct CToLisp< tuple7< A0, A1, A2, A3, A4, A5, A6 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple7< A0, A1, A2, A3, A4, A5, A6 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A6 >::alloc(a, v.v6), r);
				r = a->allocate<ConsPair>(CToLisp< A5 >::alloc(a, v.v5), r);
				r = a->allocate<ConsPair>(CToLisp< A4 >::alloc(a, v.v4), r);
				r = a->allocate<ConsPair>(CToLisp< A3 >::alloc(a, v.v3), r);
				r = a->allocate<ConsPair>(CToLisp< A2 >::alloc(a, v.v2), r);
				r = a->allocate<ConsPair>(CToLisp< A1 >::alloc(a, v.v1), r);
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + CToLisp< A1 >::desc() + CToLisp< A2 >::desc() + CToLisp< A3 >::desc() + CToLisp< A4 >::desc() + CToLisp< A5 >::desc() + CToLisp< A6 >::desc() + ")";
			}
		};
	
	template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6 >
	    struct LispToC< tuple7< A0, A1, A2, A3, A4, A5, A6 > >
		{
		    static tuple7< A0, A1, A2, A3, A4, A5, A6 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple7< A0, A1, A2, A3, A4, A5, A6 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
			    result.v1 = LispToC<A1>::unbox(Nth(p, 1));
			    result.v2 = LispToC<A2>::unbox(Nth(p, 2));
			    result.v3 = LispToC<A3>::unbox(Nth(p, 3));
			    result.v4 = LispToC<A4>::unbox(Nth(p, 4));
			    result.v5 = LispToC<A5>::unbox(Nth(p, 5));
			    result.v6 = LispToC<A6>::unbox(Nth(p, 6));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple7< A0, A1, A2, A3, A4, A5, A6 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
    template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7 >
	    struct tuple8
		{
			tuple8() { }
			tuple8(A0 a0,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7) : v0(a0),v1(a1),v2(a2),v3(a3),v4(a4),v5(a5),v6(a6),v7(a7) { }
         
		    A0 v0;
		    A1 v1;
		    A2 v2;
		    A3 v3;
		    A4 v4;
		    A5 v5;
		    A6 v6;
		    A7 v7;
		};

	template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7 >
	    struct CToLisp< tuple8< A0, A1, A2, A3, A4, A5, A6, A7 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple8< A0, A1, A2, A3, A4, A5, A6, A7 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A7 >::alloc(a, v.v7), r);
				r = a->allocate<ConsPair>(CToLisp< A6 >::alloc(a, v.v6), r);
				r = a->allocate<ConsPair>(CToLisp< A5 >::alloc(a, v.v5), r);
				r = a->allocate<ConsPair>(CToLisp< A4 >::alloc(a, v.v4), r);
				r = a->allocate<ConsPair>(CToLisp< A3 >::alloc(a, v.v3), r);
				r = a->allocate<ConsPair>(CToLisp< A2 >::alloc(a, v.v2), r);
				r = a->allocate<ConsPair>(CToLisp< A1 >::alloc(a, v.v1), r);
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + CToLisp< A1 >::desc() + CToLisp< A2 >::desc() + CToLisp< A3 >::desc() + CToLisp< A4 >::desc() + CToLisp< A5 >::desc() + CToLisp< A6 >::desc() + CToLisp< A7 >::desc() + ")";
			}
		};
	
	template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7 >
	    struct LispToC< tuple8< A0, A1, A2, A3, A4, A5, A6, A7 > >
		{
		    static tuple8< A0, A1, A2, A3, A4, A5, A6, A7 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple8< A0, A1, A2, A3, A4, A5, A6, A7 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
			    result.v1 = LispToC<A1>::unbox(Nth(p, 1));
			    result.v2 = LispToC<A2>::unbox(Nth(p, 2));
			    result.v3 = LispToC<A3>::unbox(Nth(p, 3));
			    result.v4 = LispToC<A4>::unbox(Nth(p, 4));
			    result.v5 = LispToC<A5>::unbox(Nth(p, 5));
			    result.v6 = LispToC<A6>::unbox(Nth(p, 6));
			    result.v7 = LispToC<A7>::unbox(Nth(p, 7));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple8< A0, A1, A2, A3, A4, A5, A6, A7 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
    template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8 >
	    struct tuple9
		{
			tuple9() { }
			tuple9(A0 a0,A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8) : v0(a0),v1(a1),v2(a2),v3(a3),v4(a4),v5(a5),v6(a6),v7(a7),v8(a8) { }
         
		    A0 v0;
		    A1 v1;
		    A2 v2;
		    A3 v3;
		    A4 v4;
		    A5 v5;
		    A6 v6;
		    A7 v7;
		    A8 v8;
		};

	template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8 >
	    struct CToLisp< tuple9< A0, A1, A2, A3, A4, A5, A6, A7, A8 > >
		{
			static ConsPair* alloc(Allocator* a, const tuple9< A0, A1, A2, A3, A4, A5, A6, A7, A8 >& v)
			{
				ConsPair* r = 0;
				
				r = a->allocate<ConsPair>(CToLisp< A8 >::alloc(a, v.v8), r);
				r = a->allocate<ConsPair>(CToLisp< A7 >::alloc(a, v.v7), r);
				r = a->allocate<ConsPair>(CToLisp< A6 >::alloc(a, v.v6), r);
				r = a->allocate<ConsPair>(CToLisp< A5 >::alloc(a, v.v5), r);
				r = a->allocate<ConsPair>(CToLisp< A4 >::alloc(a, v.v4), r);
				r = a->allocate<ConsPair>(CToLisp< A3 >::alloc(a, v.v3), r);
				r = a->allocate<ConsPair>(CToLisp< A2 >::alloc(a, v.v2), r);
				r = a->allocate<ConsPair>(CToLisp< A1 >::alloc(a, v.v1), r);
				r = a->allocate<ConsPair>(CToLisp< A0 >::alloc(a, v.v0), r);

				return r;
			}

			static std::string desc() {
				return "(" + CToLisp< A0 >::desc() + CToLisp< A1 >::desc() + CToLisp< A2 >::desc() + CToLisp< A3 >::desc() + CToLisp< A4 >::desc() + CToLisp< A5 >::desc() + CToLisp< A6 >::desc() + CToLisp< A7 >::desc() + CToLisp< A8 >::desc() + ")";
			}
		};
	
	template < typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8 >
	    struct LispToC< tuple9< A0, A1, A2, A3, A4, A5, A6, A7, A8 > >
		{
		    static tuple9< A0, A1, A2, A3, A4, A5, A6, A7, A8 > unbox(Value* v)
			{
			    ConsPair* p = assert_type<ConsPair>(v);
				tuple9< A0, A1, A2, A3, A4, A5, A6, A7, A8 > result;
				
			    result.v0 = LispToC<A0>::unbox(Nth(p, 0));
			    result.v1 = LispToC<A1>::unbox(Nth(p, 1));
			    result.v2 = LispToC<A2>::unbox(Nth(p, 2));
			    result.v3 = LispToC<A3>::unbox(Nth(p, 3));
			    result.v4 = LispToC<A4>::unbox(Nth(p, 4));
			    result.v5 = LispToC<A5>::unbox(Nth(p, 5));
			    result.v6 = LispToC<A6>::unbox(Nth(p, 6));
			    result.v7 = LispToC<A7>::unbox(Nth(p, 7));
			    result.v8 = LispToC<A8>::unbox(Nth(p, 8));
				
				return result;
			}

			typedef gcguard<Value> PrimT;
			static tuple9< A0, A1, A2, A3, A4, A5, A6, A7, A8 > unpack(PrimT v) { return unbox(v.ptr()); }
			static PrimT evaluate(ConsPair* args, int idx, EnvironmentFrame* env) { return PrimT(Eval(Nth(args, idx), env)); }
		};
  
}

#endif

