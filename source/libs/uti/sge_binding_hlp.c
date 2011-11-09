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

/* this code is used by shepherd */
#include <ctype.h>
#include "sgermon.h"
#include "sge_string.h"

#include "sge_mtutil.h"
#include "sge_log.h"
#include "uti/sge_binding_hlp.h"

/****** sge_binding_hlp/parse_binding_parameter_string() ***********************
*  NAME
*     parse_binding_parameter_string() -- Parses binding parameter string. 
*
*  SYNOPSIS
*     bool parse_binding_parameter_string(const char* parameter, u_long32* 
*     type, dstring* strategy, int* amount, int* stepsize, int* firstsocket, 
*     int* firstcore, dstring* socketcorelist, dstring* error) 
*
*  FUNCTION
*     Parses binding parameter string and returns the values of the parameter.
*     Please check output values in dependency of the strategy string.
*
*  INPUTS
*     const char* parameter   - binding parameter string 
*
*  OUTPUT 
*     u_long32* type          - type of binding (pe = 0| env = 1|set = 2)
*     dstring* strategy       - binding strategy string
*     int* amount             - amount of cores to bind to 
*     int* stepsize           - step size between cores (or -1)
*     int* firstsocket        - first socket to use (or -1)
*     int* firstcore          - first core to use (on "first socket") (or -1)
*     dstring* socketcorelist - list of socket,core pairs with prefix explicit or NULL
*     dstring* error          - error as string in case of return false
*
*  RESULT
*     bool - true in case parsing was successful false in case of errors
*
*  NOTES
*     MT-NOTE: parse_binding_parameter_string() is MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool parse_binding_parameter_string(const char* parameter, binding_type_t* type, 
      dstring* strategy, int* amount, int* stepsize, int* firstsocket, 
      int* firstcore, dstring* socketcorelist, dstring* error)
{
   bool retval = true;

   if (parameter == NULL)
   { 
      sge_dstring_sprintf(error, "input parameter was NULL");
      return false;
   }
   
   /* check the type [pe|env|set] (set is default) */
   if (strstr(parameter, "pe ") != NULL)
      *type = BINDING_TYPE_PE;
   else if (strstr(parameter, "env ") != NULL)
      *type = BINDING_TYPE_ENV;
   else
      *type = BINDING_TYPE_SET;   

   if (strstr(parameter, "linear") != NULL)
   {

      *amount = binding_linear_parse_amount(parameter);

      if (*amount  < 0)
      {
         /* couldn't parse amount of cores */
         sge_dstring_sprintf(error, "couldn't parse amount (linear)");
         return false;
      }

      *firstsocket = binding_linear_parse_socket_offset(parameter);
      *firstcore   = binding_linear_parse_core_offset(parameter);
      
      if (*firstsocket < 0 || *firstcore < 0)
      {
         /* couldn't find start <socket,core> -> must be determined 
            automatically */
         sge_dstring_sprintf(strategy, "linear_automatic");

         /* this might be an error on shepherd side only */
         *firstsocket = -1;
         *firstcore = -1;
      }
      else
      {
         sge_dstring_sprintf(strategy, "linear");
      }
      
      /* set step size to dummy */ 
      *stepsize = -1;
      
   }
   else if (strstr(parameter, "striding") != NULL)
   {
      
      *amount = binding_striding_parse_amount(parameter);
      
      if (*amount  < 0)
      {
         /* couldn't parse amount of cores */
         sge_dstring_sprintf(error, "couldn't parse amount (striding)");
         return false;
      }
  
      *stepsize = binding_striding_parse_step_size(parameter);

      if (*stepsize < 0)
      {
         sge_dstring_sprintf(error, "couldn't parse stepsize (striding)");
         return false;
      }

      *firstsocket = binding_striding_parse_first_socket(parameter);
      *firstcore = binding_striding_parse_first_core(parameter);
   
      if (*firstsocket < 0 || *firstcore < 0)
      {
         sge_dstring_sprintf(strategy, "striding_automatic");   

         /* this might be an error on shepherd side only */
         *firstsocket = -1;
         *firstcore = -1;
      }
      else
      {
         sge_dstring_sprintf(strategy, "striding");   
      }

   }
   else if (strstr(parameter, "explicit") != NULL)
   {

      if (binding_explicit_has_correct_syntax(parameter))
      {
         sge_dstring_sprintf(strategy, "explicit");

         /* explicit:<socket>,<core>:... */
         if (socketcorelist != NULL)
         {
            char* pos = strstr(parameter, "explicit");
            sge_dstring_copy_string(socketcorelist, pos);
            pos = NULL;
         }
         else
         {
            sge_dstring_sprintf(error, "BUG detected: DSTRING NOT INITIALIZED");
            retval = false;  
         }
      }
      else
      {
         sge_dstring_sprintf(error, "couldn't parse <socket>,<core> list (explicit)");
         retval = false;
      }
   }
   else
   {
      
      /* error: no valid strategy found */
      sge_dstring_sprintf(error, "couldn't parse binding parameter (no strategy found)"); 
      retval = false;
   }
   
  return retval; 
}

