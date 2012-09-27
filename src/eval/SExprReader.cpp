#include <scm/parse/StreamFSM.hpp>
#include <scm/eval/Data.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/str/Util.hpp>
#include <stack>
#include <sstream>
#include <stdexcept>

using namespace str;

namespace scm {

template <typename T>
    T from_string(const std::string& s)
    {
        std::istringstream ss(s);
        T r = T();
        ss >> r;
        return r;
    }

enum SExprReadState
{
    LookingForValue,
    ReadingString,
    ReadingAtom
};

class SExprReader;
typedef parse::StreamFSM<char, SExprReadState, SExprReader> SExprReaderMachine;

class SExprReader : public SExprReaderMachine
{
public:
    SExprReader(Allocator& a) : SExprReaderMachine(LookingForValue),
        in_escape(false), in_comment(false), in_quote(false), in_unquote(false), in_split(false),
        in_delay(false), in_force(false), in_lookup_lhs(false), in_lookup_rhs(false), expect_end_list(false), allocator(a), output_value(0)
    {
        AddStateHandler(LookingForValue, &SExprReader::LookForValue);
        AddStateHandler(ReadingString,   &SExprReader::ReadString);
        AddStateHandler(ReadingAtom,     &SExprReader::ReadAtom);

        AddTransitionHandler(ReadingString, AnyState,    &SExprReader::AddString);
        AddTransitionHandler(ReadingAtom,   AnyState,    &SExprReader::AddAtom);
        AddTransitionHandler(AnyState,      EndOfStream, &SExprReader::ValidateState);
    }

    Value* value() const {
        return output_value;
    }
private:
    bool        in_escape, in_comment, in_quote, in_unquote, in_delay, in_force, in_split, in_lookup_lhs, in_lookup_rhs, expect_end_list;
    Value*      output_value;
    Allocator&  allocator;

    struct NestedExpr
    {
        NestedExpr(bool quote, bool split, bool delay, bool force, bool lookup_rhs, ConsPair* head, ConsPair* last) :
            in_quote(quote), in_split(split), in_delay(delay), in_force(force), in_lookup_rhs(lookup_rhs),
            head_node(head), last_node(last) { }

        bool in_quote, in_split, in_delay, in_force, in_lookup_rhs;
        ConsPair *head_node, *last_node;
    };

    typedef std::stack<NestedExpr> NestedExprs;
    NestedExprs nested_exprs;

    // state handlers
    void LookForValue(const char& c) {
        if (in_comment) { in_comment = (c != '\n' && c != '\r'); return; }

        switch (c) {
        case '(':
            BeginList();
            break;
        case ')':
            EndList();
            break;
        case '.':
            if (this->nested_exprs.size() == 0 || this->nested_exprs.top().head_node == 0)
                throw std::runtime_error("Unexpected '.'");

            in_split = true;
            break;
        case '~':
            in_delay = true;
            break;
        case '!':
            in_force = true;
            break;
        case '\'':
            in_quote = true;
            break;
        case '`':
            in_unquote = true;
            break;
        case ';':
        case '#':
            in_comment = true;
            break;
        case '\"':
            SetState(ReadingString);
            break;
        default:
            if (!is_whitespace(c)) {
                PutBack(c);
                SetState(ReadingAtom);
                break;
            }
        }
    }
    
	void SetStateWithNSCheck(SExprReadState s) {
		if (Peek(1) == ':') {
			Read();
			this->in_lookup_lhs = true;
		}
		SetState(s);
	}

    void ReadString(const char& c) {
        if (!in_escape) {
            if (c == '\"')
				SetStateWithNSCheck(LookingForValue);
            else if (c == '\\')
                in_escape = true;
        } else {
            in_escape = false;
        }
    }
    
    void ReadAtom(const char& c) {
        bool is_new_tok = c == '(' || c == '\"' || c == ')' || c == ':';

        if (is_whitespace(c) || is_new_tok) {
            PutBack(c);
			SetStateWithNSCheck(LookingForValue);
        }
    }
    
    // state transition handlers
    void AddString(elem_iter begin, elem_iter end) {
		if (in_lookup_lhs) {
			end = end - 1;
		}

        AddToResult(allocator.allocate<String>(unescape(std::string(begin, end - 1))));
    }
    
    void AddAtom(elem_iter begin, elem_iter end) {
		if (in_lookup_lhs) {
			end = end - 1;
		}

        std::string val(begin, end);
        
		if (is_numeric(val))
            AddToResult(allocator.allocate<Number>(from_string<double>(val)));
		else if (is_hex_numeric(val))
			AddToResult(allocator.allocate<Number>(hex_str_to_int(val)));
        else
            AddToResult(alloc_symbol(&allocator, val));
    }

    void ValidateState(elem_iter begin, elem_iter end) {
        if (nested_exprs.size() > 0)
            throw std::runtime_error("Expected ')'.");
    }

    // parse tree construction procedures
    void BeginList() {
        bool apply_quote = !this->in_unquote && (this->in_quote || (nested_exprs.size() > 0 && nested_exprs.top().in_quote));

        nested_exprs.push(NestedExpr(apply_quote, in_split, in_delay, in_force, in_lookup_rhs, 0, 0));

        in_delay = in_force = in_quote = in_unquote = in_split = in_lookup_rhs = false;
    }
    
