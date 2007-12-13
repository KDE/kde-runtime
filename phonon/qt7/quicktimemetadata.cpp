/*  This file is part of the KDE project.

    Copyright (C) 2007 Trolltech ASA. All rights reserved.

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2.1 or 3 of the License.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "quicktimemetadata.h"
#include "backendheader.h"
#include <private/qcore_mac_p.h>

namespace Phonon
{
namespace QT7
{

QuickTimeMetaData::QuickTimeMetaData()
{
    m_movieRef = 0;
    m_movieChanged = false;
}

QuickTimeMetaData::~QuickTimeMetaData()
{
}

void QuickTimeMetaData::setVideo(Movie movieRef)
{
    m_movieRef = movieRef;
    m_movieChanged = true;
}

QString QuickTimeMetaData::stripCopyRightSymbol(const QString &key)
{
    return key.right(key.length()-1);
}

QString QuickTimeMetaData::convertQuickTimeKeyToUserKey(const QString &key)
{
    if (key == QLatin1String("com.apple.quicktime.displayname"))
        return QLatin1String("nam");
    else if (key == QLatin1String("com.apple.quicktime.album"))
        return QLatin1String("alb");
    else if (key == QLatin1String("com.apple.quicktime.artist"))
        return QLatin1String("ART");
    else
        return QLatin1String("???");
}

OSStatus QuickTimeMetaData::readMetaValue(QTMetaDataRef metaDataRef, QTMetaDataItem item, QTPropertyClass propClass,
    QTPropertyID id, QTPropertyValuePtr *value, ByteCount *size)
{
	QTPropertyValueType type;
	ByteCount propSize;
	UInt32 propFlags;
	OSStatus err = QTMetaDataGetItemPropertyInfo(metaDataRef, item, propClass, id, &type, &propSize, &propFlags);
    BACKEND_ASSERT3(err == noErr, "Could not read meta data value size", NORMAL_ERROR, err)

    *value = malloc(propSize);

	err = QTMetaDataGetItemProperty(metaDataRef, item, propClass, id, propSize, *value, size);
    BACKEND_ASSERT3(err == noErr, "Could not read meta data value", NORMAL_ERROR, err)

    if (type == 'code' || type == 'itsk' || type == 'itlk') {
        // convert from native endian to big endian
    	OSTypePtr pType = (OSTypePtr)*value;
    	*pType = EndianU32_NtoB(*pType);
    }

	return err;
}

UInt32 QuickTimeMetaData::getMetaType(QTMetaDataRef metaDataRef, QTMetaDataItem item)
{
	QTPropertyValuePtr value = 0;
	ByteCount ignore = 0;
	OSStatus err = readMetaValue(
	    metaDataRef, item, kPropertyClass_MetaDataItem, kQTMetaDataItemPropertyID_DataType, &value, &ignore);
    BACKEND_ASSERT3(err == noErr, "Could not read meta data type", NORMAL_ERROR, 0)
    UInt32 type = *((UInt32 *) value);
    if (value)
	    free(value);
	return type;
}

QString QuickTimeMetaData::getMetaValue(QTMetaDataRef metaDataRef, QTMetaDataItem item, SInt32 id)
{
	QTPropertyValuePtr value = 0;
	ByteCount size = 0;
	OSStatus err = readMetaValue(metaDataRef, item, kPropertyClass_MetaDataItem, id, &value, &size);
    BACKEND_ASSERT3(err == noErr, "Could not read meta data item", NORMAL_ERROR, QString())
    BACKEND_ASSERT3(value != 0, "Could not read meta data item", NORMAL_ERROR, QString())

    QString string;
    UInt32 dataType = getMetaType(metaDataRef, item);
    switch (dataType){
    case kQTMetaDataTypeUTF8:
    case kQTMetaDataTypeMacEncodedText:
        string = QCFString::toQString(CFStringCreateWithBytes(0, (UInt8*)value, size, kCFStringEncodingUTF8, false));
        break;
    case kQTMetaDataTypeUTF16BE:
        string = QCFString::toQString(CFStringCreateWithBytes(0, (UInt8*)value, size, kCFStringEncodingUTF16BE, false));
        break;
    default:
        break;
    }

    if (value)
	    free(value);
    return string;
}

void QuickTimeMetaData::readFormattedData(QTMetaDataRef metaDataRef, OSType format, QMultiMap<QString, QString> &result)
{
	QTMetaDataItem item = kQTMetaDataItemUninitialized;
    OSStatus err = QTMetaDataGetNextItem(metaDataRef, format, item, kQTMetaDataKeyFormatWildcard, 0, 0, &item);
	while (err == noErr){
        QString key = getMetaValue(metaDataRef, item, kQTMetaDataItemPropertyID_Key);
        if (format == kQTMetaDataStorageFormatQuickTime)
            key = convertQuickTimeKeyToUserKey(key);
        else
            key = stripCopyRightSymbol(key);

	    if (!result.contains(key)){
	        QString val = getMetaValue(metaDataRef, item, kQTMetaDataItemPropertyID_Value);
            result.insert(key, val);
        }
        err = QTMetaDataGetNextItem(metaDataRef, format, item, kQTMetaDataKeyFormatWildcard, 0, 0, &item);
	}
}

void QuickTimeMetaData::readMetaData()
{
	QTMetaDataRef metaDataRef;
	OSStatus err = QTCopyMovieMetaData(m_movieRef, &metaDataRef);
    BACKEND_ASSERT2(err == noErr, "Could not QuickTime meta data", NORMAL_ERROR)

    QMultiMap<QString, QString> metaMap;
    readFormattedData(metaDataRef, kQTMetaDataStorageFormatUserData, metaMap);
    readFormattedData(metaDataRef, kQTMetaDataStorageFormatQuickTime, metaMap);
    readFormattedData(metaDataRef, kQTMetaDataStorageFormatiTunes, metaMap);

    m_metaData.clear();
    m_metaData.insert("ARTIST", metaMap.value("ART"));
    m_metaData.insert("ALBUM", metaMap.value("alb"));
    m_metaData.insert("TITLE", metaMap.value("nam"));
    m_metaData.insert("DATE", metaMap.value("day"));
    m_metaData.insert("GENRE", metaMap.value("gnre"));
    m_metaData.insert("TRACKNUMBER", metaMap.value("trk"));
    m_metaData.insert("DESCRIPTION", metaMap.value("des"));
}

QMultiMap<QString, QString> QuickTimeMetaData::metaData()
{
    if (m_movieRef && m_movieChanged)
        readMetaData();
    return m_metaData;
}

}}
