#ifndef __OWNERL_H
#define __OWNERL_H
/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum {
   O_owner = O_LOWERBOUND,    /* THIS IS VERY IMPORTANT */
   O_group
};


LISTDEF( OwnerT )
   SGE_STRING   ( O_owner, CULL_DEFAULT )
   SGE_STRING   ( O_group, CULL_DEFAULT )
LISTEND

NAMEDEF( OwnerN )
   NAME   ( "O_owner" )
   NAME   ( "O_group" )
NAMEEND

#define OwnerS sizeof(OwnerN)/sizeof(char*)

#endif /* __OWNERL_H */
