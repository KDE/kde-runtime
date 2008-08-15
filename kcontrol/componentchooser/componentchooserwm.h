/***************************************************************************
                          componentchooserwm.h  -  description
                             -------------------
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundationi                            *
 *                                                                         *
 ***************************************************************************/

#ifndef _COMPONENTCHOOSERWM_H_
#define _COMPONENTCHOOSERWM_H_

#include "ui_wmconfig_ui.h"
#include "componentchooser.h"
class KConfig;
class CfgPlugin;

class WmConfig_UI : public QWidget, public Ui::WmConfig_UI
{
public:
  WmConfig_UI( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class CfgWm: public WmConfig_UI,public CfgPlugin
{
Q_OBJECT
public:
    CfgWm(QWidget *parent);
    virtual ~CfgWm();
    virtual void load(KConfig *cfg);
    virtual void save(KConfig *cfg);
    virtual void defaults();

protected Q_SLOTS:
    void configChanged();

Q_SIGNALS:
    void changed(bool);
private:
    void loadWMs( const QString& current );
    QString currentWM() const;
    QHash< QString, QString > wms; // i18n text -> internal name
    QString oldwm; // the original value
};

#endif
