#pragma once

struct DifficultyScalar {
	float HP;
	float Dmg;
	DifficultyScalar (float pHP, float pDmg)
		:HP (pHP)
		,Dmg (pDmg)
	{
		//- if one of the value is not zero, the other shouldn't be zero but 1.0 instead,
		//-	as to not mess up when used in the multiplications
		//-
		if (operator bool ()) {
			if (HP ==0.0f) HP =1.0f;
			if (Dmg==0.0f) Dmg=1.0f;
		}
	}
	DifficultyScalar ():DifficultyScalar (0.0f, 0.0f) {}
	explicit operator bool () const { return (HP>0.0f)||(Dmg>0.0f); }
  void zero () { HP=0.0f; Dmg=0.0f; }
	//void unity () { HP=1.0f; Dmg=1.0f; }
	void operator *= (float modifier) {
		HP *= modifier;
		Dmg *= modifier;
	}
	void operator *= (DifficultyScalar other) {
		HP *= other.HP;
		Dmg *= other.Dmg;
	}
	static DifficultyScalar Unity ()
	{
		static const DifficultyScalar unity (1.0f,1.0f);
		//-
		return unity;
	}
};
