/*  This file is part of the KDE project
    Copyright (C) 2004-2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include <QtCore/QPair>
#include <QtOpenGL/QGLWidget>
#include <QtCore/QSignalMapper>
#include <QtGui/QAction>
#include <QtGui/QGraphicsProxyWidget>
#include <QtGui/QGraphicsView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include "mediaobjectitem.h"
#include "mygraphicsscene.h"
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include "audiooutputitem.h"
#include "videowidgetitem.h"
#include "pathitem.h"
#include "effectitem.h"
#include <Phonon/BackendCapabilities>

class MainWindow : public QMainWindow
{
    Q_OBJECT
    public:
        MainWindow();

    private slots:
        void init();
        void addMediaObject();
        void addEffect(int);
        void addAudioOutput();
        void addVideoWidget();

    private:
        QGraphicsView *m_view;
        MyGraphicsScene *m_scene;
};

/*
PathWidget::PathWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Raised);

    QVBoxLayout *layout = new QVBoxLayout(this);

    m_effectComboBox = new QComboBox(this);
    layout->addWidget(m_effectComboBox);
    QList<EffectDescription> effectList = BackendCapabilities::availableAudioEffects();
    m_effectComboBox->setModel(new AudioEffectDescriptionModel(effectList, m_effectComboBox));

    QPushButton *addButton = new QPushButton(this);
    layout->addWidget(addButton);
    addButton->setText("add effect");
    connect(addButton, SIGNAL(clicked()), SLOT(addEffect()));

    QPushButton *button = new QPushButton(this);
    layout->addWidget(button);
    button->setText("add VolumeFader");
    connect(button, SIGNAL(clicked()), SLOT(addVolumeFader()));
}

void PathWidget::addEffect()
{
    int current = m_effectComboBox->currentIndex();
    if (current < 0) {
        return;
    }
    QList<EffectDescription> effectList = BackendCapabilities::availableAudioEffects();
    if (current < effectList.size()) {
        Effect *effect = m_path.insertEffect(effectList[current]);
        QGroupBox *gb = new QGroupBox(effectList[current].name(), this);
        layout()->addWidget(gb);
        gb->setFlat(true);
        gb->setCheckable(true);
        gb->setChecked(true);
        (new QHBoxLayout(gb))->addWidget(new EffectWidget(effect, gb));
        gb->setProperty("AudioEffect", QVariant::fromValue(static_cast<QObject *>(effect)));
        connect(gb, SIGNAL(toggled(bool)), SLOT(effectToggled(bool)));
    }
}

void PathWidget::effectToggled(bool checked)
{
    if (checked) {
        return;
    }
    QVariant v = sender()->property("AudioEffect");
    if (!v.isValid()) {
        return;
    }
    QObject *effect = v.value<QObject *>();
    if (!effect) {
        return;
    }
    delete effect;
    sender()->deleteLater();
}

void PathWidget::addVolumeFader()
{
    VolumeFaderEffect *effect = new VolumeFaderEffect(this);
    QGroupBox *gb = new QGroupBox("VolumeFader", this);
    layout()->addWidget(gb);
    gb->setFlat(true);
    gb->setCheckable(true);
    gb->setChecked(true);
    (new QHBoxLayout(gb))->addWidget(new EffectWidget(effect, gb));
    m_path.insertEffect(effect);
    gb->setProperty("AudioEffect", QVariant::fromValue(static_cast<QObject *>(effect)));
    connect(gb, SIGNAL(toggled(bool)), SLOT(effectToggled(bool)));
}

bool PathWidget::connectOutput(OutputWidget *w)
{
    if (m_sink && m_path.isValid()) {
        m_path.disconnect();
    }
    m_sink = w->output();
    if (m_source) {
        m_path = createPath(m_source, m_sink);
        return m_path.isValid();
    }
    return true;
}

bool PathWidget::connectInput(MediaObject *m)
{
    m_source = m;
}
*/

