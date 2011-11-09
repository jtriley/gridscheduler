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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef WINDOWS
#include <unistd.h>

#include "basis_types.h"
#include "sge_os.h"
#include "msg_utilbin.h"
#include "sge_language.h"
#include "sge_prog.h"
#include "sge_host.h"
#include "sgermon.h"
#include "uti/sge_binding_hlp.h"
#include "uti/binding_api.h"
#include "sgeobj/sge_binding.h"
#else
#include "msg_utilbin.h"
#include <windows.h>
#include <io.h>
#endif

static void print_mem_load(char *, char *, int, double, char*);
static void check_core_binding(void);

#if defined(SOLARISPSET)
static void test_solaris_binding(void);
#endif 

#if defined(THREADBINDING)
static void test_binding(void);
#endif 

#if defined(THREADBINDING) || defined(SOLARISPSET)
static void fill_socket_core_topology(dstring* msocket, dstring* mcore, dstring* mtopology);
#endif

static void usage(void)
{
   fprintf(stderr, "%s loadcheck [-cb] | [-int] [-loadval name]\n", MSG_UTILBIN_USAGE);
   fprintf(stderr, "\t\t[-cb] \t\t\t\t Shows core binding and processor topology related information\n");
   fprintf(stderr, "\t\t[-int]\t\t\t\t Print as integer\n");
   fprintf(stderr, "\t\t[-loadval]\t\t\t Select specific load value\n");
   exit(1);
}
   
