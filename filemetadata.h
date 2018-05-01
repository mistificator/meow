#ifndef FILEMETADATA_H
#define FILEMETADATA_H

#include <QFile>

class FileMetadata
{
public:
    FileMetadata();
    virtual ~FileMetadata();

    bool setImage(QString filename);

    enum Tags
    {
        TagImageTitle = 0x0320,
        TagDocumentName = 0x010D,
        TagImageDescription = 0x010E,
        TagExifUserComment = 0x9286,
        TagXPKeywords = 0x9c9e
    };
    bool setField(int tag, QByteArray value); // see tags in https://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/EXIF.html
private:
    struct Data;
    Data * d;
};

#endif // FILEMETADATA_H
