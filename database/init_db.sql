
CREATE TABLE IF NOT EXISTS log_headers (
    id BIGSERIAL PRIMARY KEY,
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

CREATE OR REPLACE VIEW log_headers_x_init_logs
AS
SELECT * FROM log_headers INNER JOIN init_logs ON log_headers.id = init_logs.header_id
;

CREATE OR REPLACE VIEW log_headers_x_internal_messages
AS
SELECT * FROM log_headers INNER JOIN internal_messages ON log_headers.id = internal_messages.header_id
;

CREATE OR REPLACE VIEW log_headers_x_protocol_messages
AS
SELECT * FROM log_headers INNER JOIN protocol_messages ON log_headers.id = protocol_messages.header_id
;

CREATE OR REPLACE VIEW all_tables
AS SELECT * FROM (log_headers INNER JOIN init_logs ON log_headers.id = init_logs.header_id)
                              NATURAL JOIN internal_messages
                              NATURAL JOIN protocol_messages
;

CREATE OR REPLACE VIEW full_view AS
SELECT LogHeaders.*, 
       InitLogs.header_id as init_logs_header_id,
       InitLogs.status,
       InternalMessages.header_id as internal_messages_header_id,
       InternalMessages.message_type,
       InternalMessages.message_content,
       ProtocolMessages.header_id as protocol_messages_header_id,
       ProtocolMessages.protocol_type,
       ProtocolMessages.raw_data,
       ProtocolMessages.parsed_data

FROM  log_headers LogHeaders CROSS JOIN init_logs InitLogs
                          CROSS JOIN internal_messages InternalMessages
                          CROSS JOIN protocol_messages ProtocolMessages
;