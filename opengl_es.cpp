#include <assert.h>


#include "bcm_host.h"  // bcm_host_init() and graphics_get_display_size()
#include "opengl_es.h"
#include "SDL_image.h"  // used in egl_load_texture() to easily loading JPGs


/**/


#define check() assert(glGetError() == 0)


/**/


// Declaring private functions
void reset_matrix(GLfloat matrix[]);
void scale_matrix(GLfloat matrix[], long screenWidth, long screenHeight, long textureWidth, long textureHeight);
void egl_render_current(EGL_TYPE *egl);
void egl_render_transition(EGL_TYPE *egl);


/**/


void egl_init(EGL_TYPE *egl)
{
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	const EGLint context_attributes[] = 
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLConfig config;

	/**/
	
	bcm_host_init();  // required for any RPi graphics tasks


	memset(egl, 0, sizeof(*egl) );  // Clear egl of any garbage

	// get an EGL display connection
	egl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(egl->display!=EGL_NO_DISPLAY);

	// initialize the EGL display connection
	result = eglInitialize(egl->display, NULL, NULL);
	assert(EGL_FALSE != result);

	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(egl->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);

	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);

	// create an EGL rendering context
	egl->context = eglCreateContext(egl->display, config, EGL_NO_CONTEXT, context_attributes);
	assert(egl->context!=EGL_NO_CONTEXT);

	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */, &egl->screen_width, &egl->screen_height);
	assert( success >= 0 );

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = egl->screen_width;
	dst_rect.height = egl->screen_height;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = egl->screen_width << 16;
	src_rect.height = egl->screen_height << 16;        

	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );

	dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
		0/*layer*/, &dst_rect, 0/*src*/,
		&src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);

	vc_dispmanx_display_set_background(dispman_update, dispman_display, 0, 0, 0);

	egl->nativewindow.element = dispman_element;
	egl->nativewindow.width = egl->screen_width;
	egl->nativewindow.height = egl->screen_height;
	vc_dispmanx_update_submit_sync( dispman_update );

	egl->surface = eglCreateWindowSurface( egl->display, config, &egl->nativewindow, NULL );
	assert(egl->surface != EGL_NO_SURFACE);

	// connect the context to the surface
	result = eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context);
	assert(EGL_FALSE != result);

	glViewport ( 0, 0, egl->screen_width, egl->screen_height ); check();

	// Set background color and clear buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); check();
	glClear( GL_COLOR_BUFFER_BIT ); check();
}


void egl_init_shaders(EGL_TYPE *egl)
{
   const GLchar *vshader_source =
		"attribute vec2 vertex;"
		"attribute vec2 uv;" // The x,y coordinates of the texture
		"uniform mat4 Pmatrix;" // Creating a projection matrix, which represents the viewable area.  This value will remain constant during render.
		"uniform mat4 Vmatrix;" // Movement matrix for the view (camera)
		"uniform mat4 Mmatrix;" // Movement matrix for the object
		"uniform mat4 Smatrix;" // Scaling matrix for the object
		"varying vec2 vUV;" // Used to give the pixel data to the fragment shader.  This value will change during render
		"void main(void) {"
		" gl_Position = Pmatrix * Vmatrix * Mmatrix * Smatrix * vec4(vertex, 1, 1);"
		" vUV = uv;"
		"}";

   const GLchar *fshader_source =
		"precision mediump float;"  // Specify math precision level.
		"uniform sampler2D texture;" // The texture.
		"uniform float alpha;"
		"varying vec2 vUV;" // Get the pixel coordinates from the vertex shader vUV variable.
		"void main(void) {"
		" vec4 pixel = texture2D(texture, vUV);"
		" gl_FragColor = vec4(pixel.rgb, alpha);"
		"}";

	GLuint vshader;
	GLuint fshader;

	vshader = glCreateShader(GL_VERTEX_SHADER); check();
	glShaderSource(vshader, 1, &vshader_source, 0); check();
	glCompileShader(vshader); check();

	fshader = glCreateShader(GL_FRAGMENT_SHADER); check();
	glShaderSource(fshader, 1, &fshader_source, 0); check();
	glCompileShader(fshader); check();

	egl->shaderProgram = glCreateProgram(); check();
	glAttachShader(egl->shaderProgram, vshader); check();
	glAttachShader(egl->shaderProgram, fshader); check();
	glLinkProgram(egl->shaderProgram); check();

	glUseProgram(egl->shaderProgram); check();
}