/****** sge_binding_hlp/binding_linear_parse_amount() ******************************
*  NAME
*    binding_linear_parse_amount() -- Parse the amount of cores to occupy. 
*
*  SYNOPSIS
*     int binding_linear_parse_amount(const char* parameter) 
*
*  FUNCTION
*    Parses a string in order to get the amount of cores requested. 
* 
*    The string has following format: "linear:<amount>:[<socket>,<core>]" 
*
*  INPUTS
*     const char* parameter - The first character of the string  
*
*  RESULT
*     int - if a value >= 0 then this reflects the number of cores
*           if a value < 0 then there was a parsing error
*
*  NOTES
*     MT-NOTE: binding_linear_parse_amount() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_linear_parse_amount(const char* parameter) 
{
   int retval = -1;

   /* expect string "linear" or "linear:<amount>" or linear 
      "linear:<amount>:<firstsocket>,<firstcore>" */

   if (parameter != NULL && strstr(parameter, "linear") != NULL)
   {
      /* get number after linear: and before \0 or : */ 
      if (sge_strtok(parameter, ":") != NULL)
      {
         char* n = sge_strtok(NULL, ":");
         if (n != NULL)
            return atoi(n);
      }   
   } 

   /* parsing error */
   return retval;
}

/****** sge_binding_hlp/bindingLinearParseSocketOffset() ***************************
*  NAME
*     bindingLinearParseSocketOffset() -- ??? 
*
*  SYNOPSIS
*     int bindingLinearParseSocketOffset(const char* parameter) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const char* parameter - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: bindingLinearParseSocketOffset() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_linear_parse_socket_offset(const char* parameter)
{
   /* offset is like "linear:<N>:<socket>,<core>) */
   if (parameter != NULL && strstr(parameter, "linear") != NULL) {
      /* fetch linear */
      if (sge_strtok(parameter, ":") != NULL) {
         /* fetch first number (if any) */
         if (sge_strtok(NULL, ":") != NULL) {
            char* offset = sge_strtok(NULL, ",");
            if (offset != NULL) { 
               /* offset points to <socket> */
               return atoi(offset);
            } 
         }
      }
   }
   
   /* wasn't able to parse */
   return -1;
}

/****** sge_binding_hlp/bindingLinearParseCoreOffset() *****************************
*  NAME
*     bindingLinearParseCoreOffset() -- ??? 
*
*  SYNOPSIS
*     int bindingLinearParseCoreOffset(const char* parameter) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const char* parameter - ??? 
*
*  RESULT
*     int - 
*
*  EXAMPLE
*     ??? 
*
*  NOTES
*     MT-NOTE: bindingLinearParseCoreOffset() is not MT safe 
*
*  BUGS
*     ??? 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_linear_parse_core_offset(const char* parameter)
{
   /* offset is like "linear:<N>:<socket>,<core> (optional ":") */
   if (parameter != NULL && strstr(parameter, "linear") != NULL) {
      /* fetch linear */
      if (sge_strtok(parameter, ":") != NULL) {
         /* fetch first number (if any) */
         if (sge_strtok(NULL, ":") != NULL) {
            char* offset = sge_strtok(NULL, ",");
            if (offset != NULL && 
                  (offset = sge_strtok(NULL, ":")) != NULL) {
               /* offset points to <core> */
               return atoi(offset);
            }
         }
      }
   }
  
   /* wasn't able to parse */
   return -1;
}

