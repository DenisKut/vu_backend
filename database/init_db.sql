CREATE TABLE IF NOT EXISTS log_headers (
    id SERIAL PRIMARY KEY,
    src TEXT NOT NULL,
    timestamp_micro BIGINT NOT NULL,
    date DATE NOT NULL,
    hrs SMALLINT NOT NULL CHECK (hrs BETWEEN 0 AND 23),
    minutes SMALLINT NOT NULL CHECK (minutes BETWEEN 0 AND 59),
    seconds SMALLINT NOT NULL CHECK (seconds BETWEEN 0 AND 59),
    msec SMALLINT NOT NULL CHECK (msec BETWEEN 0 AND 999),
    mksec INTEGER NOT NULL CHECK (mksec BETWEEN 0 AND 999999),
    log_level TEXT NOT NULL CHECK (log_level IN (
        'TRACE', 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'FATAL', 'MESSAGESENT', 'MESSAGERECEIVED'
    ))
);

CREATE TABLE IF NOT EXISTS init_logs (
    header_id INTEGER PRIMARY KEY REFERENCES log_headers(id) ON DELETE CASCADE,
    status TEXT NOT NULL CHECK (status IN ('LOADED', 'NOT_LOADED_ERROR_CRITICAL', 'NOT_LOADED'))
);

CREATE TABLE IF NOT EXISTS internal_messages (
    header_id INTEGER PRIMARY KEY REFERENCES log_headers(id) ON DELETE CASCADE,
    message_type TEXT NOT NULL,
    message_content TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS protocol_messages (
    header_id INTEGER PRIMARY KEY REFERENCES log_headers(id) ON DELETE CASCADE,
    protocol_type TEXT NOT NULL,
    raw_data BYTEA,
    parsed_data TEXT
);
