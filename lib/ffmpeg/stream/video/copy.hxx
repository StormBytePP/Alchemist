#pragma once

#include "base.hxx"

namespace StormByte::VideoConvert::Stream::Video {
	class Copy: public Base {
		public:
			Copy(unsigned short stream_id);
			Copy(const Copy& copy);
			Copy(Copy&& copy) = default;
			Copy& operator=(const Copy& copy);
			~Copy() = default;
			StormByte::VideoConvert::Stream::Base* copy() const;

			std::list<std::string> ffmpeg_parameters() const;
	};
}