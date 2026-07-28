const char *vs = "\n\
#version 430 core\n\
layout(location=0) in vec2 posin;\n\
uniform ivec2 texres;\n\
out vec2 pos;\n\
void main(){\n\
    gl_Position=vec4(posin.x,posin.y,1.0,1.0);\n\
    pos=posin*0.5+0.5;\n\
}\n\
";
const char *fsinit="\n\
#version 430 core\n\
in vec2 pos;\n\
layout(location=0) out vec4 fragcolor;\n\
int sobelx[3]={1,2,1};\n\
int sobely[3]={1,0,-1};\n\
uniform sampler2D source;\n\
uniform ivec2 texres;\n\
void main(){\n\
    float resultx=0.,resulty=0.;\n\
    for(int i=-1;i<=1;i++){\n\
        for(int j=-1;j<=1;j++){\n\
            float light=texture(source,pos+vec2(i,j)/texres).r;\n\
            //result+=abs(light*sobelx[i+1]*sobely[j+1])+abs(light*sobelx[j+1]*sobely[i+1]);\n\
            resultx+=light*sobelx[i+1]*sobely[j+1];\n\
            resulty+=light*sobely[i+1]*sobelx[j+1];\n\
        }\n\
    }\n\
    vec2 nearest=(abs(resultx)+abs(resulty))<4?vec2(100,100):pos;\n\
    fragcolor=vec4(nearest,step(texture(source,pos).r,.2)-.5,1.);\n\
}\n\
";
const char *fsjfa="\n\
#version 430 core\n\
in vec2 pos;\n\
out vec4 fragcolor;\n\
uniform sampler2D source;\n\
uniform ivec2 texres;\n\
uniform int ratio;\n\
void main(){\n\
    vec2 nearestold=texture(source,pos).rg;\n\
    vec2 nearestnew=nearestold;\n\
    float dist=distance(nearestold*texres,pos*texres);\n\
    for(int i=-1;i<=1;i++){\n\
        for(int j=-1;j<=1;j++){\n\
            vec2 uv2=pos+(vec2(i,j)*ratio)/texres;\n\
            uv2=clamp(uv2,0,1);\n\
            vec2 nearestget=texture(source,uv2).rg;\n\
            float distnew=distance(nearestget*texres,pos*texres);\n\
            if(distnew<dist){\n\
                nearestnew=nearestget;\n\
                dist=distnew;\n\
            }\n\
        }\n\
    }\n\
    fragcolor=vec4(nearestnew,sign(texture(source,pos).b)*dist,1.);\n\
}\n\
";
const char *fspost="\n\
#version 430 core\n\
in vec2 pos;\n\
out vec4 fragcolor;\n\
uniform sampler2D sdftex;\n\
uniform sampler2D bg;\n\
uniform sampler2D bgblur;\n\
uniform vec2 uvbg0;\n\
uniform vec2 uvbg1;\n\
uniform ivec2 scrsize;\n\
uniform ivec2 texsize;\n\
uniform float scrratio;\n\
uniform float opacity;\n\
uniform float refraction;\n\
uniform float brightness;\n\
uniform vec3 glasscolor;\n\
void main(){\n\
    vec2 uv=pos;\n\
    vec2 uvbg=uv;\n\
    //vec2 uvbg=mix(uvbg0,uvbg1,uv);\n\
    uv.y=1-uv.y;\n\
    vec2 st=uv*scrsize;\n\
    float sdf=texture(sdftex,uv).b-20.;\n\
    sdf=sdf*scrratio;\n\
    if(sdf>5.){\n\
        discard;\n\
    }\n\
    vec2 grad=vec2(0);//;texture(sdftex,uv+v);\n\
    for(int i=-1;i<=1;i+=2){\n\
        for(int j=-1;j<=1;j+=2){\n\
            float df=texture(sdftex,uv+vec2(i,j)/scrsize).b-2.-sdf;\n\
            grad+=.25*df/vec2(i,j)*scrsize;\n\
        }\n\
    }\n\
    grad=normalize(grad);\n\
    uvbg+=grad*0.08*refraction*pow( smoothstep(-12.0,0.0,sdf), 2.5)*step(sdf,0.);\n\
    uvbg+=sin(uvbg.x*50.0)*sin(uvbg.y*40.0)*0.003*step(sdf,2.);\n\
    uvbg=mix(uvbg0,uvbg1,uvbg);\n\
    vec3 bgcolor=mix(texture(bg,uvbg),texture(bgblur,uvbg),clamp(smoothstep(-16.,-2.,sdf),0.9,1)*step(sdf,0.)).rgb;\n\
    bgcolor=mix(bgcolor,glasscolor,step(sdf,0.)*opacity);\n\
    //vec3 light=smoothstep(5.,0.,sdf)*step(0.,sdf)*vec3(0.2);\n\
    vec3 light=smoothstep(5,0.,abs(sdf))*vec3(.25*brightness)*pow(abs(dot(vec2(0.707,0.707),grad)),2.5);\n\
    fragcolor=vec4(bgcolor+light*step(sdf,0),1.);\n\
}\n\
";
const char *fsbox="\n\
#version 430 core\n\
in vec2 pos;\n\
out vec4 fragcolor;\n\
uniform sampler2D bg;\n\
uniform sampler2D bgblur;\n\
uniform vec2 uvbg0;\n\
uniform vec2 uvbg1;\n\
uniform ivec2 scrsize;\n\
uniform float scrratio;\n\
uniform float edge;\n\
uniform float opacity;\n\
uniform float refraction;\n\
uniform float brightness;\n\
uniform vec3 glasscolor;\n\
void main(){\n\
    vec2 uv=pos;\n\
    vec2 uvbg=uv;\n\
    uv.y=1-uv.y;\n\
    vec2 st=uv*scrsize;\n\
    float sdf;\n\
    vec2 grad;\n\
    if(st.x<scrratio*35){\n\
        sdf=distance(st,scrratio*vec2(35));\n\
        grad=st-scrratio*vec2(35);\n\
    }else if(st.x<285*scrratio){\n\
        sdf=abs(st.y-scrsize.y/2);\n\
        grad=vec2(0,sign(st.y-scrsize.y/2));\n\
    }else{\n\
        sdf=distance(st,scrratio*vec2(285,35));\n\
        grad=st-scrratio*vec2(285,35);\
    }\n\
    sdf-=25*scrratio;\n\
    sdf/=scrratio*edge;\n\
    grad=normalize(grad);\n\
    if(sdf>5.){\n\
        discard;\n\
    }\n\
    uvbg+=grad*refraction*0.15*pow( smoothstep(-12.0,0.0,sdf), 2.5)*step(sdf,0.);\n\
    //uvbg+=sin(uvbg.x*50.0)*sin(uvbg.y*40.0)*0.003*step(sdf,2.);\n\
    uvbg=mix(uvbg0,uvbg1,uvbg);\n\
    vec3 bgcolor=mix(texture(bg,uvbg),texture(bgblur,uvbg),clamp(smoothstep(-16.,-2.,sdf),0.8,1)*step(sdf,0.)).rgb;\n\
    bgcolor=mix(bgcolor,glasscolor,step(sdf,0.)*opacity);\n\
    vec3 light=smoothstep(5,0.,abs(sdf))*vec3(.25*brightness)*pow(abs(dot(vec2(0.707,0.707),grad)),2.5);\n\
    fragcolor=vec4(bgcolor+light*step(sdf,0),1.);\n\
}\n\
";
const char *fsbg="\n\
#version 430 core\n\
in vec2 pos;\n\
out vec4 fragcolor;\n\
uniform sampler2D source;\n\
uniform int flipy;\n\
void main(){\n\
    vec2 uv=pos;\n\
    if(flipy==0){\n\
        uv.y=1.-uv.y;\n\
    }\n\
    fragcolor=texture(source,uv);\n\
}\n\
";
const char *fsblur="\n\
#version 430 core\n\
#define PI 3.1415926535\n\
#define SI 2.\n\
in vec2 pos;\n\
out vec4 fragcolor;\n\
uniform sampler2D bg;\n\
uniform vec2 vert;\n\
uniform ivec2 texsize;\n\
void main(){\n\
    vec3 color=vec3(0);\n\
    vec2 uv=pos;\n\
    //uv.y=1.-uv.y;\n\
    for(int i=-3;i<=3;i++){\n\
        color+=texture(bg,uv+vert*3.*i/texsize).rgb*(1./pow(2*PI,.5)/SI)*exp(-i*i/(2.*SI*SI));\n\
    }\n\
    //color/=7.;\n\
    color/=0.92;\n\
    fragcolor=vec4(color,1.0);\n\
}\n\
";
const char *fselement="\n\
#version 430 core\n\
in vec2 pos;\n\
out vec4 fragcolor;\n\
uniform sampler2D bgblur;\n\
uniform sampler2D ele;\n\
uniform vec4 color;\n\
uniform vec2 uvbg0;\n\
uniform vec2 uvbg1;\n\
void main(){\n\
    vec2 uv=pos;\n\
    uv.y=1.-uv.y;\n\
    vec2 uvbg=mix(uvbg0,uvbg1,pos);\n\
    float alpha=texture(ele,uv).r;\n\
    vec3 bgcolor=texture(bgblur,uvbg).rgb;\n\
    fragcolor=vec4(mix(bgcolor,color.rgb,alpha*color.a),step(0.01,alpha));\n\
}\n\
";
const char *fsavatar="\n\
#version 430 core\n\
in vec2 pos;\n\
out vec4 fragcolor;\n\
uniform sampler2D source;\n\
void main(){\n\
    vec2 uv=pos;\n\
    uv.y=1.-uv.y;\n\
    float radius=length(uv-.5);\n\
    fragcolor=vec4(texture(source,uv).rgb+step(.47,radius)*vec3(1),step(radius,.5));\n\
}\n\
";

