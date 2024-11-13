#include "parser.h"

using namespace std;

bool parse_label(TokenStream& ts, string& label)
{
    if (ts.match({ REGULAR, COLON }))
    {
        label = ts[0].str;
        ts.advance(2);

        return true;
    }

    return false;
}