#pragma once

#include "task/cli/base.hxx"
#include "ffprobe/ffprobe.hxx"
#include "database/sqlite3.hxx"

#include <map>

namespace StormByte::VideoConvert::Frontend::Task {
	class Interactive: public VideoConvert::Task::CLI::Base {
		public:
			Interactive();
			Interactive(const Interactive& interactive) = delete;
			Interactive(Interactive&& interactive) noexcept = delete;
			Interactive& operator=(const Interactive& interactive) = delete;
			Interactive& operator=(Interactive&& interactive) noexcept = delete;
			~Interactive() noexcept = default;

		private:
			VideoConvert::Task::STATUS pre_run_actions() noexcept override;
			VideoConvert::Task::STATUS do_work(std::optional<pid_t>&) noexcept override;

			using stream_map_t					= std::map<FFprobe::stream::TYPE, std::map<short, Database::Data::film::stream>>;
			using stream_id_t					= std::pair<FFprobe::stream::TYPE, short>; // Not unsigned as can be -1 for all
			using group_file_info_t					= std::pair<std::list<Types::path_t>, std::list<Types::path_t>>; // First valid files, second invalid files (not having supported extensions)
			using film_group_t						= std::list<Database::Data::film>;

			Types::optional_path_t	ask_title();
			Database::Data::film::priority			ask_priority();
			bool									ask_animation();
			FFprobe									get_film_data();
			void									update_title_renamed(const FFprobe&, const stream_map_t&, Types::optional_path_t& title);
			stream_map_t							initialize_stream_map();
			void									display_stream_map(const FFprobe&, const stream_map_t&);
			std::optional<stream_id_t>				ask_stream_id(const FFprobe&);
			void									ask_stream(const FFprobe&, const stream_id_t&, stream_map_t&);
			Database::Data::film					generate_film(const stream_map_t&, const Database::Data::film::priority&, const Types::optional_path_t& title, const bool& animation);
			std::optional<unsigned int>				insert_film(const Database::Data::film&);
			#ifdef ENABLE_HEVC
			group_file_info_t						find_files_recursive();
			bool									ask_group_confirmation(const group_file_info_t&);
			Database::Data::film::group				insert_group();
			std::list<Database::Data::film::stream>	generate_default_streams_for_group(const bool& animation);
			film_group_t							generate_film_group_t(const group_file_info_t&, const Database::Data::film::group& group, const Database::Data::film::priority&, const bool& animation);
			bool									insert_film_group_t(const film_group_t&);
			#endif

			std::string m_buffer_str;
			int m_buffer_int;
			bool m_buffer_bool;
			Types::database_t m_database;
	};
}