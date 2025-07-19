#pragma once

#include "aes.h"
#include "cJSON.h"

#include <iostream>
#include <cstring>
#ifndef BOLDYELLOW
#define BOLDYELLOW "\033[1;33m"
#define RESET "\033[0m"
#endif

class NeteaseMusicMetadata {

private:
	std::string mAlbum;
	std::string mArtist;
	std::string mFormat;
	std::string mName;
	int mDuration;
	int mBitrate;

private:
	cJSON* mRaw;

public:
	NeteaseMusicMetadata(cJSON*);
	NeteaseMusicMetadata(const std::string& json_str);
	~NeteaseMusicMetadata();
    const std::string& name() const { return mName; }
    const std::string& album() const { return mAlbum; }
    const std::string& artist() const { return mArtist; }
    const std::string& format() const { return mFormat; }
    const int duration() const { return mDuration; }
    const int bitrate() const { return mBitrate; }

};

namespace TagLib {
	class ByteVectorStream;
}

class NeteaseCrypt {

private:
    static const unsigned char sCoreKey[17];
    static const unsigned char sModifyKey[17];
    static const unsigned char mPng[8];
    enum NcmFormat { MP3, FLAC };
	std::vector<uint8_t> mDecryptedAudioData;

private:
    NcmFormat mFormat;
    std::string mImageData;
    std::vector<char> mInputBuffer;
    size_t mReadPos;
    unsigned char mKeyBox[256]{};
    NeteaseMusicMetadata* mMetaData;

private:
    bool isNcmFileInMemory();
	bool seekInBuffer(std::streamsize offset, std::ios_base::seekdir way);
	void buildKeyBox(const unsigned char *key, int keyLen);
	std::string mimeType(std::string &data);
	int readFromBuffer(char *s, std::streamsize n);

public:
    NeteaseCrypt(const uint8_t* data, size_t size);
	~NeteaseCrypt();
	void DumpToMemory();
    void FixMetadata();
	const std::vector<uint8_t>& getDecryptedAudioData() const { return mDecryptedAudioData; };
};