CREATE TABLE IF NOT EXISTS log_headers (
    id SERIAL PRIMARY KEY,
    src TEXT NOT NULL,
    timestamp_micro BIGINT NOT NULL,
    timestamp_milli BIGINT NOT NULL,
    log_level TEXT NOT NULL CHECK (log_level IN ('TRACE', 'DEBUG', 'INFO', 'WARNING', 'ERROR', 'FATAL', 'MESSAGESENT', 'MESSAGERECEIVED'))
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
