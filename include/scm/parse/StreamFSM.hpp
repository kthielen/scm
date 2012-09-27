#ifndef PARSE_STREAM_FSM_H_INCLUDED
#define PARSE_STREAM_FSM_H_INCLUDED

/*
    StreamFSM
	Purpose:
	    Allows the definition of basic finite state machines over std::basic_istream sequences.
*/


#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <string>

namespace parse
{

template <typename Element, typename State, typename Machine>
class StreamFSM
{
public:
    typedef typename std::basic_istream<Element> ElementStream;
    
    StreamFSM(const State& initial_state) : state(initial_state), stream(NULL), attached(true), predictor_index(0)
    {
    }

    virtual ~StreamFSM()
    {
    }

    void Run(ElementStream& input)
    {
        Initialize(input);

        ProcessImpliedTransitions(StartOfStream, this->state);

        while (Attached())
            ProcessElement();

        ProcessImpliedTransitions(this->state, EndOfStream);
    }
protected:
    typedef typename std::vector<Element>::const_iterator elem_iter;
    typedef void (Machine::*PSTATEHANDLER)(const Element&);
    typedef void (Machine::*PTRANSITIONHANDLER)(elem_iter, elem_iter);

    enum MetaStateSymbol { StartOfStream, AnyState, EndOfStream };
    class MetaState
    {
    public:
        MetaState(const State& st) : is_hos(false), state(st) { }
        MetaState(const MetaStateSymbol& mss) : is_hos(true), meta_state(mss) { }

        bool operator==(const MetaState& rhs) const
        {
            return (this->is_hos && rhs.is_hos) && ((this->is_hos && this->meta_state == rhs.meta_state) || (!this->is_hos && this->state == rhs.state));
        }

        bool operator<(const MetaState& rhs) const
        {
            if (this->is_hos != rhs.is_hos)
                return !this->is_hos;
            else if (this->is_hos)
                return this->meta_state < rhs.meta_state;
            else
                return this->state < rhs.state;		
        }
    private:
        bool is_hos;

        union
        {
            State state;
            MetaStateSymbol meta_state;
        };
    };

    /*
        AddStateHandler - adds a mapping between a state and an element handler function
    */
    void AddStateHandler(const State& st, PSTATEHANDLER fn)
    {
        this->state_handlers[st].push_back(fn);
    }

    /*
        AddTransitionHandler - adds a mapping between a state transition and an element sequence handler function
    */
    void AddTransitionHandler(const MetaState& ms1, const MetaState& ms2, PTRANSITIONHANDLER fn)
    {
        this->state_transition_handlers[StateTransition(ms1, ms2)].push_back(fn);
    }

    /*
        PutBack - pretend you didn't just read that element
    */
    void PutBack(const Element& c)
    {
        if (this->accumulator.size() > 0)
            this->accumulator.resize(this->accumulator.size() - 1);

        this->predictor.push_back(c);
    }

    /*
        Peek - peek N characters ahead
    */
    Element Peek(int n, bool* failed = 0)
    {
        if ((this->predictor_index + n - 1) < this->predictor.size())
            return this->predictor[this->predictor_index + n - 1];

        Element c;

        for (int i = 0; i < n; ++i)
        {
            this->stream->get(c);
            if (!(*(this->stream))) { if (failed) *failed = true; return Element(0); }
            this->predictor.push_back(c);
        }

        if (failed) *failed = false;
        return c;
    }

    /*
        PeekStr - peek and accumulate N characters
    */
    std::basic_string<Element> PeekStr(int n, bool* failed = 0)
    {
        typedef typename std::basic_string<Element> Str;
        Str ret;

        for (int i = 0; i < n; ++i)
        {
            if ((this->predictor_index + i) < this->predictor.size())
            {
                ret.append(1, this->predictor[this->predictor_index + i]);
            }
            else
            {
                Element c;

                this->stream->get(c);
                if (!(*(this->stream))) { if (failed) *failed = true; return ret; }
                this->predictor.push_back(c);

                ret.append(1, c);
            }
        }

        if (failed) *failed = false;
        return ret;
    }

