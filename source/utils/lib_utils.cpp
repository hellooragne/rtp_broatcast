#include "lib_utils.h"
#include <assert.h>

BOOL8 strToLower(char *  pchStr) {
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
    
    return TRUE_B8;
    /*#]*/
}

/* compare two strings case-insentively 
        TRUE_B8 -- same 
        FALSE_B8 -- different
 */
BOOL8 StrICmp(BYTE *  pbstr1, BYTE *  pbstr2)
{
    BYTE bLocation=0;
    
    while((*pbstr1!=0)&&(*pbstr2!=0))
    {
        if((*pbstr1==*pbstr2))
        {   
            pbstr1++;
            pbstr2++;
        }
        else
        {
            if( ((*pbstr1<=90)&&(*pbstr1>=65)&&(*pbstr2==(*pbstr1+32)))
              ||((*pbstr2<=90)&&(*pbstr2>=65)&&(*pbstr1==(*pbstr2+32))) )
            {
    			pbstr1++;
    			pbstr2++;
            }
            else
            {
    			return FALSE_B8;
            }
        }
    }

    if((*pbstr1==0)&&(*pbstr1==*pbstr2))
    	return TRUE_B8;
        
    return FALSE_B8;
}