MainWindow::MainWindow()
{
    m_scene = new MyGraphicsScene(this);
    QGLWidget *glWidget = new QGLWidget;
    QHBoxLayout *l = new QHBoxLayout(glWidget);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new QGraphicsView(m_scene, glWidget);
    l->addWidget(m_view);
    m_scene->setView(m_view);
    m_view->setRenderHints(QPainter::Antialiasing);
    setCentralWidget(glWidget);
    QAction *action;
    action = new QAction(i18n("add MediaObject"), m_view);
    connect(action, SIGNAL(triggered()), SLOT(addMediaObject()));
    m_view->addAction(action);

    action = new QAction(i18n("add Effect"), m_view);
    QMenu *menu = new QMenu("Title", m_view);
    QList<EffectDescription> effectList = Phonon::BackendCapabilities::availableAudioEffects();
    QSignalMapper *mapper = new QSignalMapper(menu);
    connect(mapper, SIGNAL(mapped(int)), SLOT(addEffect(int)));
    foreach (const EffectDescription &d, effectList) {
        QAction *subAction = menu->addAction(d.name(), mapper, SLOT(map()));
        mapper->setMapping(subAction, d.index());
    }
    action->setMenu(menu);
    m_view->addAction(action);

    action = new QAction(i18n("add AudioOutput"), m_view);
    connect(action, SIGNAL(triggered()), SLOT(addAudioOutput()));
    m_view->addAction(action);
    action = new QAction(i18n("add VideoWidget"), m_view);
    connect(action, SIGNAL(triggered()), SLOT(addVideoWidget()));
    m_view->addAction(action);
    m_view->setContextMenuPolicy(Qt::ActionsContextMenu);

    resize(800, 600);

    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

typedef QPair<MediaObjectItem *, WidgetRectItem *> MediaItemPair;
typedef QPair<AudioOutputItem *, WidgetRectItem *> AudioItemPair;

static MediaItemPair addMediaObject(const QPoint &pos, QGraphicsView *view, QGraphicsScene *scene)
{
    WidgetRectItem *rect = new WidgetRectItem(view->mapToScene(pos),
            QColor(255, 100, 100, 150), QLatin1String("Media Object"));
    scene->addItem(rect);
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(rect);
    MediaObjectItem *m = new MediaObjectItem;
    proxy->setWidget(m);
    proxy->setPos(16.0, 17.0);
    return MediaItemPair(m, rect);
}

static AudioItemPair addAudioOutput(const QPoint &pos, QGraphicsView *view, QGraphicsScene *scene)
{
    WidgetRectItem *rect = new WidgetRectItem(view->mapToScene(pos),
            QColor(100, 255, 100, 150), QLatin1String("Audio Output"));
    scene->addItem(rect);
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(rect);
    AudioOutputItem *a = new AudioOutputItem;
    proxy->setWidget(a);
    proxy->setPos(16.0, 17.0);
    return AudioItemPair(a, rect);
}

void MainWindow::init()
{
    MediaItemPair source = ::addMediaObject(QPoint(200, 50), m_view, m_scene);
    AudioItemPair sink = ::addAudioOutput(QPoint(700, 350), m_view, m_scene);
    Phonon::Path p = Phonon::createPath(source.first->mediaNode(), sink.first->mediaNode());
    if (p.isValid()) {
        m_scene->addItem(new PathItem(source.second, sink.second, p));
    }
}

void MainWindow::addMediaObject()
{
    ::addMediaObject(QCursor::pos(), m_view, m_scene);
}

void MainWindow::addEffect(int effectIndex)
{
    const EffectDescription &desc = EffectDescription::fromIndex(effectIndex);
    QGraphicsRectItem *rect = new WidgetRectItem(m_view->mapToScene(QCursor::pos()),
            QColor(255, 200, 0, 150), desc.name());
    m_scene->addItem(rect);
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(rect);
    proxy->setWidget(new EffectItem(desc));
    proxy->setPos(16.0, 17.0);
}

void MainWindow::addAudioOutput()
{
    ::addAudioOutput(QCursor::pos(), m_view, m_scene);
}

void MainWindow::addVideoWidget()
{
    QGraphicsRectItem *rect = new WidgetRectItem(m_view->mapToScene(QCursor::pos()),
            QColor(100, 100, 255, 150), "Video Widget");
    m_scene->addItem(rect);
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget(rect);
    proxy->setWidget(new VideoWidgetItem);
    proxy->setPos(16.0, 17.0);
}

int main(int argc, char **argv)
{
    KAboutData about("phonontester", 0, ki18n("KDE Multimedia Test"),
            "0.2", KLocalizedString(),
            KAboutData::License_LGPL);
    about.setProgramIconName("phonon");
    about.addAuthor(ki18n("Matthias Kretz"), KLocalizedString(), "kretz@kde.org");
    KCmdLineArgs::init(argc, argv, &about);
    KApplication app;
    MainWindow w;
    w.show();
    return app.exec();
}

#include "main.moc"

// vim: sw=4 ts=4
