/*
    Copyright 2011 Sebastian Kügler <sebas@kde.org>
    Copyright 2010 Richard Moore <rich@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "webcreator.h"

#include <time.h>

#include <KDebug>
#include <KUrl>
#include <KWebPage>

#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QTimer>
#include <QUrl>
#include <QWebPage>
#include <QWebFrame>
/*
class WebCreatorPrivate
{
public:
    WebCreatorPrivate() :
        page(0)
    {
        kDebug() << " === PRIVATE";
    }

    QWebPage *page;
    QImage thumbnail;
    QSize size;
    QUrl url;
    QEventLoop eventLoop;

};
*/
extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new WebCreator;
    }
}

WebCreator::WebCreator()
    : m_page(0)
{
}

WebCreator::~WebCreator()
{
    delete m_page;
}

bool WebCreator::create(const QString &path, int width, int height, QImage &img)
{
    kDebug() << "WEBCREATOR URL: " << path << width << "x" << height;
    if (!m_page) {
        //m_url = QUrl("http://lwn.net");
        m_url = QUrl(path);
        kDebug() << " === NEW PAGE";
        m_page = new QWebPage();
        kDebug() << " === SIZE";
        m_page->setViewportSize(QSize(width, height));
        m_page->mainFrame()->setScrollBarPolicy( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
        m_page->mainFrame()->setScrollBarPolicy( Qt::Vertical, Qt::ScrollBarAlwaysOff );
        qreal zoomFactor = (qreal)width / 800.0; // assume pages render well enough at 600px width
        kDebug() << "ZoOOOMFACTOR: " << zoomFactor;
        m_page->mainFrame()->setZoomFactor(zoomFactor);
        kDebug() << " === LOAD:  " << m_url;
        m_page->mainFrame()->load( m_url );
        connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(completed(bool)));

        /*
        m_html = new KHTMLPart;
        connect(m_html, SIGNAL(completed()), SLOT(slotCompleted()));
        m_html->setJScriptEnabled(false);
        m_html->setJavaEnabled(false);
        m_html->setPluginsEnabled(false);
        m_html->setMetaRefreshEnabled(false);
        m_html->setOnlyLocalReferences(true);
        */
    }

    // 30 sec timeout
    int t = startTimer(30000);
    m_failed = false;

    // The event loop will either be terminated by the page loadFinished(),
    // or by the timeout
    m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    killTimer(t);

    if (m_failed) {
        return false;
    }

    // Render QImage from WebPage
    QPixmap pix(width, height);
    pix.fill( Qt::transparent );

    kDebug() <<  " ++++++++++++++ NOW PAINTING ++++++++++++" << m_page->mainFrame()->contentsSize() << pix.size();
    // render and rescale
    QPainter p;
    p.begin(&pix);

    m_page->setViewportSize(m_page->mainFrame()->contentsSize());
    m_page->mainFrame()->render(&p);

    //p.setPen(Qt::blue);
    //p.setFont(QFont("Arial", 30));
    //p.drawText(QRect(0, 0, width, height), Qt::AlignCenter, "Poep.");
    p.end();

    kDebug() <<  " ++++++++++++++ DONE PAINTING ++++++++++++";

    delete m_page;
    m_page = 0;
    //delete d;
    //d = 0;

    //img = img.scaled(d->size, Qt::KeepAspectRatioByExpanding,
    //                          Qt::SmoothTransformation);
    kDebug() <<  " ++++++++++++++ toImage ++++++++++++";

    pix.save("/tmp/webcreatorpreviewpix.png");
    img = pix.toImage();
    kDebug() << "SAVING ........";
    img.save("/tmp/webcreatorpreview.png");

    /*
    // render the HTML page on a bigger pixmap and use smoothScale,
    // looks better than directly scaling with the QPainter (malte)
    QPixmap pix;
    if (width > 400 || height > 600)
    {
        if (height * 3 > width * 4)
            pix = QPixmap(width, width * 4 / 3);
        else
            pix = QPixmap(height * 3 / 4, height);
    }
    else
        pix = QPixmap(400, 600);

    // light-grey background, in case loadind the page failed
    pix.fill( QColor( 245, 245, 245 ) );

    int borderX = pix.width() / width,
        borderY = pix.height() / height;
    QRect rc(borderX, borderY, pix.width() - borderX * 2, pix.height() - borderY * 2);

    QPainter p;
    p.begin(&pix);
    m_html->paint(&p, rc);
    p.end();

    img = pix.toImage();
    m_html->closeUrl();
    */

    return true;
}

void WebCreator::timerEvent(QTimerEvent *)
{
    kDebug() << " WEBCREATOR:timerEvent";
    m_failed = true;
    m_eventLoop.quit();
}

void WebCreator::completed(bool sucess)
{
    kDebug() << " WEBCREATOR:completed" << sucess;
    m_failed = !sucess;
    m_eventLoop.quit();
}

ThumbCreator::Flags WebCreator::flags() const
{
    return DrawFrame;
}

#include "webcreator.moc"

