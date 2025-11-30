#include "interactive.hxx"
#include "configuration/configuration.hxx"
#include "utils/input.hxx"
#include "utils/display.hxx"
#include "help.hxx"

#include <csignal>
#include <boost/algorithm/string.hpp> // For string lowercase

using namespace StormByte::VideoConvert;

Frontend::Task::Interactive::Interactive():VideoConvert::Task::CLI::Base() {
	// Reset signals so app can be exited from CLI
	signal(SIGTERM,	SIG_DFL);
	signal(SIGINT,	SIG_DFL);
	signal(SIGUSR1,	SIG_DFL);
}

Task::STATUS Frontend::Task::Interactive::pre_run_actions() noexcept {
	Task::Help().header();

	assert(m_config);

	const Frontend::Configuration* const config = dynamic_cast<Frontend::Configuration*>(m_config.get());

	try {
		m_database.reset(new Database::SQLite3(*config->get_database_file()));
	}
	catch (const std::exception& e) {
		std::cerr << red(e.what()) << std::endl;
		return VideoConvert::Task::HALT_ERROR;
	}

	const Types::path_t full_path = *config->get_input_folder() / *config->get_interactive_parameter();

	if (!std::filesystem::exists(full_path)) {
		std::cerr << red("File " + std::string(std::filesystem::is_directory(full_path) ? "path " : "") + full_path.string() + " does not exist") << std::endl;
		return VideoConvert::Task::HALT_ERROR;
	}
	else if (m_database->is_film_in_database(*config->get_interactive_parameter())) {
		std::cerr << red("Film " + config->get_interactive_parameter()->string() + " is already in database!") << std::endl;
		return VideoConvert::Task::HALT_ERROR;
	}
	else if (std::filesystem::is_directory(full_path)) {
		#ifdef ENABLE_HEVC
		if (m_database->is_group_in_database(*config->get_interactive_parameter())) {
			std::cerr << red("Film group (folder) " + config->get_interactive_parameter()->string() + " is already in database") << std::endl;
			return VideoConvert::Task::HALT_ERROR;
		}
		else {
			auto files = find_files_recursive();
			if (files.first.empty()) {
				std::cerr << red("Folder contains 0 valid files");
				if (!files.second.empty())
					std::cerr << gray(" (but " + std::to_string(files.second.size()) + " unsupported files)");
				std::cerr << red("!") << std::endl;
				return VideoConvert::Task::HALT_ERROR;
			}
		}
		#else
		std::cerr << red("Folder adition is only allowed with HEVC support but it was not compiled in!") << std::endl;
		return VideoConvert::Task::HALT_ERROR;
		#endif
	}

	return VideoConvert::Task::RUNNING;
}

Types::optional_path_t Frontend::Task::Interactive::ask_title() {
	Types::optional_path_t result;

	std::cout << "Type new film title " << gray("(empty to not rename original file)") << ": ";
	std::getline(std::cin, m_buffer_str);

	if (!m_buffer_str.empty())
		result = m_buffer_str;

	return result;
}

Database::Data::film::priority Frontend::Task::Interactive::ask_priority() {
	do {
		std::cout << "Which priority (default NORMAL)? " << gray("LOW(0), NORMAL(1), HIGH(2), IMPORTANT(3)") << ": ";
		std::getline(std::cin, m_buffer_str);
	} while (m_buffer_str != "" && !Utils::Input::to_int_in_range(m_buffer_str, m_buffer_int, 0, 3, true));

	return static_cast<Database::Data::film::priority>(m_buffer_str == "" ? 1 : m_buffer_int);
}

bool Frontend::Task::Interactive::ask_animation() {
	do {
		std::cout << "Is an animated movie (default no)? " << gray("[(y)es/(n)o]") << ": ";
		std::getline(std::cin, m_buffer_str);
	} while (m_buffer_str != "" && !Utils::Input::in_yes_no(m_buffer_str, m_buffer_bool, true));
	if (m_buffer_str == "") m_buffer_bool = false;

	return m_buffer_bool;
}

