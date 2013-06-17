/**
 * VStats
 *
 * Author: Mark Bowker
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_globals.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_vstats.h"
#include <uuid/uuid.h>

/* Module Static Globals */
#define VSTATS_SOCKET_SOURCE "127.0.0.1"
#define VSTATS_SOCKET_PORT 14092
#define VSTATS_PACKET_MALLOC 256

ZEND_DECLARE_MODULE_GLOBALS(vstats)

/* Module Standard Functions */
static int vstats_send_packet(char *label, zval *message TSRMLS_DC);
static char *generate_message_string(zval *arr_message TSRMLS_DC);
static char *url_encode(char *str);
static char from_hex(char ch);
static char to_hex(char code);
static char *generate_uuid();

/* Module PHP Functions */
zend_function_entry vstats_functions[] = {
  PHP_FE(vstats_enabled,	NULL)
  PHP_FE(vstats_send,	NULL)
  PHP_FE(vstats_set_prefix,	NULL)
  {NULL, NULL, NULL}
};

/* Module Details */
zend_module_entry vstats_module_entry = {
  STANDARD_MODULE_HEADER,
  "vstats",
  vstats_functions,
  PHP_MINIT(vstats),
  PHP_MSHUTDOWN(vstats),
  PHP_RINIT(vstats),
  PHP_RSHUTDOWN(vstats),
  PHP_MINFO(vstats),
  NO_VERSION_YET,
  PHP_MODULE_GLOBALS(vstats),
  STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_VSTATS
ZEND_GET_MODULE(vstats)
#endif

/* Module php.ini Entries */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("vstats.default_prefix", "", PHP_INI_ALL, OnUpdateString, default_prefix, zend_vstats_globals, vstats_globals)
    STD_PHP_INI_ENTRY("vstats.send_request_uuid", "1", PHP_INI_ALL, OnUpdateBool, send_request_uuid, zend_vstats_globals, vstats_globals)
    STD_PHP_INI_ENTRY("vstats.send_request_peak_mem_usage", "1", PHP_INI_ALL, OnUpdateBool, send_request_peak_mem_usage, zend_vstats_globals, vstats_globals)
    STD_PHP_INI_ENTRY("vstats.send_request_time", "1", PHP_INI_ALL, OnUpdateBool, send_request_time, zend_vstats_globals, vstats_globals)
PHP_INI_END()

static void php_vstats_init_globals(zend_vstats_globals *vstats_globals)
{
  vstats_globals->default_prefix = '\0';
  vstats_globals->send_request_uuid = TRUE;
  vstats_globals->send_request_peak_mem_usage = TRUE;
  vstats_globals->send_request_time = TRUE;
}

/**
 * Module Startup
 *
 * Setup the socket for sending data later on.
 */
PHP_MINIT_FUNCTION(vstats)
{
  REGISTER_INI_ENTRIES( );

  /* Shortcuts */
  int *vstats_enabled = (void *) &VSTATS_G(vstats_enabled);
  int *socket_fd = (void *) &VSTATS_G(socket_fd);
  struct sockaddr_in *remote = &VSTATS_G(remote);

  *socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (*socket_fd == -1)
  {
    *vstats_enabled = FALSE;
    return SUCCESS;
  }

  remote->sin_family = AF_INET;
  remote->sin_addr.s_addr = inet_addr(VSTATS_SOCKET_SOURCE);
  remote->sin_port = htons(VSTATS_SOCKET_PORT);

  *vstats_enabled = TRUE;
  return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(vstats)
{
  UNREGISTER_INI_ENTRIES( );

  return SUCCESS;
}

/**
 * Request Startup
 */
PHP_RINIT_FUNCTION(vstats)
{
  gettimeofday(&VSTATS_G(request_start_time), NULL);

  if (strlen(INI_STR("vstats.default_prefix")) > 0)
  {
    VSTATS_G(default_prefix) = estrdup(INI_STR("vstats.default_prefix")); // if changed to realloc, remember to include 1 extra for nul-termination
  }
  else
  {
    VSTATS_G(default_prefix) = estrdup("");
  }

  // Request UUID
  char buf[37];
  uuid_t out;
  uuid_generate(out);
  uuid_unparse_lower(out, buf);
  VSTATS_G(request_uuid) = estrdup(buf);

  return SUCCESS;
}

/**
 * Request Shutdown
 */
PHP_RSHUTDOWN_FUNCTION(vstats)
{
  int send_shutdown_stats = FALSE;
  zval *stats_data;

  // any stats we send during rshutdown
  MAKE_STD_ZVAL(stats_data);
  array_init(stats_data);

  if (VSTATS_G(send_request_peak_mem_usage))
  {
    add_assoc_long(stats_data, "request_peak_mem_usage", zend_memory_peak_usage(FALSE TSRMLS_CC));
    send_shutdown_stats = TRUE;
  }

  if (VSTATS_G(send_request_time))
  {
    struct timeval request_end_time;
    gettimeofday(&request_end_time, NULL);

    // in microseconds
    long long request_time = ((request_end_time.tv_sec-VSTATS_G(request_start_time).tv_sec)*1000000LL)
                           + request_end_time.tv_usec-VSTATS_G(request_start_time).tv_usec;

    add_assoc_long(stats_data, "request_time", request_time);
    send_shutdown_stats = TRUE;
  }

  if (send_shutdown_stats)
  {
    vstats_send_packet("request_stats", stats_data TSRMLS_CC);
  }

  zval_ptr_dtor(&stats_data);
  efree(VSTATS_G(default_prefix));
  efree(VSTATS_G(request_uuid));

  return SUCCESS;
}

/**
 * phpinfo() Entries
 */
PHP_MINFO_FUNCTION(vstats)
{
  php_info_print_table_start();
  php_info_print_table_row(2, "vstats support", VSTATS_G(vstats_enabled) == TRUE ? "enabled" : "disabled");
  php_info_print_table_end();

  DISPLAY_INI_ENTRIES();
}

PHP_FUNCTION(vstats_enabled)
{
  RETURN_BOOL(VSTATS_G(vstats_enabled));
}

/**
 * With the given data, append the current timestamp and label, then send out the data.
 */
PHP_FUNCTION(vstats_send)
{
  zval *data;
  HashTable *arr_ht;
  char *arr_key, *label;
  int arr_key_len, label_len;
  int prefix_len = strlen(VSTATS_G(default_prefix));

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &label, &label_len, &data) == FAILURE)
  {
    RETURN_NULL();
  }

  RETURN_BOOL(vstats_send_packet(label, data TSRMLS_CC));
}

