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
	// 创建一个 PLC 客户端
	//TS7Client Client;
    m_TS7Client = new TS7Client;
	const char* plcIP = "192.168.0.133";
	// 连接到模拟的 PLC
	int result = m_TS7Client->ConnectTo(plcIP, 0, 1);  // rack=0, slot=1 对于 S7-PLCSIM
	if (result != 0)
	{
		std::cerr << "Connect PLC Error ! err msg : " << CliErrorText(result).data() << std::endl;
		//return ;
	}

	initUdp();
	// 创建定时器
	m_timer = new QTimer(this);
	// 设置定时器超时时间为3000毫秒（3秒）
	m_timer->setInterval(500);
	// 连接定时器信号到槽函数
	connect(m_timer, &QTimer::timeout, this, &MainWindow::slot_timeOutRsfresh);
	// 启动定时器
	m_timer->start();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readIni()
{
	qDebug() << QString::fromUtf8("测试字符串"); // 确保输出是正确的
	QString exeDir = QCoreApplication::applicationDirPath();
	QString strIniPath = exeDir + "/SoftwareSnap7.ini";
	// 创建QSettings对象并指定INI文件路径
	QSettings settings(strIniPath, QSettings::IniFormat);

	// 获取INI文件中的所有节
	QStringList sections = settings.childGroups();

	// 遍历节列表
	foreach(const QString & section, sections)
	{
		// 切换到指定的节
		settings.beginGroup(section);

		// 获取该节下的所有键
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
	nlohmann::json jsonData;// 创建 JSON 对象
	for (int i = 0; i < m_outputNames.size(); i++)
	{
		int iValue=-1;
		if (m_mapTagData.contains(m_outputNames[i]))
		{
			auto& tagInfo = m_mapTagData[m_outputNames[i]];
			
			if (tagInfo.strDataType == "bool")
			{
				bool bValue = ReadSingleBool(tagInfo.iNumber, tagInfo.iStart, tagInfo.iPosition); // DB1,  5代表第 6 个布尔值
				iValue = bValue;
			}
			else if (tagInfo.strDataType == "float")
			{
				char readBuffer[4]; // 用于读取的 char 缓冲区
				m_TS7Client->DBRead(tagInfo.iNumber, tagInfo.iStart, tagInfo.iPosition, readBuffer); // 从 DB1 的 startRealByte 开始读取
				// 将读取的 char 数组转换为 float
				float readRealValue = ConvertToFloat(readBuffer); // 调用转换函数
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
						std::cout << "成功写入布尔值: " << bValue << std::endl;
					}
				}
				else if (tagInfo.strDataType == "float")
				{
					
					float realValue = iWriteValue; // 要写入的 REAL 值

					char realBuffer[4];
					ConvertToByteArray(realValue, realBuffer); // 使用转换函数// 将 REAL 值转换为 char

					int result = m_TS7Client->DBWrite(tagInfo.iNumber, tagInfo.iStart, sizeof(realBuffer), realBuffer); // DB1, Start, Size, Buffer
					if (result != 0)
					{
						std::cerr << "写入 REAL 值失败:"  << std::endl;
					}
				}
			}
		
		}

	}
}

// 函数用于将小端字节数组转换为 float
float MainWindow::ConvertToFloat(const char* buffer) {
	float value;
	// 手动调整字节顺序（假设 buffer[0] 是最低位）
	char reversedBuffer[4];
	reversedBuffer[0] = buffer[3];
	reversedBuffer[1] = buffer[2];
	reversedBuffer[2] = buffer[1];
	reversedBuffer[3] = buffer[0];
	memcpy(&value, reversedBuffer, sizeof(value));
	return value;
}

// 函数用于将 float 转换为小端字节数组
void MainWindow::ConvertToByteArray(float value, char* buffer) {
	// 手动调整字节顺序
	char reversedBuffer[4];
	memcpy(reversedBuffer, &value, sizeof(value));
	buffer[0] = reversedBuffer[3];
	buffer[1] = reversedBuffer[2];
	buffer[2] = reversedBuffer[1];
	buffer[3] = reversedBuffer[0];
}
// 将布尔值写入 char 缓冲区
void MainWindow::WriteBoolToChar(char* buffer, int position, bool value) {
	int byteIndex = position / 8; // 获取字节索引
	int bitIndex = position % 8; // 获取位索引

	if (value) {
		buffer[byteIndex] |= (1 << bitIndex); // 设置位
	}
	else {
		buffer[byteIndex] &= ~(1 << bitIndex); // 清除位
	}
}

// 从 char 缓冲区读取布尔值
bool MainWindow::ReadBoolFromChar(const char* buffer, int position) {
	int byteIndex = position / 8; // 获取字节索引
	int bitIndex = position % 8; // 获取位索引
	return (buffer[byteIndex] & (1 << bitIndex)) != 0; // 返回位的值
}

bool MainWindow::ReadSingleBool( int dbNumber, int start, int position) {
	// 创建一个缓冲区来存储读取的数据
	char buffer[1]; // 1 字节足够存储 8 个布尔值

	// 读取 DB 区域中包含目标布尔值的字节
	int result = m_TS7Client->DBRead(dbNumber, start, sizeof(buffer), buffer);

	if (result != 0) {
		char errorText[256];

		std::cerr << "读取布尔值失败: " << errorText << std::endl;
		return false; // 或者抛出异常
	}

	// 计算目标布尔值所在的字节
	int bitIndex = position; // 获取位索引

	// 将目标布尔值从字节中提取出来
	bool boolValue = (buffer[0] & (1 << bitIndex)) != 0;

	return boolValue;
}

bool MainWindow::WriteSingleBool( int dbNumber, int start, int position, bool value) {
	// 创建一个缓冲区来读取和写入数据
	char buffer[1]; // 1 字节足够存储 8 个布尔值

	// 读取包含目标布尔值的字节
	int result = m_TS7Client->DBRead(dbNumber, start, sizeof(buffer), buffer);

	if (result != 0) {
		char errorText[256];

		std::cerr << "读取布尔值失败: " << errorText << std::endl;
		return false; // 或者抛出异常
	}

	// 计算目标布尔值所在的位索引
	int bitIndex = position;

	// 根据需要设置或清除特定的布尔位
	if (value) {
		buffer[0] |= (1 << bitIndex); // 设置布尔值为 true
	}
	else {
		buffer[0] &= ~(1 << bitIndex); // 设置布尔值为 false
	}

	// 将修改后的字节写回PLC
	//result = client.DBWrite(dbNumber, position / 8, sizeof(buffer), buffer);
	result = m_TS7Client->DBWrite(dbNumber, start, sizeof(buffer), buffer);
	if (result != 0) {
		char errorText[256];

		std::cerr << "写入布尔值失败: " << errorText << std::endl;
		return false; // 或者抛出异常
	}

	return true; // 成功写入
}

void MainWindow::readPlcValue(stTagInfo& tagInfo, bool& bValue)
{
	//if (tagInfo.strDataType == "bool")
	//{
	//	bool bValue = ReadSingleBool(tagInfo.iNumber, tagInfo.iStart, tagInfo.iPosition); // DB1,  5代表第 6 个布尔值
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
