#include <iostream>

#include <QSettings>
#include <QTimer>
#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "snap7.h"
#include "CtrlNetwork.h"
#include "json.hpp"
#include "Protocol.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	readIni();
	// ����һ�� PLC �ͻ���
	//TS7Client Client;
    m_TS7Client = new TS7Client;
	const char* plcIP = "192.168.0.133";
	// ���ӵ�ģ��� PLC
	int result = m_TS7Client->ConnectTo(plcIP, 0, 1);  // rack=0, slot=1 ���� S7-PLCSIM
	if (result != 0)
	{
		std::cerr << "Connect PLC Error ! err msg : " << CliErrorText(result).data() << std::endl;
		//return ;
	}

	initUdp();
	// ������ʱ��
	m_timer = new QTimer(this);
	// ���ö�ʱ����ʱʱ��Ϊ3000���루3�룩
	m_timer->setInterval(500);
	// ���Ӷ�ʱ���źŵ��ۺ���
	connect(m_timer, &QTimer::timeout, this, &MainWindow::slot_timeOutRsfresh);
	// ������ʱ��
	m_timer->start();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readIni()
{
	qDebug() << QString::fromUtf8("�����ַ���"); // ȷ���������ȷ��
	QString exeDir = QCoreApplication::applicationDirPath();
	QString strIniPath = exeDir + "/SoftwareSnap7.ini";
	// ����QSettings����ָ��INI�ļ�·��
	QSettings settings(strIniPath, QSettings::IniFormat);

	// ��ȡINI�ļ��е����н�
	QStringList sections = settings.childGroups();

	// �������б�
	foreach(const QString & section, sections)
	{
		// �л���ָ���Ľ�
		settings.beginGroup(section);

		// ��ȡ�ý��µ����м�
		QStringList keys = settings.allKeys();

		if (section == "UDP")
		{
			m_sendPort = settings.value("sendPort").toInt();
			m_sendIP = settings.value("sendIp").toString();
			m_listenPort = settings.value("listenPort").toInt();



			m_sockaddr_in.sin_family = AF_INET;
			m_sockaddr_in.sin_addr.S_un.S_addr = inet_addr(m_sendIP.toStdString().c_str());
			m_sockaddr_in.sin_port = htons(m_sendPort);
		}
		else if (section == "input")
		{
			m_inputNames.clear();
			QString strNames = settings.value("names").toString();
			QStringList listNames = strNames.split(":");
			for (auto& name : listNames)
			{
				m_inputNames.append(name);
			}

		}
		else if (section == "output")
		{
			m_outputNames.clear();
			QString strNames = settings.value("names").toString();
			QStringList listNames = strNames.split(":");
			for (auto& name : listNames)
			{
				m_outputNames.append(name);
			}
		}
		else
		{
			stTagInfo tagInfo;
			tagInfo.strName = section;

			tagInfo.strType = settings.value("type").toString();
			tagInfo.iNumber = settings.value("number").toInt();
			tagInfo.iStart = settings.value("start").toInt();
			tagInfo.iPosition = settings.value("position").toInt();
			tagInfo.strDataType = settings.value("dataType").toString();
			tagInfo.iSize = settings.value("size").toInt();

			m_mapTagData.insert(tagInfo.strName, tagInfo);
		}
		settings.endGroup();
	}
}

void MainWindow::initUdp()
{
	m_udp = new CCtrlNetwork();
	m_udp->init(m_listenPort);
}

void MainWindow::slot_timeOutRsfresh()
{
	//updateRecvUdpJson();
	sendJsonPlcData();
}

void MainWindow::sendJsonPlcData()
{
	bool value = false;
	nlohmann::json jsonData;// ���� JSON ����
	for (int i = 0; i < m_outputNames.size(); i++)
	{
		int iValue=-1;
		if (m_mapTagData.contains(m_outputNames[i]))
		{
			auto& tagInfo = m_mapTagData[m_outputNames[i]];
			
			if (tagInfo.strDataType == "bool")
			{
				bool bValue = ReadSingleBool(tagInfo.iNumber, tagInfo.iStart, tagInfo.iPosition); // DB1,  5����� 6 ������ֵ
				iValue = bValue;
			}
			else if (tagInfo.strDataType == "float")
			{
				char readBuffer[4]; // ���ڶ�ȡ�� char ������
				m_TS7Client->DBRead(tagInfo.iNumber, tagInfo.iStart, tagInfo.iPosition, readBuffer); // �� DB1 �� startRealByte ��ʼ��ȡ
				// ����ȡ�� char ����ת��Ϊ float
				float readRealValue = ConvertToFloat(readBuffer); // ����ת������
				iValue = readRealValue;
			}
		}
		
		jsonData["plcdata"][i] = iValue;

	}
	std::string data = jsonData.dump();//jsonCreateString();
	if (data == "null")
		return;
	const char* sendData = data.c_str();
	int sendSize = m_udp->sendDataTo(sendData, strlen(sendData), (sockaddr*)&m_sockaddr_in);
	if (sendSize == -1) {
		std::cout << "Failed to send data: " << strerror(errno) << std::endl;
	}
}