FFprobe Frontend::Task::Interactive::get_film_data() {
	const Frontend::Configuration* const config = dynamic_cast<Frontend::Configuration*>(m_config.get());
	const Types::path_t full_path = *config->get_input_folder() / *config->get_interactive_parameter(); 
	
	return FFprobe::from_file(full_path);
}

void Frontend::Task::Interactive::update_title_renamed(const FFprobe& film_data, const stream_map_t& stream_data, Types::optional_path_t& title) {
	if (title) {
		const auto resolution = film_data.get_resolution();
		std::string new_title = *title;
		if (resolution)
			new_title += " - m" + FFprobe::stream::RESOLUTION_STRING.at(*resolution);

		if (stream_data.at(FFprobe::stream::VIDEO).begin()->second.m_hdr)
			new_title += " HDR";
		title = new_title + ".mkv";
	}
}

Frontend::Task::Interactive::stream_map_t Frontend::Task::Interactive::initialize_stream_map() {
	// Return using the typedef `stream_map_t` so inner maps use `short` keys (supports -1 sentinel)
	return stream_map_t{
		{ FFprobe::stream::VIDEO,    {} },
		{ FFprobe::stream::AUDIO,    {} },
		{ FFprobe::stream::SUBTITLE, {} }
	};
}

void Frontend::Task::Interactive::display_stream_map(const FFprobe& probe, const stream_map_t& stream_map) {
	std::cout << "Film stream list:" << std::endl;
	for (FFprobe::stream::TYPE type: { FFprobe::stream::VIDEO, FFprobe::stream::AUDIO, FFprobe::stream::SUBTITLE }) {
		const auto& strm_probe = probe.get_stream(type);
		const auto& strm_map = stream_map.at(type);
		std::string line;
		for (size_t i = 0; i < strm_probe.size(); i++) {
			line = magenta(std::string(1, type) + ":" + std::to_string(i));
			line += " " + blue(strm_probe[i].codec_name);
			if (strm_probe[i].language) {
				line += gray(" (" + *strm_probe[i].language + ")");
			}
			if (strm_probe[i].channels) {
				line += gray(" (" + std::to_string(*strm_probe[i].channels) + " channels)");
			}
			line += "-> ";
			if (strm_map.contains(i)) {	
				line += light_green(Database::Data::film::stream::codec_string.at(strm_map.at(i).m_codec));
			}
			else if (strm_map.contains(-1)) {
				line += light_green(Database::Data::film::stream::codec_string.at(strm_map.at(-1).m_codec));
			}
			else
				line += light_red("(not selected)");

			std::cout << line << std::endl;
		}
	}
}

std::optional<Frontend::Task::Interactive::stream_id_t> Frontend::Task::Interactive::ask_stream_id(const FFprobe& probe) {
	std::optional<Task::Interactive::stream_id_t> result;

	// Generate a list of available options
	std::list<std::string> available_options;
	for (auto type: { FFprobe::stream::VIDEO, FFprobe::stream::AUDIO, FFprobe::stream::SUBTITLE }) {
		auto stream = probe.get_stream(type);
		if (!stream.empty()) {
			std::string stream_type_str = std::string(1, type);
			available_options.push_back(stream_type_str + ":-1");
			for (size_t id = 0; id < stream.size(); id++)
				available_options.push_back(stream_type_str + ":" + std::to_string(id));
		}
	}
	available_options.push_back("f");
	available_options.push_back("F");

	do {
		std::cout << "Select stream ID (-1 for all of that type) or (f)inish: ";
		std::getline(std::cin, m_buffer_str);
	} while (!Utils::Input::in_options(m_buffer_str, available_options, true));

	if (m_buffer_str != "f" && m_buffer_str != "F") {
		/* Mini tokenizer */
		std::vector<std::string> tokens;
		std::stringstream ss (m_buffer_str);
		std::string item;

		while (std::getline (ss, item, ':')) {
			tokens.push_back (item);
		}
		assert(tokens.size() == 2);
		
		Utils::Input::to_int(tokens[1], m_buffer_int);
		Frontend::Task::Interactive::stream_id_t selected_stream_id = {
			static_cast<FFprobe::stream::TYPE>(tokens[0][0]),
			m_buffer_int
		};

		result = selected_stream_id;
	}
	return result;
}

