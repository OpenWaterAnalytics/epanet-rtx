/*
 * Copyright (c) 2013-2014 InfluxDB
 * Pierre-Olivier Mercier <pomercier@novaquark.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

#include <curl/curl.h>
#include <string.h>

#include "influx_api.h"

#pragma mark - Private Forward Declarations

// non-opaque definition of main db struct
struct influx {
  char *schema;
  char *host;
  char *username;
  char *password;
  char *database;
  char ssl;
  influx_time_precision precision;
  char _err[1024];
};

struct influx_result_set {
  json_object *_js;
  int _row;
  int _series_idx;
  char _err[1024];
};


/**
 * Perform a DELETE request to API
 *
 * \param client A initialized client
 * \param path Relative path from API root
 * \param body JSON sent as request content, can be NULL
 * \return HTTP status code or CURLcode (if < 100)
 */
//int influx_client_delete(influx *client, char *path, json_object *body);

/**
 * Perform a GET request to API
 *
 * \param client A initialized client
 * \param path Relative path from API root
 * \param res Parsed JSON response, can be NULL
 * \return HTTP status code or CURLcode (if < 100)
 */
//int influx_client_get(influx *client, char *path, json_object **res);

/**
 * Perform a POST request to API
 *
 * \param client A initialized client
 * \param path Relative path from API root
 * \param body JSON sent as request content, can be NULL
 * \param res Parsed JSON response, can be NULL
 * \return HTTP status code or CURLcode (if < 100)
 */
//int influx_client_post(influx *client, char *path, json_object *body, json_object **res);

//typedef void *(*influx_client_object_extract)(json_object *obj);

/**
 * Return a list of elements
 *
 * \param client A initialized client
 * \param path Relative path from API root
 * \param db_list The non-malloced list where store response, can be NULL (just
 *count)
 * \return The list size
 */
//size_t influx_client_list_something(influx *client, char *path, void ***list, influx_client_object_extract extractor);



//int influx_query_get(influx *client, char *path, json_object **res);
size_t influx_get_url(influx *db, const char *endpoint, const char *query, char *url_out);











#pragma mark - Implementation

int influx_open(const char *host, const char *user, const char *password,
                const char *database, bool ssl, influx **db) {

  influx *c = calloc(1, sizeof(influx));

  c->schema = ssl ? "https" : "http";
  c->host = strdup(host);
  c->username = strdup(user);
  c->password = strdup(password);
  c->database = curl_easy_escape(NULL, database, 0);
  c->ssl = ssl;
  c->precision = influx_seconds;
  *db = c;
  return INFLUX_OK;
}

int influx_close(influx *db) {
  //free(db->schema);
  free(db->host);
  free(db->username);
  free(db->password);
  curl_free(db->database);
  free(db);
  return INFLUX_OK;
}

int influx_set_time_precision(influx *db, influx_time_precision precision) {
  db->precision = precision;
  return INFLUX_OK;
}


/***
 escape the query, send to the selected database (with set time precision), and pass back the json response
 */

int influx_json_from_query(influx *db, const char *query, json_object **body) {
  
  char url[INFLUX_URL_MAX_SIZE];
  char *buffer = calloc(1, sizeof(char));
  
  size_t len = influx_get_url(db, "query", query, url);
  int status = influx_client_curl(url, NULL, NULL, &buffer);
  
  json_object *res;
  
  if (status >= 200 && status < 300 && res != NULL)
    res = json_tokener_parse(buffer);
  
  *body = res;
  return status;
}



int influx_execute_query(influx *db, const char *query, influx_result_set **result) {
  int err = INFLUX_OK;
  
  influx_result_set *r;
  r = calloc(1, sizeof(influx_result_set));
  r->_js = NULL;
  r->_row = -1;
  r->_series_idx = -1;
  
  json_object *jo;
  err = influx_json_from_query(db, query, &jo);
  
  if (err != INFLUX_OK) {
    return err;
  }
  
  // drill down to the series array
  json_object *res_json = json_object_object_get(jo, "results");
  
  if (result != NULL) {
    r->_js = res_json;
    *result = r;
  }
  
  
  return INFLUX_OK;
}


