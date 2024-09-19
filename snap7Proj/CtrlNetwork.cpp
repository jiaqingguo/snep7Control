#include <iostream>
#include <WinSock2.h>
#include "CtrlNetwork.h"

#pragma comment(lib, "Ws2_32.lib")

#define SOCKET_LENGTH 1024*1024
CCtrlNetwork::CCtrlNetwork()
{
	m_socket = INVALID_SOCKET;
}

CCtrlNetwork::~CCtrlNetwork()
{
	WSACleanup();
}

int CCtrlNetwork::init(int nLocalPort)
{

	WSADATA wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
		return false;

	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == m_socket)
		return false;

	u_long iMode = 1;
	if (0 != ioctlsocket(m_socket, FIONBIO, &iMode))
	{
		std::cout << ("ioctlsocket() failed!\n");
		//std::cout << "GetOperatingState: " << state << std::endl;
		return false;
	}

	int nBufLen = SOCKET_LENGTH;
	if (0 != setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&nBufLen, sizeof(nBufLen)))
	{
		std::cout << ("setsockopt(SO_RCVBUF) failed!\n");
		return false;
	}

	if (0 != setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&nBufLen, sizeof(nBufLen)))
	{
		std::cout << ("setsockopt(SO_SNDBUF) failed!\n");
		return false;
	}

	sockaddr_in sSockAddr;
	sSockAddr.sin_family = AF_INET;
	sSockAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	sSockAddr.sin_port = htons(nLocalPort);

	if (0 != bind(m_socket, (sockaddr*)&sSockAddr, sizeof(sSockAddr)))
	{
		std::cout << ("bind() failed!(port:%d)\n", nLocalPort);
		return false;
	}
	return true;
}

int CCtrlNetwork::recvData(char* recvbuf, int size)
{
	sockaddr from;
	int addrLen = sizeof(from);
	return recvfrom(m_socket, recvbuf, size, 0, (sockaddr*)&from, &addrLen);
}

int CCtrlNetwork::sendDataTo(const char* sendbuf, int size, const struct sockaddr* to)
{
	int nSendLen = sendto(m_socket, sendbuf, size, 0, to, sizeof(sockaddr));
	return nSendLen;
}