void Frontend::Task::Interactive::ask_stream([[maybe_unused]]const FFprobe& probe, const stream_id_t& strm_id, stream_map_t& strm_map) {
	std::list<int> options;
	if (strm_id.first == FFprobe::stream::VIDEO) {
		options = {
			Database::Data::film::stream::VIDEO_COPY,
			#ifdef ENABLE_HEVC
			Database::Data::film::stream::VIDEO_HEVC
			#endif
		};
		options.sort();
	}
	else if (strm_id.first == FFprobe::stream::AUDIO) {
		options = {
			Database::Data::film::stream::AUDIO_COPY,
			#ifdef ENABLE_AAC
			Database::Data::film::stream::AUDIO_AAC,
			#endif
			#ifdef ENABLE_FDKAAC
			Database::Data::film::stream::AUDIO_FDKAAC,
			#endif
			#ifdef ENABLE_AC3
			Database::Data::film::stream::AUDIO_AC3,
			#endif
			#ifdef ENABLE_EAC3
			Database::Data::film::stream::AUDIO_EAC3,
			#endif
			#ifdef ENABLE_OPUS
			Database::Data::film::stream::AUDIO_OPUS,
			#endif
		};
		options.sort();
	}
	else if (strm_id.first == FFprobe::stream::SUBTITLE) {
		options = {
			Database::Data::film::stream::SUBTITLE_COPY
		};
		options.sort();
	}
	
	std::cout << "Available codecs:" << std::endl;
	for (int i : options) {
		std::cout << magenta(i) << ": " << blue(Database::Data::film::stream::codec_string.at(static_cast<Database::Data::film::stream::codec>(i))) << std::endl;
	}

	do {
		std::cout << "Select desired codec: " << Utils::Display::list_to_string(options, ansi_code(LIGHT_GRAY) + "[", ", ", "]\033[0m") << ": ";
		std::getline(std::cin, m_buffer_str);
	} while (!Utils::Input::to_int_positive(m_buffer_str, m_buffer_int, true) || !Utils::Input::in_options(m_buffer_int, options, true));

	Database::Data::film::stream stream;

	// Construct the stream
	stream.m_id = strm_id.second;
	stream.m_codec = static_cast<Database::Data::film::stream::codec>(m_buffer_int);

	// Final tunings depending on type
	if (strm_id.first == FFprobe::stream::VIDEO) {
		#ifdef ENABLE_HEVC
		if (probe.is_HDR_detected()) {
			std::cout << green("HDR detected!") << std::endl;
			stream.m_hdr = probe.get_HDR().data();
		}
		else if (probe.is_HDR_factible()) {
			do {
				std::cout << yellow("HDR not found but is factible, put default HDR?") << light_gray("[(y)es/(n)o]") << ": ";
				std::getline(std::cin, m_buffer_str);
			} while (!Utils::Input::in_yes_no(m_buffer_str, m_buffer_bool, true));
			if (m_buffer_bool) {
				stream.m_hdr = probe.get_HDR().data();
			}
		}
		#endif
	}
	    else if (strm_id.first == FFprobe::stream::AUDIO) {
		    // Only access probe stream by index when a specific stream was selected
		    if (strm_id.second >= 0) {
			auto const& audio_streams = probe.get_stream(strm_id.first);
			if (static_cast<size_t>(strm_id.second) < audio_streams.size())
			    stream.m_channels = audio_streams[static_cast<size_t>(strm_id.second)].channels;
		    }
	    }
	else if (strm_id.first == FFprobe::stream::SUBTITLE) {
		// Nothing yet
	}

	// If -1 is selected we delete previous stuff
	if (strm_id.second == -1) {
		strm_map.at(strm_id.first).clear();
	}
	// Else we could have before a -1 and now we want specific
	else if (strm_map.at(strm_id.first).count(-1) == 1) {
		// In which case we need to copy that -1 as if it were specific
		auto stream_info = strm_map.at(strm_id.first)[static_cast<short>(-1)];
		strm_map.at(strm_id.first).clear();
		auto const& streams = probe.get_stream(strm_id.first);
		for (int i = 0; i < static_cast<int>(streams.size()); ++i) {
			stream_info.m_id = i;
			strm_map.at(strm_id.first)[static_cast<short>(i)] = stream_info;
		}
	}

	strm_map.at(strm_id.first)[strm_id.second] = stream;
}

