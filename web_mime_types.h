#ifndef WEB_MIME_TYPES_H
#define WEB_MIME_TYPES_H

#include <QList>
#include <QStringList>

namespace Meow {
// from https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Complete_list_of_MIME_types
static const QStringList WebMimeList = QStringList() <<
".aac" << "AAC audio" << "audio/aac" <<
".abw" << "AbiWord document" << "application/x-abiword" <<
".arc" << "Archive document (multiple files embedded)" << "application/octet-stream" <<
".avi" << "AVI: Audio Video Interleave" << "video/x-msvideo" <<
".azw" << "Amazon Kindle eBook format" << "application/vnd.amazon.ebook" <<
".bin" << "Any kind of binary data" << "application/octet-stream" <<
".bz" << "BZip archive" << "application/x-bzip" <<
".bz2" << "BZip2 archive" << "application/x-bzip2" <<
".csh" << "C-Shell script" << "application/x-csh" <<
".css" << "Cascading Style Sheets (CSS)" << "text/css" <<
".csv" << "Comma-separated values (CSV)" << "text/csv" <<
".doc" << "Microsoft Word" << "application/msword" <<
".docx" << "Microsoft Word (OpenXML)" << "application/vnd.openxmlformats-officedocument.wordprocessingml.document" <<
".eot" << "MS Embedded OpenType fonts" << "application/vnd.ms-fontobject" <<
".epub" << "Electronic publication (EPUB)" << "application/epub+zip" <<
".es" << "ECMAScript (IANA Specification) (RFC 4329 Section 8.2)" << "application/ecmascript" <<
".gif" << "Graphics Interchange Format (GIF)" << "image/gif" <<
".html" << "HyperText Markup Language (HTML)" << "text/html" <<
".htm" << "HyperText Markup Language (HTML)" << "text/html" <<
".ico" << "Icon format" << "image/x-icon" <<
".ics" << "iCalendar format" << "text/calendar" <<
".jar" << "Java Archive (JAR)" << "application/java-archive" <<
".jpg" << "JPEG images" << "image/jpeg" <<
".jpeg" << "JPEG images" << "image/jpeg" <<
".js" << "JavaScript (IANA Specification) (RFC 4329 Section 8.2)" << "application/javascript" <<
".json" << "JSON format" << "application/json" <<
".mid" << "Musical Instrument Digital Interface (MIDI)" << "audio/midi" <<
".midi" << "Musical Instrument Digital Interface (MIDI)" << "audio/midi" <<
".mpeg" << "MPEG Video" << "video/mpeg" <<
".mpkg" << "Apple Installer Package" << "application/vnd.apple.installer+xml" <<
".odp" << "OpenDocument presentation document" << "application/vnd.oasis.opendocument.presentation" <<
".ods" << "OpenDocument spreadsheet document" << "application/vnd.oasis.opendocument.spreadsheet" <<
".odt" << "OpenDocument text document" << "application/vnd.oasis.opendocument.text" <<
".oga" << "OGG audio" << "audio/ogg" <<
".ogv" << "OGG video" << "video/ogg" <<
".ogx" << "OGG" << "application/ogg" <<
".otf" << "OpenType font" << "font/otf" <<
".png" << "Portable Network Graphics" << "image/png" <<
".pdf" << "Adobe Portable Document Format (PDF)" << "application/pdf" <<
".ppt" << "Microsoft PowerPoint" << "application/vnd.ms-powerpoint" <<
".pptx" << "Microsoft PowerPoint (OpenXML)" << "application/vnd.openxmlformats-officedocument.presentationml.presentation" <<
".rar" << "RAR archive" << "application/x-rar-compressed" <<
".rtf" << "Rich Text Format (RTF)" << "application/rtf" <<
".sh" << "Bourne shell script" << "application/x-sh" <<
".svg" << "Scalable Vector Graphics (SVG)" << "image/svg+xml" <<
".swf" << "Small web format (SWF) or Adobe Flash document" << "application/x-shockwave-flash" <<
".tar" << "Tape Archive (TAR)" << "application/x-tar" <<
".tif" << "Tagged Image File Format (TIFF)" << "image/tiff" <<
".tiff" << "Tagged Image File Format (TIFF)" << "image/tiff" <<
".ts" << "Typescript file" << "application/typescript" <<
".ttf" << "TrueType Font" << "font/ttf" <<
".vsd" << "Microsoft Visio" << "application/vnd.visio" <<
".wav" << "Waveform Audio Format" << "audio/wav" <<
".weba" << "WEBM audio" << "audio/webm" <<
".webm" << "WEBM video" << "video/webm" <<
".webp" << "WEBP image" << "image/webp" <<
".woff" << "Web Open Font Format (WOFF)" << "font/woff" <<
".woff2" << "Web Open Font Format (WOFF)" << "font/woff2" <<
".xhtml" << "XHTML" << "application/xhtml+xml" <<
".xls" << "Microsoft Excel" << "application/vnd.ms-excel" <<
".xlsx" << "Microsoft Excel (OpenXML)" << "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" <<
".xml" << "XML" << "application/xml" <<
".xul" << "XUL" << "application/vnd.mozilla.xul+xml" <<
".zip" << "ZIP archive" << "application/zip" <<
".3gp" << "3GPP audio/video container" << "video/3gpp" <<
".3gp" << "3GPP audio/video container if it doesn't contain video" << "audio/3gpp" <<
".3g2" << "3GPP2 audio/video container" << "video/3gpp2" <<
".3gp" << "3GPP audio/video container if it doesn't contain video" << "audio/3gpp2" <<
".7z" << "7-zip archive" << "application/x-7z-compressed";

struct WebMimeStruct
{
    QString suffix;
    QString description;
    QString content_type;
};

static const QList<WebMimeStruct> & webMimeList()
{
    static QList<WebMimeStruct> list;
    if (list.isEmpty())
    {
        for (int i = 0 ; i < WebMimeList.count(); i+=3)
        {
            WebMimeStruct mime;
            mime.suffix = WebMimeList.at(i);
            mime.description = WebMimeList.at(i + 1);
            mime.content_type = WebMimeList.at(i + 2);
            list << mime;
        }
    }
    return list;
}

static QString adjustedContentType(QString content_type)
{
    if (content_type.contains(";"))
    {
        content_type = content_type.left(content_type.indexOf(";"));
    }
    return content_type;
}

static QString suffixForContentType(QString content_type)
{
    content_type = adjustedContentType(content_type);
    foreach (const WebMimeStruct & mime, webMimeList())
    {
        if (mime.content_type.compare(content_type, Qt::CaseInsensitive) == 0)
        {
            return mime.suffix;
        }
    }
    return QString();
}

static QString descriptionForContentType(QString content_type)
{
    foreach (const WebMimeStruct & mime, webMimeList())
    {
        if (mime.content_type.compare(content_type, Qt::CaseInsensitive) == 0)
        {
            return mime.description;
        }
    }
    return QString();
}

static QString contentTypeForSuffix(QString suffix)
{
    if (!suffix.startsWith("."))
    {
        suffix = "." + suffix;
    }
    foreach (const WebMimeStruct & mime, webMimeList())
    {
        if (mime.suffix.compare(suffix, Qt::CaseInsensitive) == 0)
        {
            return mime.content_type;
        }
    }
    return QString();
}

} // end of namespace

#endif // WEB_MIME_TYPES_H