int main(int argc, char *argv[])
{
   double avg[3];
   int loads;
   char *name = NULL;
#if defined(THREADBINDING) || defined(SOLARISPSET)
   dstring msocket   = DSTRING_INIT;
   dstring mcore     = DSTRING_INIT;
   dstring mtopology = DSTRING_INIT;
#endif

#ifdef SGE_LOADMEM
   sge_mem_info_t mem_info;
#endif

#ifdef SGE_LOADCPU
   double total = 0.0;	
#endif

   int i, pos = 0, print_as_int = 0, precision = 0, core_binding = 0;
   char *m = "";

#ifndef WINDOWS
   DENTER_MAIN(TOP_LAYER, "loadcheck");
#endif

#ifdef __SGE_COMPILE_WITH_GETTEXT__   
   /* init language output for gettext() , it will use the right language */
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */

   if (argc == 2 && !strcmp(argv[1], "-cb"))
   {
      core_binding = 1;
   }
   else
   {
      for (i = 1; i < argc;)
      {
         if (!strcmp(argv[i], "-int"))
            print_as_int = 1;
         else if (!strcmp(argv[i], "-loadval"))
         {
            if (i + 1 < argc)
               pos=i+1;
            else
               usage();
            i++;
         }
         else
            usage();
         i++;
      }
   }   
   
   if (core_binding)
   {
      check_core_binding();
#ifndef WINDOWS
      DEXIT;
#endif
      return 1;
   }
   else if (print_as_int)
   {
      m = "";
      precision = 0;
   }   
   else
   {
      m = "M";
      precision = 6;
   }   

   if ((pos && !strcmp("arch", argv[pos])) || !pos)
   {
      const char *arch = "";
#if defined(WINDOWS)
      arch = "win32-x86";
#else
      arch = sge_get_arch();
#endif 
      printf("arch            %s\n", arch);
   }
      
   if ((pos && !strcmp("num_proc", argv[pos])) || !pos)
   {
      int nprocs = 1;
#if defined(WINDOWS)
      SYSTEM_INFO system_info;
      char        buf[100];

      GetSystemInfo(&system_info);
      nprocs = system_info.dwNumberOfProcessors;
      sprintf(buf, "num_proc        %d", nprocs);
      fflush(stdout);
      write(1, (const void*)buf, (unsigned int)strlen(buf));
      write(1, (const void*)"\0x0a", (unsigned int)1);
#else
      nprocs = sge_nprocs();
      printf("num_proc        %d\n", nprocs);
#endif
   }

#if defined(THREADBINDING) || defined(SOLARISPSET)
   fill_socket_core_topology(&msocket, &mcore, &mtopology);

   if ((pos && !strcmp("m_socket", argv[pos])) || !pos)
      printf("m_socket        %s\n", sge_dstring_get_string(&msocket));

   if ((pos && !strcmp("m_core", argv[pos])) || !pos)
      printf("m_core          %s\n", sge_dstring_get_string(&mcore));

   if ((pos && !strcmp("m_topology", argv[pos])) || !pos)
      printf("m_topology      %s\n", sge_dstring_get_string(&mtopology));

#else 
   if ((pos && !strcmp("m_socket", argv[pos])) || !pos)
      printf("m_socket        -\n");

   if ((pos && !strcmp("m_core", argv[pos])) || !pos)
      printf("m_core          -\n");

   if ((pos && !strcmp("m_topology", argv[pos])) || !pos)
      printf("m_topology      -\n");

#endif 

#if defined(WINDOWS)
   loads = 0;
   avg[0] = avg[1] = avg[2] = 0;
#else
   loads = sge_getloadavg(avg, 3);
#endif

   if (loads>0 && ((pos && !strcmp("load_short", argv[pos])) || !pos)) 
      printf("load_short      %.2f\n", avg[0]);

   if (loads>1 && ((pos && !strcmp("load_medium", argv[pos])) || !pos)) 
      printf("load_medium     %.2f\n", avg[1]);

   if (loads>2 && ((pos && !strcmp("load_long", argv[pos])) || !pos))
      printf("load_long       %.2f\n", avg[2]);
      
   if (pos)
      name = argv[pos];
   else
      name = NULL;

#ifdef SGE_LOADMEM
   /* memory load report */
   memset(&mem_info, 0, sizeof(sge_mem_info_t));
   if (sge_loadmem(&mem_info))
   {
      fprintf(stderr, MSG_SYSTEM_RETMEMORYINDICESFAILED );
      fprintf(stderr, "\n");
#ifndef WINDOWS
      DEXIT;
#endif
#if defined(THREADBINDING) || defined(SOLARISPSET)
      sge_dstring_free(&mcore);
      sge_dstring_free(&msocket);
      sge_dstring_free(&mtopology);
#endif
      return 1;
   }

   print_mem_load(LOAD_ATTR_MEM_FREE, name, precision, mem_info.mem_free, m); 
   print_mem_load(LOAD_ATTR_SWAP_FREE, name, precision, mem_info.swap_free, m); 
   print_mem_load(LOAD_ATTR_VIRTUAL_FREE, name, precision, mem_info.mem_free  + mem_info.swap_free, m); 

   print_mem_load(LOAD_ATTR_MEM_TOTAL, name, precision, mem_info.mem_total, m); 
   print_mem_load(LOAD_ATTR_SWAP_TOTAL, name, precision, mem_info.swap_total, m); 
   print_mem_load(LOAD_ATTR_VIRTUAL_TOTAL, name, precision, mem_info.mem_total + mem_info.swap_total, m);

   print_mem_load(LOAD_ATTR_MEM_USED, name, precision, mem_info.mem_total - mem_info.mem_free, m); 
   print_mem_load(LOAD_ATTR_SWAP_USED, name, precision, mem_info.swap_total - mem_info.swap_free, m); 
   print_mem_load(LOAD_ATTR_VIRTUAL_USED, name, precision,(mem_info.mem_total + mem_info.swap_total) - 
                                          (mem_info.mem_free  + mem_info.swap_free), m); 
#  ifdef IRIX
   print_mem_load(LOAD_ATTR_SWAP_USED, name, precision, mem_info.swap_rsvd, m); 
#  endif
#endif /* SGE_LOADMEM */

#ifdef SGE_LOADCPU
   loads = sge_getcpuload(&total);
   sleep(1);
   loads = sge_getcpuload(&total);

   if (loads != -1)
      print_mem_load("cpu", name,  1, total, "%");

#endif /* SGE_LOADCPU */
#ifndef WINDOWS
   DEXIT;
#endif

#if defined(THREADBINDING) || defined(SOLARISPSET)
   sge_dstring_free(&mcore);
   sge_dstring_free(&msocket);
   sge_dstring_free(&mtopology);
#endif
   return 0;
}

static void print_mem_load(char *name, char *thisone, int precision, double value, char *m)
{
   if ((thisone && !strcmp(name, thisone)) || !thisone)
      printf("%-15s %.*f%s\n", name, precision, value, m);
}

/****** loadcheck/check_core_binding() *****************************************
*  NAME
*     check_core_binding() -- Checks core binding functionality on current host. 
*
*  SYNOPSIS
*     void check_core_binding() 
*
*  FUNCTION
*     Checks core binding functionality on current host. 
*
*  INPUTS
*
*  RESULT
*     void - No result
*
*******************************************************************************/
static void check_core_binding()
{
   #if defined(THREADBINDING)
      printf("Your SGE uses %s for core binding functionality!\n", topology_api_name());   
      test_binding();
   #endif

   #if defined(SOLARISPSET)
      printf("Your SGE Solaris version has built-in core binding functionality!\n");
      test_solaris_binding();
   #endif

   #if !defined(THREADBINDING) && !defined(SOLARISPSET)
      printf("Core binding is not enabled!\n");
   #endif
}

