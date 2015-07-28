// FFGLLightBrush.h - FFGL plugin for light painting
//
// Copyright 2015 Seppo Enarvi
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FFGLLIGHTBRUSH_H
#define FFGLLIGHTBRUSH_H

#include "FFGLPluginSDK.h"

class FFGLLightBrush :
	public CFreeFrameGLPlugin
{
public:
	FFGLLightBrush();
	virtual ~FFGLLightBrush();

	// factory method

	static DWORD __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance)
	{
		*ppInstance = new FFGLLightBrush();
		if (*ppInstance != NULL) return FF_SUCCESS;
		return FF_FAIL;
	}

	// helper methods

	void compileShader();
	void initializeTexture(GLuint texture, GLuint width, GLuint height) const;
	void renderToTexture(GLuint texture) const;
	void copyFramebuffer(GLuint src, GLuint dst) const;
	void clearState();

	// FreeFrame plugin methods

	DWORD SetParameter(const SetParameterStruct* pParam);
	DWORD GetParameter(DWORD dwIndex);
	DWORD ProcessOpenGL(ProcessOpenGLStruct* pGL);
	DWORD InitGL(const FFGLViewportStruct *vp);
	DWORD DeInitGL();

protected:
	FFGLViewportStruct viewport_;
	GLuint displayList_ = 0;

	// parameters
	float threshold_;
	float darkening_;

	GLuint shaderProgram_;
	GLuint framebuffer_;
	GLuint textures_[2];
	int stateTextureIndex_;
	int outputTextureIndex_;

	// locations of the global GLSL variables
	GLint inputSamplerLocation_;
	GLint stateSamplerLocation_;
	GLint thresholdLocation_;
	GLint darkeningLocation_;
};


#endif
