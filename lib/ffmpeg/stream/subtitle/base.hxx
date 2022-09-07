#pragma once

#include "../base.hxx"

namespace StormByte::VideoConvert::Stream::Subtitle {
	class Base: public StormByte::VideoConvert::Stream::Base {
		public:
			Base(unsigned short stream_id, const std::string& encoder);
			Base(const Base& base);
			Base(Base&& base) noexcept;
			Base& operator=(const Base& base);
			Base& operator=(Base&& base) noexcept;
			virtual ~Base() = default;
	};
}