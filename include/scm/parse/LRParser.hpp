#ifndef PARSE_LRPARSER_H_INCLUDED
#define PARSE_LRPARSER_H_INCLUDED

#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <string>

namespace parse
{

/*
    LRParse
        Purpose:
            Implements the LR table parsing algorithm for std::istreams.
    Type parameters:
        StateCount       - The number of states (the y axis of the action table)
        NonTerminalCount - The number of viable non-terminals (the x axis of the goto table)
        RuleCount        - The number of rules (the sum of the number of rules for each non-terminal)
        ActionCell       - The type used to represent cells in the action table (see notes)
        MaxStackSize     - The maximum size of the state stack
        Ctx              - Some data structure used to represent (or collect) the parsing context in calls to ReduceFn
        ReduceFn         - The type of the function called when a reduce action is taken
        ErrorFn          - The type of the function called when an error is encountered in the input
    Value parameters:
        input            - The input stream to read
        action_table     - The LR action table a[s][c] with s the current state, and c the current input
        rule_2_nt        - Maps rule indeces to non-terminal indeces (each viable non-terminal has 1 or more rules)
        goto_table       - The LR goto table g[r2nt[r]] telling the parser which state to go to after a reduce
        bt_table         - The LR backtrack table mapping a rule index to the number of states it covers

        char_buff        - The character buffer to fill with input (if fill_chars)
        cbidx            - The current free index into char_buff
        fill_chars       - If true, characters should be written to char_buff and cbidx incremented

        ctx              - A representation of the parse context
        rfn              - The function to call on a reduce
        efn              - The function to call on an error
	Notes:
        The action cell has the following structure:
        Bit 0 | Bit 1 | Bit 2 | Bit 3-n
        ================================
          AE     SR     AEOSR     RS

        Meaning:
            Code:	Explanation:
            AE          The 'accept' or 'error' bit (1 means 'accept', 0 means 'error')
            SR          The 'shift' or 'reduce' bit (1 means 'shift', 0 means 'reduce')
            AEOSR       Indicates whether to use AE or SR (1 means AE, 0 means SR)
            RS          The reduce rule (if SR = 0) or the shift state (if SR = 1)
*/
template <unsigned int StateCount, unsigned int NonTerminalCount, unsigned int RuleCount, typename ActionCell, unsigned int MaxStackSize, typename Ctx, typename ReduceFn, typename ErrorFn>
    inline bool LRStep
    (
        unsigned int  abs_char,
        unsigned int  line_num,
        unsigned int  rel_char,
	    
        short               c,
        const ActionCell    action_table[StateCount][257],
        unsigned int        state_stack[MaxStackSize],
        unsigned int&       ss_top,
        const unsigned int  rule_2_nt[RuleCount],
        const ActionCell    goto_table[StateCount][NonTerminalCount],
        const unsigned int  bt_table[RuleCount],

        Ctx           ctx,
        ReduceFn      rfn,
        ErrorFn       efn
	)
    {
        // reduce until we reach a shift state
        while (true)
        {
            ActionCell ac = action_table[state_stack[ss_top]][c];

            bool AEOSR = 0 != ((ac >> (8 * sizeof(ActionCell) - 3)) & 1);
            bool SR    = 0 != ((ac >> (8 * sizeof(ActionCell) - 2)) & 1);

            // should we look at AE or SR?
            if (AEOSR)
            {
                bool AE = 0 != ((ac >> (8 * sizeof(ActionCell) - 1)) & 1);

                if (!AE)
                    efn(abs_char, line_num, rel_char, state_stack[ss_top], c);
                else
                    rfn(0, ctx);

                return true; // stop processing input
            }
            else
            {
                ActionCell RS = ac & ~(((ActionCell)7) << (8 * sizeof(ActionCell) - 3));

                if (SR) // shift
                {
                    state_stack[++ss_top] = RS;
                    break;
                }
                else    // reduce
                {
                    rfn(RS, ctx);

                    ss_top -= bt_table[RS];
                    unsigned int new_state = goto_table[state_stack[ss_top]][rule_2_nt[RS]];
                    state_stack[++ss_top]  = new_state;
                }
            }
        }

        return false;
    }

template <unsigned int StateCount, unsigned int NonTerminalCount, unsigned int RuleCount, typename ActionCell, unsigned int MaxStackSize, typename Ctx, typename ReduceFn, typename ErrorFn>
    void LRParse
    (
        std::istream&       input,
        const ActionCell    action_table[StateCount][257],
        const unsigned int  rule_2_nt[RuleCount],
        const ActionCell    goto_table[StateCount][NonTerminalCount],
        const unsigned int  bt_table[RuleCount],

        Ctx                 ctx,
        ReduceFn            rfn,
        ErrorFn             efn
    )
    {
        unsigned int abs_char = 0;
        unsigned int line_num = 0;
        unsigned int rel_char = 0;

        unsigned int state_stack[MaxStackSize];
        unsigned int ss_top = 0;
        state_stack[0] = 0;

        while (true)
        {
			std::istream::int_type lc = input.get();
            if (lc >= 256 || lc < 0 || input.eof() || input.bad()) break;
			char c = (char)lc;

            ++abs_char; ++rel_char;
            if (c == '\n') { ++line_num; rel_char = 0; }

            if (LRStep<StateCount, NonTerminalCount, RuleCount, ActionCell, MaxStackSize, Ctx, ReduceFn, ErrorFn>
            (
                abs_char,
                line_num,
                rel_char,

                c,
                action_table,
                state_stack,
                ss_top,
                rule_2_nt,
                goto_table,
                bt_table,

                ctx,
                rfn,
                efn
            ))
                return;
        }

        if (!LRStep<StateCount, NonTerminalCount, RuleCount, ActionCell, MaxStackSize, Ctx, ReduceFn, ErrorFn>
        (
            abs_char,
            line_num,
            rel_char,

            256,
            action_table,
            state_stack,
            ss_top,
            rule_2_nt,
            goto_table,
            bt_table,

            ctx,
            rfn,
            efn
        ))
        {
            efn(abs_char, line_num, rel_char, state_stack[ss_top], 256);
        }
    }

}

#endif
