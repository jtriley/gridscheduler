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

/* this code is used by execd & shepherd */
#include <ctype.h>
#include "sgermon.h"
#include "sge_string.h"

#include "sge_mtutil.h"
#include "uti/sge_binding_hlp.h"
#include "uti/binding_api.h"

#if defined(HWLOC)
char *topology_api_name(void)
{
  return "hwloc";
}

hwloc_topology_t hwloc_init(void)
{
   static hwloc_topology_t topology = NULL;

   if (topology == NULL)
   {
     hwloc_topology_init(&topology);
     hwloc_topology_load(topology);
   }
   return topology;
}

bool has_topology_information(void)
{
  const struct hwloc_topology_support *support;
  hwloc_topology_t topology = hwloc_init();

  if (topology == NULL)
    return false;

  support = hwloc_topology_get_support(topology);

  if (support != NULL)
    return support->discovery->pu;
  else
    return 0;
}

bool has_core_binding(void)
{
  const struct hwloc_topology_support *support;
  hwloc_topology_t topology = hwloc_init();

  if (topology == NULL)
    return 0;

  support = hwloc_topology_get_support(topology);

  if (support != NULL)
    return support->cpubind->set_thisproc_cpubind;
  else
    return 0;
}

int get_amount_of_sockets(void)
{
   int num_sockets = 0;
   hwloc_topology_t topology = hwloc_init();

   if (topology && has_topology_information())
   {
    int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);

    if (depth != HWLOC_TYPE_DEPTH_UNKNOWN)
       num_sockets = hwloc_get_nbobjs_by_depth(topology, depth);
   }

   return num_sockets;
}

static int get_amount_of_cores_helper(hwloc_topology_t topology, hwloc_obj_t obj, int depth)
{
    int i, num_cores = 0;
    hwloc_obj_type_t type = hwloc_get_depth_type(topology, depth);

    if (type == -1)
      return 0;
    else if (type == HWLOC_OBJ_CORE)
      return 1;

    for (i = 0; i < obj->arity; i++)
      num_cores += get_amount_of_cores_helper(topology, obj->children[i], depth + 1);

    return num_cores;
}

int get_amount_of_cores(int socket_number)
{
  hwloc_topology_t topology = hwloc_init();

  if (topology && has_topology_information())
  {
     int socket_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);

     if (socket_depth != HWLOC_TYPE_DEPTH_UNKNOWN)
     {
       int idx, num_sockets = hwloc_get_nbobjs_by_depth(topology, socket_depth);

       for (idx=0; idx < num_sockets; idx++)
       {
            hwloc_obj_t socket_obj = hwloc_get_obj_by_depth(topology, socket_depth, idx);

            if (socket_obj != NULL && socket_obj->logical_index == socket_number)
            {
              int i, num_cores = 0;

              for (i = 0; i < socket_obj->arity; i++)
                num_cores += get_amount_of_cores_helper(topology, socket_obj->children[i], socket_depth+1);

              return num_cores;
            }
       }
     }
  }
  return 0;
}

int get_total_amount_of_cores()
{
   int num_cores = 0;
   hwloc_topology_t topology = hwloc_init();

   if (topology && has_topology_information())
   {
     int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);

     if (depth != HWLOC_TYPE_DEPTH_UNKNOWN)
       num_cores = hwloc_get_nbobjs_by_depth(topology, depth);
   }

   return num_cores;
}

static void get_topology_helper(hwloc_topology_t topology, hwloc_obj_t obj, int depth, dstring *d_topology, int *length)
{
   hwloc_obj_type_t type = hwloc_get_depth_type(topology, depth);

   switch (type)
   {
     case HWLOC_OBJ_SOCKET:
      sge_dstring_append_char(d_topology, 'S');
      (*length)++;
      break;

     case HWLOC_OBJ_CORE:
      sge_dstring_append_char(d_topology, 'C');
      (*length)++;
      break;

     case HWLOC_OBJ_PU:
      sge_dstring_append_char(d_topology, 'T');
      (*length)++;
      break;

     default:
      break;
   }

   if (type == -1)
     return;

   if (!(type == HWLOC_OBJ_CORE && obj->arity <= 1))
   {
     int i;

     for (i = 0; i < obj->arity; i++)
       get_topology_helper(topology, obj->children[i], depth + 1, d_topology, length);
   }
}

