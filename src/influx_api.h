#ifndef influx_api_h
#define influx_api_h

#ifndef bool
typedef enum { false = 0, true = 1 } bool;
#endif

#define INFLUX_OK 200
#ifdef HAVE_JSON_0
#include <json/json.h>
#else
#include <json-c/json.h>
#endif

#define INFLUX_URL_MAX_SIZE 1024

// useful enumerated types
typedef enum {
  influx_seconds,
  influx_milliseconds,
  influx_microseconds
} influx_time_precision;

typedef enum {
  influx_int,
  influx_double,
  influx_string,
  influx_null
} influx_column_type_t;

// Opaque type definitions
typedef struct influx_result_set influx_result_set;
typedef struct influx influx;

/* init and connect */
int influx_open(const char *host, const char *user, const char *password,
                const char *database, bool ssl, influx **db);

int influx_set_time_precision(influx *db, influx_time_precision precision);

/* close influx connection */
int influx_close(influx *db);

/* raw queries */
int influx_json_from_query(influx *db, const char *query, json_object **body);

/* get collection of series */
int influx_execute_query(influx *db, const char *query, influx_result_set **result);

/* close result set and free memory */
int influx_finalize(influx_result_set *result);

/* methods for getting series information */
size_t influx_series_count(influx_result_set *result);
bool influx_next_series(influx_result_set *result);

size_t influx_series_measurement(influx_result_set *result, char *measurement, size_t max_len);
size_t influx_series_length(influx_result_set *result);
size_t influx_tag_count(influx_result_set *result);
int influx_tag(influx_result_set *result, int tag_idx, char *key, char *value, size_t max_length, size_t *key_length, size_t *value_length);

size_t influx_column_count(influx_result_set *result);
int influx_column_name(influx_result_set *result, int col_idx, char *name, size_t max_length, size_t *name_length);

bool influx_next_row(influx_result_set *result);
influx_column_type_t influx_column_type(influx_result_set *result, int col_idx);

influx_column_type_t influx_column_type(influx_result_set *result, int col_idx);
long influx_row_time(influx_result_set *result);
double influx_row_double_value(influx_result_set *result, int col_index);
int influx_row_int_value(influx_result_set *result, int col_index);
size_t influx_row_char_value(influx_result_set *result, int col_index, char *value, size_t max_len);

int influx_row_get_col(influx_result_set *result, int col_idx, json_object **js);


/*****  writing data  *****/


// if you prefer the raw format:
int influx_send_raw(influx *db, const char *data);




#endif
