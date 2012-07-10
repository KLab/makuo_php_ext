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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/time.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_makuo.h"
#include "ext/standard/php_smart_str.h"
#include "main/snprintf.h"

/* If you declare any globals in php_makuo.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(makuo)
*/

/* True global resources - no need for thread safety here */
static int le_makuo;

/* {{{ makuo_methods[]
 */
zend_function_entry makuo_methods[] = {
    PHP_ME(makuo, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, __destruct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, close, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, send, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, sync, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, status, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, members, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, check, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, exclude_add, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, exclude_del, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, exclude_list, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, exclude_clear, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, connect_tcp, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(makuo, connect_unix, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL} /* Must be the last line in makuo_methods[] */
};
/* }}} */

/* {{{ makuo_module_entry
 */
zend_module_entry makuo_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "makuo",
    NULL,
    PHP_MINIT(makuo),
    PHP_MSHUTDOWN(makuo),
    PHP_RINIT(makuo),       /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(makuo),   /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(makuo),
#if ZEND_MODULE_API_NO >= 20010901
    "0.1", /* Replace with version number for your extension */
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MAKUO
ZEND_GET_MODULE(makuo)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("makuo.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_makuo_globals, makuo_globals)
    STD_PHP_INI_ENTRY("makuo.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_makuo_globals, makuo_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_makuo_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_makuo_init_globals(zend_makuo_globals *makuo_globals)
{
    makuo_globals->global_value = 0;
    makuo_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(makuo)
{
    /* Register class */
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Makuosan", makuo_methods);
    makuo_ce = zend_register_internal_class(&ce TSRMLS_CC);

    /* Register properties */
    zend_declare_property_long(makuo_ce, "socket_", strlen("socket_"),
                               -1, ZEND_ACC_PRIVATE TSRMLS_CC);

    zend_declare_property_long(makuo_ce, "rcv_timeout_", strlen("rcv_timeout_"),
                               kDefaultRcvTimeout, ZEND_ACC_PRIVATE TSRMLS_CC);

    zend_declare_property_string(makuo_ce, "error", strlen("error"),
                                 "", ZEND_ACC_PUBLIC TSRMLS_CC);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(makuo)
{
    /* uncomment this line if you have INI entries
    UNREGISTER_INI_ENTRIES();
    */
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(makuo)
{
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(makuo)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(makuo)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "makuo support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */


/* from msync.c */
static inline
int writeline(int s, const char *buff)
{
  int r;
  int clen;
  int size;

  clen = strlen(buff);
  size = clen;
  while(size){
    r = write(s, buff + clen - size, size);
    if(r == -1){
      return(-1);
    }
    size -= r;
  }
  return(0);
}

static inline
int wait_prompt(int s, char* std_out, int std_out_size, char* std_err,
                int std_err_size){
    char buff[kMaxReadBuffSize];
    char* line_start = buff;
    char* read_end = buff;

    while (1) {
        /* reached to the prompt */
        if (read_end - line_start >= 2 && !memcmp(line_start, "> ", 2)) return 1;

        /* find next newline */
        char* line_end;
        for (line_end = line_start; line_end < read_end; ++line_end) {
            if (*line_end == '\r') *line_end = '\0';
            if (*line_end == '\n') { *line_end = '\0'; break; }
        }
        if (line_end < read_end) {
            /* now we can access to a line through line_start */
            if (!strcmp(line_start, "alive")) {
            } else if (!memcmp(line_start, "error:", 6)) {
                if (std_err) {
                    int written = ap_php_snprintf(std_err, std_err_size, "%s\r\n", line_start);
                    std_err += written;
                    std_err_size -= written;
                    if (std_err_size <= 0) return -1;  /* std_err over flow */
                }
            } else {
                if (std_out) {
                    int written = ap_php_snprintf(std_out, std_out_size, "%s\r\n", line_start);
                    std_out += written;
                    std_out_size -= written;
                    if (std_out_size <= 0) return -1;  /* std_out over flow */
                }
            }

            line_start = line_end + 1;
        } else {
            /* shift remaining characters and read new ones */
            int remain = read_end - line_start;
            if (remain == sizeof(buff)) return -1;  /* over flow */
            memmove(buff, line_start, remain);

            int r = read(s, buff + remain, sizeof(buff) - remain);
            if (r == -1) return -1;  /* read error */
            if (r == 0) return -1;  /* socket was closed by the remote host */
            read_end = buff + remain + r;
            line_start = buff;
        }
    }
}

static inline
void close_socket_if_not_closed(zval *obj TSRMLS_DC) {
    zval *socket_ = zend_read_property(Z_OBJCE_P(obj), obj, "socket_",
                                       strlen("socket_"), 1 TSRMLS_CC);
    if (socket_->value.lval != -1) {
        close(socket_->value.lval);
        socket_->value.lval = -1;
    }
}

static inline
int do_command(zval *obj, const char* message, char* std_out, int std_out_size,
               char* std_err, int std_err_size, struct timeval* end_tv TSRMLS_DC) {
    struct timeval tv;
    if (end_tv == NULL) {
        gettimeofday(&tv, NULL);
        tv.tv_sec += get_rcv_timeout(obj TSRMLS_CC);
        end_tv = &tv;
    }

    zval *socket_ = zend_read_property(Z_OBJCE_P(obj), obj, "socket_",
                                       strlen("socket_"), 1 TSRMLS_CC);
    int s = socket_->value.lval;
    if (s == -1) {
        /* not connected yet */
        return -1;
    }

    /* store original timeout */
    struct timeval org_rcv_tv, org_snd_tv;
    socklen_t rcv_len, snd_len;
    getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (void *)&org_rcv_tv, &rcv_len);
    getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (void *)&org_snd_tv, &snd_len);

    if (set_socket_timeout(s, end_tv) == -1) {
        /* connection timed out */
        return -1;
    }

    if (writeline(s, message) == -1) {
        /* write error */
        return -1;
    }

    if (set_socket_timeout(s, end_tv) == -1) {
        /* connection timed out */
        return -1;
    }

    if (wait_prompt(socket_->value.lval, std_out, std_out_size, std_err,
                    std_err_size) == -1) {
        /* read error */
        return -1;
    }

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (void *)&org_rcv_tv, sizeof(org_rcv_tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (void *)&org_snd_tv, sizeof(org_snd_tv));

    return 0;
}

static inline
int connect_socket_tcp(int s, const char* host, int port) {
    in_addr_t ia = inet_addr(host);
    if (ia == INADDR_NONE) {
        /* invalid host */
        return -1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = ia;

    int c = connect(s, (const struct sockaddr*)&server, sizeof(server));
    if (c == -1) {
        /* connect failed */
        return -2;
    }

    return 0;
}

static inline
int connect_socket_unix(int s, const char* path) {
    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strncpy(server.sun_path, path, sizeof(server.sun_path));

    int c = connect(s, (const struct sockaddr*)&server, sizeof(server));
    if (c == -1) {
        /* connect failed */
        return -2;
    }

    return 0;
}

static inline
int set_socket_timeout(int sock, const struct timeval* end_tv) {
    struct timeval tv, cur_tv;

    gettimeofday(&cur_tv, NULL);

    if (timercmp(&cur_tv, end_tv, >=)) {
        return -1;
    }

    timersub(end_tv, &cur_tv, &tv);

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));
    return 0;
}

/*
 * accessors
 */
static inline
void set_socket(zval *obj, int sock TSRMLS_DC) {
    zval *socket_ = zend_read_property(Z_OBJCE_P(obj), obj, "socket_",
                                       sizeof("socket_") - 1, 1 TSRMLS_CC);
    socket_->value.lval = sock;
}

static inline
int get_socket(zval *obj, int sock TSRMLS_DC) {
    zval *socket_ = zend_read_property(Z_OBJCE_P(obj), obj, "socket_",
                                       sizeof("socket_") - 1, 1 TSRMLS_CC);
    return socket_->value.lval;
}

static inline
void set_rcv_timeout(zval *obj, int timeout TSRMLS_DC) {
    zval *timeout_ = zend_read_property(Z_OBJCE_P(obj), obj, "rcv_timeout_",
                                        sizeof("rcv_timeout_") - 1, 1 TSRMLS_CC);
    timeout_->value.lval = timeout;
}

static inline
int get_rcv_timeout(zval *obj TSRMLS_DC) {
    zval *timeout_ = zend_read_property(Z_OBJCE_P(obj), obj, "rcv_timeout_",
                                        sizeof("rcv_timeout_") - 1, 1 TSRMLS_CC);
    return timeout_->value.lval;
}

static inline
void set_error(zval *obj, const char* error_msg TSRMLS_DC) {
    zend_update_property_string(makuo_ce, obj, "error", sizeof("error") - 1,
                                error_msg TSRMLS_CC);
}

static inline
void set_options(char* mopt, int mopt_size, zval* options) {
    zval **v;
    if (zend_hash_find(Z_ARRVAL_P(options), RECURSIVE, sizeof(RECURSIVE), (void**)&v) == SUCCESS) {
        convert_to_long_ex(v);
        if (Z_LVAL_PP(v)) {
            strncat(mopt, " -r", mopt_size);
        }
    }
    if (zend_hash_find(Z_ARRVAL_P(options), DRYRUN, sizeof(DRYRUN), (void**)&v) == SUCCESS) {
        convert_to_long_ex(v);
        if (Z_LVAL_PP(v)) {
            strncat(mopt, " -n", mopt_size);
        }
    }
    if (zend_hash_find(Z_ARRVAL_P(options), TARGET_HOST, sizeof(TARGET_HOST), (void**)&v) == SUCCESS &&
        Z_TYPE_PP(v) == IS_STRING) {
        strncat(mopt, " -t ", mopt_size);
        strncat(mopt, Z_STRVAL_PP(v), mopt_size);
    }
}

static void makuo_send(INTERNAL_FUNCTION_PARAMETERS, const char* command) {
    char *path;
    int path_len;
    char buff[kMaxSendBuffSize];
    zval *options = NULL;
    char mopt[256];
    char std_err[kMaxReadBuffSize];

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a!", &path, &path_len, &options) == FAILURE) {
        RETURN_FALSE;
    }

    memset(mopt, 0, sizeof(mopt));
    if (options) {
        set_options(mopt, sizeof(mopt), options);
    }

    /* makuo_send() must be finished before end_tv. */
    struct timeval end_tv;
    gettimeofday(&end_tv, NULL);
    end_tv.tv_sec += get_rcv_timeout(getThis() TSRMLS_CC);

    zval **v;
    if (options && zend_hash_find(Z_ARRVAL_P(options), DELETE, sizeof(DELETE), (void**)&v) == SUCCESS) {
        convert_to_long_ex(v);
        if (Z_LVAL_PP(v)) {
            /* do dsync */

            if (ap_php_snprintf(buff, sizeof(buff), "dsync%s %s\r\n", mopt, path) >= sizeof(buff)) {
                /* too long */
                set_error(getThis(), ERROR_TOOLONGPARAM TSRMLS_CC);
                RETURN_FALSE;
            }

            int dc = do_command(getThis(), buff, NULL, 0, NULL, 0, &end_tv TSRMLS_CC);
            if (dc < 0) {
                set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
                RETURN_FALSE;
            }
        }
    }

    if (ap_php_snprintf(buff, sizeof(buff), "%s%s %s\r\n", command, mopt, path) >= sizeof(buff)) {
        /* too long */
        set_error(getThis(), ERROR_TOOLONGPARAM TSRMLS_CC);
        RETURN_FALSE;
    }

    memset(std_err, 0, sizeof(std_err));
    int dc = do_command(getThis(), buff, NULL, 0, std_err, sizeof(std_err), &end_tv TSRMLS_CC);
    if (dc < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        RETURN_FALSE;
    }

    if (strlen(std_err) != 0) {
        set_error(getThis(), std_err TSRMLS_CC);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* {{{ proto void Makuosan::__construct(array options)
   */
PHP_METHOD(makuo, __construct) {
    zval *options = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &options) == FAILURE) {
        return;
    }

    if (options) {
        zval **v;
        if (zend_hash_find(Z_ARRVAL_P(options), RCV_TIMEOUT, sizeof(RCV_TIMEOUT), (void**)&v) == SUCCESS) {
            convert_to_long_ex(v);
            set_rcv_timeout(getThis(), Z_LVAL_PP(v) TSRMLS_CC);
        }
    }
}
/* }}} */

/* {{{ proto void Makuosan::__destruct()
   */
PHP_METHOD(makuo, __destruct) {
    close_socket_if_not_closed(getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ proto void Makuosan::close()
   */
PHP_METHOD(makuo, close) {
    close_socket_if_not_closed(getThis() TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool Makuosan::send(string path [, array options])
   */
PHP_METHOD(makuo, send) {
    makuo_send(INTERNAL_FUNCTION_PARAM_PASSTHRU, "send");
}
/* }}} */

/* {{{ proto bool Makuosan::sync(string path [, array options])
   */
PHP_METHOD(makuo, sync) {
    makuo_send(INTERNAL_FUNCTION_PARAM_PASSTHRU, "sync");
}
/* }}} */

/* {{{ proto mixed Makuosan::status()
   */
PHP_METHOD(makuo, status) {
    zval *ret;
    char buff[kMaxReadBuffSize];

    /* initialize array */
    MAKE_STD_ZVAL(ret);
    array_init(ret);

    /* request status string */
    memset(buff, 0, sizeof(buff));
    int dc = do_command(getThis(), "status\r\n", buff, sizeof(buff), NULL, 0, NULL TSRMLS_CC);
    if (dc < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        RETURN_FALSE;
    }

    /* parse status string and push into array */
    char* saveptr;
    char* p = strtok_r(buff, "\r\n", &saveptr);
    while (p) {
        char* pos = strstr(p, ": ");
        if (pos) {
            *pos = '\0';
            char* key = p;
            char* val = pos + 2;
            add_assoc_string(ret, key, val, 1);
        }
        p = strtok_r(NULL, "\r\n", &saveptr);
    }

    RETURN_ZVAL(ret, 0, 1);
}
/* }}} */

/* {{{ proto mixed Makuosan::members()
   */
PHP_METHOD(makuo, members) {
    zval *ret;
    char buff[kMaxReadBuffSize];
    char address[64];

    /* initialize array */
    MAKE_STD_ZVAL(ret);
    array_init(ret);

    /* request status string */
    memset(buff, 0, sizeof(buff));
    int dc = do_command(getThis(), "members\r\n", buff, sizeof(buff), NULL, 0, NULL TSRMLS_CC);
    if (dc < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        RETURN_FALSE;
    }

    /* parse status string and push into array */
    char* saveptr;
    char* p = strtok_r(buff, "\r\n", &saveptr);
    while (p && memcmp(p, "Total:", 6)) {
        sscanf(p, " %*s %s ", address);
        add_next_index_stringl(ret, address, strlen(address), 1);
        p = strtok_r(NULL, "\r\n", &saveptr);
    }

    RETURN_ZVAL(ret, 0, 1);
}
/* }}} */

/* {{{ proto mixed Makuosan::check()
   */
PHP_METHOD(makuo, check) {
    zval *ret;
    char buff[kMaxReadBuffSize];

    /* initialize array */
    MAKE_STD_ZVAL(ret);
    array_init(ret);

    /* request status string */
    memset(buff, 0, sizeof(buff));
    int dc = do_command(getThis(), "check -r\r\n", buff, sizeof(buff), NULL, 0, NULL TSRMLS_CC);
    if (dc < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        RETURN_FALSE;
    }

    /* parse status string and push into array */
    char* saveptr;
    char* p = strtok_r(buff, "\r\n", &saveptr);
    while (p) {
        add_next_index_stringl(ret, p, strlen(p), 1);
        p = strtok_r(NULL, "\r\n", &saveptr);
    }

    RETURN_ZVAL(ret, 0, 1);
}
/* }}} */

/* {{{ proto bool Makuosan::exclude_add(array paths)
   */
PHP_METHOD(makuo, exclude_add) {
    zval *paths = NULL;
    char buff[kMaxSendBuffSize];

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &paths) == FAILURE) {
        RETURN_FALSE;
    }

    if (paths) {
        const HashTable* ht = Z_ARRVAL_P(paths);
        const Bucket* p = ht->pListHead;
        for (; p; p = p->pListNext) {
            const zval* z = *((zval**)p->pData);
            if (Z_TYPE_P(z) != IS_STRING) continue;
            if (ap_php_snprintf(buff, sizeof(buff), "exclude add %s\r\n", Z_STRVAL_P(z)) >= sizeof(buff)) {
                set_error(getThis(), ERROR_TOOLONGPARAM TSRMLS_CC);
                RETURN_FALSE;
            }
            int dc = do_command(getThis(), buff, NULL, 0, NULL, 0, NULL TSRMLS_CC);
            if (dc < 0) {
                set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
                RETURN_FALSE;
            }
        }
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Makuosan::exclude_del(string path)
   */
PHP_METHOD(makuo, exclude_del) {
    char *path;
    int path_len;
    char buff[kMaxSendBuffSize];

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (ap_php_snprintf(buff, sizeof(buff), "exclude del %s\r\n", path) >= sizeof(buff)) {
        /* too long */
        set_error(getThis(), ERROR_TOOLONGPARAM TSRMLS_CC);
        RETURN_FALSE;
    }

    int dc = do_command(getThis(), buff, NULL, 0, NULL, 0, NULL TSRMLS_CC);
    if (dc < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto mixed Makuosan::exclude_list()
   */
PHP_METHOD(makuo, exclude_list) {
    zval *ret;
    char buff[kMaxReadBuffSize];

    /* initialize array */
    MAKE_STD_ZVAL(ret);
    array_init(ret);

    /* request status string */
    memset(buff, 0, sizeof(buff));
    int dc = do_command(getThis(), "exclude list\r\n", buff, sizeof(buff), NULL, 0, NULL TSRMLS_CC);
    if (dc < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        RETURN_FALSE;
    }

    /* parse status string and push into array */
    char* saveptr;
    char* p = strtok_r(buff, "\r\n", &saveptr);
    while (p) {
        add_next_index_stringl(ret, p, strlen(p), 1);
        p = strtok_r(NULL, "\r\n", &saveptr);
    }

    RETURN_ZVAL(ret, 0, 1);
}
/* }}} */

/* {{{ proto bool Makuosan::exclude_clear()
   */
PHP_METHOD(makuo, exclude_clear) {
    int dc = do_command(getThis(), "exclude clear\r\n", NULL, 0, NULL, 0, NULL TSRMLS_CC);
    if (dc < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool Makuosan::connect_tcp([string host, int port, int timeout])
   */
PHP_METHOD(makuo, connect_tcp) {
    /* default parameters */
    const char *host = "127.0.0.1";
    int host_len = strlen(host);
    int port = 5000;
    int timeout = kDefaultConTimeout;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sll", &host, &host_len, &port, &timeout) == FAILURE) {
        RETURN_FALSE;
    }

    struct timeval connect_end_tv;
    gettimeofday(&connect_end_tv, NULL);
    connect_end_tv.tv_sec += timeout;

    int s = socket(AF_INET, SOCK_STREAM, 0);
    set_socket(getThis(), s TSRMLS_CC);

    if (s == -1) {
        /* socket failed */
        set_error(getThis(), ERROR_SOCKCREATE TSRMLS_CC);
        goto bail;
    }

    /* store original timeout */
    struct timeval org_rcv_tv, org_snd_tv;
    socklen_t rcv_len, snd_len;
    getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (void *)&org_rcv_tv, &rcv_len);
    getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (void *)&org_snd_tv, &snd_len);

    if (set_socket_timeout(s, &connect_end_tv) == -1) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    int c = connect_socket_tcp(s, host, port);
    if (c == -1) {
        /* invalid host */
        set_error(getThis(), ERROR_INVALIDHOST TSRMLS_CC);
        goto bail;
    } else if (c == -2) {
        /* connect failed */
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    if (set_socket_timeout(s, &connect_end_tv) == -1) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    if (wait_prompt(s, NULL, 0, NULL, 0) == -1) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    if (do_command(getThis(), "alive on\r\n", NULL, 0, NULL, 0, &connect_end_tv
                   TSRMLS_CC) < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    /* restore original timeout */
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (void *)&org_rcv_tv, sizeof(org_rcv_tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (void *)&org_snd_tv, sizeof(org_snd_tv));

    RETURN_TRUE;

 bail:
    close_socket_if_not_closed(getThis() TSRMLS_CC);
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool Makuosan::connect_unix(string path [, int timeout])
   */
PHP_METHOD(makuo, connect_unix) {
    /* default parameters */
    char *path;
    int path_len;
    int timeout = kDefaultConTimeout;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &path, &path_len, &timeout) == FAILURE) {
        RETURN_FALSE;
    }

    struct timeval connect_end_tv;
    gettimeofday(&connect_end_tv, NULL);
    connect_end_tv.tv_sec += timeout;

    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    set_socket(getThis(), s TSRMLS_CC);

    if (s == -1) {
        set_error(getThis(), ERROR_SOCKCREATE TSRMLS_CC);
        goto bail;
    }

    /* store original timeout */
    struct timeval org_rcv_tv, org_snd_tv;
    socklen_t rcv_len, snd_len;
    getsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (void *)&org_rcv_tv, &rcv_len);
    getsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (void *)&org_snd_tv, &snd_len);

    if (set_socket_timeout(s, &connect_end_tv) == -1) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    int c = connect_socket_unix(s, path);
    if (c == -1) {
        /* invalid path */
        set_error(getThis(), ERROR_INVALIDHOST TSRMLS_CC);
        goto bail;
    } else if (c == -2) {
        /* connect failed */
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    if (set_socket_timeout(s, &connect_end_tv) == -1) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    if (wait_prompt(s, NULL, 0, NULL, 0) == -1) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    if (do_command(getThis(), "alive on\r\n", NULL, 0, NULL, 0, &connect_end_tv
                   TSRMLS_CC) < 0) {
        set_error(getThis(), ERROR_CONNFAILED TSRMLS_CC);
        goto bail;
    }

    /* restore original timeout */
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (void *)&org_rcv_tv, sizeof(org_rcv_tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (void *)&org_snd_tv, sizeof(org_snd_tv));

    RETURN_TRUE;

 bail:
    close_socket_if_not_closed(getThis() TSRMLS_CC);
    RETURN_FALSE;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
