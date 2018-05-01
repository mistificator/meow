#include "filemetadata.h"
#include <QImageReader>
#include <QDebug>

// based on https://msdn.microsoft.com/en-us/library/windows/desktop/ms533832(v=vs.85).aspx
#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

struct FileMetadata::Data
{
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    CLSID  clsid;
    QString filename;
    QString format;
    bool image_set_up;
    Data(): image_set_up(false)
    {
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    ~Data()
    {
        GdiplusShutdown(gdiplusToken);
    }
    // from https://msdn.microsoft.com/ru-ru/library/windows/desktop/ms533843%28v=vs.85%29.aspx
    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
    {
       UINT  num = 0;          // number of image encoders
       UINT  size = 0;         // size of the image encoder array in bytes

       ImageCodecInfo * pImageCodecInfo = NULL;

       GetImageEncodersSize(&num, &size);
       if(size == 0)
       {
          return -1;  // Failure
       }

       pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
       if(pImageCodecInfo == NULL)
       {
          return -1;  // Failure
       }
       GetImageEncoders(num, size, pImageCodecInfo);

       for(UINT j = 0; j < num; ++j)
       {
          if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
          {
             *pClsid = pImageCodecInfo[j].Clsid;
             free(pImageCodecInfo);
             return j;  // Success
          }
       }

       free(pImageCodecInfo);
       return -1;  // Failure
    }
    static int tagType(int tag)
    {
        switch (tag)
        {
        case FileMetadata::TagImageTitle:
        case FileMetadata::TagDocumentName:
        case FileMetadata::TagImageDescription:
        case FileMetadata::TagExifUserComment:
            return PropertyTagTypeASCII;
        case FileMetadata::TagXPKeywords:
            return PropertyTagTypeByte;
        }
        return PropertyTagTypeUndefined;
    }
};

FileMetadata::FileMetadata(): d(new Data())
{

}

FileMetadata::~FileMetadata()
{
    delete d;
}

bool FileMetadata::setImage(QString filename)
{
    d->filename = filename;
    d->format = "image/" + QImageReader(filename).format().toLower().replace("jpg", "jpeg");
    // Get the CLSID of encoder.
    d->image_set_up = Data::GetEncoderClsid((wchar_t *)d->format.utf16(), &d->clsid) >= 0;
    return d->image_set_up;
}

bool FileMetadata::setField(int tag, QByteArray value)
{
    if (!d->image_set_up)
    {
        return false;
    }
    if (tag == FileMetadata::TagXPKeywords)
    {
        value = QByteArray((char *)QString(value).utf16(), value.count() * sizeof(ushort)); // convert to utf16
    }
    const int count = value.count();
    char * propertyValue = new char[count + sizeof(ushort)];
    propertyValue[count] = 0; propertyValue[count + 1] = 0;
    memcpy(propertyValue, value.constData(), value.count());
    PropertyItem propertyItem;
    propertyItem.id = tag;
    propertyItem.type = Data::tagType(tag);
    propertyItem.length = propertyItem.type == PropertyTagTypeASCII ? count + 1 : count;
    propertyItem.value = propertyValue;
    Status stat;
    {
        Image image((wchar_t *)d->filename.utf16());
        stat = image.SetPropertyItem(& propertyItem);
        if (stat == Ok)
        {
            stat = image.Save((wchar_t *)(d->filename + "_copy").utf16(), &d->clsid); // we must save as copy, details https://msdn.microsoft.com/en-us/library/ms535407%28v=vs.85%29.aspx
            if (stat != Ok)
            {
                qDebug() << "Bitmap::Save failed" << stat;
            }
        }
        else
        {
            qDebug() << "Bitmap::SetPropertyItem failed" << stat;
        }
    }
    delete [] propertyValue;
    if (stat == Ok)
    {
        if (QFile::exists(d->filename))
        {
            if (QFile::remove(d->filename))
            {
                QFile::rename(d->filename + "_copy", d->filename);
            }
        }
    }
    return stat == Ok;
}
