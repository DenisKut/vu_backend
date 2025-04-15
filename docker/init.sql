CREATE TABLE initialization_logs (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP NOT NULL,
    message TEXT NOT NULL
);

CREATE TABLE internal_logs (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP NOT NULL,
    src TEXT NOT NULL,
    dst TEXT NOT NULL,
    message TEXT NOT NULL
);

CREATE TABLE method_logs (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP NOT NULL,
    src TEXT NOT NULL,
    dst TEXT NOT NULL,
    method_name TEXT NOT NULL,
    status TEXT NOT NULL
);

CREATE TABLE tcp_logs (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP NOT NULL,
    description TEXT NOT NULL,
    data_hex TEXT NOT NULL
);