/**
 * Append to or replace the current label prefix.
 */
PHP_FUNCTION(vstats_set_prefix)
{
  char *set_prefix;
  int set_prefix_len;
  zend_bool append_prefix = TRUE;
  int current_len = strlen(VSTATS_G(default_prefix));

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &set_prefix, &set_prefix_len, &append_prefix) == FAILURE)
  {
    RETURN_NULL();
  }

  if (append_prefix && current_len)
  {
    VSTATS_G(default_prefix) = erealloc(VSTATS_G(default_prefix), strlen(VSTATS_G(default_prefix)) + strlen(set_prefix) + 2); // +1 for the . and +1 for nul-termination
    strcat(VSTATS_G(default_prefix), ".");
    strcat(VSTATS_G(default_prefix), set_prefix);
  }
  else
  {
    VSTATS_G(default_prefix) = erealloc(VSTATS_G(default_prefix), strlen(set_prefix) + 1);
    strcpy(VSTATS_G(default_prefix), set_prefix);
  }

  RETURN_TRUE;
}

/**
 * Sends a UDP packet via the established socket with the given data from the zval array.
 */
static int vstats_send_packet(char *label, zval *arr_message TSRMLS_DC)
{
  if (VSTATS_G(vstats_enabled) != TRUE)
  {
    return FALSE;
  }

  char *message;
  int send_success = FALSE;
  HashTable *arr_ht = Z_ARRVAL_P(arr_message);
  int label_len = strlen(label);
  int prefix_len = strlen(VSTATS_G(default_prefix));

  // Add timestamp
  if (zend_hash_exists(arr_ht, "timestamp", strlen("timestamp")) == FALSE)
  {
    add_assoc_long(arr_message, "timestamp", (unsigned) time(NULL));
  }

  // Add label
  if (zend_hash_exists(arr_ht, "label", strlen("label")) == FALSE)
  {
    char *new_label;  // Cannot alter the given 'label' for some reason
    if (prefix_len)
    {
      new_label = emalloc(strlen(VSTATS_G(default_prefix)) + label_len + 2);
      strcpy(new_label, VSTATS_G(default_prefix));
      strcat(new_label, ".");
      strcat(new_label, label);
    }
    else
    {
      new_label = estrdup(label);
    }

    add_assoc_string(arr_message, "label", new_label, TRUE);
    efree(new_label);
  }

  // Add request UUID
  if (VSTATS_G(send_request_uuid) && zend_hash_exists(arr_ht, "request_uuid", strlen("request_uuid")) == FALSE)
  {
    add_assoc_string(arr_message, "request_uuid", VSTATS_G(request_uuid), TRUE);
  }

  message = generate_message_string(arr_message TSRMLS_CC);

  // Send message string
  struct sockaddr_in *remote = &VSTATS_G(remote);
  if (sendto(VSTATS_G(socket_fd), message, strlen(message), 0, (struct sockaddr *) remote, sizeof(*remote)) == -1)
  {
    VSTATS_G(vstats_enabled) = FALSE;  // Stop attempting to send packets.
    send_success = FALSE;
  }
  else
  {
    send_success = TRUE;
  }

  efree(message);

  return send_success;
}