Database::Data::film Frontend::Task::Interactive::generate_film(const stream_map_t& stream_map, const Database::Data::film::priority& priority, const Types::optional_path_t& title, const bool& animation) {
	const Frontend::Configuration* const config = dynamic_cast<Frontend::Configuration*>(m_config.get());
	Database::Data::film result;
	result.m_priority = priority;
	result.m_file = *config->get_interactive_parameter();
	result.m_title = title;
	std::list<Database::Data::film::stream> streams;

	// For FFmpeg map order matters so we do in order
	for (FFprobe::stream::TYPE type: { FFprobe::stream::VIDEO, FFprobe::stream::AUDIO, FFprobe::stream::SUBTITLE }) {
		for (auto it = stream_map.at(type).begin(); it != stream_map.at(type).end(); it++) {
			Database::Data::film::stream tmp_stream = it->second;
			if (tmp_stream.m_codec == Database::Data::film::stream::VIDEO_HEVC)
				tmp_stream.m_is_animation = animation;
			streams.push_back(std::move(tmp_stream));
		}
	}
	result.m_streams = std::move(streams);

	return result;
}

std::optional<unsigned int> Frontend::Task::Interactive::insert_film(const Database::Data::film& film) {
	return m_database->insert_film(film);
}

#ifdef ENABLE_HEVC
Frontend::Task::Interactive::group_file_info_t Frontend::Task::Interactive::find_files_recursive() {
	const Frontend::Configuration* const config = dynamic_cast<Frontend::Configuration*>(m_config.get());
	const Types::path_t full_folder = *config->get_input_folder() / *config->get_interactive_parameter();

	std::list<Types::path_t> valid, invalid;

	for (std::filesystem::recursive_directory_iterator it(full_folder), end; it != end; ++it) {
		if (!is_directory(it->path())) {
			const std::string extension = it->path().extension();
			Types::path_t relative = std::filesystem::relative(it->path(), *config->get_input_folder());
			if (!config->is_extension_supported(extension))
				invalid.push_back(relative);
			else
				valid.push_back(relative);
		}
	}

	return { valid, invalid };
}

bool Frontend::Task::Interactive::ask_group_confirmation(const group_file_info_t& group_info) {
	bool result = true;
	const Frontend::Configuration* const config = dynamic_cast<Frontend::Configuration*>(m_config.get());

	if (group_info.first.empty()) {
		std::cerr 	<< red(	"No valid files found in path "
							+ config->get_interactive_parameter()->string()
							+ " with accepted extensions "
							+ Utils::Display::list_to_string(config->get_supported_multimedia_extensions(), ansi_code(LIGHT_GRAY) + "(", ", ", ")")
					)
					<< std::endl;
		result = false;
	}
	else {
		std::cout << green("Found " + std::to_string(group_info.first.size()) + " film(s)") << std::endl;
		if (!group_info.second.empty()) {
			std::cout << yellow("Warning: found " + std::to_string(group_info.second.size()) + " invalid file(s) that will be ignored.") << std::endl;
			for (Types::path_t file: group_info.second)
				std::cout << light_yellow("\t* " + file.string()) << std::endl;
		}
		do {
			std::cout << "Do you wish yo continue? " << light_gray("[(y)es/(n)o]") << ": ";
			std::getline(std::cin, m_buffer_str);
		} while(!Utils::Input::in_yes_no(m_buffer_str, m_buffer_bool, true));
		result = m_buffer_bool;
	}

	return result;
}

Database::Data::film::group Frontend::Task::Interactive::insert_group() {
	const Frontend::Configuration* const config = dynamic_cast<Frontend::Configuration*>(m_config.get());
	return *m_database->insert_group(*config->get_interactive_parameter());
}

