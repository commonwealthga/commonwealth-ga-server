
// skal: structure to pass around mission timings for optional altering during mission initialization

struct MissionTimings {
	int timeSecs;
	int minBossTimeSecs;
	int overtimeSecs;
	bool allowOvertime;
};
