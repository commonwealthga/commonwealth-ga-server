#pragma once

#include <cstdint>
#include <string>

struct ControlServerConfig;

// Sites and nodes as database rows.
//
// Both used to live in control-server.json. They moved because three things
// needed to read them from somewhere queryable: the dashboard (which is a
// read-only database client and cannot read a JSON file), the website (region
// labels for instance display), and live add/remove/enable without a restart.
//
// The in-memory model does not change: Load() fills `ControlServerConfig::sites`
// and `ControlServerConfig::nodes`, so placement, NodeRegistry and the login
// reply all keep using the same structures. Only the source of truth moved.
namespace NodeStore {

// Fills cfg.sites / cfg.nodes from the database.
//
// On the first run against a database with empty tables, anything in the config
// file is imported (keys sealed on the way in) and logged, so an existing setup
// migrates without a manual step. After that the database wins and the file's
// entries are ignored -- one source of truth, never a merge.
bool Load(ControlServerConfig& cfg, std::string& err);

// Bumps cs_settings.config_rev. The running master polls it and reloads, which is
// how a dashboard edit reaches a live process without a restart.
bool BumpRevision();

// Current revision, or -1 if it cannot be read.
int64_t Revision();

// --- Editing (dashboard) ----------------------------------------------------
//
// Every mutation bumps the revision, so the running master reloads without being
// told. None of these touch the in-memory registry: the caller reloads after a
// successful write, which keeps one code path for "config changed".

struct NodeRecord {
    int         id       = 0;
    std::string name;
    int         site     = 0;
    std::string address;
    int         capacity = 0;
    bool        home_map = false;
    bool        enabled  = true;
    bool        has_key  = false;   // a sealed key is stored (never its value)
    std::string key;                // plaintext, input only ("" = leave unchanged)
};

struct SiteRecord {
    int         id          = 0;
    int         name_msg_id = 0;
    std::string label;
    bool        enabled     = true;
    int         sort_order  = 0;
};

bool SaveNode(const NodeRecord& node, std::string& err);
bool DeleteNode(int id, std::string& err);
bool SaveSite(const SiteRecord& site, std::string& err);
bool DeleteSite(int id, std::string& err);

}  // namespace NodeStore
