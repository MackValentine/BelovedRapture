/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _EASYRPG_AUDIO_DECODER_LIBSNDFILE_H_
#define _EASYRPG_AUDIO_DECODER_LIBSNDFILE_H_
#ifdef HAVE_LIBSNDFILE
// Headers
#include "audio_decoder.h"
#include <string>
#include <sndfile.h>
#include <memory>

/**
 * Audio decoder for WAV powered by libsndfile
 */
class LibsndfileDecoder : public AudioDecoder {
public:
	LibsndfileDecoder();

	~LibsndfileDecoder();

	bool Open(FILE* file) override;

	bool Seek(size_t offset, Origin origin) override;

	bool IsFinished() const override;

	void GetFormat(int& frequency, AudioDecoder::Format& format, int& channels) const override;

	bool SetFormat(int frequency, AudioDecoder::Format format, int channels) override;

private:
	int FillBuffer(uint8_t* buffer, int length) override;
	Format output_format;
	FILE * file_;
	bool finished;

	SNDFILE *soundfile;
	SF_INFO soundinfo;
};
#endif
#endif
