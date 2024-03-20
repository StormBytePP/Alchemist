#pragma once

#include "../subtitle.hxx"

namespace Alchemist::Media::Codec::Subtitle {
	class DLL_PUBLIC Copy final: public Base {
		public:
			Copy();

			std::list<Decoder::Type> get_available_decoders() const;
			std::list<Encoder::Type> get_available_encoders() const;
	};
}