/****** sge_binding_hlp/binding_parse_type() ***********************************
*  NAME
*     binding_parse_type() -- Parses binding type out of binding string. 
*
*  SYNOPSIS
*     binding_type_t binding_parse_type(const char* parameter) 
*
*  FUNCTION
*     The execution daemon communicates with the shepherd with the config 
*     file. This function parses the type of binding out of the specific 
*     binding string from the config file. In case of binding type "set" 
*     there is no special prefix in this string. In case of "environment" 
*     the "env_" prefix in within the string. In case of setting the 
*     rankfile the "pe_" prefix can be found in this string.
*
*  INPUTS
*     const char* parameter - The binding string from the config file. 
*
*  RESULT
*     binding_type_t - The type of binding. 
*
*  NOTES
*     MT-NOTE: binding_parse_type() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
binding_type_t binding_parse_type(const char* parameter)
{
   binding_type_t type = BINDING_TYPE_SET;
   
   if (strstr(parameter, "env_") != NULL) {
      type = BINDING_TYPE_ENV;
   } else if (strstr(parameter, "pe_") != NULL) {
      type = BINDING_TYPE_PE;
   }

   return type;
}


/****** sge_binding_hlp/binding_explicit_has_correct_syntax() *********************
*  NAME
*     binding_explicit_has_correct_syntax() -- Check if parameter has correct syntax. 
*
*  SYNOPSIS
*     bool binding_explicit_has_correct_syntax(const char* parameter) 
*
*  FUNCTION
*     This function checks if the given string is a valid argument for the 
*     -binding parameter which provides a list of socket, cores which have 
*     to be selected explicitly.
* 
*     The accepted syntax is: "explicit:[1-9][0-9]*,[1-9][0-9]*(:[1-9][0-9]*,[1-9][0-9]*)*"
*
*     This is used from parse_qsub.c.
*
*  INPUTS
*     const char* parameter - A string with the parameter. 
*
*  RESULT
*     bool - True if the parameter has the expected syntax.
*
*  NOTES
*     MT-NOTE: binding_explicit_has_correct_syntax() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool binding_explicit_has_correct_syntax(const char* parameter) 
{
   
   /* DG TODO: introduce check if particles are numbers */

   /* check if the head is correct */
   if (strstr(parameter, "explicit:") == NULL) {
      return false;
   }

   if (sge_strtok(parameter, ":") != NULL) {
      char* socket = NULL;

      /* first socket,core is mandatory */ 
      if ((socket = sge_strtok(NULL, ",")) == NULL) {
         /* we have no first socket number */
         return false;
      }
      /* check for core */
      if (sge_strtok(NULL, ":") == NULL) {
         /* we have no first core number */
         return false;
      }
     
      do {
         /* get socket number */ 
         if ((socket = sge_strtok(NULL, ",")) != NULL) {

            /* we have a socket therefore we need a core number */
            if (sge_strtok(NULL, ":") == NULL) {
               /* no core found */
               return false;
            }   

         } 
      } while (socket != NULL);  /* we try to continue with the next socket if possible */ 

   } else {
      /* this should not be reachable because of the pre-check */
      return false;
   }

   return true;
}