#if defined(THREADBINDING)
static void test_binding()
{
   char* topology = NULL;

   if (!has_core_binding())
   {
      printf("Error initializing thread binding support!\n");
   }
   else if (!has_topology_information())
   {
      printf("No topology information could be retrieved!\n");
   }
   else
   {
      int s, c, length = 0;

      /* get amount of sockets */
      printf("Amount of sockets:\t\t%d\n", get_amount_of_sockets());

      /* get amount of cores   */
      printf("Amount of cores:\t\t%d\n", get_total_amount_of_cores());

      /* get topology */
      get_topology(&topology, &length);
      printf("Topology:\t\t\t%s\n", topology);
      FREE(topology); 

      printf("Mapping of logical socket and core numbers to internal\n");

      /* for each socket,core pair get the internal processor number */
      /* try multi-mapping */
      for (s = 0; s < get_amount_of_sockets(); s++)
      {
         for (c = 0; c < get_amount_of_cores(s); c++)
         {
            int *proc_ids  = NULL;
            int  amount    = 0;

            if (get_processor_ids(s, c, &proc_ids, &amount))
            {
               int i = 0;

               printf("Internal processor ids for socket %5d core %5d: ", s , c);

               for (i = 0; i < amount; i++)
                  printf(" %5d", proc_ids[i]);
               printf("\n");

               FREE(proc_ids);
            }
            else
            {
               printf("Couldn't get processor ids for socket %5d core %5d\n", s, c);
            }
         }
      }
   }   

   return;
}
#endif

#if defined(SOLARISPSET)
/****** loadcheck/test_solaris_binding() ***************************************
*  NAME
*     test_solaris_binding() -- Tests Solaris binding and topology information. 
*
*  SYNOPSIS
*     void test_solaris_binding() 
*
*  FUNCTION
*     Tests Solaris binding and checks the amount of cores and sockets 
*     on the system as well as the topology information. 
*
*  INPUTS
*
*  RESULT
*     void - returns nothing 
*
*******************************************************************************/
static void test_solaris_binding()
{
   char* topology = NULL;
   int length;
   int sockets, cores, s;

   /* get amount of sockets */
   sockets = get_amount_of_sockets();
   printf("Amount of sockets:\t\t%d\n", sockets);
   
   /* get amount of cores   */
   cores = get_total_amount_of_cores();
   printf("Amount of cores:\t\t%d\n", cores);

   /* get topology */
   if (get_execd_topology(&topology, &length))
   {
      int** matrix   = NULL;
      int mlength    = 0;
      int* cores     = NULL;
      int size;

      printf("Topology:\t\t\t%s\n", topology);
      
      if (!generate_chipID_coreID_matrix(&matrix, &mlength)) {
         printf("Couldn't get valid information from kstat cpu_info!\n");
         FREE(topology);
         return;
      }

      printf("Dumping internal (kstat) matrix:\n");
      for (s = 0; s < mlength; s++) {
         printf("chip_id: %5d core_id: %5d processor_id: %5d\n", 
                  (matrix[s])[0], (matrix[s])[1], (matrix[s])[2]);
      }
      free_matrix(matrix, mlength);
   } else {
      printf("Couldn't get the topology string!\n");
   }
  
}
#endif

#if defined(THREADBINDING) || defined(SOLARISPSET)
/****** loadcheck/fill_socket_core_topology() **********************************
*  NAME
*     fill_socket_core_topology() -- Get load values regarding processor topology. 
*
*  SYNOPSIS
*     void fill_socket_core_topology(dstring* msocket, dstring* mcore, dstring* 
*     mtopology) 
*
*  FUNCTION
*     Gets the values regarding processor topology. 
*
*  OUTPUTS 
*     dstring* msocket   - The amount of sockets the host have. 
*     dstring* mcore     - The amount of cores the host have.
*     dstring* mtopology - The topology the host have. 
*
*  RESULT
*     void - nothing 
*
*******************************************************************************/
static void fill_socket_core_topology(dstring* msocket, dstring* mcore, dstring* mtopology)
{
   int ms, mc;
   char* topo = NULL;
   int length = 0;

   if (!get_execd_topology(&topo, &length) || topo == NULL)
      topo = sge_strdup(NULL, "-");

   ms = get_amount_of_sockets();
   sge_dstring_sprintf(msocket, "%d", ms);

   mc = get_total_amount_of_cores();
   sge_dstring_sprintf(mcore, "%d", mc);

   sge_dstring_append(mtopology, topo);
   FREE(topo);
}

#endif
