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
        TagDocumentName = 0x010D,
        TagImageDescription = 0x010E,
        TagExifUserComment = 0x9286,

        StartXPTag = 0x9c9b,

        TagXPTitle = 0x9c9b,
        TagXPComment = 0x9c9c,
        TagXPAuthor = 0x9c9d,
        TagXPKeywords = 0x9c9e,
        TagXPSubject = 0x9c9f,

        EndXPTag = 0x9c9f
    };
    bool setField(int tag, QString value); // see tags in https://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/EXIF.html
private:
    struct Data;
    Data * d;
};

#endif // FILEMETADATA_H
