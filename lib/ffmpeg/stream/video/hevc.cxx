#include "hevc.hxx"

using namespace StormByte::VideoConvert;

/******************************* HDR ************************************/
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_REDX			= 34000;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_REDY			= 16000;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_GREENX			= 13250;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_GREENY			= 34500;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_BLUEX			= 7500;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_BLUEY			= 3000;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_WHITEPOINTX	= 15635;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_WHITEPOINTY	= 16450;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_LUMINANCEMIN	= 1;
const unsigned int Stream::Video::HEVC::HDR::DEFAULT_LUMINANCEMAX	= 10000000;

Stream::Video::HEVC::HDR::HDR(	const unsigned int& red_x,			const unsigned int& red_y,
								const unsigned int& green_x,		const unsigned int& green_y,
								const unsigned int& blue_x,			const unsigned int& blue_y,
								const unsigned int& white_point_x,	const unsigned int& white_point_y,
								const unsigned int& luminance_min,	const unsigned int& luminance_max) {

	m_data = {
		red_x,					red_y,
		green_x,				green_y,
		blue_x,					blue_y,
		white_point_x,			white_point_y,
		luminance_min,			luminance_max,
		std::optional<std::pair<unsigned int, unsigned int>>()
	};
}

Stream::Video::HEVC::HDR::HDR(	const unsigned int& red_x,					const unsigned int& red_y,
								const unsigned int& green_x,				const unsigned int& green_y,
								const unsigned int& blue_x,					const unsigned int& blue_y,
								const unsigned int& white_point_x,			const unsigned int& white_point_y,
								const unsigned int& luminance_min,			const unsigned int& luminance_max,
								const unsigned int& light_level_content,	const unsigned int& light_level_average) {

	m_data = {
		red_x,					red_y,
		green_x,				green_y,
		blue_x,					blue_y,
		white_point_x,			white_point_y,
		luminance_min,			luminance_max,
		std::make_pair(light_level_content,	light_level_average)
	};
}

std::string Stream::Video::HEVC::HDR::ffmpeg_parameters() const {
	/* Warning:
		* There is an specific order for master-display which is G()B()R()WP()L()
		* Wrong order will cause libx265 to ignore HDR warning it!
	*/

	std::string result = "colorprim=bt2020:colormatrix=bt2020nc:transfer=smpte2084:master-display=";
	result += "G("	+ std::to_string(m_data.green_x) 		+ "," + std::to_string(m_data.green_y) 			+ ")";
	result += "B("	+ std::to_string(m_data.blue_x)			+ "," + std::to_string(m_data.blue_y) 			+ ")";
	result += "R("	+ std::to_string(m_data.red_x)			+ "," + std::to_string(m_data.red_y) 			+ ")";
	result += "WP("	+ std::to_string(m_data.white_point_x)	+ "," + std::to_string(m_data.white_point_y) 	+ ")";
	result += "L("	+ std::to_string(m_data.luminance_min)	+ "," + std::to_string(m_data.luminance_max) 	+ ")";

	if (m_data.light_level)
		result += ":max-cll=" + std::to_string(m_data.light_level->first) + "," + std::to_string(m_data.light_level->second);

	result += ":hdr10=1";

	return result;
}





/********************************* HEVC *************************************/
const std::string Stream::Video::HEVC::DEFAULT_BUFFSIZE 	= "200M";
const std::string Stream::Video::HEVC::X265_PARAMS 			= "level=5.1:ref=4:hme=1:hme-search=umh,umh,star:subme=4:bframes=8:rd=4:rd-refine=0:qcomp=0.65:fades=1:strong-intra-smoothing=1:ctu=32:qg-size=32:aq-mode=4:sao=1:selective-sao=4:rdoq-level=1:psy-rd=4.0:psy-rdoq=15.0:limit-modes=0:limit-refs=0:limit-tu=0:weightb=1:weightp=1:rect=1:amp=1:wpp=1:pmode=0:pme=0:b-intra=1:b-adapt=2:b-pyramid=1:vbv-bufsize=160000:vbv-maxrate=160000:log-level=error";
const Stream::Video::HEVC::HDR Stream::Video::HEVC::DEFAULT_HDR = HDR(HDR::DEFAULT_REDX, HDR::DEFAULT_REDY, HDR::DEFAULT_GREENX, HDR::DEFAULT_GREENY, HDR::DEFAULT_BLUEX, HDR::DEFAULT_BLUEY, HDR::DEFAULT_WHITEPOINTX, HDR::DEFAULT_WHITEPOINTY, HDR::DEFAULT_LUMINANCEMIN, HDR::DEFAULT_LUMINANCEMAX);

Stream::Video::HEVC::HEVC(const unsigned short& stream_id):Stream::Video::Base(stream_id, "libx265", Database::Data::film::stream::VIDEO_HEVC) { }

Stream::Video::HEVC::HEVC(unsigned short&& stream_id):Stream::Video::Base(std::move(stream_id), "libx265", Database::Data::film::stream::VIDEO_HEVC) { }


std::list<std::string> Stream::Video::HEVC::ffmpeg_parameters() const {
	std::list<std::string> result = Stream::Video::Base::ffmpeg_parameters();
	std::string x265_params = "\"" + X265_PARAMS + ":" + (m_is_animation ? "deblock=-2,-2" : "deblock=-4,-4");
	if (m_hdr)
		x265_params += ":" + m_hdr->ffmpeg_parameters();
	x265_params += "\"";

	result.push_back("-profile:"		+ ffmpeg_stream_id());		result.push_back("main10");
	result.push_back("-level:"			+ ffmpeg_stream_id());		result.push_back("5.1");
	result.push_back("-x265-params:"	+ ffmpeg_stream_id());		result.push_back(x265_params);
	result.push_back("-pix_fmt:" 		+ ffmpeg_stream_id());		result.push_back("yuv420p10le");
	result.push_back("-bufsize:" 		+ ffmpeg_stream_id());		result.push_back(DEFAULT_BUFFSIZE);
	if (m_is_animation) {
		result.push_back("-tune:"		+ ffmpeg_stream_id());		result.push_back("animation");
	}

	return result;
}
