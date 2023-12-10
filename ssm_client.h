#ifndef INC_SSM_CLIENT_H
#define INC_SSM_CLIENT_H


/**/


#include <time.h>	// timespec structure

#include "opengl_es.h"	// for the EGL_TYPE declaration


/**/


#define DEFAULT_IMAGE_PATH "./images/black.png"
#define DEFAULT_PLAY_STATE "play"
#define DEFAULT_TRANSITION_TYPE "dissolve"


/**/


void ssm_init();

void get_next_photo(EGL_TYPE *egl, const char *server_name);
void get_play_state(EGL_TYPE *egl, const char *server_name);

void clock_addtimes(timespec time1, timespec time2, timespec *result);
void clock_subtracttimes(timespec time1, timespec time2, timespec *result);

int clock_cmptimes(timespec time1, timespec time2);


/**/


#endif  /* INC_SSM_CLIENT_H */
