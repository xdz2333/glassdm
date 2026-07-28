#include <string.h>
#include <stdlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include "utils.h"
#define u32 unsigned int
#define u8 unsigned char
#define max(a,b) a>b?a:b
#define min(a,b) a>b?b:a
#define contif(val) if(val){continue;}
extern FT_Library library;
extern FT_Face face;
u32 *splitutf8(u8 *utf8in,int *lenout){
    int len=0;
    u32 *splitout=(u32*)malloc(strlen((char*)utf8in)*sizeof(u32));
    u8 *splitting=utf8in;
    while(splitting<utf8in+strlen((char*)utf8in)){
        if(*splitting>>7==(u8)0){
            splitout[len]=(u32)splitting[0];
            splitting++;
        }else if(*splitting>>5==(u8)0b110){
            splitout[len]=(((u32)splitting[0]&0b1111)<<6)+(((u32)splitting[1]&0b111111));
            splitting+=2;
        }else if(*splitting>>4==(u8)0b1110){
            splitout[len]=(((u32)splitting[0]&0b1111)<<12)+(((u32)splitting[1]&0b111111)<<6)+(( (u32)splitting[2]&0b111111 ));
            splitting+=3;
        }else if(*splitting>>3==(u8)0b11110){
            splitout[len]=(((u32)splitting[0]&0b1111)<<18)+(((u32)splitting[1]&0b111111)<<12)+(( (u32)splitting[2]&0b111111 )<<6)+((u32)splitting[3]&0b111111);
            splitting+=4;
        }
        len++;
    }
    *lenout=len;
    return splitout;
}
GLuint compileShader(const char *vs,const char *fs){
    char buffer[512];
    GLuint program=glCreateProgram();
    GLuint vsobj=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsobj,1,&vs,NULL);
    glCompileShader(vsobj);
    GLint ok;
    glGetShaderiv(vsobj, GL_COMPILE_STATUS, &ok);
    if(!ok){
        glGetShaderInfoLog(vsobj,512,nullptr,buffer);
        printf("error vsinfo: %s\n",buffer);
    }
    glAttachShader(program,vsobj);
    GLuint fsobj=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsobj,1,&fs,NULL);
    glCompileShader(fsobj);
    glGetShaderiv(fsobj, GL_COMPILE_STATUS, &ok);
    if(!ok){
        glGetShaderInfoLog(fsobj,512,nullptr,buffer);
        printf("error fsinfo: %s\n",buffer);
    }
    glAttachShader(program,fsobj);
    glLinkProgram(program);
    glDeleteShader(vsobj);
    glDeleteShader(fsobj);
    return program;
}
void setImageParas(GLuint tex){
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
char *renderstring(u8 *strin,int width,int height,int *realwidth,int *realheight,float *percents){
    int len;
    u32 *str=splitutf8(strin,&len);
    char *buffers=(char*)malloc(width*height*len*2);
    memset(buffers,0,width*height*len*2);
    int xoffset=0,top[len],bottom[len],xoffsets[len+1];
    xoffsets[0]=0;
    FT_Set_Pixel_Sizes(face,width,height);  
    for(int i=0;i<len;i++){
        FT_Load_Char(face,str[i], FT_LOAD_RENDER);
        int cols=face->glyph->bitmap.width;
        int rows=face->glyph->bitmap.rows;
        int bearingx=face->glyph->metrics.horiBearingX>>6;
        int bearingy=face->glyph->metrics.horiBearingY>>6;
        top[i]=height-bearingy;
        bottom[i]=height-bearingy+rows-1;
        for(int row=0;row<rows;row++){
            memcpy(buffers+width*len*(row+height-bearingy)+xoffset+bearingx,face->glyph->bitmap.buffer+row*cols,cols);
        }
        xoffset+=(face->glyph->metrics.horiAdvance)>>6;
        xoffsets[i+1]=xoffset;
    }
    int smallest=height*2,biggest=0;
    for(int i=0;i<len;i++){
        biggest=max(biggest,bottom[i]);
        smallest=min(smallest,top[i]);
    }
    //stbi_write_png("./str.png",width*len,height*2,1,buffers,0);
    int padding=height*0.05;
    int newwidth=xoffset+padding*2;
    int newheight=biggest-smallest+padding*2;
    char *newbuffer=(char*)malloc(newwidth*newheight);
    memset(newbuffer,0,newwidth*newheight);
    char *startline=newbuffer+newwidth*padding;
    for(int i=0;i<biggest-smallest;i++){
        memcpy(startline+newwidth*i+padding,buffers+width*len*(i+smallest),xoffset);
    }
    for(int i=0;i<(percents?len:0);i++){
        int rowbias=newheight*percents[i];
        if(rowbias==0)
            continue;
        for(int row=newheight-1;row>=rowbias;row--){
            memcpy(newbuffer+newwidth*row+padding+xoffsets[i],newbuffer+newwidth*(row-rowbias)+padding+xoffsets[i],xoffsets[i+1]-xoffsets[i]);
        }
        for(int row=0;row<rowbias;row++){
            memset(newbuffer+newwidth*row+padding+xoffsets[i],0,xoffsets[i+1]-xoffsets[i]);
        }
    }
    free(buffers);
    free(str);
    *realwidth=newwidth;
    *realheight=newheight;
    //stbi_write_png("./strnew.png",newwidth,newheight,1,newbuffer,0);
    return newbuffer;
}
