#pragma once
#include<vector>
#include"object/map_object/map_object.h"
#include"map_info.h"
#include"define.h"
#include"util/collision/collisioner.h"
class MapManager
{
public:
	MapManager() {
		
	};
	~MapManager(){}
	void LoadMap(const std::string& path);
	bool CheckCollision(BoxCollision& obj_collision);
	bool CheckInRange(BoxCollision& collision);
	bool CheckInRange(const Vector3& pos,OBJ_TYPE map_type);
	Vector2 GetRandomSpawnPoint();
	const std::vector<MapObj>& GetMapObjVec() const
	{
		return m_map_objects;
	}
	MapObj& GetMapObjectByType(OBJ_TYPE type)
	{
		for (auto& obj : m_map_objects)
		{
			if (obj.GetType() == type)
			{
				return obj;
			}
		}
		return MapObj();
	}
	
	// 2400 300 2850 베이스 그라운드 포즈
private:
	std::vector<MapObj>m_map_objects;

};

