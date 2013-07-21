/*
 * screen.c
 *
 *  Created on: Jul 22, 2013
 *      Author: hudas
 */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "screen.h"
#include "constants.h"

/*
** The colorRatio is a canned function that will return an ANSI
** color string based on the resultant percentage of num/den. This
** is the function that was written to support hit/max, mana/max
** and move/max color prompt strings but may be used for anything
** needing this sort of relationship.
*/
char *
colorRatio( struct char_data *ch, int cookIt, int clvl, int num, int den )
{
    static char *colorRawANSI[] = {
        BKRBY,  /* 00 - 09 : BKRBY */
        BKRBY,  /* 10 - 19 : BKRBY */
        BRED,   /* 20 - 29 : BRED  */
        BYEL,   /* 30 - 39 : BYEL  */
        BYEL,   /* 40 - 49 : BYEL  */
        BYEL,   /* 50 - 59 : BYEL  */
        BCYN,   /* 60 - 69 : BCYN  */
        BCYN,   /* 70 - 79 : BCYN  */
        BGRN,   /* 80 - 89 : BGRN  */
        BGRN    /* 90 - 99 : BGRN  */
    };

    static char *colorCooked[] = {
    	BKRBY,  /* 00 - 09 : BKRBY */
    	BKRBY,  /* 10 - 19 : BKRBY */
        BRED,   /* 20 - 29 : BRED  */
        BYEL,   /* 30 - 39 : BYEL  */
        BYEL,   /* 40 - 49 : BYEL  */
        BYEL,   /* 50 - 59 : BYEL  */
        BCYN,   /* 60 - 69 : BCYN  */
        BCYN,   /* 70 - 79 : BCYN  */
        BGRN,   /* 80 - 89 : BGRN  */
    };
#   define NUM_ENTRIES (sizeof(colorCooked) / sizeof(colorCooked[0]))
#   define LAST_ENTRY  (NUM_ENTRIES-1)
    /*
    ** Calculate the percentage and then the index
    ** based on the percentage value.
    */
    int pcnt = (int)(100.0 * (float)num/(float)den);
    int cidx = pcnt / NUM_ENTRIES;
    /*
    ** Bail out here if we haven't reached the user's
    ** current color level.
    */
    if( !clr( ch, clvl ) ){ return( KNRM ); }

    /*
    ** Avoid any underflow problems.
    */
    cidx = (cidx < 0 ? 0 : cidx);

    /*
    ** Avoid any overflow problems.
    */
    cidx = (cidx > LAST_ENTRY ? LAST_ENTRY : cidx );

    /*
    ** Finally, return a pointer to the static color string.
    */
    return((cookIt == COLOR_COOKED ? colorCooked[cidx] : colorRawANSI[cidx] ));
}



