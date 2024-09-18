#include <iostream>


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "snap7.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_TS7Client = new TS7Client;
	const char* plcIP = "192.168.0.133";
	// 连接到模拟的 PLC
	int result = m_TS7Client->ConnectTo(plcIP, 0, 1);  // rack=0, slot=1 对于 S7-PLCSIM
	if (result != 0)
	{
		std::cerr << "Connect PLC Error ! err msg : " << CliErrorText(result).data() << std::endl;
		return ;
	}
}

MainWindow::~MainWindow()
{
    delete ui;
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

bool MainWindow::ReadSingleBool(TS7Client& client, int dbNumber, int start, int position) {
	// 创建一个缓冲区来存储读取的数据
	char buffer[1]; // 1 字节足够存储 8 个布尔值

	// 读取 DB 区域中包含目标布尔值的字节
	int result = client.DBRead(dbNumber, start, sizeof(buffer), buffer);

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

bool MainWindow::WriteSingleBool(TS7Client& client, int dbNumber, int start, int position, bool value) {
	// 创建一个缓冲区来读取和写入数据
	char buffer[1]; // 1 字节足够存储 8 个布尔值

	// 读取包含目标布尔值的字节
	int result = client.DBRead(dbNumber, start, sizeof(buffer), buffer);

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
	result = client.DBWrite(dbNumber, start, sizeof(buffer), buffer);
	if (result != 0) {
		char errorText[256];

		std::cerr << "写入布尔值失败: " << errorText << std::endl;
		return false; // 或者抛出异常
	}

	return true; // 成功写入
}