int influx_finalize(influx_result_set *result) {
  result->_js = NULL;
  result->_row = -1;
  return INFLUX_OK;
}


size_t influx_series_count(influx_result_set *result) {
  // check that there is something there.
  if (result->_js == NULL) {
    return 0;
  }
  // results is an array.
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  if (series_arr_js == NULL) {
    return 0;
  }
  int n_series = json_object_array_length(series_arr_js);
  return (size_t)n_series;
}


bool influx_next_series(influx_result_set *result) {
  
  size_t n_series = influx_series_count(result);
  
  if (result->_series_idx + 1 >= n_series) {
    sprintf(result->_err, "No more series");
    return false;
  }
  else {
    result->_series_idx++;
    result->_row = -1;
    return true;
  }
}


size_t influx_series_measurement(influx_result_set *result, char *measurement, size_t max_len) {
  if (max_len == 0 || result->_series_idx < 0) {
    sprintf(result->_err, "maxlen is zero, or must call influx_next_series()");
    return 0;
  }
  
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, result->_series_idx);
  json_object *name = json_object_object_get(series_js, "name");
  
  strncpy(measurement, json_object_get_string(name), max_len);
  return strlen(measurement);
}


size_t influx_series_length(influx_result_set *result) {
  
  if (result->_series_idx < 0) {
    sprintf(result->_err, "must call influx_next_series()");
    return 0;
  }
  
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, result->_series_idx);
  
  json_object *values = json_object_object_get(series_js, "values");
  int n_vals = json_object_array_length(values);
  
  return (size_t)n_vals;
}


size_t influx_tag_count(influx_result_set *result) {
  if (result->_series_idx < 0) {
    sprintf(result->_err, "must call influx_next_series()");
    return 0;
  }
  
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, result->_series_idx);
  
  json_object *tags_js = json_object_object_get(series_js, "tags");
  int n_tags = json_object_object_length(tags_js);
  
  return (size_t)n_tags;
}

int influx_tag(influx_result_set *result, int tag_idx, char *out_key, char *out_value, size_t max_length, size_t *key_length, size_t *value_length) {
  // allocate space for the tag keys and values
  size_t n_tags = influx_tag_count(result);
  if (tag_idx >= n_tags) {
    sprintf(result->_err, "tag index out-of-bounds");
    return 404;
  }
  
  // iterate through the list and copy tag key/values
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, result->_series_idx);
  json_object *tags_js = json_object_object_get(series_js, "tags");
  
  int i_tag = 0;
  json_object_object_foreach(tags_js, key, val) {
    if (i_tag == tag_idx) {
      // val will always be a string?
      const char *str_val = json_object_get_string(val);
      snprintf(out_key, max_length, "%s", key);
      snprintf(out_value, max_length, "%s", str_val);
    }
    ++i_tag;
  }
  return INFLUX_OK;
}

size_t influx_column_count(influx_result_set *result) {
  
  // check if we are even looking at a series.
  if (result->_series_idx < 0) {
    sprintf(result->_err, "must call influx_next_series()");
    return false;
  }
  
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, result->_series_idx);
  json_object *cols_js = json_object_object_get(series_js, "columns");
  int n_cols = json_object_array_length(cols_js) - 1; // not considering time to be a first-class "column"
  
  return (size_t)n_cols;
}

int influx_column_name(influx_result_set *result, int col_idx, char *name, size_t max_length, size_t *name_length) {
  // check if we are even looking at a series.
  if (result->_series_idx < 0) {
    sprintf(result->_err, "must call influx_next_series()");
    return 404;
  }
  
  if (col_idx >= influx_column_count(result)) {
    sprintf(result->_err, "column index out of bounds");
    return 404;
  }
  
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, result->_series_idx);
  json_object *cols_js = json_object_object_get(series_js, "columns");
  json_object *name_js = json_object_array_get_idx(cols_js, col_idx + 1);
  
  snprintf(name, max_length, "%s", json_object_get_string(name_js));
  return INFLUX_OK;
}