bool get_topology(char** topology, int* length)
{
   bool success = false;

   *length = 0;

   if (has_topology_information())
   {
     dstring d_topology = DSTRING_INIT;
     hwloc_topology_t hwloc_topology = hwloc_init();

     get_topology_helper(hwloc_topology, hwloc_get_root_obj(hwloc_topology), 0, &d_topology, length);

     if ((*length) != 0)
     {
       (*length)++;

        (*topology) = sge_strdup(NULL, sge_dstring_get_string(&d_topology));
        success = true;
     }

     sge_dstring_free(&d_topology);
   }

   return success;
}

static void get_processor_ids_helper(hwloc_topology_t topology, hwloc_obj_t obj, int depth, int core_number, int *current_core, int *proc_ids[], int *amount)
{
   hwloc_obj_type_t type = hwloc_get_depth_type(topology, depth);

   if (type == -1)
     return;

   if (type == HWLOC_OBJ_CORE)
   {
     if ((*current_core)++ != core_number)
       return;
   }

   if (type == HWLOC_OBJ_PU)
   {
     (*amount)++;
     (*proc_ids) = (int *) realloc((*proc_ids), (*amount) * sizeof(int));
     (*proc_ids)[(*amount)-1] = obj->os_index;
     return;
   }

   {
     int i;

     for (i = 0; i < obj->arity; i++)
       get_processor_ids_helper(topology, obj->children[i], depth + 1, core_number, current_core, proc_ids, amount);
   }
}

bool get_processor_ids(int socket_number, int core_number, int *proc_ids[], int *amount)
{
  hwloc_topology_t topology = hwloc_init();

  *amount = 0;

  if (topology)
  {
    int socket_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);

    if (socket_depth != HWLOC_TYPE_DEPTH_UNKNOWN)
    {
      int idx, num_sockets = hwloc_get_nbobjs_by_depth(topology, socket_depth);

      for (idx=0; idx < num_sockets; idx++)
      {
         hwloc_obj_t socket_obj = hwloc_get_obj_by_depth(topology, socket_depth, idx);

         if (socket_obj != NULL && socket_obj->logical_index == socket_number)
         {
           int i, current_core = 0;

           for (i = 0; i < socket_obj->arity; i++)
           {
              get_processor_ids_helper(topology, socket_obj->children[i], socket_depth+1, core_number, &current_core, proc_ids, amount);
           }

           return (*amount != 0);
          }
      }
    }
  }

  return *amount;
}
#endif

#if defined(PLPA)

char *topology_api_name(void)
{
  return "PLPA";
}

/****** sge_binding_hlp/has_topology_information() *********************************
*  NAME
*     has_topology_information() -- Checks if current arch offers topology. 
*
*  SYNOPSIS
*     bool has_topology_information() 
*
*  FUNCTION
*     Checks if current architecture (on which this function is called) 
*     offers processor topology information or not.
*
*  RESULT
*     bool - true if the arch offers topology information false if not 
*
*  NOTES
*     MT-NOTE: has_topology_information() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool has_topology_information(void) 
{
   int has_topology = 0;
   
   if (plpa_have_topology_information(&has_topology) == 0 && has_topology == 1) {
      return true;
   }
    
   return false;
}

/****** sge_binding_hlp/has_core_binding() *****************************************
*  NAME
*     has_core_binding() -- Check if core binding system call is supported. 
*
*  SYNOPSIS
*     bool has_core_binding() 
*
*  FUNCTION
*     Checks if core binding is supported on the machine or not. If it is 
*     supported this does not mean that topology information (about socket 
*     and core amount) is available (which is needed for internal functions 
*     in order to perform a correct core binding).
*     Nevertheless a bitmask could be generated and core binding could be 
*     performed with this selfcreated bitmask.
*
*  RESULT
*     bool - True if core binding could be done. False if not. 
*
*  NOTES
*     MT-NOTE: has_core_binding() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool has_core_binding(void)
{
   
   /* checks if plpa is working */
   /* TODO do it only once? */
   plpa_api_type_t api_type;

   if (plpa_api_probe(&api_type) == 0 && api_type == PLPA_PROBE_OK) {
      return true;
   }

   return false;
}

/****** sge_binding_hlp/get_total_amount_of_cores() ********************************
*  NAME
*     get_total_amount_of_cores() -- Fetches the total amount of cores on system. 
*
*  SYNOPSIS
*     int get_total_amount_of_cores() 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*
*  RESULT
*     int - Total amount of cores installed on the system. 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: get_total_amount_of_cores() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int get_total_amount_of_cores(void)
{
   /* total amount of cores currently active on this system */
   int total_amount_of_cores = 0;
   
   if (has_core_binding() && has_topology_information()) {
      /* plpa_handle just for an early pre check */ 
      int nr_socket = get_amount_of_sockets();
      int cntr;
      
      /* get for each socket the amount of cores */
      for (cntr = 0; cntr < nr_socket; cntr++) {
         total_amount_of_cores += get_amount_of_cores(cntr);
      }
   }
   
   /* in case we got no information about topology we return 0 */
   return total_amount_of_cores;
}


