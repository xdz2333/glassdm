#define u8 unsigned char
#define u32 unsigned int
#define TIMERATIO .2
#include <pwd.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image_write.h"
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#define max(a,b) a>b?a:b
#define min(a,b) a>b?b:a
#include <time.h>
#include "utils.h"
GLFWwindow *window;
char passwd[64];
int passlen=0,boxbias=0,boxl,usernum,usernow=0,arrowl,arrowb,arroww,arrowh,namew,nameh,scrw,scrh,winw,winh;
float lasttime,shakeleft=0.3,lastshake,scrratio;
extern char *vs,*fsbg,*fsjfa,*fsbox,*fsinit,*fspost,*fsblur,*fsavatar,*fselement;
FT_Library library;
FT_Face face;
int timew,timeh,bgw,bgh,bgn,avatarw,avatarh;
GLuint imgin,fbomap[2],fbo[2],proginit,progjfa,progpost,vao,vbo,bgimg,progbg,progblur,fboblur[2],fboblurmap[2],sdfmap,progelement,dateimg,progavatar,avatarimg,progbox,screenfbo,screenmap;
time_t time_;
struct tm *localtime_;
u8 strtime[6];
typedef struct{
    int uid;
    char name[64];
} user_t;
typedef struct{
	int uid;
	char name[64];
	char passwd[64];
} passwdpipe;
user_t *users;
user_t *getuserlist(int *usernum){
    FILE *file=fopen("/etc/login.defs","r");
    char buf[100];
    int uidmin=0,uidmax=0,len=0;
    while((fgets(buf,100,file))){
        if(!strncmp("UID_MIN",buf,7)){
            uidmin=atoi(buf+7);
        }else if(!strncmp("UID_MAX",buf,7)){
            uidmax=atoi(buf+7);
        }
    }
    fclose(file);
    struct passwd *pwd;
    user_t *result=(user_t*)malloc(10*sizeof(user_t));
    memset(result,0,10*sizeof(user_t));
    while((pwd=getpwent())){
        if(len>10){
            break;
        }if(uidmax>=pwd->pw_uid && pwd->pw_uid>=uidmin){
            strncpy((result+len)->name,pwd->pw_name,64);
            (result+len)->uid=pwd->pw_uid;
            len++;
        }
    }
    *usernum=len;
    return result;
}
void changeAera(int x,int y,int w,int h){
    glViewport(x,y,w,h);
    glScissor(x,y,w,h);
}
void recalcsdf(float percent,u8 *strtime){
    glDisable(GL_BLEND);
    float percents[5]={percent*((int)strtime[1]==48),percent*((int)strtime[3]==48),0,percent*((int)strtime[4]==48),percent};
    char* texbuffer=renderstring(strtime,200/TIMERATIO,200/TIMERATIO,&timew,&timeh,percents);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,imgin);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,timew,timeh,0,GL_RED,GL_UNSIGNED_BYTE,texbuffer);
    free(texbuffer);
    for(int i=0;i<2;i++){
        glBindTexture(GL_TEXTURE_2D,fbomap[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,timew,timeh,0,GL_RGB,GL_FLOAT,NULL);
    }

    glUseProgram(proginit);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo[0]);
    glBindTexture(GL_TEXTURE_2D,imgin);
    glUniform1i(glGetUniformLocation(proginit,"source"),0);
    glUniform2i(glGetUniformLocation(proginit,"texres"),timew,timeh);
    changeAera(0,0,timew,timeh);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);

    glUseProgram(progjfa);
    glUniform1i(glGetUniformLocation(progjfa,"source"),0);
    glUniform2i(glGetUniformLocation(progjfa,"texres"),timew,timeh);
    int ratio=timew/2;
    for(int i=0;i<log2((float)timew/2)+1;i++){
        glUniform1i(glGetUniformLocation(progjfa,"ratio"),ratio);
        glBindTexture(GL_TEXTURE_2D,fbomap[i%2]);
        glBindFramebuffer(GL_FRAMEBUFFER,fbo[(i+1)%2]);
        changeAera(0,0,timew,timeh);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        ratio/=2;
        ratio=max(ratio,1);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    sdfmap=fbomap[((int)log2((float)timew/2)+1)%2];
    glEnable(GL_BLEND);
    timew*=TIMERATIO;
    timeh*=TIMERATIO;
}
void updateTimestr(){
    time_=time(NULL);
    localtime_=localtime(&time_);
    strftime((char*)strtime,sizeof(strtime),"%H:%M",localtime_);
}
void updateTime(float percent,bool clear){
    if(clear){
        glBindFramebuffer(GL_FRAMEBUFFER,screenfbo);
        int timel=(int)(scrw/2)-timew*scrratio/2;
        int timeb=scrh-150*scrratio-timeh*scrratio;
        changeAera(timel,timeb,timew*scrratio,timeh*scrratio);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
    }
    recalcsdf(percent,strtime);
    glBindFramebuffer(GL_FRAMEBUFFER,screenfbo);
    int timel=(int)(scrw/2)-timew*scrratio/2;
    int timeb=scrh-150*scrratio-timeh*scrratio;
    changeAera(timel,timeb,timew*scrratio,timeh*scrratio);

    glUseProgram(progpost);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,sdfmap);
    glUniform1i(glGetUniformLocation(progpost,"sdf"),0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,bgimg);
    glUniform1i(glGetUniformLocation(progpost,"bg"),1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,fboblurmap[0]);
    glUniform1i(glGetUniformLocation(progpost,"bgblur"),2);
    glUniform2i(glGetUniformLocation(progpost,"scrsize"),timew,timeh);
    glUniform1f(glGetUniformLocation(progpost,"scrratio"),scrratio*TIMERATIO);
    glUniform2f(glGetUniformLocation(progpost,"uvbg0"),(float)timel/scrw,1.-(float)timeb/scrh);
    glUniform2f(glGetUniformLocation(progpost,"uvbg1"),(float)(timel+timew*scrratio)/scrw,1.-(float)(timeb+timeh*scrratio)/scrh);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glBindFramebuffer(GL_FRAMEBUFFER,0);

}
void drawtext(int ctrx,int ctry,int txtsize,float r,float g,float b,float a,const u8 *text,int *txtwout,int *txthout,bool clear){
    glUseProgram(progelement);
    glBindFramebuffer(GL_FRAMEBUFFER,screenfbo);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,fboblurmap[0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,dateimg);
    int txtw,txth;
    char *textimg=renderstring((u8*)text,txtsize*scrratio,txtsize*scrratio,&txtw,&txth,0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,txtw,txth,0,GL_RED,GL_UNSIGNED_BYTE,textimg);
    free(textimg);
    int x=ctrx-txtw/2,y=ctry-txth/2;
    changeAera(x,y,txtw,txth);
    glUniform1i(glGetUniformLocation(progelement,"ele"),0);
    glUniform1i(glGetUniformLocation(progelement,"bgblur"),1);
    glUniform2f(glGetUniformLocation(progelement,"uvbg0"),(float)x/scrw,1.-(float)y/scrh);
    glUniform2f(glGetUniformLocation(progelement,"uvbg1"),(float)(x+txtw)/scrw,1.-(float)(y+txth)/scrh);
    glUniform4f(glGetUniformLocation(progelement,"color"),r,g,b,a);
    if(clear){
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    if(txtwout&&txthout){
        *txtwout=txtw;
        *txthout=txth;
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
}
void drawbox(bool clear){
    glBindFramebuffer(GL_FRAMEBUFFER,screenfbo);
    glUseProgram(progbox);
    int boxw=320*scrratio,boxh=70*scrratio;
    if(clear){
        changeAera(boxl,100*scrratio,boxw,boxh);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    boxl=(scrw-boxw)/2+boxbias;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,bgimg);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,fboblurmap[0]);
    glUniform1i(glGetUniformLocation(progbox,"bg"),0);
    glUniform1i(glGetUniformLocation(progbox,"bgblur"),1);
    changeAera(boxl,100*scrratio,boxw,boxh);
    glUniform2f(glGetUniformLocation(progbox,"uvbg0"),(float)boxl/scrw,1.-100./1080.);
    glUniform2f(glGetUniformLocation(progbox,"uvbg1"),1.-(float)boxl/scrw,1.-(100.+boxh)/1080.);
    glUniform2i(glGetUniformLocation(progbox,"scrsize"),boxw,boxh);
    glUniform1f(glGetUniformLocation(progbox,"scrratio"),scrratio);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);

    if(passlen==0){
        return;
    }
    int shapelen=min(10,passlen);
    u8 pswshape[shapelen*3+1];
    memset(pswshape,0,shapelen*3+1);
    for(int i=0;i<shapelen;i++){
        *(u32*)(pswshape+i*3)=0xA280E2;
    }//• utf8 0xE280A2
    drawtext(scrw/2,135*scrratio,50,.8,.8,.8,1,pswshape,0,0,false);
}
void drawuser(){
    glBindFramebuffer(GL_FRAMEBUFFER,screenfbo);
	changeAera(0,(200-15)*scrratio,scrw,30*scrratio);
	glClear(GL_COLOR_BUFFER_BIT);
    drawtext(scrw/2,200*scrratio,25,1,1,1,1,(u8*)(users+usernow)->name,0,0,true);
    glBindFramebuffer(GL_FRAMEBUFFER,screenfbo);
    glUseProgram(progavatar);
    glUniform1i(glGetUniformLocation(progavatar,"source"),0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,avatarimg);
    changeAera(scrw/2-(int)(50*scrratio),250*scrratio,100*scrratio,100*scrratio);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glBindFramebuffer(GL_FRAMEBUFFER,0);
}
void drawbg(){
    char datestr[64];
    sprintf(datestr,(const char*)u8"%d年 %02d月 %02d日",localtime_->tm_year+1900,localtime_->tm_mon+1,localtime_->tm_mday);
    drawtext(scrw/2,scrh-90*scrratio,40,1,1,1,0.5,(u8*)datestr,0,0,true);
    if(usernum>1){
        drawtext(scrw/2-(int)( 90*scrratio ),300*scrratio,50,1,1,1,0.5,(u8*)"〈",&arroww,&arrowh,false);
        arrowl=scrw/2-(int)( 70*scrratio )-arroww/2;
        arrowb=(int)(300*scrratio)-arrowh/2;
        drawtext(scrw/2+(int)( 90*scrratio ),300*scrratio,50,1,1,1,0.5,(u8*)"〉",0,0,false);
    }
    drawtext(scrw/2,50*scrratio,25,1,1,1,.5,(u8*)"请使用触控ID或输入密码",0,0,false);
    drawuser();
}
void updatescreen(){
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glUseProgram(progbg);
    glActiveTexture(GL_TEXTURE0);
    changeAera(0,0,scrw,scrh);

    glBindTexture(GL_TEXTURE_2D,bgimg);
    glUniform1i(glGetUniformLocation(progbg,"flipy"),0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);

    glBindTexture(GL_TEXTURE_2D,screenmap);
    glUniform1i(glGetUniformLocation(progbg,"flipy"),1);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glfwSwapBuffers(window);
}
void clearhandler(int signum){
    memset(passwd,0,sizeof(passwd));
    passlen=0;
    drawbox(false);
    updatescreen();
    shakeleft=0.3;
    lastshake=glfwGetTime();
}
void reloadavatar(){
    int avatarn;
    char avatarpath[128];
    sprintf(avatarpath,"/var/lib/AccountsService/icons/%s",(users+usernow)->name);
    u8 *avatarbuf=stbi_load(avatarpath,&avatarw,&avatarh,&avatarn,0);
    if(!avatarbuf){
        avatarbuf=stbi_load("./defaultavatar.png",&avatarw,&avatarh,&avatarn,0);
    }
    GLuint avatarfmt=avatarn==3?GL_RGB:GL_RGBA;
    glBindTexture(GL_TEXTURE_2D,avatarimg);
    glTexImage2D(GL_TEXTURE_2D,0,avatarfmt,avatarw,avatarh,0,avatarfmt,GL_UNSIGNED_BYTE,avatarbuf);
    glBindTexture(GL_TEXTURE_2D,0);
    stbi_image_free(avatarbuf);
    drawuser();
}
void charfunc(GLFWwindow* window, unsigned int codepoint){
    if(passlen<sizeof(passwd)){
        passwd[passlen]=char(codepoint);
        passlen++;
    }
    drawbox(false);
    updatescreen();
}
void keyfunc(GLFWwindow* window,int key,int scancode,int action,int mods){
    if(action==GLFW_PRESS && key==GLFW_KEY_BACKSPACE && passlen>0){
        passwd[passlen]=(char)NULL;
        passlen-=1;
        drawbox(false);
        updatescreen();
    }
    if(action==GLFW_PRESS && key==GLFW_KEY_ENTER){
		passwdpipe dataout;
		memset(&dataout,0,sizeof(passwdpipe));
		dataout.uid=(users+usernow)->uid;
		strncpy(dataout.name,(users+usernow)->name,sizeof(dataout.name));
		strncpy(dataout.passwd,passwd,sizeof(dataout.passwd));
		write(78,&dataout,sizeof(passwdpipe));
    }
}
void mousefunc(GLFWwindow* window, int button, int action, int mods){
	bool switchuser=false;
    if (button == GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS) {
        double x;
        double y;
        glfwGetCursorPos(window, &x, &y);
        x=x*scrw/winw;
        y=y*scrh/winh;
        y=scrh-y;
        if(usernum==1){return;}
        if(x>arrowl&&x<arrowl+arroww&&y>arrowb&&y<arrowb+arrowh){
            usernow-=1;
            usernow=usernow<0?usernum-1:usernow;
			switchuser=true;
        }else if(scrw-x>arrowl&&scrw-x<arrowl+arroww&&y>arrowb&&y<arrowb+arrowh){
            usernow+=1;
            usernow%=usernum;
			switchuser=true;
        }
    }
	if(switchuser){
		reloadavatar();
		memset(passwd,0,sizeof(passwd));
		passlen=0;
		drawbox(true);
		updatescreen();
	}
}
int main(){
    memset(passwd,0,sizeof(passwd));
    FT_Init_FreeType(&library);
    FT_New_Face( library, "./PingFangSC-Medium.otf", 0, &face); 
    glfwInitHint(GLFW_PLATFORM,GLFW_PLATFORM_WAYLAND);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_COMPAT_PROFILE);
    window = glfwCreateWindow( 1920, 1080, "glassdm greeter", glfwGetPrimaryMonitor(), NULL);
    //window = glfwCreateWindow( 500, 500, "glassdm greeter", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwGetFramebufferSize(window,&scrw,&scrh);
    glfwGetWindowSize(window,&winw,&winh);
    scrratio=scrh/1080.;
    boxl=(scrw-320*scrratio)/2;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glfwSetKeyCallback(window,keyfunc);
    glfwSetCharCallback(window, charfunc);
    glfwSetMouseButtonCallback(window,mousefunc);
    glBlendFunc( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    signal(SIGUSR1,clearhandler);

    users=getuserlist(&usernum);

    float vertex[]={ -1,1, 1,1, -1,-1, 1,-1 };
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);
    glGenBuffers(1,&vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof( vertex ),vertex,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*4,(void*)0);
    glEnableVertexAttribArray(0);
    proginit=compileShader(vs,fsinit);
    progjfa=compileShader(vs,fsjfa);
    progpost=compileShader(vs,fspost);
    progbg=compileShader(vs,fsbg);
    progblur=compileShader(vs,fsblur);
    progelement=compileShader(vs,fselement);
    progavatar=compileShader(vs,fsavatar);
    progbox=compileShader(vs,fsbox);

    glGenTextures(1,&screenmap);
    setImageParas(screenmap);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,scrw,scrh,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
    glGenFramebuffers(1,&screenfbo);
    glBindFramebuffer(GL_FRAMEBUFFER,screenfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,screenmap,0);
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    glGenTextures(1,&imgin);
    setImageParas(imgin);
    glGenTextures(1,&dateimg);
    setImageParas(dateimg);

    glGenFramebuffers(2,fbo);
    glGenTextures(2,fbomap);
    for(int i=0;i<2;i++){
        glBindFramebuffer(GL_FRAMEBUFFER,fbo[i]);
        setImageParas(fbomap[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,fbomap[i],0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    unsigned char *bgbuffer=stbi_load("./background26.jpeg",&bgw,&bgh,&bgn,0);
    GLuint bgformat=bgn==3?GL_RGB:GL_RGBA;
    glGenTextures(1,&bgimg);
    setImageParas(bgimg);
    glTexImage2D(GL_TEXTURE_2D,0,bgformat,bgw,bgh,0,bgformat,GL_UNSIGNED_BYTE,bgbuffer);

    glGenFramebuffers(2,fboblur);
    glGenTextures(2,fboblurmap);
    for(int i=0;i<2;i++){
        glBindFramebuffer(GL_FRAMEBUFFER,fboblur[i]);
        setImageParas(fboblurmap[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,bgw,bgh,0,GL_RGB,GL_UNSIGNED_BYTE,bgbuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,fboblurmap[i],0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    stbi_image_free(bgbuffer);

    glUseProgram(progblur);
    changeAera(0,0,bgw,bgh);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,bgimg);
    glUniform1i(glGetUniformLocation(progblur,"bg"),0);
    glUniform2i(glGetUniformLocation(progblur,"texsize"),bgw,bgh);
    for(int pass=0;pass<5;pass++){
        glUniform2f(glGetUniformLocation(progblur,"vert"),1.,0.);
        glBindTexture(GL_TEXTURE_2D,fboblurmap[0]);
        glBindFramebuffer(GL_FRAMEBUFFER,fboblur[1]);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP,0,4);

        glUniform2f(glGetUniformLocation(progblur,"vert"),0.,1.);
        glBindTexture(GL_TEXTURE_2D,fboblurmap[1]);
        glBindFramebuffer(GL_FRAMEBUFFER,fboblur[0]);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    glGenTextures(1,&avatarimg);
    setImageParas(avatarimg);
    reloadavatar();

    lasttime=glfwGetTime();
    updateTimestr();
    drawbg();
    updateTime(0.,false);
    drawbox(false);
    updatescreen();
    float lastupdate=-100,percent=0;
    lastshake=lasttime;
    while (!glfwWindowShouldClose(window)){
        time_=time(NULL);
        localtime_=localtime(&time_);
        float timenow=glfwGetTime();
        if(localtime_->tm_sec==0&&timenow-lastupdate>50.){
            lastupdate=timenow;
        }if(timenow-lastupdate<.3&&timenow-lasttime>1./60.){
            percent=1.-4.*(timenow-lastupdate);
            percent=max(min(percent,1.),0.);
            updateTimestr();
            updateTime(percent,true);
            updatescreen();
            lasttime=timenow;
        }else if(percent!=0){
            percent=0;
            updateTime(percent,true);
            updatescreen();
        }
        if(shakeleft>0&&timenow-lastshake>1./60.){
            boxbias=scrratio*50.*sin(104.71975512*(.3-shakeleft))*exp(-10.*(.3-shakeleft));
            shakeleft-=timenow-lastshake;
            lastshake=timenow;
            drawbox(true);
            updatescreen();
        }
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