bool influx_next_row(influx_result_set *result) {
  
  // check if we are even looking at a series.
  if (result->_series_idx < 0) {
    sprintf(result->_err, "must call influx_next_series()");
    return false;
  }
  
  size_t n_rows = influx_series_length(result);
  if (result->_row + 1 >= n_rows) {
    sprintf(result->_err, "no more rows");
    return false;
  }
  else {
    result->_row++;
    return true;
  }
}


influx_column_type_t influx_column_type(influx_result_set *result, int col_idx) {
  if (result->_series_idx < 0 || result->_row < 0) {
    sprintf(result->_err, "invalid state: must call influx_next_series() and influx_next_row()");
    return 0;
  }
  
  if (col_idx >= influx_column_count(result)) {
    sprintf(result->_err, "column out of bounds");
    return 0;
  }
  
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, result->_series_idx);
  json_object *values_js = json_object_object_get(series_js, "values");
  json_object *row_js = json_object_array_get_idx(values_js, result->_row);
  json_object *val_js = json_object_array_get_idx(row_js, col_idx + 1);
  json_type type_js = json_object_get_type(val_js);
  
  switch (type_js) {
    case json_type_double:
      return influx_double;
    case json_type_string:
      return influx_string;
    case json_type_int:
    case json_type_boolean:
      return influx_int;
    default:
      return influx_null;
      break;
  }
  
  return influx_double;
}


long influx_row_time(influx_result_set *result) {
  if (result->_series_idx < 0 || result->_row < 0) {
    sprintf(result->_err, "invalid state: must call influx_next_series() and influx_next_row()");
    return 0;
  }
  
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(result->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, result->_series_idx);
  json_object *values_js = json_object_object_get(series_js, "values");
  json_object *row_js = json_object_array_get_idx(values_js, result->_row);
  
  json_object *time_js = json_object_array_get_idx(row_js, 0);
  long time = (long)json_object_get_int64(time_js);
  return time;
}


double influx_row_double_value(influx_result_set *result, int col_index) {
  json_object *js = NULL;
  
  int err = influx_row_get_col(result, col_index, &js);
  if (err != INFLUX_OK) {
    return 404;
  }
  
  double v = json_object_get_double(js);
  return v;
}

int influx_row_int_value(influx_result_set *result, int col_index) {
  json_object *js = NULL;
  
  int err = influx_row_get_col(result, col_index, &js);
  if (err != INFLUX_OK) {
    return 404;
  }
  
  int v = json_object_get_int(js);
  return v;
}


size_t influx_row_char_value(influx_result_set *result, int col_index, char *value, size_t max_len) {
  json_object *js = NULL;
  
  int err = influx_row_get_col(result, col_index, &js);
  if (err != INFLUX_OK) {
    return 404;
  }
  
  const char *str = json_object_get_string(js);
  snprintf(value, max_len, "%s", str);
  
  return strlen(value);
}

int influx_row_get_col(influx_result_set *r, int col_idx, json_object **js) {
  if (r->_series_idx < 0 || r->_row < 0) {
    sprintf(r->_err, "invalid state: must call influx_next_series() and influx_next_row()");
    return 0;
  }
  
  if (col_idx >= influx_column_count(r)) {
    sprintf(r->_err, "column out of bounds");
    return 0;
  }
  
  json_object *series_arr_js = json_object_object_get(json_object_array_get_idx(r->_js, 0), "series");
  json_object *series_js = json_object_array_get_idx(series_arr_js, r->_series_idx);
  json_object *values_js = json_object_object_get(series_js, "values");
  json_object *row_js = json_object_array_get_idx(values_js, r->_row);
  json_object *val_js = json_object_array_get_idx(row_js, col_idx + 1);
  
  *js = val_js;
  if (val_js == NULL) {
    return 404;
  }
  
  return INFLUX_OK;
}

