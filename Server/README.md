# Server

------
## 목차
1. 구성
2. 소스코드 설명
----
## 01. 구성

```
스레드 구성(15개)
  * Worker : 12개
  * Main : 1개
  * Timer: 1개
  * DB: 1개 
전체 구성
- iocp_server(프레임 워크) <-ingame_server (상속)
- ingame_server has PacketManager
- 실질적인 로직은 PacketManager에서
- PacketManager has RoomManager, MapManager, DB
```
- <span style="background-color:#fff5b1">솔루션은 GenerateProject_2019/2022.bat 파일로 생성 가능합니다 </span>
## 02. 소스코드 설명
------
### 1. IOCPServer

  - 멤버변수
    - accept_ex : accept에서 재활용할 확장 오버렙드 구조체
    - m_s_socket: 리슨 소켓
    - m_hiocp: iocp 핸들
    - m_worker_num : worker thread 개수
    - m_worker_threads: worker 컨테이너
  - Init 
    - listen 소켓 초기화
  - BindListen
    - bind
    - listen(SOMAXCONN으로)
    - iocp 객체 생성
    - listen소켓 iocp에 등록
    - 클라이언트 소켓 초기화 및 AcceptEX() 호출
  - CreateWorker
    - Worker 생성
  - Worker
    - GQCS 호출
    - 넘어온 EXP_OVER의 comp_op에 따라 처리
      - SEND, RECV, EVENT, ACCEPT
------
### 2. InGameServer
  - 멤버변수
    - m_PacketManager: unique_ptr, 실질적인 로직 처리
  - 생성자
    - PacketManager생성 및 초기화
  - OnAccept, OnRecv, Disconnect, DoTimer
    - PacketManager 함수 호출
  - OnEvent
    - comp_op에 따라 처리
      - NPC 오퍼레이션: NPC_MOVE, NPC_ATTACK,  BASE_ATTACK
      - 게임진행 오퍼레이션: NPC_TIMER_SPAWN, NPC_SPAWN, COUNT_TIME, HEAL(palyer)
    - PacketManager에 구현한 함수 호출
  - Run
    - worker, db, timer thread 생성 & join
  - End
    - iocp 핸들, 리슨 소켓 해제   
----------
### 3. PacketManager
- 멤버변수
    - db_thread: db 이벤트 처리를 위한 스레드 
    - m_db: DB 클래스 포인터
    - m_db_queue : concurrent_queue, db 이벤트를 넣고 빼기 위한 queue
    - m_map_manager : 맵 매니저 포인터
    - m_room_manager : 룸 매니저 포인터
    - g_timer_queue: concurrent_priority_queue, 루아를 호출해서 이벤트를 넣기위해 어쩔수 없이 전역으로 선언
-----
- Init
    - MoveObjManager의 오브젝트 풀 초기화
    - RoomManager의 오브젝트 풀 초기화
    - MapManager의 맵 읽어오기
    - DB의 MSSQL 연동
-----
- ProcessRecv

  * OnRecv에서 호출
  * 패킷 재조립 함수
  * remain_data = 받은 데이터양 + 이전 남은데이터 양
  * packet_start = EXP_OVER의 net_buf의 시작점

* 실행흐름

```c
while packet_size <= remain_data
    ProcessPacket() 호출
    remain_data에서 처리한 데이터의 크기인 packet_size를 빼준다
    packet_start가 가리키는 위치를 처리한 packet_size만큼 더해서 이동한다.
    if remain_data > 0
        이 말은 패킷을 처리하고 데이터가 아직 남아있다는 의미 이므로 packet_size를 그 다음 처리해야할 패킷의 사이즈로 바꿔준다
    else break; 더이상 조립할 수 있는 패킷이 없으니 탈출

if remain_data > 0
한 개의 패킷으로 조립할 수는 없지만 잘려져 와서 데이터가 남아있다는 뜻
남은 데이터의 크기를 prev_size에 저장
EXP_OVER의 net_buf에 실질적인 데이터를 미리저장하고 DoRecv 호출

if remain == 0 이말은 깔끔하게 비웠기 때문에 prev_size를 0으로 초기화 
```

