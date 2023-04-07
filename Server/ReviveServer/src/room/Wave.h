#pragma once
#include<unordered_set>
class Enemy;
class Wave
{
public:
	Wave(int round, int max_user)
		:king_num(max_user*round),sordier_num(max_user*(round+1))
	{
		round_enemys.reserve(king_num + sordier_num);
	}
	Wave() = default;
	~Wave(){}

	const int GetKingNum() const{ return king_num;}
	const int GetSordierNum() const{ return sordier_num; }
	void PushEnemy(Enemy* en) { round_enemys.insert(en); }
	const std::unordered_set<Enemy*>& GetWaveEnemySet()const { return round_enemys; }
	int GetSize() { return round_enemys.size(); }
private:
	std::unordered_set<Enemy*>round_enemys;
	int king_num;
	int sordier_num;
};

