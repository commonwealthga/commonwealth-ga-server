#pragma once
// Agency applications (AgencyApply): an agent applies to a recruiting
// agency from the Agency Directory; leaders / ranks with the invite right see
// the applicants on the Recruiting tab and accept or decline them.
// Table ga_agency_applications (created on first use).

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace AgencyApply {

enum State { kNone = 0, kPending = 2, kAccepted = 4, kDeclined = 5 };

struct App {
    int64_t id = 0, agency_id = 0, user_id = 0, character_id = 0, applied_at = 0;
    int state = kNone;
    std::string name, text, agency_name;
    uint32_t profile_id = 0;
    int64_t xp = 0;
};

// The account's latest application (pending, or accepted/declined in the last 3 days).
std::optional<App> Latest(int64_t user_id);
// One pending application per account: replaces an older pending one.
bool Apply(int64_t agency_id, int64_t user_id, int64_t character_id, const std::string& name, const std::string& text);
bool Cancel(int64_t user_id);
std::vector<App> Pending(int64_t agency_id);
std::optional<App> PendingFor(int64_t agency_id, int64_t character_id);
void SetState(int64_t app_id, int state);

}  // namespace AgencyApply
