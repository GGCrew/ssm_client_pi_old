#include <string.h>
#include <stdio.h>
#include <ifaddrs.h>
#include <netinet/in.h>	//sockaddr_in, sockaddr_in6, INET_ADDRSTRLEN, INET6_ADDRSTRLEN
#include <arpa/inet.h>	//inet_ntop()

#include "ssm_server_scanner.h"
#include "json.h"


/**/


// Declarations for private functions
void get_local_ip_address(char *ip_address);
void scan_network_for_ssm_server(const char *client_ip_address, char *server_ip_address);
void scan_local_network_for_ssm_server(char *server_ip_address);


/**/


char * scan_for_ssm_server()
{
	// Declaring as static so the data continues to exist outside of the function
	static char server_ip_address[256];

	/**/

	scan_local_network_for_ssm_server(server_ip_address);
	//fprintf(stderr, "\tscan_local_network_for_ssm_server(server_ip_address): %s\n", server_ip_address);

	return server_ip_address;
}


void scan_local_network_for_ssm_server(char *server_ip_address)
{
	char machine_ip_address[256];

	/**/

	get_local_ip_address(machine_ip_address);
	scan_network_for_ssm_server(machine_ip_address, server_ip_address);	
}


void get_local_ip_address(char *ip_address)
{
	struct ifaddrs * ifAddrStruct=NULL;
	struct ifaddrs * ifa=NULL;
	void * tmpAddrPtr=NULL;

	/**/

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		// Skip rest of loop if no valid IP address
		if(!ifa->ifa_addr) continue;

		// Skip rest of loop if "lo" adapter name
		if(strcmp(ifa->ifa_name, "lo") == 0) continue;

		// Currently only care about IP4 addresses
		if (ifa->ifa_addr->sa_family == AF_INET)	// check it is IP4
		{
			// is a valid IP4 Address
			tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			inet_ntop(AF_INET, tmpAddrPtr, ip_address, INET_ADDRSTRLEN);
		}
	}

	if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}


void scan_network_for_ssm_server(const char *client_ip_address, char *server_ip_address)
{
	char local_ip_address[256];
	char scan_ip_address[256];
	char json_response_text[512];
	char message_type[512];
	char *ip[4];

	int counter;
	int errno;
	int ip_value;

	bool server_found;

	/**/

	// Parse incoming ip_address
	// Make a copy of the incoming ip_address so strtok doesn't mess with it
	strcpy(local_ip_address, client_ip_address);
	ip[0] = strtok(local_ip_address, ".");
	ip[1] = strtok(NULL, ".");
	ip[2] = strtok(NULL, ".");
	ip[3] = strtok(NULL, ".");
	//fprintf(stderr, "\tParsed IP Address: %s.%s.%s.%s\n", ip[0], ip[1], ip[2], ip[3]);
	
	// optional: guess starting point based on ddd value (eg 104 implies addresses start at 100 instead of 1)
	ip_value = atoi(ip[3]);
	ip_value = ((ip_value / 25) * 25);
	//ip_value++;
	if(ip_value == 0) ip_value++;
	
	// Loop through network IP addresses aaa.bbb.ccc.1-254 until we find valid server response
	server_found = false;
	for(counter = ip_value; counter < 255; counter++)
	{
		snprintf(ip[3], sizeof(ip[3]), "%d", counter);
		snprintf(scan_ip_address, sizeof(scan_ip_address), "%s.%s.%s.%s", ip[0], ip[1], ip[2], ip[3]);
		fprintf(stderr, "\tScanning IP Address: %s...\n", scan_ip_address);
		errno = json_get(scan_ip_address, "/controls/state.json", json_response_text);
		if(errno >= 0)
		{
			fprintf(stderr, "\t\tHit -- Checking response...\n");
			//{"message_type":"control","play_state":"play"}
			json_parse_string_from_json(json_response_text, "message_type", message_type);
			if(strcmp(message_type, "control") == 0)
			{
				fprintf(stderr, "\t\tResponse from %s is valid!\n", scan_ip_address);
				//snprintf(server_ip_address, sizeof(server_ip_address), "%s", scan_ip_address);
				sprintf(server_ip_address, "%s", scan_ip_address);
				server_found = true;
				break;
			}
		}
	}

	if(!server_found)
	{
		// Check the rest of the local network
		// (This loop only runs if machine_ip_address > aaa.bbb.ccc.ddd.25)
		for(counter = 1; counter < ip_value; counter++)
		{
			snprintf(ip[3], sizeof(ip[3]), "%d", counter);
			snprintf(scan_ip_address, sizeof(scan_ip_address), "%s.%s.%s.%s", ip[0], ip[1], ip[2], ip[3]);
			fprintf(stderr, "\tScanning IP Address: %s...\n", scan_ip_address);
			errno = json_get(scan_ip_address, "/controls/state.json", json_response_text);
			if(errno >= 0)
			{
				fprintf(stderr, "\t\tHit -- Checking response...\n");
				//{"message_type":"control","play_state":"play"}
				json_parse_string_from_json(json_response_text, "message_type", message_type);
				if(strcmp(message_type, "control") == 0)
				{
					fprintf(stderr, "\t\tResponse from %s is valid!\n", scan_ip_address);
					//snprintf(server_ip_address, sizeof(server_ip_address), "%s", scan_ip_address);
					sprintf(server_ip_address, "%s", scan_ip_address);
					server_found = true;
					break;
				}
			}
		}
	}
}


void display_ip_addresses()
{
	// Code from: http://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine

	struct ifaddrs * ifAddrStruct=NULL;
	struct ifaddrs * ifa=NULL;
	void * tmpAddrPtr=NULL;

	/**/

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if(!ifa->ifa_addr) {
			continue;
		}

		if (ifa->ifa_addr->sa_family == AF_INET)	// check it is IP4
		{
			// is a valid IP4 Address
			tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
		}
		else if (ifa->ifa_addr->sa_family == AF_INET6) // check it is IP6
		{
			// is a valid IP6 Address
			tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
			char addressBuffer[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
		} 
	}

	if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}