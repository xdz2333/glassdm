#pragma once

#include <KCModule>
#include <QLabel>
#include <QSlider>
#include <QHBoxLayout>

class LGDMKCM : public KCModule
{
    Q_OBJECT

    public:
        LGDMKCM( QObject *parent, const KPluginMetaData &data);
        void initslider(QHBoxLayout *box,QLabel *titlelab,QSlider *slider,QLabel *valuelab,int max,int *valueptr);
    protected:
        void save() override;
};
