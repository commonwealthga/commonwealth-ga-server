#pragma once

struct DifficultyScalar {
	float HP;
	float Dmg;
	DifficultyScalar (float pHP, float pDmg):HP (pHP), Dmg (pDmg) {}
	DifficultyScalar ():DifficultyScalar (0.0f, 0.0f) {}
	explicit operator bool () const { return (HP>0.0); }
  void reset () { HP=0.0f; Dmg=0.0f; }
	void operator *= (float modifier) {
		HP *= modifier;
		Dmg *= modifier;
	}
};
