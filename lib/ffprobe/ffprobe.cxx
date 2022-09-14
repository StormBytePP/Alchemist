#include "ffprobe.hxx"
#include "utils/input.hxx"

using namespace StormByte::VideoConvert;

void FFprobe::initialize_video_data(const std::string& json) {
	std::optional<Json::Value> root = parse_json(json);
	if (root) {
		auto json = *(root->begin());
		Json::Value item;

		try {
			for (auto it = json[0].begin(); it != json[0].end(); it++) {
				//std::cout << "Item: " << it.key() << " -> " << it->asString() << std::endl;
				if (it.key() == "pix_fmt" && !it->isNull()) m_pix_fmt						= it->asString();
				else if (it.key() == "color_space" && !it->isNull()) m_color_space			= it->asString();
				else if (it.key() == "color_primaries" && !it->isNull()) m_color_primaries	= it->asString();
				else if (it.key() == "color_transfer" && !it->isNull()) m_color_transfer	= it->asString();
				#ifdef ENABLE_HEVC
				else if (it.key() == "side_data_list" && !it->isNull()) {
					// Here it is the HDR color data
					// It will have 2 sections: color and luminance (optional)
					for (Json::Value::ArrayIndex i = 0; i < it->size(); i++) {
						for (auto it2 = (*it)[i].begin(); it2 != (*it)[i].end(); it2++) {
							if (it2.key() == "red_x" && !it2->isNull()) m_red_x 						= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "red_y" && !it2->isNull()) m_red_y 					= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "green_x" && !it2->isNull()) m_green_x				= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "green_y" && !it2->isNull()) m_green_y				= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "blue_x" && !it2->isNull()) m_blue_x 					= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "blue_y" && !it2->isNull()) m_blue_y 					= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "white_point_x" && !it2->isNull()) m_white_point_x	= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "white_point_y" && !it2->isNull()) m_white_point_y	= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "min_luminance" && !it2->isNull()) m_min_luminance	= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "max_luminance" && !it2->isNull()) m_max_luminance	= it2->asString().substr(0, it2->asString().find("/"));
							else if (it2.key() == "max_average" && !it2->isNull()) m_max_average		= it2->asString();
							else if (it2.key() == "max_content" && !it2->isNull()) m_max_content		= it2->asString();
						}
					}
				}
				#endif
			}
		}
		catch (const std::exception& e) {
			//Something went wrong but we ignore it
		}
	}
}

#ifdef ENABLE_HEVC
bool FFprobe::is_HDR_detected() const {
	return 	m_red_x && Utils::Input::is_int(*m_red_x) &&
			m_red_y && Utils::Input::is_int(*m_red_y) &&
			m_green_x && Utils::Input::is_int(*m_green_x) &&
			m_green_y && Utils::Input::is_int(*m_green_y) &&
			m_blue_x && Utils::Input::is_int(*m_blue_x) &&
			m_blue_y && Utils::Input::is_int(*m_blue_y) &&
			m_white_point_x && Utils::Input::is_int(*m_white_point_x) &&
			m_white_point_y && Utils::Input::is_int(*m_white_point_y) &&
			m_min_luminance && Utils::Input::is_int(*m_min_luminance) &&
			m_max_luminance && Utils::Input::is_int(*m_max_luminance);
}

bool FFprobe::is_HDR_factible() const {
	return 	m_pix_fmt && m_pix_fmt == "yuv420p10le" &&
			m_color_space && m_color_space == "bt2020nc" &&
			m_color_primaries && m_color_primaries == "bt2020" &&
			m_color_transfer && m_color_transfer == "smpte2084";
}
#endif

std::optional<Json::Value> FFprobe::parse_json(const std::string& json) const {
	Json::Reader reader;
    Json::Value root;
	std::optional<Json::Value> result;
    try {
		reader.parse(json, root, false);
		if (root.size() > 0 && root.type() != Json::ValueType::nullValue) result = std::move(root);
	}
	catch (const std::exception&) {
		// Something went wrong but we ignore it
	}
	return result;
}
