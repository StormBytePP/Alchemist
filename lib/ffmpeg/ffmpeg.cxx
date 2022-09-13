#include "ffmpeg.hxx"
#include "utils/logger.hxx"
#include "application.hxx"

#include <algorithm>
#include <unistd.h>

using namespace StormByte::VideoConvert;

FFmpeg::FFmpeg(const unsigned int& film_id, const std::filesystem::path& input_file, const std::optional<Database::Data::film::group>& group): m_film_id(film_id), m_input_file(input_file), m_group(group), m_container("mkv") {}

FFmpeg::FFmpeg(unsigned int&& film_id, std::filesystem::path&& input_file, std::optional<Database::Data::film::group>&& group): m_film_id(film_id), m_input_file(std::move(input_file)), m_group(std::move(group)), m_container("mkv") {}

void FFmpeg::add_stream(const Stream::Base& stream) {
	m_streams.push_back(stream.clone());
}
