#include "lib_utils.h"

BOOL strToLower(char *  pchStr) {
    /*#[ operation strToLower(char *) */
    int  i, n32Len;
    
    n32Len = strlen( pchStr );
    assert( n32Len < 20 );
    
    for (i=0; i<n32Len; i++)
    {
        if ( isalpha(*(pchStr+i)) && isupper(*(pchStr+i)) )
        {
            *(pchStr+i) += 32;
        }
    }
    
    return TRUE;
    /*#]*/
}