/****** sge_binding_hlp/get_amount_of_cores() **************************************
*  NAME
*     get_amount_of_cores() -- ??? 
*
*  SYNOPSIS
*     int get_amount_of_cores(int socket_number) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     int socket_number - Physical socket number starting at 0. 
*
*  RESULT
*     int - 
*
*  NOTES
*     MT-NOTE: get_amount_of_cores() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int get_amount_of_cores(int socket_number) 
{

   if (has_core_binding() && has_topology_information()) {
      int socket_id;
      /* convert the reals socket number into the Linux socket_id */

      if (plpa_get_socket_id(socket_number, &socket_id) == 0) {
         int number_of_cores, max_core_id;
         /* now retrieve the amount of core for this socket number */
         if (plpa_get_core_info(socket_id, &number_of_cores, &max_core_id) == 0) {
            /* return the amount of cores available */
            return number_of_cores;
         } else {
            /* error when doing library call */
            return 0;
         }   

      } else {
         /* error: we didn't get the linux socket id */
         return 0;
      }
   }

   /* we have 0 cores in case something is wrong */
   return 0;
}

/****** sge_binding_hlp/get_amount_of_sockets() ************************************
*  NAME
*     get_amount_of_sockets() -- Get the amount of available sockets.  
*
*  SYNOPSIS
*     int get_amount_of_sockets() 
*
*  FUNCTION
*     Returns the amount of sockets available on this system. 
*
*  RESULT
*     int - The amount of available sockets on system. 0 in case of 
*                  of an error.
*
*  NOTES
*     MT-NOTE: get_amount_of_sockets() is not MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int get_amount_of_sockets() 
{

   if (has_core_binding() && has_topology_information()) {
      int num_sockets, max_socket_id;
      
      if (plpa_get_socket_info(&num_sockets, &max_socket_id) == 0) {
         return num_sockets;
      } else {
         /* in case of an error we have 0 sockets */
         return 0;
      }
   }

   /* we have 0 cores in case something is wrong */
   return 0;
}

/****** sge_binding_hlp/get_processor_ids() ******************************
*  NAME
*     get_processor_ids() -- Get internal processor ids for a specific core.
*
*  SYNOPSIS
*     bool get_processor_ids(int socket_number, int core_number, int** 
*     proc_ids, int* amount) 
*
*  FUNCTION
*     Get the Linux internal processor ids for a given core (specified by a socket, 
*     core pair). 
*
*  INPUTS
*     int socket_number - Logical socket number (starting at 0 without holes) 
*     int core_number   - Logical core number on the socket (starting at 0 without holes) 
*
*  OUTPUTS
*     int** proc_ids    - Array of Linux internal processor ids.
*     int* amount       - Size of the proc_ids array.
*
*  RESULT
*     bool - Returns true when processor ids where found otherwise false.
*
*  NOTES
*     MT-NOTE: get_processor_ids() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool get_processor_ids(int socket_number, int core_number, int** proc_ids, int* amount)
{
   int retval = true;


   if (proc_ids == NULL || (*proc_ids) != NULL || amount == NULL) {
      retval = false;
   } else if (has_core_binding() && has_topology_information()) {

      /* OS internal socket id and core id of 'socket_number' and 'core_number' */
      int input_core_id   = -1;
      int input_socket_id = -1;

      /* tmp socket and core id of a processor id */
      int core_id   = -1; 
      int socket_id = -1;

      /* the max. Linux processor ID */
      int max_proc_id    = -1;
      /* the number of processors    */
      int num_processors = -1;

      (*amount) = 0;

      /* convert logical socket number into internal socket id */
      if (retval && plpa_get_socket_id(socket_number, &input_socket_id) != 0) {
         /* unable to retrieve Linux logical socket id */
         retval = false;
      }

      /* convert logical core number into internal core id */
      if (retval && plpa_get_core_id(input_socket_id, core_number, &input_core_id) != 0) {
         /* unable to retrieve Linux logical core id  */
         retval = false;
      }

      /* get the max. processor id */
      if (retval && plpa_get_processor_data(PLPA_COUNT_ONLINE, &num_processors, 
                                    &max_proc_id) == 0) {

         /* a possible OS internal processor id */ 
         int proc_cntr;

         /* for all possible processor ids, check if they map to the 
            socket and core id we are searching for */
         for (proc_cntr = 0; proc_cntr <= max_proc_id; proc_cntr++) {

            /* check if processor id is on socket, core */
            if (plpa_map_to_socket_core(proc_cntr, &socket_id, &core_id) != 0) {
               /* not a valid processor id */
               continue;
            }

            if (socket_id == input_socket_id && core_id == input_core_id) {
               /* this OS internal proc id (proc_cntr) points to a 
                  socket, core we are searching for -> in case of hyperthreading 
                  there can be more than one:
                  -> add to the output array */
               (*amount)++;   /* increment the output array size */
               (*proc_ids) = (int *) realloc((*proc_ids), (*amount) * sizeof(int));

               if (*proc_ids == NULL) {
                  /* out of memory */
                  retval = false;
                  (*amount) = 0;
                  break;
               }   
               /* store the processor id as last element in output array */
               (*proc_ids)[(*amount)-1] = proc_cntr;
            }

         }
         
      }
   }

   if ((*amount) > 0) {
      return true;
   }

   return false;
}