---------
- ProcessAccept 
```
실행흐름
  1. AccepEX에서 EXP_OVER에 넣어뒀던 소켓을 꺼낸다.
  2. MoveObjManager에서 아이디 발급
  3. 유저가 많아 아이디 발급 실패시 로그인 실패 아니라면 초기화 후 iocp에 소켓과 아이디(키) 등록 그 후 recv 호출
  4. accept_ex의 오버렙드 구조체 초기화
  5. c_socket 새로 발급 후 net_buf에 넣어준다
  6. AcceptEX 호출
```
---------
- ProcessPacket
  - protocol에 따라 함수 실행
  - protocol : CS_SIGN_IN, CS_SIGN_UP, CS_MOVE, CS_ATTACK, CS_MATCHING, CS_HIT, CS_GAME_START
```
실행함수
- ProcessSignIn: db_queue에 db_task DB_TASK_TYPE::SIGN_IN push
  
- ProcessSignUp: db_queue에 db_task DB_TASK_TYPE::SIGN_UP push 
    
- ProcessMove: 힐존인지 검사 후 broadcasting, 힐존이라면 회복 이벤트 타이머 큐에 추가
    
- ProcessAttack: 포워드 벡터 broadcasting 
    
- ProcessMatching: 매칭인원 설정, is_matching 을 true로 변경, 매칭인원과 is_matching이 true인 객체가 있다면 빈방을 가져와 초기화, 매칭된 플레이어의 is_matching을 false로 설정하고 ST_INGAME으로 상태를 바꾸고 maching ok 패킷을 보낸다, 그 후 npc를 인원수에 따라 방에 넣어준다.

- ProcessHit: 맞은 객체와 hp정보를 broadcasting, 만약 죽었다면 Dead 패킷을 보냄, 플레이어가 죽었다면 게임패배 패킷과 EndGame함수 호출

- ProcessGameStart: IsReady를 true로 설정, 모든 플레이어가 ready라면 StartGame함수 호출
```
-----------
- 이벤트 함수

- SpawnEnemy
  - 라운드 체크 및 웨이브정보 패킷 송신
  - 라운드에 따른 해골킹 및 해골병사 숫자를 정하고 현재 게임에 나오고 있지 않은 객체를 선별한다.
  - EVENT_NPC_TIMER_SPAWN 이벤트를 넣어준다.

- SpawnEnemyByTime
  - random_device와 mt19937을 이용해서 랜덤한 값을 정한다.
  - SPAWN_AREA에 해당하는 지역에 랜덤한좌표를 통해 소환하는 위치를 정한다. 
  - 적 객체의 SpawnPoint를 정한후 SendObjInfo로 패킷을 보낸다.
  - EVENT_NPC_MOVE 이벤트를 넣어준다.

- DoEnemyMove
  - 타겟의 아이디가 기지라면 기지의 위치를 target_pos로 설정
  - 아니라면 타겟 오브젝트의 위치를 가져와 target_pos로 설정
  - 일단 적 객체의 DoMove함수로 직선이동을 하고 CheckMoveOK함수를 통해 갈수있는 지 확인한다.
  - 가능하면 이동하고 아니라면 직선이동전 위치를 가져와 타겟을 기준으로 8방향 이동을 시도해본다 map에 거리와 방향을 넣는다.
  - map에 원소가 있다면 가장 첫번째 원소로 이동하고 없다면 이동하지 않고 기다린다.
  - 이렇게 과정이 끝나면 움직인 좌표를 보내주고 CallStateMachine함수를 호출한다.

- CountTime
  - 방의 round time을 가져와 현재시간과 비교한다
  - 시간의 흐름을 플레이어에게 보내준다.
  - RoundTime만큼 시간이 지났으면 다시 시간을 다시 설정하고 라운드가3 이하라면 EVENT_NPC_SPAWN 이벤트를 넣어준다.
  - 라운드가 3이라면 살아있는 적의 숫자를 파악하고 살아있는 적이없다면 SendGameWin함수 호출 후 EndGame함수를 호출한다.
  - EVENT_TIME 이벤트를 넣어주어 계속 시간을 체크할 수 있도록 넣어준다.


