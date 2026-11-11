#ifndef CLEANHOME_GLES_H
#define CLEANHOME_GLES_H

#ifdef CLEANHOME_TIZEN
/* Evas_GL.h ja traz os tipos/enums do GLES2 e recusa ser incluido depois de
 * GLES2/gl2.h (__gl2_h_ definido). No Tizen, so ele entra. */
#include <Evas_GL.h>

extern Evas_GL_API *g_evas_gl_api;

#define glActiveTexture      ((Evas_GL_API *)g_evas_gl_api)->glActiveTexture
#define glAttachShader       ((Evas_GL_API *)g_evas_gl_api)->glAttachShader
#define glBindAttribLocation ((Evas_GL_API *)g_evas_gl_api)->glBindAttribLocation
#define glBindBuffer         ((Evas_GL_API *)g_evas_gl_api)->glBindBuffer
#define glBindTexture        ((Evas_GL_API *)g_evas_gl_api)->glBindTexture
#define glBlendFunc          ((Evas_GL_API *)g_evas_gl_api)->glBlendFunc
#define glBufferData         ((Evas_GL_API *)g_evas_gl_api)->glBufferData
#define glClear              ((Evas_GL_API *)g_evas_gl_api)->glClear
#define glClearColor         ((Evas_GL_API *)g_evas_gl_api)->glClearColor
#define glCompileShader      ((Evas_GL_API *)g_evas_gl_api)->glCompileShader
#define glCreateProgram      ((Evas_GL_API *)g_evas_gl_api)->glCreateProgram
#define glCreateShader       ((Evas_GL_API *)g_evas_gl_api)->glCreateShader
#define glDeleteBuffers      ((Evas_GL_API *)g_evas_gl_api)->glDeleteBuffers
#define glDeleteProgram      ((Evas_GL_API *)g_evas_gl_api)->glDeleteProgram
#define glDeleteShader       ((Evas_GL_API *)g_evas_gl_api)->glDeleteShader
#define glDeleteTextures     ((Evas_GL_API *)g_evas_gl_api)->glDeleteTextures
#define glDisable            ((Evas_GL_API *)g_evas_gl_api)->glDisable
#define glDrawArrays         ((Evas_GL_API *)g_evas_gl_api)->glDrawArrays
#define glEnable             ((Evas_GL_API *)g_evas_gl_api)->glEnable
#define glEnableVertexAttribArray ((Evas_GL_API *)g_evas_gl_api)->glEnableVertexAttribArray
#define glGenBuffers         ((Evas_GL_API *)g_evas_gl_api)->glGenBuffers
#define glGenTextures        ((Evas_GL_API *)g_evas_gl_api)->glGenTextures
#define glGetAttribLocation  ((Evas_GL_API *)g_evas_gl_api)->glGetAttribLocation
#define glGetError           ((Evas_GL_API *)g_evas_gl_api)->glGetError
#define glGetProgramInfoLog  ((Evas_GL_API *)g_evas_gl_api)->glGetProgramInfoLog
#define glGetProgramiv       ((Evas_GL_API *)g_evas_gl_api)->glGetProgramiv
#define glGetShaderInfoLog   ((Evas_GL_API *)g_evas_gl_api)->glGetShaderInfoLog
#define glGetShaderiv        ((Evas_GL_API *)g_evas_gl_api)->glGetShaderiv
#define glGetUniformLocation ((Evas_GL_API *)g_evas_gl_api)->glGetUniformLocation
#define glLinkProgram        ((Evas_GL_API *)g_evas_gl_api)->glLinkProgram
#define glPixelStorei        ((Evas_GL_API *)g_evas_gl_api)->glPixelStorei
#define glScissor            ((Evas_GL_API *)g_evas_gl_api)->glScissor
#define glShaderSource       ((Evas_GL_API *)g_evas_gl_api)->glShaderSource
#define glTexImage2D         ((Evas_GL_API *)g_evas_gl_api)->glTexImage2D
#define glTexParameteri      ((Evas_GL_API *)g_evas_gl_api)->glTexParameteri
#define glUniform1f          ((Evas_GL_API *)g_evas_gl_api)->glUniform1f
#define glUniform1i          ((Evas_GL_API *)g_evas_gl_api)->glUniform1i
#define glUniform2f          ((Evas_GL_API *)g_evas_gl_api)->glUniform2f
#define glUniform4f          ((Evas_GL_API *)g_evas_gl_api)->glUniform4f
#define glUseProgram         ((Evas_GL_API *)g_evas_gl_api)->glUseProgram
#define glVertexAttribPointer ((Evas_GL_API *)g_evas_gl_api)->glVertexAttribPointer
#define glViewport           ((Evas_GL_API *)g_evas_gl_api)->glViewport

#else
#define SDL_USE_BUILTIN_OPENGL_DEFINITIONS
#include <SDL2/SDL_opengles2.h>
#endif

#endif
