#ifndef UTILS
#define UTILS
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
unsigned int *splitutf8(unsigned char *utf8in,int *lenout);
GLuint compileShader(const char *vs,const char *fs);
void setImageParas(GLuint tex);
char *renderstring(unsigned char *strin,int width,int height,int *realwidth,int *realheight,float *percents);
#endif
