CREATE TABLE groups(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	folder VARCHAR NOT NULL
);

CREATE TABLE films(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	file VARCHAR NOT NULL,
	title VARCHAR DEFAULT NULL,
	prio TINYINT,
	processing BOOL DEFAULT FALSE,
	unsupported BOOL DEFAULT FALSE,
	group_id INTEGER DEFAULT NULL,
	FOREIGN KEY(group_id) REFERENCES groups(id) ON DELETE CASCADE
);

CREATE TABLE streams(
	id INTEGER,
	film_id INTEGER,
	codec INTEGER NOT NULL,
	is_animation BOOL DEFAULT FALSE,
	max_rate VARCHAR DEFAULT NULL,
	bitrate VARCHAR DEFAULT NULL,
	PRIMARY KEY(id, film_id, codec),
	FOREIGN KEY(film_id) REFERENCES films(id) ON DELETE CASCADE
);

CREATE TABLE stream_hdr(
	film_id INTEGER,
	stream_id INTEGER,
	codec INTEGER,
	red_x INTEGER NOT NULL,
	red_y INTEGER NOT NULL,
	green_x INTEGER NOT NULL,
	green_y INTEGER NOT NULL,
	blue_x INTEGER NOT NULL,
	blue_y INTEGER NOT NULL,
	white_point_x INTEGER NOT NULL,
	white_point_y INTEGER NOT NULL,
	luminance_min INTEGER NOT NULL,
	luminance_max INTEGER NOT NULL,
	light_level_content INTEGER,
	light_level_average INTEGER,
	PRIMARY KEY (film_id, stream_id, codec),
	FOREIGN KEY (film_id, stream_id, codec) REFERENCES streams(id, film_id, codec) ON DELETE CASCADE
);