/****** sge_binding_hlp/get_topology() ***********************************
*  NAME
*     get_topology() -- Creates the topology string for the current host. 
*
*  SYNOPSIS
*     bool get_topology(char** topology, int* length) 
*
*  FUNCTION
*     Creates the topology string for the current host. When it was created 
*     it has top be freed from outside.
*
*  INPUTS
*     char** topology - The topology string for the current host. 
*     int* length     - The length of the topology string.  
*
*  RESULT
*     bool - when true the topology string could be generated (and memory 
*            is allocated otherwise false
*
*  NOTES
*     MT-NOTE: get_topology() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool get_topology(char** topology, int* length)
{
   bool success = false;

   /* initialize length of topology string */
   (*length) = 0;

   int has_topology = 0;

   /* check if topology is supported via PLPA */
   if (plpa_have_topology_information(&has_topology) == 0 && has_topology == 1) {
      int num_sockets, max_socket_id;
         
      /* topology string */
      dstring d_topology = DSTRING_INIT;

      /* build the topology string */
      if (plpa_get_socket_info(&num_sockets, &max_socket_id) == 0) {
         int num_cores, max_core_id, ctr_cores, ctr_sockets, ctr_threads;
         char* s = "S"; /* socket */
         char* c = "C"; /* core   */
         char* t = "T"; /* thread */

         for (ctr_sockets = 0; ctr_sockets < num_sockets; ctr_sockets++) {
            int socket_id; /* internal socket id */

            /* append new socket */
            sge_dstring_append_char(&d_topology, *s);
            (*length)++;

            /* for each socket get the number of cores */ 
            if (plpa_get_socket_id(ctr_sockets, &socket_id) != 0) {
               /* error while getting the internal socket id out of the logical */
               continue;
            } 

            /* get information about this socket */
            if (plpa_get_core_info(socket_id, &num_cores, &max_core_id) == 0) {
               /* for thread counting */
               int* proc_ids = NULL;
               int amount_of_threads = 0;

               /* check each core */
               for (ctr_cores = 0; ctr_cores < num_cores; ctr_cores++) {
                  sge_dstring_append_char(&d_topology, *c);
                  (*length)++;
                  /* check if the core has threads */
                  if (get_processor_ids(ctr_sockets, ctr_cores, &proc_ids, &amount_of_threads) 
                        && amount_of_threads > 1) {
                     /* print the threads */
                     for (ctr_threads = 0; ctr_threads < amount_of_threads; ctr_threads++) { 
                        sge_dstring_append_char(&d_topology, *t);
                        (*length)++;
                     }
                  }
                  FREE(proc_ids);
               }
            }
         } /* for each socket */
            
         if ((*length) != 0) {
            /* convert d_topolgy into topology */
            (*length)++; /* we need `\0` at the end */
            
            /* copy element */ 
            (*topology) = sge_strdup(NULL, sge_dstring_get_string(&d_topology));
            success = true;      
         }
            
         sge_dstring_free(&d_topology);
      } 

   } 

   return success;
}

#endif 

#if !defined(THREADBINDING) && !defined(SOLARISPSET)

/*
 * get the number of cores of the execution host
 *
 */
int get_total_amount_of_cores(void)
{
  return 0;
}

/*
 * get the amount of sockets of the execution host
 *
 */
int get_amount_of_sockets(void)
{
   return 0;
}
#endif