//
//
//int influx_query_get(influx *client, char *path, json_object **res) {
//  int status;
//  char url[INFLUX_URL_MAX_SIZE];
//  char *buffer = calloc(1, sizeof(char));
//  
//  influx_client_get_url(client, &url, INFLUX_URL_MAX_SIZE, path);
//
//  status = influx_client_curl(url, NULL, NULL, &buffer);
//
//  if (status >= 200 && status < 300 && res != NULL)
//    *res = json_tokener_parse(buffer);
//
//  free(buffer);
//
//  return status;
//}



size_t influx_get_url(influx *db, const char *endpoint, const char *query, char *url_out) {
  
  
  
  char *username_enc = curl_easy_escape(NULL, db->username, 0);
  char *password_enc = curl_easy_escape(NULL, db->password, 0);
  
  char *buffer = calloc(INFLUX_URL_MAX_SIZE, sizeof(char));
  
  char *precision_str;
  switch (db->precision) {
    case influx_milliseconds:
      precision_str = "m";     break;
    case influx_microseconds:
      precision_str = "u";     break;
    default:
      precision_str = "s";
  }
  
  
  if (strcmp(endpoint, "query") == 0) {
    // sanitize formatting character. do we trust the user?
    char clean_query[INFLUX_URL_MAX_SIZE];
    strncpy(clean_query, query, INFLUX_URL_MAX_SIZE);
    char *fmt_char;
    fmt_char = strchr(clean_query, '%');
    while (fmt_char != NULL) {
      *fmt_char = '_';
      fmt_char = strchr(clean_query, '%');
    }
    // also url-escape the query.
    char *escaped_query = curl_easy_escape(NULL, clean_query, 0);
    
    snprintf(buffer, INFLUX_URL_MAX_SIZE, "%s://%s/%s?db=%s&u=%s&p=%s&epoch=%s&q=%s", db->schema, db->host, endpoint, db->database, username_enc, password_enc, precision_str, escaped_query);
    
    curl_free(escaped_query);
  }
  else {
    snprintf(buffer, INFLUX_URL_MAX_SIZE, "%s://%s/%s?db=%s&precision=%s&u=%s&p=%s", db->schema, db->host, endpoint, db->database, precision_str, username_enc, password_enc);
  }
  
  strncpy(url_out, buffer, INFLUX_URL_MAX_SIZE);
  free(buffer);
  
  return strlen(buffer);
}









/**
 * Assemble the URL using given client config and parameters
 */
//int influx_client_get_url_with_credential(influx *client, char (*buffer)[],
//                                          size_t size, char *path,
//                                          char *username, char *password) {
//  char *username_enc = curl_easy_escape(NULL, username, 0);
//  char *password_enc = curl_easy_escape(NULL, password, 0);
//
//  (*buffer)[0] = '\0';
//  strncat(*buffer, client->schema, size);
//  strncat(*buffer, "://", size);
//  strncat(*buffer, client->host, size);
//  strncat(*buffer, path, size);
//
//  if (strchr(path, '?'))
//    strncat(*buffer, "&", size);
//  else
//    strncat(*buffer, "?", size);
//
//  strncat(*buffer, "u=", size);
//  strncat(*buffer, username_enc, size);
//  strncat(*buffer, "&p=", size);
//  strncat(*buffer, password_enc, size);
//
//  free(username_enc);
//  free(password_enc);
//
//  return (int)strlen(*buffer);
//}

///**
// * Assemble the URL using only given client config
// */
//int influx_client_get_url(influx *client, char (*buffer)[], size_t size,
//                          char *path) {
//  return influx_client_get_url_with_credential(
//      client, buffer, size, path, client->username, client->password);
//}

/**
 * CURL Callback reading data to userdata buffer
 */