- DoEnemyAttack
  - target_id가 BASE라면 MapManager로 BASE의 위치를 가져온다.
  - EVENT_BASE_ATTACK을 넣어준다.
  - SendNPCAttackPacket으로 npc공격 패킷을 보낸다.
  - 공격 시간은 1초에 한번이기 때문에 attack_time을 업데이트 해주고 CallStateMachine을 호출한다.

- BaseAttackTime
  - BASE의 hp를 적의 데미지만큼 깎아준다.
  - SendBaseStatus를 통해 변경된 hp를 클라에 보내준다.
  - 만약 BASE hp가 0보다 적다면 SendGameDefeat 패킷을 보내 게임패배를 알린다.
  - EndGame호출

- ActivateHealEvent
  - 플레이어의 hp 10% 회복 is_heal을 false로 바꿔준다.
  - 상태변화를 SendStatusChange를 호출해서 보내준다.
  - 플레이어의 체력이 Max hp보다 작고, 아직 힐존에 있다면 EVENT_HEAL을 넣어준다.

-------------
- Send 함수
  - palyer의 DoSend 호출 
```c++
  //MoveObject의 좌표와 last_move_time을 보낸다.
  void SendMovePacket(int c_id, int mover);
  //공격자 id와 목표 id를 보낸다.
	void SendNPCAttackPacket(int c_id,int obj_id, int target_id);
  //로그인 실패이유를 보낸다.
	void SendLoginFailPacket(int c_id, int reason);
  // 유저가 전부꽉 찼을때 사용
	void SendLoginFailPacket(SOCKET& c_socket, int reason);
  //로그인 성공 패킷
	void SendSignInOK(int c_id);
  //회원가입 성공 패킷
	void SendSignUpOK(int c_id);
  //매칭이 잡혔다는 것을 알려줌
	void SendMatchingOK(int c_id);
  //플레이어와 NPC의 정보를 보낸다(hp,damage 등)
	void SendObjInfo(int c_id, int obj_id);
	//시간을 보냄
  void SendTime(int c_id,float round_time);
  //foward 벡터와 공격자의 id를 보낸다.
	void SendAttackPacket(int c_id, int attacker,const Vector3&forward_vec);
  //기지 현재체력을 보낸다.
	void SendBaseStatus(int c_id ,int room_id);
  //NPC 또는 캐릭터의 현재체력을 보낸다.
	void SendStatusChange(int c_id, int obj_id, float hp);
  //게임의 승패를 보낸다.
	void SendGameWin(int c_id);
	void SendGameDefeat(int c_id);
  //NPC또는 Player의 죽음을 보낸다.
	void SendDead(int c_id,int obj_id);
  //라운드정보(현재 라운드, 해골병사 수, 해골킹 수)를 보낸다.
	void SendWaveInfo(int c_id,int curr_round, int king_num, int sordier_num);

```
------
- Timer

- ProcessTimer
  - timer 스레드 함수
  - 이중 while문으로 구성
  - 첫 while문은 루프를 위해서 존재한다 두번쨰 while문을 빠져나오게 됐을 때 10ms sleep한다.
  - 두번째 while 문에서는 g_timer_queue에서 timer_event를 꺼내고 이벤트에 지정해놨던 시간이라면 ProcessEvent를 호출한다.
  - 만약 시간이 안됐지만 10ms보다 같거나 작다면 남은 시간만큼 sleep하고 ProcessEvent를 호출한다.
  - 그 외에는 timer_queue에 다시 push한다. 그후 break해 while문 탈출한다.

- ProcessEvent
- EVENT_TYPE에 따라 COMP_OP를 넣어 PQCS를 호출한다.
  - EVENT_NPC_SPAWN -> OP_NPC_SPAWN
  - EVENT_NPC_TIMER_SPAWN -> OP_NPC_TIMER_SPAWN
  - EVENT_NPC_MOVE -> OP_NPC_MOVE
  - EVENT_NPC_ATTACK -> OP_NPC_ATTACK
  - EVENT_TIME -> OP_COUNT_TIME
  - EVENT_BASE_ATTACK -> OP_BASE_ATTACK
  - EVENT_HEAL -> OP_HEAL
- 예외사항
  - EVENT_REFRESH_ROOM : ResetRoom을 호출하고 방의 상태를 RT_FREE로 바꾼다.