/**
 * Takes the completed arr_message and serializes it to a string ready to be sent.
 */
static char *generate_message_string(zval *arr_message TSRMLS_DC)
{
  HashTable *arr_ht = Z_ARRVAL_P(arr_message);
  HashPosition arr_p;
  zval **data = NULL, arr_val;
  char *arr_key, *key, *val;
  int key_len, val_len;
  long index;
  char *message = emalloc(VSTATS_PACKET_MALLOC);
  int message_buf = VSTATS_PACKET_MALLOC;

  // Ensure a clean string
  message[0] = '\0';

  // Convert array zval to string message
  for (zend_hash_internal_pointer_reset_ex(arr_ht, &arr_p);
       zend_hash_get_current_data_ex(arr_ht, (void**) &data, &arr_p) == SUCCESS;
       zend_hash_move_forward_ex(arr_ht, &arr_p))
  {
    if (zend_hash_get_current_key_ex(arr_ht, &arr_key, &key_len, &index, 0, &arr_p) != HASH_KEY_IS_STRING)
    {
      sprintf(arr_key, "%d", index);
      key_len = strlen(arr_key);
    }

    // Get the string value at this position of the array
    arr_val = **data;
    zval_copy_ctor(&arr_val);
    convert_to_string(&arr_val);

    // Encode components
    key = url_encode(arr_key);
    key_len = strlen(key);
    val = url_encode(Z_STRVAL(arr_val));
    val_len = strlen(val);

    // Check for overflow (+3 for URL encoding glue + zero-termination)
    if (key_len + val_len + strlen(message) + 3 >= message_buf)
    {
      message_buf = message_buf + VSTATS_PACKET_MALLOC + key_len + val_len + 3;
      message = erealloc(message, message_buf);
    }

    // Add components to current message string
    if (strlen(message) != 0)
    {
      strcat(message, "&");
    }
    strcat(message, key);
    strcat(message, "=");
    strcat(message, val);

    // Clean-up
    zval_dtor(&arr_val);
    efree(val);
    efree(key);
  }

  return message;
}

/**
 * Returns a url-encoded version of str
 *
 * Grabbed from: http://www.geekhideout.com/urlcode.shtml (Public Domain)
 */
static char *url_encode(char *str)
{
  char *pstr = str, *buf = emalloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr)
  {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
      *pbuf++ = *pstr;
    else if (*pstr == ' ')
      *pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/**
 * Converts a hex character to its integer value
 *
 * Grabbed from: http://www.geekhideout.com/urlcode.shtml (Public Domain)
 */
static char from_hex(char ch)
{
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/**
 * Converts an integer value to its hex character
 *
 * Grabbed from: http://www.geekhideout.com/urlcode.shtml (Public Domain)
 */
static char to_hex(char code)
{
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

