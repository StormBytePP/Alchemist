#include "opus.hxx"

using namespace StormByte::VideoConvert;

const std::string Stream::Audio::Opus::OPUS_DEFAULT_ENCODER	= "libopus";
const int Stream::Audio::Opus::OPUS_DEFAULT_BITRATE_PER_CHANNEL = 128;

Stream::Audio::Opus::Opus(const unsigned short& stream_id):Stream::Audio::Base(stream_id, OPUS_DEFAULT_ENCODER, Database::Data::film::stream::AUDIO_OPUS) {}

Stream::Audio::Opus::Opus(unsigned short&& stream_id):Stream::Audio::Base(std::move(stream_id), OPUS_DEFAULT_ENCODER, Database::Data::film::stream::AUDIO_OPUS) {}

void Stream::Audio::Opus::SetChannels(const int& channels) {
  Stream::Audio::Base::SetChannels(channels);
  // This is only a temp solution
  m_bitrate = std::to_string(OPUS_DEFAULT_BITRATE_PER_CHANNEL * *m_channels) + "K";
}

std::list<std::string> Stream::Audio::Opus::ffmpeg_parameters() const {
  std::list<std::string> result = Stream::Audio::Base::ffmpeg_parameters();

  if (m_channels) {
		result.push_back("-ac:" + ffmpeg_stream_pos());
		result.push_back(std::to_string(*m_channels));

    if (*m_channels >= 6) {
      result.push_back("-filter:" + ffmpeg_stream_pos());
      result.push_back("channelmap=channel_layout=" + std::to_string(*m_channels - 1) + ".1");
    }
	}
  
  return result;
}