- static SetTimerEvent : timer_event를 초기화하는 함수 
  - 그냥 생성자로 만들껄 후회

-------------
- DB EVENT

- DB Thread: db_queue에서 db_task를 빼서 ProcessDBTask 함수를 실행한다. db_task가 없다면 10ms sleep한다.

- ProcessDBTask
```
SIGN_IN과 SIGN_UP에 따라서 DB이벤트를 처리한다. 
```
 - SIGN_IN 
    - player중 해당아이디를 가지고 플레이중인 사람이 없는지 탐색한다
    - 해당아이디를 가진 사람이 플레이 중이라면 SendLoginFailPacket으로 로그인 실패를 알린다.
    - db를 통해 해당아이디가 db에 있다면 STATE를 ST_LOGIN으로 바꾼다. 없다면 실패 패킷을 보낸다.
    - 로그인 성공 패킷을 보낸다.

  - SIGN_UP
    - player의 id가 db에 존재하지 않는다면 db에 데이터를 저장하고 성공패킷을 보낸다.
    - id가 있다면 실패를 보낸다.
----------
- 기타 함수

- IsRoomInGame
  - Room이 RT_INGAME인지 체크한다. 이후 이벤트에서 게임이 끝났을 때 이벤트가 실행되는 것을 방지하기 위해 사용

- EndGame
  - 방의 상태를 RT_RESET으로 바꾸고 방에있는 오브젝트들을 초기화한다.
  - EVENT_REFRESH_ROOM 이벤트를 넣는다.

- CallStateMachine
  - 현재의 위치에서 기지까지의 거리와 플래이어의 거리를 비교하고 가장 가까운객체를 타겟아이디로 설정한다.
  - 타겟의 설정 딜레이 시간을 초기화한다.
  - lua의 state_machine을 호출하고 target_id를 넣어준다.

- CheckMoveOk
  - 적이 활동영역을 벗어나는지 체크한다
  - 콜리전이 부딪히는 곳이 없는지 체크한다
  - 만약 이 중에 하나라도 충족 못할 경우 return false
  - 조건을 충족했다면 다른 npc와 겹치는지 체크하고 겹치지 않는다면 return true한다.    
-----------
### 04. DB

- Init
  - HENV, HDBC, HSTMT를 할당하고 초기화한다.
  - DB를 연결한다.
```c++
DB 연결  
SQLConnect(hdbc, (SQLWCHAR*)L"Revive_wireless2", SQL_NTS, (SQLWCHAR*)L"Revive_con", SQL_NTS, (SQLWCHAR*)L"revive",SQL_NTS);
```

- SaveData
  - 인자로 받아온 아이디와 패스워드를 인자로 넘겨
  -  Stored Procedure를 실행한다.
  ```c++
  wsprintf(exec, L"EXEC insert_user_info @Param1=N'%ls',@Param2=%ls" ,wname, wpassword);
	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)exec, SQL_NTS);
  ```
  - 커서를 닫고 stmt를 unbind시킨다.
  - 결과에 따라 DB_ERROR, OK를 리턴한다.
  
- CheckLoginData
  - 인자로 받아온 아이디와 패스워드를 인자로 넘겨
  -  Stored Procedure를 실행한다.
  ```c++
  wsprintf(exec, L"EXEC select_user_info @Param1=N'%ls'", wname);
	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)exec, SQL_NTS);
  ```
  - SQLFetch를 통해서 받아온 데이터가 있는지 확인한다.
  - 만약 아이디가 없으면 NO_ID
  - 있다면 비밀번호가 맞는지 체크한다
  - 커서를 닫고 stmt를 unbind시킨다.
  - 결과에 따라 NO_ID,WRONG_PASSWORD 를 리턴한다.

- CompWcMc: 멀티바이트 문자열과 와이드 캐릭터 문자열 비교하는 함수

- HandleDiagnosticRecord: 오류메세지 출력 함수

----------
LUA

- API_get_x, API_get_y, API_get_z
  - 오브젝트의 좌표를 루아에 넣어주는 API