void MainWindow::updateRecvUdpJson()
{
	if (!m_udp) return;
	const int BUF_LEN = 1024 * 100;
	static char s_buf[BUF_LEN];
	while (true)
	{

		int nLen = m_udp->recvData(s_buf, sizeof(s_buf));
		if (nLen <= 0)
		{
			break;
		}

		Json jsonData = Json::parse(s_buf);

		int jsonArraySize = jsonData["plcdata"].size();
		if (jsonArraySize > m_inputNames.size())
			return;
		for (int i = 0; i < jsonData["plcdata"].size(); i++)
		{
			int iWriteValue = jsonData["plcdata"][i];
			if (m_mapTagData.contains(m_inputNames[i]))
			{
				auto& tagInfo = m_mapTagData[m_outputNames[i]];

				if (tagInfo.strDataType == "bool")
				{
					bool bValue = iWriteValue;
					if (WriteSingleBool(tagInfo.iNumber, tagInfo.iStart, tagInfo.iPosition, bValue))
					{
						std::cout << "�ɹ�д�벼��ֵ: " << bValue << std::endl;
					}
				}
				else if (tagInfo.strDataType == "float")
				{
					
					float realValue = iWriteValue; // Ҫд��� REAL ֵ

					char realBuffer[4];
					ConvertToByteArray(realValue, realBuffer); // ʹ��ת������// �� REAL ֵת��Ϊ char

					int result = m_TS7Client->DBWrite(tagInfo.iNumber, tagInfo.iStart, sizeof(realBuffer), realBuffer); // DB1, Start, Size, Buffer
					if (result != 0)
					{
						std::cerr << "д�� REAL ֵʧ��:"  << std::endl;
					}
				}
			}
		
		}

	}
}

// �������ڽ�С���ֽ�����ת��Ϊ float
float MainWindow::ConvertToFloat(const char* buffer) {
	float value;
	// �ֶ������ֽ�˳�򣨼��� buffer[0] �����λ��
	char reversedBuffer[4];
	reversedBuffer[0] = buffer[3];
	reversedBuffer[1] = buffer[2];
	reversedBuffer[2] = buffer[1];
	reversedBuffer[3] = buffer[0];
	memcpy(&value, reversedBuffer, sizeof(value));
	return value;
}

// �������ڽ� float ת��ΪС���ֽ�����
void MainWindow::ConvertToByteArray(float value, char* buffer) {
	// �ֶ������ֽ�˳��
	char reversedBuffer[4];
	memcpy(reversedBuffer, &value, sizeof(value));
	buffer[0] = reversedBuffer[3];
	buffer[1] = reversedBuffer[2];
	buffer[2] = reversedBuffer[1];
	buffer[3] = reversedBuffer[0];
}
// ������ֵд�� char ������
void MainWindow::WriteBoolToChar(char* buffer, int position, bool value) {
	int byteIndex = position / 8; // ��ȡ�ֽ�����
	int bitIndex = position % 8; // ��ȡλ����

	if (value) {
		buffer[byteIndex] |= (1 << bitIndex); // ����λ
	}
	else {
		buffer[byteIndex] &= ~(1 << bitIndex); // ���λ
	}
}

// �� char ��������ȡ����ֵ
bool MainWindow::ReadBoolFromChar(const char* buffer, int position) {
	int byteIndex = position / 8; // ��ȡ�ֽ�����
	int bitIndex = position % 8; // ��ȡλ����
	return (buffer[byteIndex] & (1 << bitIndex)) != 0; // ����λ��ֵ
}

bool MainWindow::ReadSingleBool( int dbNumber, int start, int position) {
	// ����һ�����������洢��ȡ������
	char buffer[1]; // 1 �ֽ��㹻�洢 8 ������ֵ

	// ��ȡ DB �����а���Ŀ�겼��ֵ���ֽ�
	int result = m_TS7Client->DBRead(dbNumber, start, sizeof(buffer), buffer);

	if (result != 0) {
		char errorText[256];

		std::cerr << "��ȡ����ֵʧ��: " << errorText << std::endl;
		return false; // �����׳��쳣
	}

	// ����Ŀ�겼��ֵ���ڵ��ֽ�
	int bitIndex = position; // ��ȡλ����

	// ��Ŀ�겼��ֵ���ֽ�����ȡ����
	bool boolValue = (buffer[0] & (1 << bitIndex)) != 0;

	return boolValue;
}

bool MainWindow::WriteSingleBool( int dbNumber, int start, int position, bool value) {
	// ����һ������������ȡ��д������
	char buffer[1]; // 1 �ֽ��㹻�洢 8 ������ֵ

	// ��ȡ����Ŀ�겼��ֵ���ֽ�
	int result = m_TS7Client->DBRead(dbNumber, start, sizeof(buffer), buffer);

	if (result != 0) {
		char errorText[256];

		std::cerr << "��ȡ����ֵʧ��: " << errorText << std::endl;
		return false; // �����׳��쳣
	}

	// ����Ŀ�겼��ֵ���ڵ�λ����
	int bitIndex = position;

	// ������Ҫ���û�����ض��Ĳ���λ
	if (value) {
		buffer[0] |= (1 << bitIndex); // ���ò���ֵΪ true
	}
	else {
		buffer[0] &= ~(1 << bitIndex); // ���ò���ֵΪ false
	}

	// ���޸ĺ���ֽ�д��PLC
	//result = client.DBWrite(dbNumber, position / 8, sizeof(buffer), buffer);
	result = m_TS7Client->DBWrite(dbNumber, start, sizeof(buffer), buffer);
	if (result != 0) {
		char errorText[256];

		std::cerr << "д�벼��ֵʧ��: " << errorText << std::endl;
		return false; // �����׳��쳣
	}

	return true; // �ɹ�д��
}

void MainWindow::readPlcValue(stTagInfo& tagInfo, bool& bValue)
{
	//if (tagInfo.strDataType == "bool")
	//{
	//	bool bValue = ReadSingleBool(tagInfo.iNumber, tagInfo.iStart, tagInfo.iPosition); // DB1,  5����� 6 ������ֵ
	//	iValue = bValue;
	//}
	//else if

}

void MainWindow::readPlcValue(stTagInfo& tagInfo, int& bValue)
{
}

void MainWindow::readPlcValue(stTagInfo& tagInfo, float& bValue)
{
}