    void EndList() {
        this->expect_end_list = false;

        if (nested_exprs.size() == 0)
            throw std::runtime_error("Unexpected ')'.");

        NestedExpr ne = nested_exprs.top();
        nested_exprs.pop();
        this->in_split = ne.in_split;
		this->in_lookup_rhs = ne.in_lookup_rhs;

        Value* v = ne.head_node;
        if (ne.in_delay) v = Delay(v);
        if (ne.in_force) v = Force(v);

		if (Peek(1) == ':') {
			Read();
			this->in_lookup_lhs = true;
		}

        AddToResult(v);
    }

    void AddToResult(Value* v) {
        if (this->expect_end_list)
            throw std::runtime_error("Expected ')'.");

        // do we need to accumulate this value on a nested expression or not?
        if (nested_exprs.size() == 0) {
            if (in_delay) v = Delay(v);
            if (in_force) v = Force(v);
            if (in_quote) v = Quote(v);

			bool detach = !in_lookup_lhs;

			if (in_lookup_rhs) {
				this->output_value = Lookup(this->output_value, v);
			} else {
				this->output_value = v;
			}

			if (detach) {
				Detach();
			}
        } else {
            NestedExpr& ne       = nested_exprs.top();
            ConsPair*   cur_pair = 0;
			Value*      last_val = ne.last_node == 0 ? 0 : ne.last_node->head();

            if (ne.head_node != 0 && this->in_split) {
                cur_pair = ne.last_node;
			} else if (in_lookup_rhs) {
				cur_pair = ne.last_node;
            } else if (ne.head_node != 0) {
                cur_pair = allocator.allocate<ConsPair>((Value*)0, (Value*)0);
                ne.last_node->tail(cur_pair);
                ne.last_node = cur_pair;
            } else if (ne.in_quote) {
                // this is a quoted list: "quote" it as (list ...)
                cur_pair         = allocator.allocate<ConsPair>((Value*)0, (Value*)0);
                ConsPair* lcall  = allocator.allocate<ConsPair>(alloc_symbol(&allocator, "list"), cur_pair);
                ne.head_node  = lcall;
                ne.last_node = cur_pair;
            } else {
                cur_pair     = allocator.allocate<ConsPair>((Value*)0, (Value*)0);
                ne.head_node = cur_pair;
                ne.last_node = cur_pair;
            }

            // quote/delay/force this list member if necessary
            bool apply_quote = (!in_unquote && (ne.in_quote || in_quote)) && (expr_cast<ConsPair>(v) == 0);

            if (apply_quote)   v = Quote(v);
            if (in_delay)      v = Delay(v);
            if (in_force)      v = Force(v);
			if (in_lookup_rhs) v = Lookup(last_val, v);

            if (this->in_split && ne.in_quote) {
                ConsPair* daarg2 = allocator.allocate<ConsPair>(v, (Value*)0);
                ConsPair* daarg1 = allocator.allocate<ConsPair>(ne.head_node, daarg2);
                ConsPair* dacall = allocator.allocate<ConsPair>(alloc_symbol(&allocator, "dappend"), daarg1);
                ne.head_node = dacall;
            } else if (this->in_split) {
                cur_pair->tail(v);
            } else {
                cur_pair->head(v);
            }
        }

		in_lookup_rhs   = in_lookup_lhs;

        in_delay        = false;
        in_force        = false;
        in_quote        = false;
        in_unquote      = false;
        expect_end_list = in_split;
        in_split        = false;
		in_lookup_lhs   = false;
    }

    Value* Quote(Value* v) {
        Value* qarg = allocator.allocate<ConsPair>(v, (Value*)0);
        Value* qfnc = allocator.allocate<ConsPair>(alloc_symbol(&allocator, "quote"), qarg);

        return qfnc;
    }

    Value* Delay(Value* v) {
        return
            allocator.allocate<ConsPair>
            (
                alloc_symbol(&allocator, "lambda"),
                allocator.allocate<ConsPair>
                (
                    (Value*)0,
                    allocator.allocate<ConsPair>
                    (
                        v,
                        (Value*)0
                    )
                )
            );
    }

    Value* Force(Value* v) {
        return allocator.allocate<ConsPair>(v, (Value*)0);
    }

	Value* Lookup(Value* lhs, Value* rhs) {
		return
			allocator.allocate<ConsPair>
			(
				alloc_symbol(&allocator, "eval/env"),
				allocator.allocate<ConsPair>
				(
					rhs,
					allocator.allocate<ConsPair>
					(
						lhs,
						(Value*)0
					)
				)
			);
	}
};

Value* ReadSExpression(std::istream& input, Allocator& allocator)
{
    SExprReader reader(allocator);
    reader.Run(input);
    return reader.value();
}

Value* ReadSExpression(const std::string& input, Allocator& alloc)
{
    std::istringstream ss(input);
    return ReadSExpression(ss, alloc);
}

void PrintExpression(const Value* v, std::ostream& output)
{
    if (v == 0)
        output << "()";
    else
        v->format(output);
}

std::string PrintExpression(const Value* v)
{
    std::ostringstream ss;
    PrintExpression(v, ss);
    return ss.str();
}

}

