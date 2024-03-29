#include"pch.h"
#include "player.h"
#include"iocp/iocp_server.h"
void Player::SetMatchingState(int room_id)
{
	
	state_lock.lock();
	SetState(STATE::ST_INGAME);
	state_lock.unlock();
	SetRoomID(room_id);
	SetIsActive(true);
}
void Player::DoRecv()
{
	DWORD recv_flag = 0;
	ZeroMemory(&m_recv_over._wsa_over, sizeof(m_recv_over._wsa_over));
	m_recv_over._wsa_buf.buf = reinterpret_cast<char*>(m_recv_over._net_buf + m_prev_size);
	m_recv_over._wsa_buf.len = sizeof(m_recv_over._net_buf) - m_prev_size;
	int ret = WSARecv(m_socket, &m_recv_over._wsa_buf, 1, 0, &recv_flag, &m_recv_over._wsa_over, NULL);
	if (SOCKET_ERROR == ret) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num)
			IOCPServer::error_display(error_num);
	}
}

void Player::DoSend(int num_bytes, void* mess)
{
	if (m_socket == INVALID_SOCKET) return;
	EXP_OVER* ex_over = new EXP_OVER(COMP_OP::OP_SEND, num_bytes, mess);
	int ret = WSASend(m_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
	if (SOCKET_ERROR == ret) {
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num)
			IOCPServer::error_display(error_num);
	}
}

void Player::Init(SOCKET&c_socket)
{
	m_prev_size = 0;
	m_recv_over._comp_op = COMP_OP::OP_RECV;
	m_recv_over._wsa_buf.buf = reinterpret_cast<char*>(m_recv_over._net_buf);
	m_recv_over._wsa_buf.len = sizeof(m_recv_over._net_buf);
	ZeroMemory(&m_recv_over._wsa_over, sizeof(m_recv_over._wsa_over));
	m_socket = c_socket;

}

void Player::ResetPlayer()
{
	
	m_is_active = false;

	ZeroMemory(m_password, MAX_PASSWORD_SIZE + 1);
	m_hp = PLAYER_HP;//추후 밸런스 조정
	m_maxhp = m_hp;
	m_mach_user_size = 0;
	m_socket = INVALID_SOCKET;
}

void Player::Reset()
{
	m_is_active = false;
	m_mach_user_size = 0;
	m_last_move_time = 0;
	m_room_id = -1;
	m_hp = PLAYER_HP;//추후 밸런스 조정
	m_maxhp = m_hp;
	m_damage = PLAYER_DAMAGE;
	m_is_ready = false;

	ZeroMemory(m_name, MAX_NAME_SIZE + 1);
	ZeroMemory(m_password, MAX_PASSWORD_SIZE + 1);
	state_lock.lock();
	m_state = STATE::ST_ACCEPT;
	state_lock.unlock();

}

bool Player::IsDamaged(){
	std::lock_guard<std::mutex>guard{ this->m_hp_lock };
	if (m_hp < m_maxhp)
		return true;
	else
		return false;
}

float Player::Heal()
{
	std::lock_guard<std::mutex>guard{ this->m_hp_lock };
	m_hp += (PLAYER_HP / 10);
	if (m_hp > m_maxhp)
		m_hp = m_maxhp;
	return m_hp;
}