void egl_init_model(EGL_TYPE *egl)
{
	GLfloat vertex_data[] = {
		-1.0, -1.0,		0.0, 1.0,
		 1.0, -1.0,		1.0, 1.0,
		 1.0,  1.0,		1.0, 0.0,
		-1.0,  1.0,		0.0, 0.0
	};

	GLushort face_data[] = {
		0, 1, 2,
		0, 2, 3
	};

	// Upload vertex data to a buffer
	glGenBuffers(1, &egl->vertexBuffer); check();
	glBindBuffer(GL_ARRAY_BUFFER, egl->vertexBuffer); check();
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW); check();

	// Upload face data to a buffer
	glGenBuffers(1, &egl->faceBuffer); check();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, egl->faceBuffer); check();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(face_data), face_data, GL_STATIC_DRAW); check();

	// Link and enable GLSL attribute variables
	// glVertexAttribPointer(variable, dimension, type, normalize, total vertex size in bytes, offset in bytes)
	// dimension is the number of data elements for that variable (2 for 2D position, 3 for 3D position, 3 for color_RGB)
	// vertex size is calculated by (coordinates + (color_RGB or texture_UV) ) * size_of_data_type
	// offset is calculated by vertex_data_position * size_of_data_type
	// (GL_FLOAT is 4 bytes)
	egl->attr_vertex = glGetAttribLocation(egl->shaderProgram, "vertex"); check();
	glVertexAttribPointer(egl->attr_vertex, 2, GL_FLOAT, GL_FALSE, (2+2)*sizeof(GL_FLOAT), (void *)(0*sizeof(GL_FLOAT))); check();
	glEnableVertexAttribArray(egl->attr_vertex); check();

	egl->attr_vertex = glGetAttribLocation(egl->shaderProgram, "uv"); check();
	glVertexAttribPointer(egl->attr_vertex, 2, GL_FLOAT, GL_FALSE, (2+2)*sizeof(GL_FLOAT), (void *)(2*sizeof(GL_FLOAT))); check();
	glEnableVertexAttribArray(egl->attr_vertex); check();


	// Link GLSL uniform variables
	egl->unif_Pmatrix = glGetUniformLocation(egl->shaderProgram, "Pmatrix");
	egl->unif_Vmatrix = glGetUniformLocation(egl->shaderProgram, "Vmatrix");
	egl->unif_Mmatrix = glGetUniformLocation(egl->shaderProgram, "Mmatrix");
	egl->unif_Smatrix = glGetUniformLocation(egl->shaderProgram, "Smatrix");
	egl->unif_texture = glGetUniformLocation(egl->shaderProgram, "texture"); check();
	egl->unif_alpha   = glGetUniformLocation(egl->shaderProgram, "alpha"); check();
}


void egl_init_matrices(EGL_TYPE *egl)
{
	reset_matrix(egl->projectionMatrix);
	reset_matrix(egl->viewMatrix);
	reset_matrix(egl->movementMatrix);
	reset_matrix(egl->scalingMatrix);
}


void egl_render(EGL_TYPE *egl)
{
	// Clear the background
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); check();

	// Activate texture 0 (for later binding) and tell GLSL to use texture 0
	glActiveTexture(GL_TEXTURE0); check();
	glUniform1i(egl->unif_texture, 0); check();
	
	glUniformMatrix4fv(egl->unif_Pmatrix, 1, false, egl->projectionMatrix); check();
	glUniformMatrix4fv(egl->unif_Vmatrix, 1, false, egl->viewMatrix); check();

	switch(egl->program_state)
	{
		case NEED_PHOTO:
		case HOLD:
		case TRANSITION_COMPLETE:
			//egl_render_current(egl);
			break;
			
		case TRANSITION:
			egl_render_transition(egl);
			break;
	}

	glFlush(); check();
	glFinish(); check();

	eglSwapBuffers(egl->display, egl->surface);
	check();
}


void egl_destroy(EGL_TYPE *egl)
{
	int counter;

	/**/

	// clear screen
	glClear(GL_COLOR_BUFFER_BIT); check();
	eglSwapBuffers(egl->display, egl->surface); check();

	// Release loaded textures
	for(counter = 0; counter < IMAGE_COUNT; counter++)
	{
		egl_unload_texture(&egl->textures[counter]);
	}
	glFinish(); // Wait for all queued/in-progress GL commands to finish before continuing

	// Release EGL resources
	eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); check();
	eglDestroySurface(egl->display, egl->surface); check();
	eglDestroyContext(egl->display, egl->context); check();
	eglTerminate(egl->display); check();
}


void reset_matrix(GLfloat matrix[])
{
	matrix[0]=1.0f;		matrix[1]=0.0f;		matrix[2]=0.0f;		matrix[3]=0.0f;
	matrix[4]=0.0f;		matrix[5]=1.0f;		matrix[6]=0.0f;		matrix[7]=0.0f;
	matrix[8]=0.0f;		matrix[9]=0.0f;		matrix[10]=1.0f;	matrix[11]=0.0f;
	matrix[12]=0.0f;	matrix[13]=0.0f;	matrix[14]=0.0f;	matrix[15]=1.0f;
}


void scale_matrix(GLfloat matrix[], long screenWidth, long screenHeight, long textureWidth, long textureHeight)
{
	// Set defaults
	GLfloat xRatio = textureWidth / (GLfloat)screenWidth; //tW / cW;
	GLfloat yRatio = textureHeight / (GLfloat)screenHeight; //tH / cH;

	// Compute scaling ratios
	if ( xRatio < yRatio) {
		xRatio = xRatio / yRatio;
		yRatio = 1.0f;
	} else {
		yRatio = yRatio / xRatio;
		xRatio = 1.0f;
	}

	matrix[0]=xRatio;	matrix[1]=0;		matrix[2]=0;  matrix[3]=0;
	matrix[4]=0;  		matrix[5]=yRatio;	matrix[6]=0;  matrix[7]=0;
	matrix[8]=0;		matrix[9]=0;		matrix[10]=1; matrix[11]=0;
	matrix[12]=0;		matrix[13]=0;		matrix[14]=0; matrix[15]=1;
}