- API_attack
  - lua에서 가져온 아이디를 통해 해당 npc의 공격 타임을 체크하고 공격 시간이 아직 안되었다면 남은시간만큼 이벤트에 딜레이를 설정해서 넣어준다.
  - 공격 시간이 되었다면 바로공격할 수 있도록 넣어준다.

- API_move
  - target_id를 설정하고 EVENT_NPC_MOVE 이벤트를 넣어준다.

- 루아 스크립트
기획 데이터
```lua
skull_sordier={
	m_id= 99999,
	m_fov=900,
	m_atk_range=850,
	m_position={x=0,y=0,z=0},
	m_maxhp=0,
	m_hp=0,
	m_damege = 0,
	m_target_id=-1,
	m_curr_state="move"
}
```
상태 머신
```lua
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

	now_x=API_get_x(skull_sordier.m_id);
	now_z=API_get_z(skull_sordier.m_id);
	if math.sqrt((math.abs(pl_x-now_x)^2)+(math.abs(pl_z-now_z)^2))<=skull_sordier.m_fov then
		skull_sordier.m_target_id=t_id;
	end
	
	if math.sqrt((math.abs(pl_x-now_x)^2)+(math.abs(pl_z-now_z)^2))<=skull_sordier.m_atk_range  then
		skull_sordier.m_curr_state="attack"
		API_attack(skull_sordier.m_id,t_id);
	else
		API_move(skull_sordier.m_id,t_id);
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

	now_x=API_get_x(skull_sordier.m_id);
	now_z=API_get_z(skull_sordier.m_id);
	if math.sqrt((math.abs(pl_x-now_x)^2)+(math.abs(pl_z-now_z)^2))<=skull_sordier.m_atk_range then
		skull_sordier.m_curr_state="attack"
		API_attack(skull_sordier.m_id,t_id);
	else
		skull_sordier.m_curr_state="move"
		API_move(skull_sordier.m_id,t_id);
	end
end

function state_machine(id)
	enemy_state[skull_sordier.m_curr_state](id)
end
```
------------
### 05.Map

#### 1. class MapInfo
-  오브젝트 이름, 위치, 콜리전 및 메쉬의 개수를 저장 하기위한 클래스

#### 2. class MapManager
- 멤버변수
  - m_map_objects: MapObj를 저장하기 위한 vector

- LoadMap
  - stringstream을 통해 읽는다.
  - 읽는 정보: ActorName, BoxCollision, Position, Scale
  - 이후 읽어온 데이터 중 콜리전이 0이 아니라면
  - 각각 오브젝트의 이름에 따라 OBJTYPE을 지정하고 m_map_objects에 넣어준다.
    - OBJTYPE: OT_BASE, OT_MAPOBJ, OT_ACTIViTY_AREA, OT_HEAL_ZONE, OT_SPAWN_AREA

- CheckCollision : 콜리전을 AABB로 겹치는것이 있는지 확인한다. 있다면 true리턴

- CheckInRange : 
  - bitset을 이용하여 모든 부분에서 겹치는 것이 없는지 확인한다. return은 bitset의 all메소드로 확인 
  - 이 함수는 콜리전과 콜리전간의 비교와 콜리전과 점간의 비교도 있다. -> 오버로딩

-----------
### 06. Object

#### 1.Object
- 모든 오브젝트의 부모
- 멤버변수: id, type, pos
#### 2.MapObj(Object 상속)
- 멤버변수
  - is_blocking: 콜리전이 Area인지 블록인지 구분하는 변수
  - min_pos
  - max_pos
  - extent : 콜리전
  - ground_pos: 2d좌표
#### 3.MoveObj(Object를 상속)
- 멤버변수
  - hp, maxhp
  - damage
  - hp_lock
  - last_move_time: 성능측정을 위한 변수
  - room_id
  - color_type
  - name
  - origin_pos(초기 위치)

#### 4.Player(MoveObj 상속)
- 멤버변수
  - state_lock(상태 변화할때 사용하는 lock)
  - m_prev_size(recv했을 때 남았던 데이터양)
  - is_matching(매칭을하는 중인지 아닌지)
  - recv_over(recv에 사용하는 확장오버렙드 구조체)
  - socket
  - state(FREE, ACCEPT, LOGIN, INGAME)
  - is_ready(매칭 후 게임시작을 눌렀는지)
  - is_heal(회복중인지)
  - password
  - mach_user_size: 매칭에 요청한 인원수
