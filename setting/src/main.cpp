#include "main.h"
#include "../../dm/readcfg.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <QFont>
#include <QLineEdit>
#include <QMessageBox>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QFileDialog>
#include <KPluginFactory>
#include <KCModule>
#include <stdlib.h>
char bgimgpath[256];
char cfgcmd[128];
bool changebg=false,cfgchanged=false;
int logouttype=1,opacity=40,refraction=100,brightness=100,blur=5,edge=100,colorr=217,colorg=217,colorb=217,color=0xd9d9d9;
void checksave(){
    char cmd[512],bgcmd[300];
    bgimgpath[255]='\0';
    if(changebg){
        sprintf(bgcmd,"cp '%s' /var/lib/lgdm/bgimg",bgimgpath);
    }if(cfgchanged){
        writecfg();
    }
    if(changebg&&cfgchanged){
        sprintf(cmd,"pkexec sh -c \"cp '%s' /var/lib/lgdm/bgimg && %s\"",bgimgpath,cfgcmd);
    }else if(changebg){
        sprintf(cmd,"pkexec %s",bgcmd);
    }else if(cfgchanged){
        sprintf(cmd,"pkexec %s",cfgcmd);
    }
    system(cmd);
    changebg=false;
    cfgchanged=false;
}
void preview(){
    checksave();
    int pidpreview=fork();
    if(pidpreview==0){
        chdir("/var/lib/lgdm");
        char *argv[] = {"/var/lib/lgdm/greeter","--testmode",NULL};
        execv("/var/lib/lgdm/greeter",argv);
        exit(1);
    }
    int status;
    waitpid(pidpreview,&status,0);
}
void LGDMKCM::initslider(QHBoxLayout *box,QLabel *titlelab,QSlider *slider,QLabel *valuelab,int max,int *valueptr){
    box->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    box->setSpacing(20);
    titlelab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    slider->setFixedWidth(150);
    slider->setMinimum(0);
    slider->setMaximum(max);
    slider->setValue(*valueptr);
    slider->setOrientation(Qt::Horizontal);
    slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    char labtext[8];
    sprintf(labtext,"%d",*valueptr);
    valuelab->setText(labtext);
    valuelab->setMinimumWidth(30);
    valuelab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QObject::connect(slider, &QSlider::valueChanged, [=](int value){
        char valuetxt[5];
        sprintf(valuetxt,"%d",value);
        valuelab->setText(valuetxt);
        *valueptr=value;
        cfgchanged=true;
        setNeedsSave(true);
    });
    box->addWidget(titlelab);
    box->addWidget(slider);
    box->addWidget(valuelab);
}
LGDMKCM::LGDMKCM(QObject *parent, const KPluginMetaData &data)
: KCModule(qobject_cast<QWidget *>(parent), data)
{
    sprintf(cfgcmd,"mv '%s/.cache/lgdm.cfg' /var/lib/lgdm/config.cfg",getenv("HOME"));
    QFont title;
    title.setPointSize(25);
    readcfg();
    QWidget *root=widget();
    QVBoxLayout *body=new QVBoxLayout(root);
    body->setSpacing(50);
    body->setContentsMargins(0,0,0,0);
    body->setAlignment(Qt::AlignCenter);

    QVBoxLayout *upper=new QVBoxLayout;
    upper->setSpacing(20);
    upper->setAlignment(Qt::AlignVCenter);

    QLabel *uppertitle=new QLabel("登录界面行为设置");
    uppertitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    uppertitle->setFont(title);
    uppertitle->setAlignment(Qt::AlignCenter);
    upper->addWidget(uppertitle);

    QHBoxLayout *upper1=new QHBoxLayout;
    upper1->setAlignment(Qt::AlignVCenter);
    upper1->setSpacing(150);

    QVBoxLayout *left=new QVBoxLayout;
    left->setContentsMargins(0,0,0,0);
    left->setSpacing(20);
    left->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

    QLabel *img=new QLabel;
    QPixmap pix("/var/lib/lgdm/bgimg");
    pix = pix.scaled(300,300,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    img->setPixmap(pix);

    QPushButton *changebgbtn=new QPushButton("更改背景图片");
    connect(changebgbtn,&QPushButton::clicked,this,[=](){
        QString file=QFileDialog::getOpenFileName( widget(), "选择背景图片", "/", "Images (*.png *.jpg *.jpeg *.webp)");
        if(file.isEmpty()){
            return;
        }
        int pathlen=strlen(file.toStdString().data());
        if(pathlen>=255){
            QMessageBox::critical(nullptr, "文件路径过长", "文件路径超过256字节，为防止内存溢出，请更换图片");
            return;
        }
        changebg=true;
        memset(bgimgpath,0,sizeof(bgimgpath));
        strncpy(bgimgpath,file.toStdString().data(),sizeof(bgimgpath));
        QPixmap pix(file);
        pix = pix.scaled(300,300,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        img->setPixmap(pix);
        setNeedsSave(true);
    });

    left->addWidget(img);
    left->addWidget(changebgbtn);
    upper1->addLayout(left);

    QVBoxLayout *right=new QVBoxLayout;
    right->setContentsMargins(0,0,0,0);
    right->setSpacing(5);
    right->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

    QLabel *label=new QLabel("选择登出后行为");
    label->setAlignment(Qt::AlignCenter);

    QWidget *tty=new QWidget;
    QHBoxLayout *ttyLayout=new QHBoxLayout(tty);
    ttyLayout->setContentsMargins(0,0,0,0);
    QRadioButton *ttyRadio=new QRadioButton("切换到tty1");
    ttyLayout->addWidget(ttyRadio);

    QWidget *login=new QWidget;
    QHBoxLayout *loginLayout=new QHBoxLayout(login);
    loginLayout->setContentsMargins(0,0,0,0);
    QRadioButton *loginRadio=new QRadioButton("回到登录界面");
    loginLayout->addWidget(loginRadio);

    QButtonGroup *group=new QButtonGroup;
    group->addButton(ttyRadio,0);
    group->addButton(loginRadio,1);
    connect(group, &QButtonGroup::idClicked, this, [=](int id){
        if(id!=0&&id!=1){
            return;
        }
        logouttype=id;
        setNeedsSave(true);
        cfgchanged=true;
    });
    switch(logouttype){
        case 0:
            ttyRadio->setChecked(true);
            break;
        case 1:
            loginRadio->setChecked(true);
            break;
        default:break;
    }

    right->addWidget(label);
    right->addWidget(tty);
    right->addWidget(login);

    upper1->addLayout(right);
    upper->addLayout(upper1);
    body->addLayout(upper);

    QVBoxLayout *lower=new QVBoxLayout;
    lower->setSpacing(20);
    lower->setAlignment(Qt::AlignVCenter);

    QLabel *lowertitle=new QLabel("液态玻璃效果设置");
    lowertitle->setFont(title);
    lowertitle->setAlignment(Qt::AlignCenter);
    lowertitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    lower->addWidget(lowertitle);

    QHBoxLayout *lower1=new QHBoxLayout;
    lower1->setSpacing(150);
    lower1->setAlignment(Qt::AlignVCenter);
    QHBoxLayout *slider1=new QHBoxLayout;
    QLabel *opacitytitle=new QLabel("不透明度");
    QSlider *opacityslider=new QSlider;
    QLabel *opacityvalue=new QLabel("0");
    initslider(slider1,opacitytitle,opacityslider,opacityvalue,100,&opacity);

    QHBoxLayout *slider2=new QHBoxLayout;
    QLabel *refracttitle=new QLabel("折射强度");
    QSlider *refractslider=new QSlider;
    QLabel *refractvalue=new QLabel("0");
    initslider(slider2,refracttitle,refractslider,refractvalue,200,&refraction);
    lower1->addLayout(slider1);
    lower1->addLayout(slider2);
    lower->addLayout(lower1);

    QHBoxLayout *lower2=new QHBoxLayout;
    lower2->setSpacing(150);
    lower2->setAlignment(Qt::AlignVCenter);
    QHBoxLayout *slider3=new QHBoxLayout;
    QLabel *brighttitle=new QLabel("边缘亮度");
    QSlider *brightslider=new QSlider;
    QLabel *brightvalue=new QLabel("0");
    initslider(slider3,brighttitle,brightslider,brightvalue,200,&brightness);

    QHBoxLayout *slider4=new QHBoxLayout;
    QLabel *blurtitle=new QLabel("模糊强度");
    QSlider *blurslider=new QSlider;
    QLabel *blurvalue=new QLabel("0");
    initslider(slider4,blurtitle,blurslider,blurvalue,10,&blur);
    lower2->addLayout(slider3);
    lower2->addLayout(slider4);
    lower->addLayout(lower2);

    QHBoxLayout *lower3=new QHBoxLayout;
    lower3->setSpacing(150);
    lower3->setAlignment(Qt::AlignVCenter);
    QHBoxLayout *slider5=new QHBoxLayout;
    QLabel *edgetitle=new QLabel("边缘宽度");
    QSlider *edgeslider=new QSlider;
    QLabel *edgevalue=new QLabel("0");
    initslider(slider5,edgetitle,edgeslider,edgevalue,200,&edge);

    QHBoxLayout *colorbox=new QHBoxLayout;
    colorbox->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    colorbox->setSpacing(20);
    QLabel *colortitle=new QLabel("玻璃颜色");
    colortitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QLineEdit *colorinp=new QLineEdit;
    colorinp->setFixedWidth(180);
    colorinp->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    char colortext[10];
    sprintf(colortext,"%06x",color);
    colorinp->setText(colortext);
    connect(colorinp,&QLineEdit::textChanged,this,[=](const QString &qtext){
        char text[10];
        strncpy(text,qtext.toStdString().data(),sizeof(text));
        if(strlen(text)<6){
            return;
        }else if(strlen(text)>6){
            text[6]='\0';
            colorinp->setText(text);
            return;
        }
        for(int i=0;i<6;i++){
            char t=text[i];
            if(t!='1'&&t!='2'&&t!='3'&&t!='4'&&t!='5'&&t!='6'&&t!='7'&&t!='8'&&t!='9'&&t!='0'&&t!='a'&&t!='b'&&t!='c'&&t!='d'&&t!='e'&&t!='f'){
                return;
            }
        }
        color=strtol(text,0,16);
        cfgchanged=true;
        setNeedsSave(true);
    });
    colorbox->addWidget(colortitle);
    colorbox->addWidget(colorinp);

    lower3->addLayout(slider5);
    lower3->addLayout(colorbox);
    lower->addLayout(lower3);

    body->addLayout(lower);
    QPushButton *previewbtn=new QPushButton("预览并应用设置");
    connect(previewbtn,&QPushButton::clicked,this,[=](){
        preview();
        setNeedsSave(false);
    });
    body->addWidget(previewbtn);
}
void LGDMKCM::save(){
    checksave();
    setNeedsSave(false);
}

K_PLUGIN_CLASS_WITH_JSON(LGDMKCM, "../metadata.json")
#include "main.moc"
