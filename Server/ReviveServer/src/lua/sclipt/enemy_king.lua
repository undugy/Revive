
skull_king={
	m_id= 99999,
	m_fov=900,
	m_atk_range=300,
	m_position={x=0,y=0,z=0},
	m_maxhp=0,
	m_hp=0,
	m_damege = 0,
	m_target_id=-1,
	m_curr_state="move"
}
base_pos={x=0,y=0,z=0}
base_id=-1
now_x=0;
now_z=0;

enemy_state={}

enemy_state["move"]=function (target_id)
	local t_id=target_id;
	local  pl_x=0
	local  pl_z=0
	if t_id==base_id then
		pl_x=base_pos.x
		pl_z=base_pos.z
	else
		pl_x=API_get_x(t_id);
		pl_z=API_get_z(t_id);
	end

	now_x=API_get_x(skull_king.m_id);
	now_z=API_get_z(skull_king.m_id);
	if math.sqrt(math.abs((pl_x-now_x)*(pl_x-now_x))+math.abs((pl_z-now_z)*(pl_z-now_z)))<=skull_king.m_fov then
		skull_king.m_target_id=t_id;
	end

	if math.sqrt(math.abs((pl_x-now_x)*(pl_x-now_x))+math.abs((pl_z-now_z)*(pl_z-now_z)))<=skull_king.m_atk_range then
		skull_king.m_curr_state="attack"
		API_attack(skull_king.m_id,t_id);--����� �ֱ�
	else
		API_move(skull_king.m_id,t_id);
	end

end

enemy_state["attack"]=function (target_id)
	local t_id=target_id;
	local  pl_x=0
	local  pl_z=0
	if t_id==base_id then
		pl_x=base_pos.x
		pl_z=base_pos.z
	else
		pl_x=API_get_x(t_id);
		pl_z=API_get_z(t_id);
	end

	now_x=API_get_x(skull_king.m_id);
	now_z=API_get_z(skull_king.m_id);
	if math.sqrt(math.abs((pl_x-now_x)*(pl_x-now_x))+math.abs((pl_z-now_z)*(pl_z-now_z)))<=skull_king.m_atk_range then
		skull_king.m_curr_state="attack"
		API_attack(skull_king.m_id,t_id);
	else
		skull_king.m_curr_state="move"
		API_move(skull_king.m_id,t_id);
	end
end

--state 1:idle, 2:move, 3: attack, 4:hit, 5:dead 

function initializEnemy(id, x, y, z, hp, damege,b_x,b_y,b_z,b_id)
	skull_king.m_id = id;
	skull_king.m_position.x = x;
	skull_king.m_position.y = y;
	skull_king.m_position.z = z;
	now_x=x;
	now_z=z;
	skull_king.m_maxhp=hp;
	skull_king.m_hp=hp;
	skull_king.m_damege = damege;

	base_pos.x=b_x;
	base_pos.y=b_y;
	base_pos.z=b_z;
	base_id=b_id;
	skull_king.m_target_id=b_id;
end


function event_agro_fallow( player )
   player_x = API_get_x(player);
   player_y = API_get_y(player);
   nowx = API_get_x(myid);
   nowy = API_get_y(myid);
   
end


function event_test(npc_id)
	now_x = API_get_x(npc_id);
    now_y = API_get_y(npc_id);
	API_test_lua(now_x,now_y);
end

function state_machine(id)
	enemy_state[skull_king.m_curr_state](id)
end