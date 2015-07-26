#include <cassert>
#include <vector>
#include <GL/glew.h>
#include <FFGL.h>
#include "FFGLLightBrush.h"

#define FFPARAM_THRESHOLD (0)

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo(
	FFGLLightBrush::CreateInstance,     // Create method
	"LtBr",                             // Plugin unique ID
	"FFGLLightBrush",                   // Plugin name											
	1,                                  // API major version number 													
	000,                                // API minor version number	
	1,                                  // Plugin major version number
	000,                                // Plugin minor version number
	FF_EFFECT,                          // Plugin type
	"FFGL plugin for light painting",   // Plugin description
	"by Seppo Enarvi - users.marjaniemi.com/seppo" // About
	);

static const char * vertexShaderSource =
"void main()"
"{"
"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
"    gl_TexCoord[0] = gl_MultiTexCoord0;"
"    gl_FrontColor = gl_Color;"
"}";

// A fragment is the data necessary to shade a pixel (and test whether it is
// visible).
static const char * fragmentShaderSource =
"uniform sampler2D inputSampler;"
"uniform sampler2D stateSampler;"
"uniform float threshold;"
"const vec4 grayScaleWeights = vec4(0.30, 0.59, 0.11, 0.0);"
"const vec4 red = vec4(1.0, 0.0, 0.0, 1.0);"
"const vec4 green = vec4(0.0, 1.0, 0.0, 1.0);"
"const vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);"
"void main()"
"{"
"    vec4 inputColor = texture2D(inputSampler, gl_TexCoord[0].st);"
"    vec4 scaledInputColor = inputColor * grayScaleWeights;"
"    float inputLuminance = scaledInputColor.r + scaledInputColor.g + scaledInputColor.b;"
"    vec4 stateColor = texture2D(stateSampler, gl_TexCoord[0].st);"
"    vec4 scaledStateColor = stateColor * grayScaleWeights;"
"    float stateLuminance = scaledStateColor.r + scaledStateColor.g + scaledStateColor.b;"
"    if (inputLuminance >= stateLuminance) {"
"        gl_FragColor = red;"
"    }"
"    else if (stateLuminance >= threshold) {"
"        gl_FragColor = green;"
"    } else {"
"        gl_FragColor = blue;"
"    }"
"}";
/*
"    if (inputLuminance >= stateLuminance) {"
"        gl_FragColor = inputColor;"
"    }"
"    else if (stateLuminance >= threshold) {"
"        gl_FragColor = stateColor;"
"    } else {"
"        gl_FragColor = inputColor;"
"    }"
"}";
*/

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FFGLLightBrush::FFGLLightBrush()
: CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	// Parameters
	threshold_ = 0.5f;
	SetParamInfo(FFPARAM_THRESHOLD, "Threshold", FF_TYPE_STANDARD, threshold_);
}

FFGLLightBrush::~FFGLLightBrush()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

