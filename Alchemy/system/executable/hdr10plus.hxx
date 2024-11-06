#pragma once

#include <StormByte/system/executable.hxx>

#include <memory>

namespace Alchemist::System {
	class ALCHEMY_PRIVATE HDR10Plus final: public StormByte::System::Executable {
		public:
			HDR10Plus(const HDR10Plus&)				= delete;
			HDR10Plus(HDR10Plus&&)					= delete;
			HDR10Plus& operator=(const HDR10Plus&)	= delete;
			HDR10Plus& operator=(HDR10Plus&&)		= delete;
			~HDR10Plus() noexcept					= default;

			static std::unique_ptr<HDR10Plus> hdr_plus_detect();

		private:
			HDR10Plus(const std::filesystem::path&, std::vector<std::string>&&);
	};
}