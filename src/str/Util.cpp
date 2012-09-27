
#include <scm/str/Util.hpp>
#include <scm/stream/util.hpp>
#include <iostream>
#include <math.h>

namespace str {

static const char benc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int invbenc(char c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    else if (c >= 'a' && c <= 'z')
        return 1 + (c - 'a') + ('Z' - 'A');
    else if (c >= '0' && c <= '9')
        return 2 + (c - '0') + ('Z' - 'A') + ('z' - 'a');
    else if (c == '+')
        return 3 + ('Z' - 'A') + ('z' - 'a') + ('9' - '0');
    else if (c == '/')
        return 3 + ('Z' - 'A') + ('z' - 'a') + ('9' - '0') + 1;
    else
        return -1;
}

static unsigned char bmasks[] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

std::string base64_encode(const std::vector<char>& s) {
    int         slen   = s.size();
    std::string result;
    int         sidx   = 0;
    int         scount = (int)ceil(((double)(slen * 8)) / 6.0);

#   define GS2B(si) (((double)(si*6))/8.0)
#   define LS2B(si) ((int)(floor(GS2B(si))))
#   define HS2B(si) ((int)(ceil(GS2B(si))))
#   define LB(si)   s[LS2B(si)]
#   define HB(si)   ((HS2B(si) >= slen) ? 0 : s[HS2B(si)])
#   define LBC(si)  (2*(si%4))

	result.resize(((2 + slen - ((slen + 2) % 3)) * 4 / 3) + 1);

    for (sidx = 0; sidx < scount; ++sidx) {
        char          lb  = LB(sidx);
        unsigned char lbc = LBC(sidx);
        char          hb  = HB(sidx);
        unsigned char hbc = 6 - lbc;
        unsigned char ei  = ((lb & bmasks[lbc]) << hbc) + ((hb & ((bmasks[hbc] << (8 - hbc)))) >> (8 - hbc));

        result[sidx] = benc[ei];
    }

    for (slen = ((int)(4.0*ceil(((double)scount)/4.0)))-scount; slen > 0; --slen)
        result[sidx++] = '=';

	result.resize(sidx);
    return result;
}

std::string base64_encode(std::istream& in) {
	return base64_encode(stream::slurpvec(in));
}

/*
t_lpr_str base64_encode(t_lpr_str s)
{
    int         slen   = strlen(s);
    t_lpr_str   result = 0;
    t_lpr_error error  = 0;
    int         sidx   = 0;
    int         scount = (int)ceil(((double)(slen * 8)) / 6.0);

#   define GS2B(si) (((double)(si*6))/8.0)
#   define LS2B(si) ((int)(floor(GS2B(si))))
#   define HS2B(si) ((int)(ceil(GS2B(si))))
#   define LB(si)   s[LS2B(si)]
#   define HB(si)   ((HS2B(si) >= slen) ? 0 : s[HS2B(si)])
#   define LBC(si)  (2*(si%4))

    error = lpr_mm_alloc(((2 + slen - ((slen + 2) % 3)) * 4 / 3) + 1, (t_lpr_ptr)&result, LPR_FALSE);
    LPR_ASSERT(LPR_SUCCESS(error));

    for (sidx = 0; sidx < scount; ++sidx)
    {
        char          lb  = LB(sidx);
        unsigned char lbc = LBC(sidx);
        char          hb  = HB(sidx);
        unsigned char hbc = 6 - lbc;
        unsigned char ei  = ((lb & bmasks[lbc]) << hbc) + ((hb & ((bmasks[hbc] << (8 - hbc)))) >> (8 - hbc));

        result[sidx] = benc[ei];
    }

    for (slen = ((int)(4.0*ceil(((double)scount)/4.0)))-scount; slen > 0; --slen)
        result[sidx++] = '=';

    result[sidx] = '\0';
    return result;
}

t_lpr_str base64_decode(t_lpr_str s)
{
    int         sidx   = 0;
    int         scount = strlen(s);
    t_lpr_str   result = 0;
    t_lpr_error error  = 0;

    error = lpr_mm_alloc(scount + 1, (t_lpr_ptr)&result, LPR_TRUE);
    LPR_ASSERT(LPR_SUCCESS(error));

    for (sidx = 0; (sidx < scount) && (s[sidx] != '='); ++sidx)
    {
        unsigned char dbp = invbenc(s[sidx]);
        int           li  = LS2B(sidx);
        unsigned char lbc = LBC(sidx);
        int           hi  = HS2B(sidx);
        unsigned char hbc = 6 - lbc;

        result[li] |= (dbp & (bmasks[lbc] << hbc)) >> hbc;
        result[hi] |= (dbp & bmasks[hbc]) << (8-hbc);
    }

    return result;
}
*/
}