    /*
        Read - get (and consume) the next character
    */
    Element Read(bool* failed = 0)
    {
        Element c;

        if (this->predictor_index < this->predictor.size())
        {
            c = this->predictor[this->predictor_index++];
        }
        else
        {
            this->stream->get(c);
            if (!(*(this->stream))) { if (failed) *failed = true; return Element(0); }
            if (failed) *failed = false;
        }

        this->accumulator.push_back(c);
        return c;
    }

    /*
        SetState - switches the active state of this FSM
    */
    void SetState(const State& st)
    {
        State old_state = this->state;
        this->state = st;
        ProcessImpliedTransitions(old_state, st);
    }

    /*
        Detach - finishes reading the input
    */
    void Detach()
    {
        this->attached = false;
    }

    /*
        Attached - indicates whether or not the input has been completely read
    */
    inline bool Attached() const
    {
        return attached && stream != NULL && !stream->eof() && !stream->bad();
    }

    /*
        Accumulator - Returns the start of the accumulated elements
    */
    elem_iter Accumulator() const { return this->accumulator.begin(); }
    unsigned int AccumulatorLen() const { return this->accumulator.size(); }

    /*
        Discard - Remove the last N elements from the accumulator
    */
    void Discard(unsigned int n = 1)
    {
        if (n >= this->accumulator.size())
            this->accumulator.clear();
        else if (n > 0)
            this->accumulator.resize(this->accumulator.size() - n);
    }
private:
    // state | transition handlers
    typedef std::list<PSTATEHANDLER>                StateHandlers;
    typedef typename std::map<State, StateHandlers> StateHandlersMap;

    typedef std::list<PTRANSITIONHANDLER>                 TransitionHandlers;
    typedef std::pair<MetaState, MetaState>               StateTransition;
    typedef std::map<StateTransition, TransitionHandlers> StateTransitionHandlersMap;

    StateHandlersMap           state_handlers;
    StateTransitionHandlersMap state_transition_handlers;
    State                      state;

    // accumulator | stream handler
    typedef typename std::vector<Element> ElementSequence;
    ElementSequence accumulator;
    ElementSequence predictor;
    int             predictor_index;
    ElementStream*  stream;

    bool attached;

    inline void Initialize(ElementStream& input)
    {
        this->attached = true;
        this->stream   = &input;
    }

    inline void ProcessElement()
    {
        // take an input character (and accumulate it if necessary)
        Element c;

        if (this->predictor_index >= this->predictor.size())
        {
            this->stream->get(c);
            if (!(*(this->stream))) return; // prevent processing the EOF char
        }
        else
        {
            c = this->predictor[this->predictor_index++];

            if (this->predictor_index >= this->predictor.size())
            {
                this->predictor.clear();
                this->predictor_index = 0;
            }
        }

        this->accumulator.push_back(c);
	
        // now process this character
        StateHandlers& handlers = this->state_handlers[this->state];

        for (typename StateHandlers::const_iterator shi = handlers.begin(); shi != handlers.end(); ++shi)
            (dynamic_cast<Machine*>(this)->*(*shi))(c);
    }

    inline void ProcessTransition(const MetaState& ms1, const MetaState& ms2)
    {
        TransitionHandlers& handlers = this->state_transition_handlers[StateTransition(ms1, ms2)];

        for (typename TransitionHandlers::const_iterator thi = handlers.begin(); thi != handlers.end(); ++thi)
            (dynamic_cast<Machine*>(this)->*(*thi))(this->accumulator.begin(), this->accumulator.end());
    }

    inline void ProcessImpliedTransitions(const MetaState& s1, const MetaState& s2)
    {
        ProcessTransition(AnyState, AnyState);
        ProcessTransition(s1,       AnyState);
        ProcessTransition(AnyState, s2);
        ProcessTransition(s1,       s2);

        this->accumulator.clear();
    }
};

}

#endif
