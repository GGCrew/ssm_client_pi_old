#include <string.h>
#include <unistd.h> // access(), fork(), execl()
#include <sys/wait.h> // waitpid()

#include <stdio.h> // printf()

#include "ssm_client.h"
#include "json.h"


/**/


// Declarations for private functions
void download_file(const char *protocol_prefix, const char *server_name, const char *server_path, char *local_path);
void delete_downloaded_files();


/**/


void ssm_init()
{
	delete_downloaded_files();
}


void get_next_photo(EGL_TYPE *egl, const char *server_name)
{
	char json_response_text[512];
	char full_path[512];
	char local_path[512];
	char transition_type[512];

	int hold_duration;
	int transition_duration;

	int textureIndex;

	/**/

	json_get(server_name, "/photos/next.json", json_response_text);
	json_parse_string_from_json(json_response_text, "full_path", full_path);
	json_parse_string_from_json(json_response_text, "transition_type", transition_type);
	hold_duration = json_parse_int_from_json(json_response_text, "hold_duration");
	transition_duration = json_parse_int_from_json(json_response_text, "transition_duration");

	if(full_path[0] == '\0')
		strcpy(local_path, DEFAULT_IMAGE_PATH);  // Error handling
	else
		download_file("http://", server_name, full_path, local_path);

	if(transition_type[0] == '\0')
		strcpy(transition_type, DEFAULT_TRANSITION_TYPE);	// Error handling

	// Set transition_duration for current-into-next transition_duration
	if(transition_duration > 0)
		egl->transition_duration = transition_duration;

	if(hold_duration > 0)
		egl->hold_duration = hold_duration;

	textureIndex = (egl->textureCurrent + 1) % IMAGE_COUNT;

	egl_load_texture(&egl->textures[textureIndex], local_path);
//	egl->hold_durations[textureIndex] = hold_duration;
}


void get_play_state(EGL_TYPE *egl, const char *server_name)
{
	char json_response_text[512];
	char play_state[512];

	/**/

	json_get(server_name, "/controls/state.json", json_response_text);
	json_parse_string_from_json(json_response_text, "play_state", play_state);

	if(play_state[0] == '\0')
		strcpy(play_state, DEFAULT_PLAY_STATE);	// Error handling

	if(strcmp(play_state, "play") == 0) egl->play_state = PLAY;
	else if(strcmp(play_state, "pause") == 0) egl->play_state = PAUSE;
	else if(strcmp(play_state, "stop") == 0) egl->play_state = STOP;
}


void download_file(const char *protocol_prefix, const char *server_name, const char *server_path, char *local_path)
{
	char url[512];

	pid_t pid;
	int pid_status;

	/**/

	// Convert "full_path" from an absolute address to a local address
	strcpy(local_path, ".");
	strcat(local_path, server_path);

	// Check if local file path exists
	if( access( local_path, F_OK ) != -1 )
	{
		// local copy exists!  Nothing else to do.
	}
	else
	{
		// No local copy!  Let's get one.
		strcpy(url, protocol_prefix);
		strcat(url, server_name);
		strcat(url, server_path);

		// http://www.gnu.org/software/libc/manual/html_node/Process-Creation-Example.html
		pid = fork();
		if(pid == 0)
		{
			// We're in the child process!
			// Execute the command...
			execl("/usr/bin/wget", "/usr/bin/wget", url, "--no-host-directories", "--force-directories", "--no-verbose", NULL);

			// The following line will only run if the executed command fails.
			_exit(EXIT_FAILURE);
		}
		else if(pid < 0)
		{
			// The fork() failed.
		}
		else
		{
			// We're still in the parent process
			// Stall until the child process finishes...
			if(waitpid(pid, &pid_status, 0) != pid)
			{
				// Something went wrong with the child process
			}
		}
	}
}


void delete_downloaded_files()
{
	pid_t pid;
	int pid_status;

	/**/
	
	pid = fork();
	if(pid == 0)
	{
		// We're in the child process!
		// Execute the command...
		//execl("/bin/rm", "/bin/rm", "--recursive", "--force", "--verbose", "./photos", NULL);
		fprintf(stdout, "Deleting existing photos...\n");
		execl("/bin/rm", "/bin/rm", "--recursive", "--force", "./photos", NULL);

		// The following line will only run if the executed command fails.
		_exit(EXIT_FAILURE);
	}
	else if(pid < 0)
	{
		// The fork() failed.
	}
	else
	{
		// We're still in the parent process
		// Stall until the child process finishes...
		if(waitpid(pid, &pid_status, 0) != pid)
		{
			// Something went wrong with the child process
		}
		fprintf(stdout, "Deleting existing photos - DONE!\n");
	}
}


void clock_addtimes(timespec time1, timespec time2, timespec *result)
{
	result->tv_sec = time1.tv_sec + time2.tv_sec;
	result->tv_nsec = time1.tv_nsec + time2.tv_nsec;
	
	while(result->tv_nsec >= 1000000000)
	{
		result->tv_sec += 1;
		result->tv_nsec -= 1000000000;
	}
}


void clock_subtracttimes(timespec time1, timespec time2, timespec *result)
{
	result->tv_sec = time1.tv_sec - time2.tv_sec;
	result->tv_nsec = time1.tv_nsec - time2.tv_nsec;
	
	while(result->tv_nsec < 0)
	{
		result->tv_sec -= 1;
		result->tv_nsec += 1000000000;
	}
}


int clock_cmptimes(timespec time1, timespec time2)
{
	//return an integer less than, equal to, or greater than zero if time1 is found, respectively, to be less than, to match, or be greater than s2. 
	int return_value;

	/**/

	if(time1.tv_sec < time2.tv_sec)
		return_value = -1;
	else if(time1.tv_sec > time2.tv_sec)
		return_value = 1;
	else
		if(time1.tv_nsec < time2.tv_nsec)
			return_value = -1;
		else if(time1.tv_nsec > time2.tv_nsec)
			return_value = 1;
		else
			return_value = 0;

	return return_value;
}