/****** sge_binding_hlp/binding_striding_parse_first_core() ************************
*  NAME
*     binding_striding_parse_first_core() -- Parses core number from command line. 
*
*  SYNOPSIS
*     int binding_striding_parse_first_core(const char* parameter) 
*
*  FUNCTION
*     Parses the core number from command line in which to start binding 
*     in "striding" case. 
*
*     -binding striding:<amount>:<stepsize>:<socket>,<core>
*
*  INPUTS
*     const char* parameter - Pointer to first character of CL string. 
*
*  RESULT
*     int - -1 in case the string is corrupt or core number is not set
*           >= 0 in case the core number could parsed successfully.
*
*  NOTES
*     MT-NOTE: binding_striding_parse_first_core() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_first_core(const char* parameter)
{
   /* "striding:<amount>:<stepsize>:<socket>,<core>" */
   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      /* fetch "striding" */
      if (sge_strtok(parameter, ":") != NULL) {
         /* fetch <amount> */
         if (sge_strtok(NULL, ":") != NULL) {
            /* fetch <stepsize> */
            if (sge_strtok(NULL, ":") != NULL) {
               /* fetch first <socket> */
               if (sge_strtok(NULL, ",") != NULL) {
                  /* fetch first <core> */ 
                  char* first_core = NULL;
                  /* end usually with line end */
                  if ((first_core = sge_strtok(NULL, ";")) != NULL) {
                     return atoi(first_core);
                  } 
               }
            }
         }
      }   
   }

   return -1;
}


/****** sge_binding_hlp/binding_striding_parse_first_socket() **********************
*  NAME
*     binding_striding_parse_first_socket() -- Parses the socket to begin binding on. 
*
*  SYNOPSIS
*     int binding_striding_parse_first_socket(const char* parameter) 
*
*  FUNCTION
*     Parses the "striding:" parameter string for the socket number.
*
*     The string is expected to have following syntax: 
*    
*           "striding:<amount>:<stepsize>[:<socket>,<core>]"
*
*  INPUTS
*     const char* parameter - Points to the string with the query. 
*
*  RESULT
*     int - Returns the socket number in case it could be parsed otherwise -1
*
*  NOTES
*     MT-NOTE: binding_striding_parse_first_socket() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_first_socket(const char* parameter)
{
   /* "striding:<amount>:<stepsize>:<socket>,<core>" */
   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      /* fetch "striding" */
      if (sge_strtok(parameter, ":") != NULL) {
         /* fetch amount*/
         if (sge_strtok(NULL, ":") != NULL) {
            /* fetch stepsize */
            if (sge_strtok(NULL, ":") != NULL) {
               /* fetch first socket */ 
               char* first_socket = NULL;
               if ((first_socket = sge_strtok(NULL, ",")) != NULL) {
                  return atoi(first_socket);
               } 
            }
         }
      }   
   }

   return -1;
}


/****** sge_binding_hlp/binding_striding_parse_amount() ****************************
*  NAME
*     binding_striding_parse_amount() -- Parses the amount of cores to bind to. 
*
*  SYNOPSIS
*     int binding_striding_parse_amount(const char* parameter) 
*
*  FUNCTION
*     Parses the amount of cores to bind to out of "striding:" parameter string.
*
*     The string is expected to have following syntax: 
*    
*           "striding:<amount>:<stepsize>[:<socket>,<core>]"
*
*  INPUTS
*     const char* parameter - Points to the string with the query. 
*
*  RESULT
*     int - Returns the amount of cores to bind to otherwise -1.
*
*  NOTES
*     MT-NOTE: binding_striding_parse_amount() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_amount(const char* parameter)
{
   /* striding:<amount>:<step-size>:[starting-socket,starting-core] */

   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      
      /* fetch "striding:" */
      if (sge_strtok(parameter, ":") != NULL) {
         char* amount = NULL;

         if ((amount = sge_strtok(NULL, ":")) != NULL) {
            /* get the number from amount */
            /* DG TODO check if this is really a number */
            return atoi(amount);
         }      
      }
   }

   /* couldn't parse it */
   return -1;
}