void egl_load_texture(TEXTURE_TYPE* texture, char filename[])
{
	SDL_Surface* image;

	/**/

	// OpenGL prep work to store texture data

	// Create the texture
	if(texture->glTextureID == 0) {glGenTextures(1, &texture->glTextureID); check();}

	// Bind the texture to the context
	glBindTexture(GL_TEXTURE_2D, texture->glTextureID); check();


	// Load the texture via SDL, copy worthwhile data, and free the SDL memory
	// Using SDL to load non-BMP picture formats
	image = IMG_Load(filename);

	// Load	the image data into the texture
	glTexImage2D(GL_TEXTURE_2D, 0,
					GL_RGB, image->w, image->h,
					0,
					GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
	check();

	// Store other worthwhile values before the SDL object is destroyed
	texture->width = image->w;
	texture->height = image->h;

	// unload the image (because it's already been copied into GL memory)
	SDL_FreeSurface(image);


	// Continue setting OpenGL values for the new texture data
	// Set magnification filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); check();

	// Set minification filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); check();

	// Assume the texture is not POT (Power Of Two). 
	// We cannot use default clamp mode which is GL.REPEAT (tiling). Switching to "stretch/squash" mode. 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); check();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); check();

	// Free the context (optional)
	//glBindTexture(GL_TEXTURE_2D, NULL); check()
}


void egl_unload_texture(TEXTURE_TYPE* texture)
{
	if(texture->glTextureID > 0)
	{
		glDeleteTextures(1, &texture->glTextureID); check();
		glFinish(); check();
	}
	//texture->glTextureID = 0;
	texture->width = 0;
	texture->height = 0;
}


void egl_render_current(EGL_TYPE *egl)
{
	scale_matrix(egl->scalingMatrix, egl->screen_width, egl->screen_height, egl->textures[egl->textureCurrent].width, egl->textures[egl->textureCurrent].height);
	glUniformMatrix4fv(egl->unif_Smatrix, 1, false, egl->scalingMatrix);

	reset_matrix(egl->movementMatrix);
	glUniformMatrix4fv(egl->unif_Mmatrix, 1, false, egl->movementMatrix);


	// Bind the current texture to the active GL texture (should be GL_TEXTURE0)
	glBindTexture(GL_TEXTURE_2D, egl->textures[egl->textureCurrent].glTextureID); check();

	glDisable(GL_BLEND); check();

	glDrawElements(GL_TRIANGLES, 1*2*3, GL_UNSIGNED_SHORT, 0); check();
}


void egl_render_transition(EGL_TYPE *egl)
{
	int textureActive;
	int textureCounter;
	int frameCounterMod;

	//int transitionDuration = 60;
	int transitionDuration;

	GLfloat alpha;

	/**/

	// Display seems to render at 60fps.  Convert 1000msec (1sec) to 60 frames:
	transitionDuration = (egl->transition_duration * 60) / 1000;

	for(textureCounter = 0; textureCounter < IMAGE_COUNT; textureCounter++)
	{
		if(egl->textures[textureCounter].glTextureID > 0)	// Skip unloaded textures
		{
			textureActive = (egl->textureCurrent + textureCounter) % IMAGE_COUNT;

			switch(textureActive)
			{
				case 0:
					if(egl->frameCounter < transitionDuration) 
						{alpha = (transitionDuration - egl->frameCounter) / (float)transitionDuration;}
					else 
						{alpha = 0.0f;}
					break;

				case 1:
					if(egl->frameCounter < transitionDuration)
						{alpha = egl->frameCounter / (float)transitionDuration;}
					else 
						{alpha = 1.0f;}
					break;
			}

			scale_matrix(egl->scalingMatrix, egl->screen_width, egl->screen_height, egl->textures[textureCounter].width, egl->textures[textureCounter].height);
			glUniformMatrix4fv(egl->unif_Smatrix, 1, false, egl->scalingMatrix);

			reset_matrix(egl->movementMatrix);
			glUniformMatrix4fv(egl->unif_Mmatrix, 1, false, egl->movementMatrix);


			// Bind the current texture to the active GL texture (should be GL_TEXTURE0)
			glBindTexture(GL_TEXTURE_2D, egl->textures[textureCounter].glTextureID); check();

			glEnable(GL_BLEND); check();
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); check();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE); check();

			glUniform1f(egl->unif_alpha, alpha); check();

			glDrawElements(GL_TRIANGLES, 1*2*3, GL_UNSIGNED_SHORT, 0); check();
		}
	}

	// Check exit condition
	if(egl->frameCounter >= transitionDuration) {egl->program_state = TRANSITION_COMPLETE;}
}



