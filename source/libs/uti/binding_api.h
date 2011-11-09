#ifndef __BINDING_API_H
#define __BINDING_API_H

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

#include "uti/sge_dstring.h"
#include "uti/sge_binding_parse.h"

#if defined(PLPA)
#  include "plpa.h"
#  include <dlfcn.h>
#endif 

#if defined(HWLOC)
#  include <hwloc.h>
#endif


int get_amount_of_sockets(void);
int get_total_amount_of_cores(void);


#if defined(THREADBINDING)

#if defined(PLPA)
 typedef plpa_cpu_set_t sge_cpuset_t;
 #define sge_cpuset_set_mask(mask, cpuid) PLPA_CPU_SET(mask, cpuid);
#endif

#if defined(HWLOC)
 typedef hwloc_bitmap_t sge_cpuset_t;
 #define sge_cpuset_set_mask(cpuid, mask) hwloc_bitmap_set(*mask, cpuid);

 hwloc_topology_t hwloc_init(void);
#endif

char *topology_api_name(void);
bool has_topology_information(void);
bool get_topology(char** topology, int* length);
bool get_processor_ids(int socket_number, int core_number, int **proc_ids, int *amount);
int get_amount_of_cores(int socket_number);
bool has_core_binding(void);

#endif

#endif /* __BINDING_API_H */
