#ifndef CE_HTTP_CLIENT_H
#define CE_HTTP_CLIENT_H

#include <Arduino.h>
#include "ce_data_types.h"

/**
 * @brief Send HTTP GET request
 * @return HTTP status code
 */
int http_get(const char *path, char *response, uint16_t max_len);

/**
 * @brief Send HTTP POST with JSON
 * @return HTTP status code
 */
int http_post(const char *path, const char *json_body, char *response, uint16_t max_len);

/**
 * @brief Parse JSON command response
 */
int http_parse_commands(const char *json, Command *commands, int max_count);

#endif
