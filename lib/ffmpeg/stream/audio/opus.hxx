#pragma once

#include "base.hxx"

namespace StormByte::VideoConvert::Stream::Audio {
	class Opus: public Base {
		public:
			Opus(const unsigned short& stream_id);
			Opus(unsigned short&& stream_id);
			Opus(const Opus& opus) = default;
			Opus(Opus&& opus) noexcept = default;
			Opus& operator=(const Opus& opus) = default;
			Opus& operator=(Opus&& opus) noexcept = default;
			~Opus() = default;

			std::list<std::string> ffmpeg_parameters() const override;

		private:
			static const std::string OPUS_DEFAULT_ENCODER;

			inline Opus* copy() const override { return new Opus(*this); }
	};
}