- DoRecv
  - 오버렙드 구조체 초기화
  - WSABUF에 이전 net_buf에 남은 사이즈를 미리 저장해 두었기 때문에 그만큼 이동한 위치를 넣어준다.
  - WSARecv를 호출한다.

- DoSend
  - EXP_OVER 할장 후 WSASend호출

- Reset
  - 멤버변수 초기화, state(ACCEPT)

#### 5. Enemy(MoveObj 상속)

- 멤버변수
  - in_use(방에 할당한 npc 인가)
  - in_game(게임에서 살아서 움직이는 중인가)
  - lua_lock
  - move_time(움직임 주기 체크용)
  - attack_time(공격주기 체크용)
  - check_time(target 설정 주기 체크용)
  - collision
  - prev_collision
  - L(루아 VM 포인터)
  - m_look(방향벡터 움직일 때 사용)
  - target_id
- InitEnemy
  - 변수 초기화 및 루아할당
- SetSpawnPoint : 초기 위치 설정
- Reset
  - 변수 초기화 및 루아 할당해제
- DoMove
  - 타겟의 벡터를 노말라이즈하고 MAX_SPEED를 곱해 현재 위치에 더해준다.
  - collision또한 같이 이동해준다.

- DoPrevMove
  - 움직이기 이전의 좌표에서 이동해보는 함수
  - DoMove와 로직은 똑같음

#### 6. MoveObjManager
- 싱글톤 패턴을 이용함
- 멤버변수
 - m_pInst(싱글톤을 위한 인스턴스)
 - moveobj_arr(MAX_USER+MAX_NPC 크기의 배열, 오브젝트풀로 사용)

- IsNear: 시야범위안에 객체있는지 확인

- ObjDistance: 객체간의 거리측정

- InitLua: 루아의 initializEnemy 호출해 초기화한 값을 루아에 넘겨준다

- RegisterAPI: 루아함수등록

- GetNewID
  - 배열을 순회하면서 ST_FREE인 객체를 찾는다.
  - 만약 있다면 해당아이디 반환 없다면 -1을 리턴한다.

- Disconnect: player의 소켓을 closesocket하고 ResetPlayer 호출

- DestroyObject: 할당된 오브젝트를 해제한다.
--------
### 07.Room

#### 1. class Room
- 멤버변수
  - base_hp_lock
  - state_lock
  - room_id
  - max_user(user_num * NPC_PER_USER)
  - curr_round
  - room_state(FREE, INGAME, RESET)
  - obj_list(할당된 객체의 아이디 저장)
  - round_time(웨이브 시간체크용)
  - base_hp

- Init : max_user,max_npc,curr_round 초기화

- EnterRoom : obj_list에 c_id push

- ResetRoom : 멤버변수 초기화

#### 2. RoomManager
- 멤버변수
  - rooms(MAX_ROOM_SIZE 크기 만큼의 array)

- InitRoom : rooms에 객체를 할당

- DestroyRoom : 할당된 객체를 delete해준다.

- GetEmptyRoom : RT_FREE인 room을 찾아서 해당룸의 아이디를 반환하고 없다면 -1리턴
-------
### 08. Util

#### 1. Collision
- class Collisioner
  - 멤버변수: scale, local_pos, center_pos
  - center = player_pos+(local_pos* scale)

- class BoxCollision
  - 멤버변수: extent, min_pos, max_pos
  - UpdateCollision: 센터를 new_pos+local_pos로 업데이트
  - min_pos, max_pos도 업데이트 한다.

- class CollisionChecker
  - CheckCollisions: 2d aabb로 충돌을 체크한다.
  - CheckInRange: 영역에 점이 있는지 체크

#### 2.Vector 2,3,4

- Normalrize: 벡터 정규화
- VectorScale: 벡터 크기
- Dist: 점간 거리
- Cross: 외적

---------
### 09. 정의

#### 1. define

- constant value
  - BUFSIZE: EXP_OVER의 net_buf 사이즈
  - ATTACK_RANGE: player 공격 사거리
  - GROUND_HEIGHT : 땅의 시작점(0이 아니다)

