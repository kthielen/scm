#include <scm/parse/StreamFSM.hpp>
#include <scm/eval/Data.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/XMLReader.hpp>
#include <scm/str/Util.hpp>
#include <stack>
#include <sstream>
#include <stdexcept>
#include <assert.h>

using namespace str;

namespace scm {

enum XMLReadState
{
    ReadingBody,
    ReadingComment,
    ReadingTagName,
    ReadingAttributes,
    ReadingAttributeName,
    ReadingAttributeValue,
    ReadingAttributeValueStr,
    ReadingTermTagName
};

class XMLReader;
typedef parse::StreamFSM<char, XMLReadState, XMLReader> XMLReaderMachine;

class XMLReader : public XMLReaderMachine
{
public:
    XMLReader(Allocator& a) : XMLReaderMachine(ReadingBody),
        term_tag(false),
        allocator(a), output_value(0)
    {
        AddStateHandler(ReadingBody,              &XMLReader::ReadBody);
        AddStateHandler(ReadingComment,           &XMLReader::ReadComment);
        AddStateHandler(ReadingTagName,           &XMLReader::ReadTagName);
        AddStateHandler(ReadingAttributes,        &XMLReader::ReadAttributes);
        AddStateHandler(ReadingAttributeName,     &XMLReader::ReadAttributeName);
        AddStateHandler(ReadingAttributeValue,    &XMLReader::ReadAttributeValue);
        AddStateHandler(ReadingAttributeValueStr, &XMLReader::ReadAttributeValueStr);
        AddStateHandler(ReadingTermTagName,       &XMLReader::ReadTermTagName);

        AddTransitionHandler(ReadingBody,              AnyState,                 &XMLReader::ProcessBodyString);
        AddTransitionHandler(ReadingTagName,           AnyState,                 &XMLReader::ProcessTagBegin);
        AddTransitionHandler(ReadingTagName,           ReadingBody,              &XMLReader::ProcessTagHeaderEnd);
        AddTransitionHandler(ReadingAttributes,        ReadingBody,              &XMLReader::ProcessTagHeaderEnd);
        AddTransitionHandler(ReadingAttributeName,     ReadingBody,              &XMLReader::ProcessTagHeaderEnd);
        AddTransitionHandler(ReadingAttributeValue,    ReadingBody,              &XMLReader::ProcessTagHeaderEnd);
        AddTransitionHandler(ReadingAttributeValueStr, ReadingBody,              &XMLReader::ProcessTagHeaderEnd);
        AddTransitionHandler(ReadingAttributeName,     ReadingAttributes,        &XMLReader::ProcessEmptyAttribute);
        AddTransitionHandler(ReadingAttributeName,     ReadingBody,              &XMLReader::ProcessEmptyAttribute);
        AddTransitionHandler(ReadingAttributeName,     ReadingAttributeValue,    &XMLReader::ProcessAttributeBegin);
        AddTransitionHandler(ReadingAttributeName,     ReadingAttributeValueStr, &XMLReader::ProcessAttributeBegin);
        AddTransitionHandler(ReadingAttributeValue,    AnyState,                 &XMLReader::ProcessAttributeEnd);
        AddTransitionHandler(ReadingAttributeValueStr, AnyState,                 &XMLReader::ProcessAttributeEnd);
        AddTransitionHandler(ReadingTermTagName,       AnyState,                 &XMLReader::ProcessTagEnd);
        AddTransitionHandler(AnyState,                 EndOfStream,              &XMLReader::ValidateState);
    }

    Value* value() const
    {
        return nested_exprs.top().head_node;
    }

    void Run(ElementStream& input)
    {
        while (!nested_exprs.empty()) nested_exprs.pop();
        nested_exprs.push(NestedExpr(">>root", 0, 0));

        XMLReaderMachine::Run(input);
    }
private:
    Value*      output_value;
    Allocator&  allocator;
    bool        term_tag;

    struct NestedExpr
    {
        NestedExpr(const std::string& tname, ConsPair* head, ConsPair* last) :
            tag_name(tname), head_node(head), last_node(last) { }

        std::string tag_name;
        ConsPair *head_node, *last_node;

        void append(Allocator* alloc, Value* v)
        {
            ConsPair* n = alloc->allocate<ConsPair>(v, (Value*)0);
            if (!head_node) { head_node = n; }
            else            { last_node->tail(n); }
            last_node = n;
        }
    };

    typedef std::stack<NestedExpr> NestedExprs;
    NestedExprs nested_exprs;

    void merge_result()
    {
        Value* r = nested_exprs.top().head_node;
        nested_exprs.pop();
        nested_exprs.top().append(&allocator, r);
    }