std::list<Database::Data::film::stream> Frontend::Task::Interactive::generate_default_streams_for_group(const bool& animation) {
	Database::Data::film::stream video, audio, subtitle;

	video.m_id = 0;
	video.m_codec = Database::Data::film::stream::VIDEO_HEVC;
	video.m_is_animation = animation;
	
	audio.m_id = -1;
	audio.m_codec = Database::Data::film::stream::AUDIO_COPY;

	subtitle.m_id = -1;
	subtitle.m_codec = Database::Data::film::stream::SUBTITLE_COPY;

	return { video, audio, subtitle };
}

Frontend::Task::Interactive::film_group_t Frontend::Task::Interactive::generate_film_group_t(const group_file_info_t& film_group_t_info, const Database::Data::film::group& group, const Database::Data::film::priority& priority, const bool& animation) {
	film_group_t result;

	const std::list<Database::Data::film::stream> streams = generate_default_streams_for_group(animation);

	for (Types::path_t film_path: film_group_t_info.first) {
		Database::Data::film film;
		film.m_file = film_path;
		film.m_group = group;
		film.m_priority = priority;
		film.m_streams = streams;
		result.push_back(std::move(film));
	}

	return result;
}

bool Frontend::Task::Interactive::insert_film_group_t(const film_group_t& film_group_t) {
	bool operation = m_database->insert_films(film_group_t);
	
	if (operation) {
		std::cout << green("Inserted " + std::to_string(film_group_t.size()) + " film(s) in database") << std::endl;
	}
	else {
		std::cerr << red("There was an error inserting and therefore no films were added!") << std::endl;
	}

	return operation;
}
#endif

StormByte::VideoConvert::Task::STATUS Frontend::Task::Interactive::do_work(std::optional<pid_t>&) noexcept {
	const Frontend::Configuration* const config = dynamic_cast<Frontend::Configuration*>(m_config.get());
	Types::optional_path_t title;
	// Title is only displayed when not in batch mode (path)
	if (!std::filesystem::is_directory(*config->get_input_folder() / *config->get_interactive_parameter()))
		title = ask_title();
	Database::Data::film::priority priority = ask_priority();
	bool animation = ask_animation();

	#ifdef ENABLE_HEVC
	if (std::filesystem::is_directory(*config->get_input_folder() / *config->get_interactive_parameter())) {
		group_file_info_t files = find_files_recursive();
		if (files.first.empty()) {
			std::cerr << red("Directory is empty!") << std::endl;
			return VideoConvert::Task::HALT_ERROR;
		}
		if (ask_group_confirmation(files)) {
			Database::Data::film::group group = insert_group();
			if (!insert_film_group_t(
				generate_film_group_t(
					files,
					group,
					priority,
					animation
				)
			)) {
				return VideoConvert::Task::HALT_ERROR;
			}
		}
	}
	else {
	#endif
		Database::Data::film film;
		FFprobe film_data = get_film_data();

		auto stream_map = initialize_stream_map();
		
		bool continue_asking = true;
		do {
			display_stream_map(film_data, stream_map);
			std::optional<stream_id_t> strm_id = ask_stream_id(film_data);

			if (!strm_id) continue_asking = false;
			else {
				ask_stream(film_data, *strm_id, stream_map);
			}
		} while (continue_asking);

		update_title_renamed(film_data, stream_map, title);
		film = generate_film(stream_map, priority, title, animation);

		if (film.m_streams.empty()) {
			std::cerr << light_red("There were no streams selected for film " + config->get_interactive_parameter()->string()) << ", " << green(bold("no changes were made to database")) << std::endl;
			return VideoConvert::Task::HALT_ERROR;
		}
		else {
			std::optional<unsigned int> film_id = insert_film(film);

			if (film_id) {
				std::cout << green("Film " + config->get_interactive_parameter()->string() + " was inserted with ID " + std::to_string(*film_id)) << std::endl;
			}
			else {
				std::cerr << red("Could NOT insert film!") << std::endl;
				return VideoConvert::Task::HALT_ERROR;
			}
		}
	#ifdef ENABLE_HEVC
	}
	#endif
	return VideoConvert::Task::HALT_OK;
}
