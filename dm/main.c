#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <security/pam_appl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <signal.h>
int logouttype=1;
void readcfg(){
    char linebuf[64];
    FILE *cfg=fopen("/var/lib/lgdm/config.cfg","r");
	if(!cfg){
		return;
	}
    while(fgets(linebuf,sizeof(linebuf),cfg)){
        if(!strncmp(linebuf,"logouttype=",sizeof("logouttype="))){
            logouttype=atoi(linebuf+sizeof("logouttype="));
            if(logouttype!=0&&logouttype!=1){
                logouttype=1;
            }
            break;
        }
    }
    fclose(cfg);
}
int my_conv(int num_msg,const struct pam_message **msg,struct pam_response **resp,void *data)
{
	*resp=calloc(num_msg,sizeof(struct pam_response));

	for(int i=0;i<num_msg;i++)
	{
		if(msg[i]->msg_style==PAM_PROMPT_ECHO_OFF)
		{
			(*resp)[i].resp=strdup((char*)data);
		}
	}

	return PAM_SUCCESS;
}
int void_conv(int num_msg,const struct pam_message **msg,struct pam_response **resp,void *data){
	return PAM_SUCCESS;
}
typedef struct{
	int uid;
	char name[64];
	char passwd[64];
} passwdpipe;
struct pam_conv greeterconv={void_conv,NULL};
pam_handle_t *pamgreeter=NULL;
pam_handle_t *pamlogin=NULL;
int main(){
	int tty=open("/dev/tty2",O_RDWR);
	ioctl(tty,VT_ACTIVATE,2);
	ioctl(tty,VT_WAITACTIVE,2);
	ioctl(tty,KDSETMODE,KD_GRAPHICS);
	struct passwd *lgdmuser=getpwnam("lgdm");
	if(!lgdmuser){
		ioctl(tty,VT_ACTIVATE,1);
		ioctl(tty,VT_WAITACTIVE,1);
		return 1;
	}
	int pipegreet[2],piperoot[2];
	int status;
	passwdpipe datain;
	startgreeter:
	memset(&datain,0,sizeof(datain));
	pipe(pipegreet);
	pipe(piperoot);
	int greeterroot=fork();
	if(greeterroot==0){
		close(piperoot[0]);
		int ret=pam_start("lgdmgreeter","lgdm",&greeterconv,&pamgreeter);
		ret=pam_set_item(pamgreeter,PAM_TTY,"tty2");
		ret=pam_open_session(pamgreeter,0);
		char *argv[]={"/usr/bin/kwin_wayland","--drm","--no-lockscreen","--no-global-shortcuts","--locale1",NULL};
		char **envp=pam_getenvlist(pamgreeter);
		int pidc=fork();
		if(pidc==0){
			initgroups("lgdm",lgdmuser->pw_uid);
			setgid(lgdmuser->pw_uid);
			setuid(lgdmuser->pw_uid);
			execve("/usr/bin/kwin_wayland",argv,envp);
			return 0;
		}
		char waylandpath[64];
		sprintf(waylandpath,"/run/user/%d/wayland-0",lgdmuser->pw_uid);
		while(access(waylandpath,F_OK)){
			sleep(1);
		}
		char *argv2[]={"/var/lib/lgdm/greeter",NULL};
		int pidc2=fork();
		if(pidc2==0){
			dup2(pipegreet[1],78);
			close(pipegreet[0]);
			chdir("/var/lib/lgdm/");
			initgroups("lgdm",lgdmuser->pw_uid);
			setgid(lgdmuser->pw_uid);
			setuid(lgdmuser->pw_uid);
			execve("/var/lib/lgdm/greeter",argv2,envp);
			return 0;
		}
		close(pipegreet[1]);
		while(true){
			read(pipegreet[0],&datain,sizeof(datain));
			struct pam_conv conv={my_conv,datain.passwd};
			ret=pam_start("lgdmlogin",datain.name,&conv,&pamlogin);
			ret=pam_authenticate(pamlogin,0);
			if(ret==PAM_SUCCESS){
				break;
			}
			pam_end(pamlogin,ret);
			memset(&datain,0,sizeof(datain));
			kill(pidc2,SIGUSR1);
		}
		kill(pidc,SIGTERM);
		waitpid(pidc,&status,0);
		kill(pidc2,SIGTERM);
		waitpid(pidc2,&status,0);
		pam_end(pamlogin,PAM_SUCCESS);
		pam_close_session(pamgreeter,0);
		pam_end(pamgreeter,PAM_SUCCESS);
		int magic=0x137891;
		write(piperoot[1],&magic,sizeof(int));
		write(piperoot[1],&datain,sizeof(datain));
		close(piperoot[1]);
		return 0;
	}
	close(piperoot[1]);
	int magic;
	read(piperoot[0],&magic,sizeof(int));
	if(magic!=0x137891){
		kill(greeterroot,SIGKILL);
		waitpid(greeterroot,&status,0);
		return 1;
	}
	read(piperoot[0],&datain,sizeof(datain));
	close(piperoot[0]);
	waitpid(greeterroot,&status,0);
	int loginpid=fork();
	if(loginpid==0){
		int ret=pam_start("lgdmlogin",datain.name,&greeterconv,&pamlogin);
		ret=pam_set_item(pamlogin,PAM_TTY,"tty2");
		ret=pam_open_session(pamlogin,0);
		char *argv3[]={"/usr/bin/startplasma-wayland",NULL};
		char **envp=pam_getenvlist(pamlogin);
		int pidc=fork();
		if(pidc==0){
			initgroups(datain.name,datain.uid);
			setgid(datain.uid);
			setuid(datain.uid);
			execve("/usr/bin/startplasma-wayland",argv3,envp);
			return 1;
		}
		waitpid(pidc,&status,0);
		pam_close_session(pamlogin,0);
		pam_end(pamlogin,PAM_SUCCESS);
		return 0;
	}
	waitpid(loginpid,&status,0);
	readcfg();
    switch(logouttype){
        case 0:
            ioctl(tty,VT_ACTIVATE,1);
            ioctl(tty,VT_WAITACTIVE,1);
            close(tty);
            break;
        case 1:
            goto startgreeter;
            break;
    }
	return 0;
}