    void ReadBody(const char& c)
    {
        switch (c)
        {
        case '<':
            Discard();

            switch (Peek(1))
            {
            case '?':
                Read(); Discard();
                SetState(ReadingTagName);
                break;
            case '!':
                Read(); Discard();
                SetState(ReadingComment);
                break;
            case '/':
                Read(); Discard();
                SetState(ReadingTermTagName);
                break;
            default:
                SetState(ReadingTagName);
                break;
            }
            break;
        }
    }

    void ReadComment(const char& c)
    {
        if (c == '-' && PeekStr(2) == "->")
        {
            Read(); Read();
            SetState(ReadingBody);
        }
    }

    void ReadTagName(const char& c)
    {
        if (!XMLTagHead(c) && is_whitespace(c))
            { Discard(); SetState(ReadingAttributes); }
    }

    void ReadAttributes(const char& c)
    {
        if (!XMLTagHead(c) && !is_whitespace(c))
            { PutBack(c); SetState(ReadingAttributeName); }
    }

    void ReadAttributeName(const char& c)
    {
        if (!XMLTagHead(c))
        {
            if (c == '=' && Peek(1) != '"')
                { Discard(); SetState(ReadingAttributeValue); }
            else if (c == '=' && Peek(1) == '"')
                { Read(); Discard(2); SetState(ReadingAttributeValueStr); }
            else if (is_whitespace(c))
                { Discard(); SetState(ReadingAttributes); }
        }
    }

    void ReadAttributeValue(const char& c)
    {
        if (!XMLTagHead(c) && is_whitespace(c))
            SetState(ReadingAttributes);
    }

    void ReadAttributeValueStr(const char& c)
    {
        if (c == '"')
            { Discard(); SetState(ReadingAttributes); }
    }

    void ReadTermTagName(const char& c)
    {
        if (c == '>')
            { Discard(); SetState(ReadingBody); }
    }

    // transition handlers
    void ProcessBodyString(elem_iter begin, elem_iter end)
    {
        std::string bs(begin, end);

        if (!trim(bs).empty())
            nested_exprs.top().append(&allocator, allocator.allocate<String>(bs));
    }

    void ProcessTagBegin(elem_iter begin, elem_iter end)
    {
        std::string tn(begin, end);

        nested_exprs.push(NestedExpr(tn, 0, 0));
        nested_exprs.top().append(&allocator, alloc_symbol(&allocator, lcase(tn)));
        nested_exprs.push(NestedExpr(">>attr", 0, 0));
    }

    void ProcessTagHeaderEnd(elem_iter begin, elem_iter end)
    {
        merge_result();
        if (term_tag) merge_result();
        term_tag = false;
    }

    void ProcessEmptyAttribute(elem_iter begin, elem_iter end)
    {
        std::string an(begin, end);
        nested_exprs.top().append(&allocator, alloc_symbol(&allocator, lcase(an)));
    }

    void ProcessAttributeBegin(elem_iter begin, elem_iter end)
    {
        std::string an(begin, end);
        nested_exprs.push(NestedExpr(">>attrv", 0, 0));
        nested_exprs.top().append(&allocator, alloc_symbol(&allocator, lcase(an)));
    }

    void ProcessAttributeEnd(elem_iter begin, elem_iter end)
    {
        std::string av(begin, end);
        nested_exprs.top().head_node->tail(allocator.allocate<String>(av));
        merge_result();
    }

    void ProcessTagEnd(elem_iter begin, elem_iter end)
    {
        std::string tn(begin, end);
        if (ucase(nested_exprs.top().tag_name) != ucase(tn))
            throw std::runtime_error("Unexpected end for tag '" + tn + "' while looking for end to tag '" + nested_exprs.top().tag_name + "'.");

        assert(nested_exprs.size() > 1);

        merge_result();
    }

    void ValidateState(elem_iter begin, elem_iter end)
    {
        if (nested_exprs.size() > 1)
            throw std::runtime_error("Unexpected EOF while looking for end to '" + nested_exprs.top().tag_name + "' tag.");
        
        assert(nested_exprs.size() == 1);
    }

    // utilities ...
    bool XMLTagHead(char c)
    {
        if (c == '/' && Peek(1) != '>')
            throw std::runtime_error("The '/' character must terminate a tag.");
        else if (c == '?' && Peek(1) != '>')
            throw std::runtime_error("The '?' character must terminate a tag.");
        else if (c == '/' || c == '?')
            { Discard(); term_tag = true; return true; }
        else if (c == '>')
            { Discard(); SetState(ReadingBody); return true; }
        else
            return false;
    }
};

Value* ReadXML(std::istream* input, Allocator* allocator)
{
    XMLReader reader(*allocator);
    reader.Run(*input);
    return reader.value();
}

Value* ReadXML(const std::string& input, Allocator* alloc)
{
    std::istringstream ss(input);
    return ReadXML(&ss, alloc);
}

}
