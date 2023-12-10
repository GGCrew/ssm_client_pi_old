#ifndef INC_OPENGL_ES_H
#define INC_OPENGL_ES_H


/**/


#include "GLES2/gl2.h"
#include "EGL/egl.h"


/**/


#define IMAGE_COUNT 2


/**/


enum program_states {
	NEED_PHOTO,
	HOLD,
	TRANSITION,
	TRANSITION_COMPLETE
};


enum play_states {
	PLAY,
	PAUSE,
	STOP
};


/**/


typedef struct
{
	GLuint glTextureID;
	int width;
	int height;
} TEXTURE_TYPE;


typedef struct
{
	uint32_t screen_width;
	uint32_t screen_height;

	// OpenGL|ES objects
	EGL_DISPMANX_WINDOW_T nativewindow;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;

	// GLSL objects
	GLuint shaderProgram;
	GLuint vertexBuffer;
	GLuint faceBuffer;
	GLuint attr_vertex;
	GLuint unif_alpha;
	GLuint unif_texture;
	GLuint unif_Pmatrix;
	GLuint unif_Vmatrix;
	GLuint unif_Mmatrix;
	GLuint unif_Smatrix;

	// program variables that feed into GLSL
	GLfloat alpha;
	TEXTURE_TYPE textures[IMAGE_COUNT];
	GLfloat projectionMatrix[16];
	GLfloat viewMatrix[16];
	GLfloat movementMatrix[16];
	GLfloat scalingMatrix[16];

	// program variables
	int frameCounter;
	int textureCurrent;
	int hold_duration;
	int transition_duration;
	char transition_type[32];
	enum program_states program_state;
	enum play_states play_state;
} EGL_TYPE;


/**/


void egl_init(EGL_TYPE *egl);
void egl_init_shaders(EGL_TYPE *egl);
void egl_init_model(EGL_TYPE *egl);
void egl_init_matrices(EGL_TYPE *egl);
void egl_render(EGL_TYPE *egl);
void egl_destroy(EGL_TYPE *egl);
void egl_load_texture(TEXTURE_TYPE* texture, char filename[]);
void egl_unload_texture(TEXTURE_TYPE* texture);


/**/


#endif  /* INC_OPENGL_ES_H */
