
#if BUILD_WIN32
#include <windows.h>
#include <gl/gl.h>
#include "ext/wglext.h"

typedef void (WINAPI *PFNGLCLEARPROC) (GLbitfield mask);
typedef void (WINAPI *PFNGLCLEARCOLORPROC) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (WINAPI *PFNGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void (WINAPI *PFNGLDELETETEXTURESPROC) (GLsizei n, const GLuint *textures);
typedef void (WINAPI *PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef void (WINAPI *PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (WINAPI *PFNGLTEXPARAMETERIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (WINAPI *PFNGLTEXPARAMETERFVPROC) (GLenum target, GLenum pname, GLfloat *param);
typedef void (WINAPI *PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (WINAPI *PFNGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (WINAPI *PFNGLENABLEPROC) (GLenum cap);
typedef void (WINAPI *PFNGLCULLFACEPROC) (GLenum mode);
typedef void (WINAPI *PFNGLDRAWBUFFERPROC) (GLenum buf);
typedef void (WINAPI *PFNGLREADBUFFERPROC) (GLenum buf);

#else
#error "OpenGL includes for platform not supported."
#endif
#include "ext/glext.h"

#define GLPrefix(name) GL_##name

#define GLProc(name, type) PFNGL##type##PROC GLPrefix(name) = 0;
#include "opengl_procedure_list.inc"

internal void
LoadAllOpenGLProcedures(void)
{
#define GLProc(name, type) GLPrefix(name) = os->LoadOpenGLProcedure("gl" #name);
#include "opengl_procedure_list.inc"
}