- EXP_OVER
  - 멤버변수
    - wsa_over(오버렙드 구조체)
    - COMP_OP(어떤 명령인지)
    - wsa_buf
    - net_buf
    - target_id
    - room_id

- timer_event
  - 타이머 이벤트에 넘기는 구조체
  - 우선순위 큐에 넣기위해 비교 operator 정의
  ```c++
  constexpr bool operator < (const timer_event& _Left) const
	{
		return (start_time > _Left.start_time);
	}
  ``` 

- db_task: db에 로그인 또는 회원가입에 이용

### 2. Protocol
```c++ 
기획데이터
const short SERVER_PORT = 9000;

//cm기준, 타일 : 36 x 16
const int  WORLD_HEIGHT = 4800;// 전체맵 크기는 아님
const int  WORLD_WIDTH = 10800;

const int  MAX_NAME_SIZE = 20; // 아이디 최대 사이즈
const int  MAX_PASSWORD_SIZE = 20;// 비밀 번호 최대 사이즈
const int  MAX_CHAT_SIZE = 100;// 채팅 최대 사이즈
const int  MAX_ROOM_SIZE = 2500;//방 최대 사이즈


constexpr int  MAX_USER = MAX_ROOM_SIZE * 3; //최대 동접 가능 인원
const int  NPC_PER_USER = 15;//사람하나당 최대 npc
const int  SORDIER_PER_USER = 9;//사람하나당 최대 해골 병사
const int  KING_PER_USER = 6;//사람하나당 최대 해골킹
constexpr int  MAX_NPC = MAX_USER * NPC_PER_USER; //최대 npc 개수

const float FramePerSecond = 0.05f;
const float SPEED_PER_SECOND = 225.0f;
const float MAX_SPEED = SPEED_PER_SECOND * FramePerSecond; //추후 수정, 플레이어 이동 속도 //225 cm/s
const float MOVE_DISTANCE = 1.0f;//플레이어 이동 거리
const float PLAYER_DAMAGE = 1.0f;
const float FOV_RANGE = 900.0f;

const float SKULL_HP = 5 * PLAYER_DAMAGE;
const float SKULLKING_HP = 10 * PLAYER_DAMAGE;

const float PLAYER_HP = 30.0f;
const float BASE_HP = 50.0f;

const float KING_DAMAGE = 2.0f;
const float SORDIER_DAMAGE = 1.0f;

constexpr int NPC_ID_START = MAX_USER;
constexpr int NPC_ID_END = MAX_USER + MAX_NPC - 1;
constexpr int BASE_ID = NPC_ID_END + 2;
```

```c++
프로토콜
const char CS_PACKET_SIGN_IN = 1;
const char CS_PACKET_SIGN_UP = 2;
const char CS_PACKET_MOVE = 3;
const char CS_PACKET_ATTACK = 4;
const char CS_PACKET_CHAT = 5;
const char CS_PACKET_MATCHING = 6;
const char CS_PACKET_HIT = 7;
const char CS_PACKET_GAME_START = 8;
const char CS_PACKET_DAMAGE_CHEAT = 9;


const char SC_PACKET_SIGN_IN_OK = 1;
const char SC_PACKET_SIGN_UP_OK = 2;
const char SC_PACKET_MOVE = 3;
const char SC_PACKET_PUT_OBJECT = 4;
const char SC_PACKET_REMOVE_OBJECT = 5;
const char SC_PACKET_CHAT = 6;
const char SC_PACKET_LOGIN_FAIL = 7;
const char SC_PACKET_STATUS_CHANGE = 8;
const char SC_PACKET_MATCHING = 9;
const char SC_PACKET_OBJ_INFO = 10;
const char SC_PACKET_TIME = 11;
const char SC_PACKET_TEST = 12;
const char SC_PACKET_NPC_ATTACK = 13;
const char SC_PACKET_ATTACK = 14;
const char SC_PACKET_BASE_STATUS = 15;
const char SC_PACKET_WIN = 16;
const char SC_PACKET_DEFEAT = 17;
const char SC_PACKET_DEAD = 18;
const char SC_PACKET_WAVE_INFO = 19;
```
