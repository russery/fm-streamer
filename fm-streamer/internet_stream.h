#ifndef __INTERNET_STREAM_H
#define __INTERNET_STREAM_H

#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

class InternetStream {
	static const uint AUDIO_SOURCE_BUFFER_BYTES = 10240;

public:
	InternetStream(const char* stream_url, uint buffer_size_bytes = AUDIO_SOURCE_BUFFER_BYTES);
	~InternetStream();

	bool DoStream(void);

private:
	AudioGeneratorMP3 *mp3_;
	AudioFileSourceICYStream *file_;
	AudioFileSourceBuffer *buff_;
	AudioOutputI2S *out_;
};


#endif // __INTERNET_STREAM_H