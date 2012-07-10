/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Keisuke Kishimoto <kishimoto-k@klab.org>                     |
  +----------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1 2007/01/01 19:32:09 iliaa Exp $ */

#ifndef PHP_MAKUO_H
#define PHP_MAKUO_H

extern zend_module_entry makuo_module_entry;
#define phpext_makuo_ptr &makuo_module_entry

#ifdef PHP_WIN32
#define PHP_MAKUO_API __declspec(dllexport)
#else
#define PHP_MAKUO_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(makuo);
PHP_MSHUTDOWN_FUNCTION(makuo);
PHP_RINIT_FUNCTION(makuo);
PHP_RSHUTDOWN_FUNCTION(makuo);
PHP_MINFO_FUNCTION(makuo);

PHP_METHOD(makuo, __construct);
PHP_METHOD(makuo, __destruct);
PHP_METHOD(makuo, close);
PHP_METHOD(makuo, send);
PHP_METHOD(makuo, sync);
PHP_METHOD(makuo, status);
PHP_METHOD(makuo, members);
PHP_METHOD(makuo, check);
PHP_METHOD(makuo, exclude_add);
PHP_METHOD(makuo, exclude_del);
PHP_METHOD(makuo, exclude_list);
PHP_METHOD(makuo, exclude_clear);
PHP_METHOD(makuo, connect_tcp);
PHP_METHOD(makuo, connect_unix);

static zend_class_entry *makuo_ce;

/*
    Declare any global variables you may need between the BEGIN
    and END macros here:

ZEND_BEGIN_MODULE_GLOBALS(makuo)
    long  global_value;
    char *global_string;
ZEND_END_MODULE_GLOBALS(makuo)
*/

/* In every utility function you add that needs to use variables 
   in php_makuo_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as MAKUO_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define MAKUO_G(v) TSRMG(makuo_globals_id, zend_makuo_globals *, v)
#else
#define MAKUO_G(v) (makuo_globals.v)
#endif

#define DRYRUN "dryrun"
#define RECURSIVE "recursive"
#define TARGET_HOST "target_host"
#define DELETE "delete"
#define RCV_TIMEOUT "rcv_timeout"

#define ERROR_SOCKCREATE "Socket creation failed"
#define ERROR_INVALIDHOST "Invalid host"
#define ERROR_CONNFAILED "Connection failed"
#define ERROR_TOOLONGPARAM "Too long parameter"

static const int kDefaultConTimeout = 1;
static const int kDefaultRcvTimeout = 30;
static const int kMaxSendBuffSize = 1024;
static const int kMaxReadBuffSize = 8192;

/*
 * helper functions
 */
static inline int writeline(int s, const char *buff);
static inline int wait_prompt(int s, char* std_out, int std_out_size,
                              char* std_err, int std_err_size);
static inline void close_socket_if_not_closed(zval *obj TSRMLS_DC);
static inline int do_command(zval *obj, const char* message, char* std_out,
                             int std_out_size, char* std_err, int std_err_size,
                             struct timeval* end_tv TSRMLS_DC);
static inline int connect_socket_tcp(int s, const char* host, int port);
static inline int connect_socket_unix(int s, const char* path);
static inline int set_socket_timeout(int sock, const struct timeval* end_tv);

/*
 * accessors
 */
static inline void set_socket(zval *obj, int sock TSRMLS_DC);
static inline int get_socket(zval *obj, int sock TSRMLS_DC);
static inline void set_rcv_timeout(zval *obj, int timeout TSRMLS_DC);
static inline int get_rcv_timeout(zval *obj TSRMLS_DC);
static inline void set_error(zval *obj, const char* error_msg TSRMLS_DC);

#endif  /* PHP_MAKUO_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
