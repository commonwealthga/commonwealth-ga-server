#include "src/ControlServer/BugReports/BugReports.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <cctype>
#include <ctime>
#include <regex>

namespace BugReports {

namespace {

struct Stmt {
    sqlite3_stmt* s = nullptr;
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) {
            Logger::Log("bug-report", "[Bug] prepare failed: %s\n", sqlite3_errmsg(db));
            s = nullptr;
        }
    }
    ~Stmt() { if (s) sqlite3_finalize(s); }
    explicit operator bool() const { return s != nullptr; }
};

// Printable ASCII only, capped; keeps the row safe to show on a web page.
std::string Clean(const std::string& in, size_t cap) {
    std::string out;
    for (unsigned char c : in) {
        if (out.size() >= cap) break;
        out.push_back((c >= 0x20 && c < 0x7f) ? static_cast<char>(c) : ' ');
    }
    return out;
}

std::string MaskAddresses(const std::string& line) {
    static const std::regex ipv4(R"(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)");
    return std::regex_replace(line, ipv4, "x.x.x.x");
}

} // namespace

void EnsureSchema() {
    sqlite3* db = Database::GetConnection();
    const char* sql =
        "CREATE TABLE IF NOT EXISTS ga_bug_reports ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " kind TEXT NOT NULL DEFAULT 'bug',"
        " user_id INTEGER NOT NULL DEFAULT 0,"
        " username TEXT NOT NULL DEFAULT '',"
        " character_id INTEGER NOT NULL DEFAULT 0,"
        " agent_name TEXT NOT NULL DEFAULT '',"
        " target_name TEXT NOT NULL DEFAULT '',"
        " instance_id INTEGER NOT NULL DEFAULT 0,"
        " map_name TEXT NOT NULL DEFAULT '',"
        " position TEXT NOT NULL DEFAULT '',"
        " text TEXT NOT NULL DEFAULT '',"
        " context TEXT NOT NULL DEFAULT '',"
        " status TEXT NOT NULL DEFAULT 'new',"
        " staff_note TEXT NOT NULL DEFAULT '',"
        " created_at INTEGER NOT NULL,"
        " updated_at INTEGER NOT NULL DEFAULT 0,"
        " fed_at INTEGER NOT NULL DEFAULT 0);"
        "CREATE INDEX IF NOT EXISTS idx_ga_bug_reports_user ON ga_bug_reports(user_id, created_at);";
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Log("bug-report", "[Bug] schema failed: %s\n", err ? err : "?");
        sqlite3_free(err);
    }
}

Result Submit(const std::string& kind, const std::string& session_guid,
              const std::string& agent_name, const std::string& target,
              const std::string& text) {
    Result r;
    const std::string body = Clean(text, kMaxTextBytes);
    if (body.find_first_not_of(' ') == std::string::npos) {
        r.reply = kind == "player"
            ? "*** Usage: -report <player> <what happened> ***"
            : "*** Usage: -bug <what went wrong> ***";
        return r;
    }

    auto info = PlayerSessionStore::GetByGuid(session_guid);
    const int64_t user_id = info ? info->user_id : 0;
    const int64_t character_id = info ? info->selected_character_id : 0;
    if (user_id == 0) {
        r.reply = "*** Could not file the report (not logged in) - please try again ***";
        return r;
    }

    sqlite3* db = Database::GetConnection();
    const int64_t now = static_cast<int64_t>(std::time(nullptr));
    {
        Stmt st(db, "SELECT COUNT(*) FROM ga_bug_reports WHERE user_id = ? AND created_at > ?");
        if (!st) { r.reply = "*** Could not file the report - please try again later ***"; return r; }
        sqlite3_bind_int64(st.s, 1, user_id);
        sqlite3_bind_int64(st.s, 2, now - 3600);
        if (sqlite3_step(st.s) == SQLITE_ROW && sqlite3_column_int(st.s, 0) >= kMaxPerHour) {
            Logger::Log("bug-report", "[Bug] user=%lld outcome=rate_limited\n",
                static_cast<long long>(user_id));
            r.reply = "*** You have sent " + std::to_string(kMaxPerHour)
                + " reports this hour - please wait a bit ***";
            return r;
        }
    }

    std::string username;
    {
        Stmt st(db, "SELECT username FROM ga_users WHERE id = ?");
        if (st) {
            sqlite3_bind_int64(st.s, 1, user_id);
            if (sqlite3_step(st.s) == SQLITE_ROW) {
                const unsigned char* t = sqlite3_column_text(st.s, 0);
                if (t) username = reinterpret_cast<const char*>(t);
            }
        }
    }

    int64_t instance_id = 0;
    std::string map_name;
    if (auto at = InstanceRegistry::GetInstancePlayerTaskForce(session_guid)) {
        instance_id = at->first;
        if (auto inst = InstanceRegistry::GetInstanceById(instance_id)) map_name = inst->map_name;
    }

    std::vector<std::string> needles = {session_guid, agent_name};
    if (!username.empty() && username != agent_name) needles.push_back(username);
    if (instance_id) needles.push_back("instance=" + std::to_string(instance_id));
    std::string context;
    for (const auto& line : Logger::Recent(needles, kContextLines)) {
        context += Clean(MaskAddresses(line), kContextLineBytes);
        context += '\n';
    }

    Stmt st(db,
        "INSERT INTO ga_bug_reports (kind, user_id, username, character_id, agent_name, target_name,"
        " instance_id, map_name, text, context, created_at) VALUES (?,?,?,?,?,?,?,?,?,?,?)");
    if (!st) { r.reply = "*** Could not file the report - please try again later ***"; return r; }
    const std::string target_clean = Clean(target, 64);
    sqlite3_bind_text(st.s, 1, kind.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.s, 2, user_id);
    sqlite3_bind_text(st.s, 3, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.s, 4, character_id);
    sqlite3_bind_text(st.s, 5, agent_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 6, target_clean.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.s, 7, instance_id);
    sqlite3_bind_text(st.s, 8, map_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 9, body.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 10, context.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.s, 11, now);
    if (sqlite3_step(st.s) != SQLITE_DONE) {
        Logger::Log("bug-report", "[Bug] insert failed: %s\n", sqlite3_errmsg(db));
        r.reply = "*** Could not file the report - please try again later ***";
        return r;
    }
    r.id = sqlite3_last_insert_rowid(db);
    Logger::Log("bug-report", "[Bug] #%lld kind=%s user=%lld agent='%s' instance=%lld map='%s' bytes=%zu\n",
        static_cast<long long>(r.id), kind.c_str(), static_cast<long long>(user_id),
        agent_name.c_str(), static_cast<long long>(instance_id), map_name.c_str(), body.size());
    r.reply = (kind == "player" ? "Player report #" : "Bug report #")
        + std::to_string(r.id) + " sent - thanks!";
    return r;
}

} // namespace BugReports
