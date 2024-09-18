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
	// ���ӵ�ģ��� PLC
	int result = m_TS7Client->ConnectTo(plcIP, 0, 1);  // rack=0, slot=1 ���� S7-PLCSIM
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

bool MainWindow::ReadSingleBool(TS7Client& client, int dbNumber, int start, int position) {
	// ����һ�����������洢��ȡ������
	char buffer[1]; // 1 �ֽ��㹻�洢 8 ������ֵ

	// ��ȡ DB �����а���Ŀ�겼��ֵ���ֽ�
	int result = client.DBRead(dbNumber, start, sizeof(buffer), buffer);

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

bool MainWindow::WriteSingleBool(TS7Client& client, int dbNumber, int start, int position, bool value) {
	// ����һ������������ȡ��д������
	char buffer[1]; // 1 �ֽ��㹻�洢 8 ������ֵ

	// ��ȡ����Ŀ�겼��ֵ���ֽ�
	int result = client.DBRead(dbNumber, start, sizeof(buffer), buffer);

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
	result = client.DBWrite(dbNumber, start, sizeof(buffer), buffer);
	if (result != 0) {
		char errorText[256];

		std::cerr << "д�벼��ֵʧ��: " << errorText << std::endl;
		return false; // �����׳��쳣
	}

	return true; // �ɹ�д��
}