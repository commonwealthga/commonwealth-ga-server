-- 175: sites and nodes become database rows.
--
-- Both lived in control-server.json, which meant the dashboard (a read-only DB
-- client) could not show which node an instance runs on, the website could not
-- join an instance to a region label for display, and adding or disabling a node
-- needed a file edit plus a master restart.
--
-- Node pre-shared keys are the one thing here that is a secret: they let a node
-- attach to the master and receive player bundles. They are stored sealed with a
-- secret held only in control-server.json (node_key_secret), so a copy of this
-- database is not enough to impersonate a node. See NodeKeyVault.

CREATE TABLE IF NOT EXISTS ga_sites (
    id          INTEGER PRIMARY KEY,          -- retail SYS_SITE_ID
    name_msg_id INTEGER NOT NULL DEFAULT 0,   -- asm_data_set_msg_translations id
    label       TEXT    NOT NULL DEFAULT '',  -- log-only name
    enabled     INTEGER NOT NULL DEFAULT 1,
    sort_order  INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS ga_nodes (
    id          INTEGER PRIMARY KEY,           -- node id; 0 is reserved (the master)
    name        TEXT    NOT NULL UNIQUE,       -- wire identity
    site        INTEGER NOT NULL DEFAULT 0,
    address     TEXT    NOT NULL DEFAULT '',   -- public address its instances are reachable at
    capacity    INTEGER NOT NULL DEFAULT 0,    -- max concurrent instances; 0 = unlimited
    home_map    INTEGER NOT NULL DEFAULT 0,
    enabled     INTEGER NOT NULL DEFAULT 1,
    key_sealed  TEXT,                          -- nonce||cipher||mac, hex; NULL = not provisioned
    updated_at  INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_ga_nodes_site ON ga_nodes(site);

-- Mutation counter. The master polls this one row instead of hashing the whole
-- config, and bumps it from any config write, so a live edit reaches the running
-- node registry without a restart.
--
-- NOTE: cs_settings is ("key", value) -- not the (config_key, config_value) shape
-- ga_user_preferences uses. "key" is quoted because it reads as a keyword.
CREATE TABLE IF NOT EXISTS cs_settings ("key" TEXT PRIMARY KEY, value TEXT);
INSERT OR IGNORE INTO cs_settings ("key", value) VALUES ('config_rev', '1');
