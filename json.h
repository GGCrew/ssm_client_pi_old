#ifndef INC_SSM_JSON_H
#define INC_SSM_JSON_H


/**/


#include <json/json.h>


/**/


int json_get(const char *server_name, const char *url, char *json_response_text);
void json_parse_string_from_json(const char *json_response_text, const char *key, char *value);
int json_parse_int_from_json(const char *json_response_text, const char *key);
bool json_parse_bool_from_json(const char *json_response_text, const char *key);


/**/


#endif  /* INC_SSM_JSON_H */