size_t influx_client_callback_write_data(char *buf, size_t size, size_t nmemb,
                                void *userdata) {
  size_t realsize = size * nmemb;
  if (userdata != NULL) {
    char **buffer = userdata;

    *buffer = realloc(*buffer, strlen(*buffer) + realsize + 1);

    strncat(*buffer, buf, realsize);
  }
  return realsize;
}

/**
 * Low level function performing real HTTP request
 */
int influx_client_curl(char *url, char *reqtype, json_object *body, char **response) {
  CURLcode c;
  CURL *handle = curl_easy_init();

  if (reqtype != NULL) {
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, reqtype);
  }
  curl_easy_setopt(handle, CURLOPT_URL, url);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, influx_client_callback_write_data);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, response);
  if (body != NULL) {
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS,
                     json_object_to_json_string(body));
  }

  c = curl_easy_perform(handle);

  if (c == CURLE_OK) {
    long status_code = 0;
    if (curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status_code) ==
        CURLE_OK) {
      c = (CURLcode)status_code;
    }
  }

  curl_easy_cleanup(handle);
  return c;
}

//int influx_client_delete(influx *client, char *path, json_object *body) {
//  char url[INFLUX_URL_MAX_SIZE];
//
//  influx_client_get_url(client, &url, INFLUX_URL_MAX_SIZE, path);
//
//  return influx_client_curl(url, "DELETE", body, NULL);
//}

//int influx_client_get(influx *client, char *path, json_object **res) {
//  int status;
//  char url[INFLUX_URL_MAX_SIZE];
//  char *buffer = calloc(1, sizeof(char));
//
//  influx_client_get_url(client, &url, INFLUX_URL_MAX_SIZE, path);
//
//  status = influx_client_curl(url, NULL, NULL, &buffer);
//
//  if (status >= 200 && status < 300 && res != NULL)
//    *res = json_tokener_parse(buffer);
//
//  free(buffer);
//
//  return status;
//}
//
//int influx_client_post(influx *client, char *path, json_object *body,
//                       json_object **res) {
//  int status;
//  char url[INFLUX_URL_MAX_SIZE];
//  char *buffer = calloc(1, sizeof(char));
//
//  influx_client_get_url(client, &url, INFLUX_URL_MAX_SIZE, path);
//
//  status = influx_client_curl(url, NULL, body, &buffer);
//
//  if (status >= 200 && status < 300 && res != NULL)
//    *res = json_tokener_parse(buffer);
//
//  free(buffer);
//
//  return status;
//}
//

/*
size_t influx_client_list_something(influx *client, char *path, void ***list,
                                    influx_client_object_extract extractor) {
  json_object *jo = NULL;
  size_t len, i;

  if (influx_client_get(client, path, &jo) != 200)
    return 0;

  len = json_object_array_length(jo);

  if (list != NULL) {
    *list = malloc(sizeof(char *) * (len + 1));

    for (i = 0; i < len; i++) {
      (*list)[i] = extractor(json_object_array_get_idx(jo, (int)i));
    }
  }

  json_object_put(jo);

  return len;
}
*/





int influx_send_raw(influx *db, const char *data) {
  
  int c;
  CURL *handle = curl_easy_init();
  char *response = calloc(1, sizeof(char));
  char url[INFLUX_URL_MAX_SIZE];
  
  influx_get_url(db, "write", NULL, url);
  
  curl_easy_setopt(handle, CURLOPT_URL, url);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, influx_client_callback_write_data);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);
  if (data != NULL) {
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data);
  }
  
  c = curl_easy_perform(handle);
  
  if (c == CURLE_OK) {
    long status_code = 0;
    if (curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status_code) ==
        CURLE_OK) {
      c = (CURLcode)status_code;
    }
  }
  
  curl_easy_cleanup(handle);
  
  if (c != INFLUX_OK) {
    snprintf(db->_err, 1024, "%s", response);
  }
  
  return c;
  
}







