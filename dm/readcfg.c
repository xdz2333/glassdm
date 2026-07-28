#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define contif(val) if(val){continue;}
extern int logouttype,opacity,refraction,brightness,blur,edge,colorr,colorg,colorb,color;
char linebuf[64];
bool compareline(const char *strin,int *outptr,int base){
    if(!strncmp(linebuf,strin,strlen(strin))){
        *outptr=strtol(linebuf+strlen(strin),NULL,base);
        return true;
    }
    return false;
}
void readcfg(){
    FILE *cfg=fopen("/var/lib/lgdm/config.cfg","r");
    if(!cfg){
        return;
    }
    while(fgets(linebuf,sizeof(linebuf),cfg)){
        contif(compareline("logouttype=",&logouttype,10));
        contif(compareline("opacity=",&opacity,10));
        contif(compareline("refraction=",&refraction,10));
        contif(compareline("brightness=",&brightness,10));
        contif(compareline("blur=",&blur,10));
        contif(compareline("edge=",&edge,10));
        contif(compareline("color=",&color,16));
    }
    fclose(cfg);
    colorr=(color&0xff0000)>>16;
    colorg=(color&0x00ff00)>>8;
    colorb=color&0x0000ff;
}
void writecfg(){
    char cfgpath[128];
    sprintf(cfgpath,"%s/.cache/lgdm.cfg",getenv("HOME"));
    FILE *cfg=fopen(cfgpath,"w");
    fprintf(cfg,"logouttype=%d\n",logouttype);
    fprintf(cfg,"opacity=%d\n",opacity);
    fprintf(cfg,"refraction=%d\n",refraction);
    fprintf(cfg,"brightness=%d\n",brightness);
    fprintf(cfg,"blur=%d\n",blur);
    fprintf(cfg,"edge=%d\n",edge);
    fprintf(cfg,"color=%06x\n",color);
    fclose(cfg);
}
