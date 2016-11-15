/*
QImageIO Routines to read/write WebP images.

Copyright (c) 2012,2013 Martin Koller <kollix@aon.at>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>

#include "webp.h"
#include <webp/decode.h>
#include <webp/encode.h>

#include <QImage>
#include <QVariant>

//---------------------------------------------------------------------

WebPHandler::WebPHandler()
  : quality(75)
{
}

//---------------------------------------------------------------------

bool WebPHandler::canRead() const
{
    if (canRead(device())) {
        setFormat("webp");
        return true;
    }
    return false;
}

//---------------------------------------------------------------------

bool WebPHandler::read(QImage *retImage)
{
    QByteArray data = device()->readAll();

    WebPBitstreamFeatures features;
    VP8StatusCode ret = WebPGetFeatures(reinterpret_cast<const uint8_t*>(data.constData()), data.size(), &features);
    if ( ret != VP8_STATUS_OK ) {
        return false;
    }

    if ( features.has_alpha ) {
        *retImage = QImage(features.width, features.height, QImage::Format_ARGB32);
    } else {
        *retImage = QImage(features.width, features.height, QImage::Format_RGB32);
    }

    if ( retImage->isNull() ) {  // out of memory
        return false;
    }

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    if ( WebPDecodeARGBInto(reinterpret_cast<const uint8_t*>(data.constData()),
                            data.size(), reinterpret_cast<uint8_t*>(retImage->bits()),
                            retImage->byteCount(), retImage->bytesPerLine()) == 0 ) {
        return false;
    }
#else
    if ( WebPDecodeBGRAInto(reinterpret_cast<const uint8_t*>(data.constData()),
                            data.size(), reinterpret_cast<uint8_t*>(retImage->bits()),
                            retImage->byteCount(), retImage->bytesPerLine()) == 0 ) {
        return false;
    }
#endif

    return true;
}

//---------------------------------------------------------------------

bool WebPHandler::write(const QImage &image)
{
    // limitation in WebP
    if ( (image.height() > 16383) || (image.height() == 0) ||
         (image.width() > 16383)  || (image.width() == 0) )
        return false;

    uint8_t *imageData = new uint8_t[image.width() * image.height() * (3 + image.hasAlphaChannel())];

    size_t idx = 0;
    for (int y = 0; y < image.height(); y++) {
        const QRgb *scanline = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); x++) {
            imageData[idx++] = qRed(scanline[x]);
            imageData[idx++] = qGreen(scanline[x]);
            imageData[idx++] = qBlue(scanline[x]);

            if ( image.hasAlphaChannel() ) {
                imageData[idx++] = qAlpha(scanline[x]);
            }
        }
    }

    uint8_t *output = 0;
    size_t size;
    if ( image.hasAlphaChannel() ) {
        size = WebPEncodeRGBA(imageData, image.width(), image.height(), image.width() * 4, quality, &output);
    } else {
        size = WebPEncodeRGB(imageData, image.width(), image.height(), image.width() * 3, quality, &output);
    }
    delete [] imageData;

    if ( size == 0 ) {
        free(output);
        return false;
    }

    device()->write(reinterpret_cast<const char*>(output), size);
    free(output);

    return true;
}

//---------------------------------------------------------------------

QByteArray WebPHandler::format() const
{
    return "webp";
}

//---------------------------------------------------------------------

bool WebPHandler::supportsOption(ImageOption option) const
{
    return (option == Quality) || (option == Size);
}

//---------------------------------------------------------------------

QVariant WebPHandler::option(ImageOption option) const
{
    switch ( option )
    {
        case Quality:
            return quality;

        case Size: {
            QByteArray data = device()->peek(26);

            int width = 0, height = 0;

            if ( WebPGetInfo(reinterpret_cast<const uint8_t*>(data.constData()),
                             data.size(), &width, &height) == 0 )
                return QSize();  // header error

            return QSize(width, height);
        }

        default: return QVariant();
    }
}

//---------------------------------------------------------------------

void WebPHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == Quality) {
        if ( value.toInt() >= 0 )
            quality = qBound(0, value.toInt(), 100);
    }
}

//---------------------------------------------------------------------

bool WebPHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("WebPHandler::canRead() called with no device");
        return false;
    }

    // WebP file header: 4 bytes "RIFF", 4 bytes length, 4 bytes "WEBP"
    QByteArray header = device->peek(12);

    return (header.size() == 12) && header.startsWith("RIFF") && header.endsWith("WEBP");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

class WebPPlugin : public QImageIOPlugin
{
public:
    QStringList keys() const;
    Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};

//---------------------------------------------------------------------

QStringList WebPPlugin::keys() const
{
    return QStringList() << "webp";
}

//---------------------------------------------------------------------

QImageIOPlugin::Capabilities WebPPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "webp")
        return Capabilities(CanRead | CanWrite);
    if (!format.isEmpty())
        return 0;
    if (!device->isOpen())
        return 0;

    Capabilities cap;
    if (device->isReadable() && WebPHandler::canRead(device))
        cap |= CanRead;
    if (device->isWritable())
        cap |= CanWrite;
    return cap;
}

//---------------------------------------------------------------------

QImageIOHandler *WebPPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new WebPHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

//---------------------------------------------------------------------

Q_EXPORT_STATIC_PLUGIN(WebPPlugin)
Q_EXPORT_PLUGIN2(webp, WebPPlugin)
