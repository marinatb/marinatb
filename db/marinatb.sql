-- The MarinaTB PostgreSQL Database

CREATE EXTENSION pgcrypto;

CREATE TABLE users (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  name text UNIQUE NOT NULL
);

CREATE TABLE projects (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  name text UNIQUE NOT NULL,
  owner UUID REFERENCES users NOT NULL
);

CREATE TABLE blueprints (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  project UUID REFERENCES projects NOT NULL,
  doc JSONB
);

CREATE TABLE materializations (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  blueprint UUID REFERENCES blueprints UNIQUE NOT NULL,
  doc JSONB
);

CREATE TABLE hw_topology (
  id integer NOT NULL DEFAULT 1 CONSTRAINT singleton CHECK( id = 1 ),
  doc JSONB,
  UNIQUE (id)
);

INSERT INTO users (name) values ('murphy');
INSERT INTO projects (name, owner) values(
  'backyard', 
  (SELECT id FROM users WHERE name='murphy')
);

CREATE UNIQUE INDEX bp_unique ON blueprints (project, (doc->>'name'));

