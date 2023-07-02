#include "game_maniacs.h"

namespace ManiacsBattle
{
	int damage_CE;
	int damage_Var;

	void Set_DamageCE(int i) {
		damage_CE = i;
	}
	int Get_DamageCE() {
		return damage_CE;
	}

	void Set_DamageVar(int i) {
		damage_Var = i;
	}
	int Get_DamageVar() {
		return damage_Var;
	}

	int target_CE;
	int target_Var;

	void Set_TargetCE(int i) {
		target_CE = i;
	}
	int Get_TargetCE() {
		return target_CE;
	}

	void Set_TargetVar(int i) {
		target_Var = i;
	}
	int Get_TargetVar() {
		return target_Var;
	}

	bool autoSelect;
	void Set_AutoSelect(bool i) {
		autoSelect = i;
	}
	bool Get_AutoSelect() {
		return autoSelect;
	}

	int atb_CE;
	int atb_Var;

	void Set_ATBCE(int i) {
		atb_CE = i;
	}
	int Get_ATBCE() {
		return atb_CE;
	}

	void Set_ATBVar(int i) {
		atb_Var = i;
	}
	int Get_ATBVar() {
		return atb_Var;
	}

	int state_CE;
	int state_Var;

	void Set_StateCE(int i) {
		state_CE = i;
	}
	int Get_StateCE() {
		return state_CE;
	}

	void Set_StateVar(int i) {
		state_Var = i;
	}
	int Get_StateVar() {
		return state_Var;
	}

	int stats_CE;
	int stats_Var;

	void Set_StatsCE(int i) {
		stats_CE = i;
	}
	int Get_StatsCE() {
		return stats_CE;
	}

	void Set_StatsVar(int i) {
		stats_Var = i;
	}
	int Get_StatsVar() {
		return stats_Var;
	}

	bool UpdateEventWhileAnimation = false;
	void SetUpdateEventWhileAnimation(bool b) {
		UpdateEventWhileAnimation = b;
	}
	bool GetUpdateEventWhileAnimation() {
		return UpdateEventWhileAnimation;
	}

}