/****** sge_binding_hlp/binding_striding_parse_step_size() *************************
*  NAME
*     binding_striding_parse_step_size() -- Parses the step size out of the "striding" query. 
*
*  SYNOPSIS
*     int binding_striding_parse_step_size(const char* parameter) 
*
*  FUNCTION
*     Parses the step size for the core binding strategy "striding" out of the 
*     query.
* 
*     The query string is expected to have following syntax: 
*    
*           "striding:<amount>:<stepsize>[:<socket>,<core>]"
*
*  INPUTS
*     const char* parameter - Points to the string with the query. 
*
*  RESULT
*     int - Returns the step size or -1 when it could not been parsed. 
*
*  NOTES
*     MT-NOTE: binding_striding_parse_step_size() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
int binding_striding_parse_step_size(const char* parameter)
{
   /* striding:<amount>:<step-size>:  */ 
   if (parameter != NULL && strstr(parameter, "striding") != NULL) {
      /* fetch "striding:" */
      if (sge_strtok(parameter, ":") != NULL) {
         if (sge_strtok(NULL, ":") != NULL) {
            /* fetch step size */
            char* stepsize = NULL;
            if ((stepsize = sge_strtok(NULL, ":")) != NULL) {
               /* the step size must be followed by " " or ":" or "\0" 
                  in order to avoid garbage like "striding:2:0,0" */
               if ((stepsize+1) == NULL || *(stepsize+1) == ' ' || 
                     *(stepsize+1) == ':' || *(stepsize+1) == '\0') {
                  /* return step size */
                  return atoi(stepsize);
               }     
            }
         }
      }
   }
   
   /* in default case take each core */
   return -1;
}

/****** uti/binding_hlp/binding_get_topology_for_job() ********************
*  NAME
*     binding_get_topology_for_job() -- Returns topology string. 
*
*  SYNOPSIS
*     const char *
*     binding_get_topology_for_job(const char *binding_result)
*
*  FUNCTION
*     Returns the topology string of a host where the cores are 
*     marked with lowercase letters for those cores that were
*     "bound" to a certain job.
*     
*     It is assumed the 'binding_result' parameter that is
*     passed to this function was previously returned by
*     create_binding_strategy_string()
*     create_binding_strategy_string_solaris()
*
*  INPUTS
*     const char *binding_result - string returned by
*                         create_binding_strategy_string() 
*                         create_binding_strategy_string_solaris() 
*
*  RESULT
*     const char * - topology string like "SCc"
*******************************************************************************/
const char *
binding_get_topology_for_job(const char *binding_result) {
   const char *topology_result = NULL;

   if (binding_result != NULL) {
      /* find test after last colon character including this character */
      topology_result = strrchr(binding_result, ':');

      /* skip colon character */
      if (topology_result != NULL) {
         topology_result++;
      }
   }
   return topology_result;
}

/****** sge_binding_hlp/topology_string_to_socket_core_lists() *****************
*  NAME
*     topology_string_to_socket_core_lists() -- Converts a topology into socket,core lists. 
*
*  SYNOPSIS
*     bool topology_string_to_socket_core_lists(const char* topology, int** 
*     sockets, int** cores, int* amount) 
*
*  FUNCTION
*     Converts a topology string into lists of cores and sockets which are marked 
*     as beeing used and returns them.
*
*  INPUTS
*     const char* topology - Pointer to a topology string.
*
*  OUTPUTS
*     int** sockets        - Pointer to the location of the socket array.
*     int** cores          - Pointer to the location of the core array. 
*     int* amount          - Length of the arrays.  
*
*  RESULT
*     bool - false when problems occured true otherwise
*
*  NOTES
*     MT-NOTE: topology_string_to_socket_core_lists() is MT safe 
*
*  SEE ALSO
*     ???/???
*******************************************************************************/
bool
topology_string_to_socket_core_lists(const char* topology, int** sockets, 
                                     int** cores, int* amount) {
   bool retval = true;

   int current_socket = -1;
   int current_core   = -1;

   *amount = 0;
   
   if (topology == NULL || *sockets != NULL || *cores != NULL) {
      /* we expect to have clean input */
      retval = false;
   } else {
   
      while (*topology != '\0') {

         if (*topology == 'S' || *topology == 's') {
            current_socket++;
            current_core = -1;
         } else if (*topology == 'C') {
            /* this core is not in use */
            current_core++;
         } else if (*topology == 'c') {
            /* this core is in use hence we are collecting it */
            (*amount)++;
            current_core++;
            *sockets = realloc(*sockets, (*amount) * sizeof(int));
            *cores   = realloc(*cores,   (*amount) * sizeof(int));
            (*sockets)[(*amount)-1] = current_socket;
            (*cores)[(*amount)-1]   = current_core;
         }

         topology++;
      }
      
   }

   return retval;
}
