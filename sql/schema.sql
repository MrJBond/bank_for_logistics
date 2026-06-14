-- Database Schema for AI-Driven Financial Management System

CREATE TABLE Client (
    id_client SERIAL PRIMARY KEY,
    name_client VARCHAR(160) NOT NULL,
    legal_address VARCHAR(80),
    boss_name VARCHAR(80),
    boss_phone VARCHAR(13),
    accountant_name VARCHAR(80),
    accountant_phone VARCHAR(13)
);

CREATE TABLE ClientAuth (
    id_client INTEGER PRIMARY KEY REFERENCES Client(id_client) ON DELETE CASCADE,
    username VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(64) NOT NULL,
    face_encoding TEXT -- Base64 or stringified vector from DeepFace
);

CREATE TABLE Account (
    id_account SERIAL PRIMARY KEY,
    amount DOUBLE PRECISION DEFAULT 0.0,
    currency CHAR(1) DEFAULT '$',
    id_client INTEGER REFERENCES Client(id_client) ON DELETE CASCADE
);

CREATE TABLE Transaction (
    id_transaction SERIAL PRIMARY KEY,
    date_transaction TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    amount DOUBLE PRECISION NOT NULL,
    id_account INTEGER REFERENCES Account(id_account),
    id_accountTo INTEGER,
    description TEXT,
    category TEXT,
    icon TEXT,
    location TEXT
);

CREATE TABLE logistics_routes (
    id SERIAL PRIMARY KEY,
    driver_id INTEGER REFERENCES Client(id_client),
    origin_address VARCHAR(255),
    destination_address VARCHAR(255),
    distance_km DOUBLE PRECISION,
    estimated_hours DOUBLE PRECISION,
    route_geometry JSONB, -- Stores GeoJSON from OpenRouteService
    status VARCHAR(50) DEFAULT 'PLANNED'
);

CREATE TABLE loan_fuzzy_rules (
    id SERIAL PRIMARY KEY,
    income_level VARCHAR(50),
    stability_level VARCHAR(50),
    debt_level VARCHAR(50),
    recommendation VARCHAR(50)
);

CREATE TABLE chatbot_training_data (
    id SERIAL PRIMARY KEY,
    phrase VARCHAR(255) NOT NULL,
    intent VARCHAR(100) NOT NULL
);

-- Insert some default mock data for ML models
INSERT INTO chatbot_training_data (phrase, intent) VALUES 
('Hello there!', 'greeting'),
('How much money do I have?', 'check_balance'),
('I want to take a loan', 'request_loan_recommendation'),
('Evaluate my creditworthiness', 'assess_risk');