void FFGLLightBrush::compileShader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint isCompiled = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
	assert(isCompiled == GL_TRUE);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
	assert(isCompiled == GL_TRUE);

	shaderProgram_ = glCreateProgram();
	glAttachShader(shaderProgram_, fragmentShader);
	glAttachShader(shaderProgram_, vertexShader);
	glLinkProgram(shaderProgram_);
	GLint isLinked = 0;
	glGetProgramiv(shaderProgram_, GL_LINK_STATUS, (int *)&isLinked);
	assert(isLinked == GL_TRUE);
	glDetachShader(shaderProgram_, vertexShader);
	glDetachShader(shaderProgram_, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void FFGLLightBrush::initializeTexture(GLuint texture, GLuint width, GLuint height) const
{
	glBindTexture(GL_TEXTURE_2D, texture);
	vector<GLubyte> data(width * height * 4, 0x7f);
	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		width, height,
		0,
		GL_BGRA,
		GL_UNSIGNED_INT_8_8_8_8_REV,
		&data[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void FFGLLightBrush::render() const
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
	glViewport(viewport_.x, viewport_.y, viewport_.width, viewport_.height);
	glCallList(displayList_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FFGLLightBrush::copyFramebuffer(GLuint src, GLuint dst) const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);
	glBlitFramebuffer(
		0, 0, viewport_.width, viewport_.height,
		0, 0, viewport_.width, viewport_.height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

DWORD FFGLLightBrush::InitGL(const FFGLViewportStruct *vp)
{
	// GLEW helps to load dynamically some extensions.
	glewInit();
	assert(GLEW_VERSION_2_0);

	viewport_.x = 0;
	viewport_.y = 0;
	viewport_.width = vp->width;
	viewport_.height = vp->height;

	// Generate name for one framebuffer and two textures, so that we can
	// render to one texture and read the previous state from one texture.
	glGenFramebuffers(1, &framebuffer_);
	glGenTextures(2, textures_);

	// Compile the GLSL shader.
	compileShader();

	// To pass data to the shader, we need the location of the uniforms (global
	// variables defined in the shader code), and then call one of the
	// glUniform* methods.
	inputTextureLocation_ = glGetUniformLocation(shaderProgram_, "inputSampler");
	assert(inputTextureLocation_ != -1);
	stateTextureLocation_ = glGetUniformLocation(shaderProgram_, "stateSampler");
	assert(stateTextureLocation_ != -1);
	thresholdLocation_ = glGetUniformLocation(shaderProgram_, "threshold");
	assert(thresholdLocation_ != -1);

	// The input and state textures are always bound to texture units 0 and 1
	// respectively.
	glUseProgram(shaderProgram_);
	glUniform1i(inputTextureLocation_, 0);
	glUniform1i(stateTextureLocation_, 1);
	glUseProgram(0);

	// Initialize the textures with correct size.
	initializeTexture(textures_[0], viewport_.width, viewport_.height);
	initializeTexture(textures_[1], viewport_.width, viewport_.height);

	// Create a list of operations for drawing a quad that fills the entire
	// viewport.
	if (displayList_ == 0) {
		displayList_ = glGenLists(1);
		glNewList(displayList_, GL_COMPILE);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glBegin(GL_QUADS);
		//lower left
		glTexCoord2d(0.0, 0.0);
		glVertex2f(-1, -1);
		//upper left
		glTexCoord2d(0.0, 1.0);
		glVertex2f(-1, 1);
		//upper right
		glTexCoord2d(1.0, 1.0);
		glVertex2f(1, 1);
		//lower right
		glTexCoord2d(1.0, 0.0);
		glVertex2f(1, -1);
		glEnd();
		glEndList();
	}

	return FF_SUCCESS;
}

DWORD FFGLLightBrush::DeInitGL()
{
	glDeleteFramebuffers(1, &framebuffer_);
	glDeleteTextures(2, textures_);
	glDeleteProgram(shaderProgram_);
	return FF_SUCCESS;
}

DWORD FFGLLightBrush::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	if (pGL->numInputTextures < 1)
		return FF_FAIL;
	if (pGL->inputTextures[0] == NULL)
		return FF_FAIL;
	FFGLTextureStruct &inputTexture = *(pGL->inputTextures[0]);

	glUseProgram(shaderProgram_);

	// Pass the current threshold value to the shader program.
	glUniform1f(thresholdLocation_, threshold_);

	// Bind input texture to texture unit 0.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexture.Handle);

	// Bind state texture to texture unit 1.
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures_[0]);

	// Bind output texture to the framebuffer object.
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures_[1], 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	render();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);

	// Copy the texture to host framebuffer object.
	copyFramebuffer(framebuffer_, pGL->HostFBO);

	return FF_SUCCESS;
}

DWORD FFGLLightBrush::GetParameter(DWORD dwIndex)
{
	DWORD dwRet;

	switch (dwIndex) {

	case FFPARAM_THRESHOLD:
		//sizeof(DWORD) must == sizeof(float)
		*((float *)(unsigned)(&dwRet)) = threshold_;
		return dwRet;

	default:
		return FF_FAIL;
	}
}

DWORD FFGLLightBrush::SetParameter(const SetParameterStruct* pParam)
{
	if (pParam != NULL) {

		switch (pParam->ParameterNumber) {

		case FFPARAM_THRESHOLD:
			//sizeof(DWORD) must == sizeof(float)
			threshold_ = *((float *)(unsigned)&(pParam->NewParameterValue));
			break;

		default:
			return FF_FAIL;
		}

		return FF_SUCCESS;

	}

	return FF_FAIL;
}