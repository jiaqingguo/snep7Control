/*!
 * \file CtrlNetwork.h
 * \date 2018/11/23 14:14
 *
 * \author Mr.Lu
 *
 * \brief ��ά�۲�ϯ��������ͨ��;
 *
 * TODO: long description
 *
 * \note
*/
class CCtrlNetwork
{
public:
	CCtrlNetwork();
	~CCtrlNetwork();
	// ���ؼ����˿�;
	int init(int nLocalPort);
	int recvData(char* recvbuf, int size);
	int sendDataTo(const char* sendbuf, int size, const struct sockaddr* to);
protected:
	unsigned __int64 m_socket;
private:
};