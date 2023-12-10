#include <unistd.h> // read(), write(), close()
#include <string.h> // strcpy(), strcat(), bzero(), bcopy()
#include <netdb.h> // socket(), gethostbyname(), htons(), connect()
#include <stdio.h> // fprintf()
#include <errno.h> // EINVAL


#include "json.h"


/**/


int json_get(const char *server_name, const char *url, char *json_response_text)
{
	int sockfd;
	int err_code;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char message[256];

	/**/

	// Attempt to open a socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	{
		// Error handling goes here
		fprintf(stderr, "\tjson_get() -- socket(AF_INET, SOCK_STREAM, 0)\n");
		fprintf(stderr, "\t\tError!!!\n");
		strcpy(json_response_text, "\0");
		return -1;
	}

	// Perform a DNS lookup and get IP address
	//server = gethostbyname("net-night.com");
	server = gethostbyname(server_name);
	if (server == NULL) {
		// Error handling goes here
		fprintf(stderr, "\tjson_get() -- gethostbyname(\"%s\")\n", server_name);
		fprintf(stderr, "\t\tError!!!\n");
		strcpy(json_response_text, "\0");
		return -1;
	}

	/********** connect to the server **********/
	// initialize the socket data structure
	bzero((char *) &serv_addr, sizeof(serv_addr)); // wipe it clean!
	bcopy((char *)server->h_addr, // Copy server address into the structure.  Joy of strings in C...
		 (char *)&serv_addr.sin_addr.s_addr,
		 server->h_length);
	serv_addr.sin_family = AF_INET; // Specify socket type (this is the only option, currently)
	serv_addr.sin_port = htons(80); // Copy port # into the structure.

	// Attempt to connect to the server
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{
		// Error handling goes here
		//fprintf(stderr, "\tjson_get() -- connect(\"%s\")\n", server_name);
		//fprintf(stderr, "\t\tError!!!\n");
		strcpy(json_response_text, "\0");
		return -1;
	}

	// Send user input to server
	strcpy(message, "GET ");
	strcat(message, url);
	strcat(message, "\r\n");
	err_code = write(sockfd, message, strlen(message));
	if (err_code < 0)
	{
		// Error handling goes here
		fprintf(stderr, "\tjson_get() -- write()\n");
		fprintf(stderr, "\t\tError!!!\n");
		fprintf(stderr, "\t\terr_code: %u\n", err_code);
		fprintf(stderr, "\t\tmessage: %s\n", message);
		strcpy(json_response_text, "\0");
		return -1;
	}

	// Read response from server
	bzero(json_response_text, 512);
	err_code = read(sockfd, json_response_text, 512 - 1);
	if (err_code < 0) 
	{
		// Error handling goes here
		fprintf(stderr, "\tjson_get() -- read()\n");
		fprintf(stderr, "\t\tError!!!\n");
		fprintf(stderr, "\t\terr_code: %u\n", err_code);
		fprintf(stderr, "\t\tjson_response_text: %s\n", json_response_text);
		strcpy(json_response_text, "\0");
		return -1;
	}

	// Clean up
	//fprintf(stderr, "\tjson_get() -- close()\n");
	close(sockfd);
	
	return 0;
}


void json_parse_string_from_json(const char *json_response_text, const char *key, char *value)
{
	json_object *json_response;
	const char *local_value;

	/**/

	// Convert the JSON response text into a JSON object
	json_response = json_tokener_parse(json_response_text);

	// Check if json_response is valid (or a NULL pointer)
	if(json_response)
	{
		// Get a pointer to the requested data
		local_value = json_object_get_string(json_object_object_get(json_response, key));

		// Check if local_value is a NULL pointer
		if(local_value)
		{
			// Copy the requested data into the returned variable
			strcpy(value, local_value);
		}
		else
		{
			// Error handling
			fprintf(stderr, "json_parse_string_from_json -- NULL value for %s\n", key);
			strcpy(value, "");
		}

		json_object_put(json_response);	// attempt to clean up after ourselves
	}
	else
	{
		// Error handling
		fprintf(stderr, "json_parse_string_from_json -- NULL value for json_tokener_parse()\n");
		strcpy(value, "");
	}
}


int json_parse_int_from_json(const char *json_response_text, const char *key)
{
	json_object *json_response;
	int value;

	/**/

	// Convert the JSON response text into a JSON object
	json_response = json_tokener_parse(json_response_text);

	// Check if json_response is valid (or a NULL pointer)
	if(json_response)
	{
		// Copy the requested data into the returned variable
		value = json_object_get_int(json_object_object_get(json_response, key));
		if(errno == EINVAL)
		{
			// Error handling
			fprintf(stderr, "json_parse_int_from_json -- INVALID value for %s\n", key);
			value = -1;
		}

		json_object_put(json_response);	// attempt to clean up after ourselves
	}
	else
	{
		// Error handling
		fprintf(stderr, "json_parse_int_from_json -- NULL value for json_tokener_parse()\n");
		value = -1;
	}

	return value;
}


bool json_parse_bool_from_json(const char *json_response_text, const char *key)
{
	json_object *json_response;
	bool value;

	/**/

	// Convert the JSON response text into a JSON object
	json_response = json_tokener_parse(json_response_text);

	// Check if json_response is valid (or a NULL pointer)
	if(json_response)
	{
		// Copy the requested data into the returned variable
		value = json_object_get_boolean(json_object_object_get(json_response, key));
		if(errno == EINVAL)
		{
			// Error handling
			fprintf(stderr, "json_parse_bool_from_json -- INVALID value for %s\n", key);
			value = false;
		}

		json_object_put(json_response);	// attempt to clean up after ourselves
	}
	else
	{
		// Error handling
		fprintf(stderr, "json_parse_bool_from_json -- NULL value for json_tokener_parse()\n");
		value = false;
	}

	return value;
}
