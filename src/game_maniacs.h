#include <vector>

namespace ManiacsBattle
{
	void Set_DamageCE(int i);
	int Get_DamageCE();

	void Set_DamageVar(int i);
	int Get_DamageVar();


	void Set_TargetCE(int i);
	int Get_TargetCE();

	void Set_TargetVar(int i);
	int Get_TargetVar();

	void Set_AutoSelect(bool i);
	bool Get_AutoSelect();

	void Set_ATBCE(int i);
	int Get_ATBCE();

	void Set_ATBVar(int i);
	int Get_ATBVar();

	void Set_StateCE(int i);
	int Get_StateCE();

	void Set_StateVar(int i);
	int Get_StateVar();

	void Set_StatsCE(int i);
	int Get_StatsCE();

	void Set_StatsVar(int i);
	int Get_StatsVar();

	bool GetUpdateEventWhileAnimation();
	void SetUpdateEventWhileAnimation(bool b);

}
