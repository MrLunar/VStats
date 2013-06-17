/**
 * VStats
 *
 * Author: Mark Bowker
 */

#ifndef PHP_VSTATS_H
#define PHP_VSTATS_H

extern zend_module_entry vstats_module_entry;
#define phpext_vstats_ptr &vstats_module_entry

#ifdef PHP_WIN32
#define PHP_VSTATS_API __declspec(dllexport)
#else
#define PHP_VSTATS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Module Includes */
#include <netinet/in.h>

/* Module Initialisation/Termination Functions */
PHP_MINIT_FUNCTION(vstats);
PHP_MSHUTDOWN_FUNCTION(vstats);
PHP_RINIT_FUNCTION(vstats);
PHP_RSHUTDOWN_FUNCTION(vstats);
PHP_MINFO_FUNCTION(vstats);

/* Module Functions for PHP */
PHP_FUNCTION(vstats_enabled);
PHP_FUNCTION(vstats_send);
PHP_FUNCTION(vstats_set_prefix);

/* Module Global Variables */
ZEND_BEGIN_MODULE_GLOBALS(vstats)
  // INI Settings
  char *default_prefix;
  zend_bool send_request_uuid;
  zend_bool send_request_peak_mem_usage;
  zend_bool send_request_time;
  // Other
  int vstats_enabled;
  long socket_fd;
	struct sockaddr_in remote;
  char *request_uuid;
  struct timeval request_start_time;
ZEND_END_MODULE_GLOBALS(vstats)

/* In every utility function you add that needs to use variables
   in php_extname_globals, call TSRMLS_FETCH(); after declaring other
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as EXTNAME_G(variable).  You are
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define VSTATS_G(v) TSRMG(vstats_globals_id, zend_vstats_globals *, v)
#else
#define VSTATS_G(v) (vstats_globals.v)
#endif

#endif
