#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <WinSock2.h>

class TS7Client;
class CCtrlNetwork;
class   QTimer;

struct stTagInfo;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

	void readIni();

	void initUdp();

private slots:
	void slot_timeOutRsfresh();
private:
	void sendJsonPlcData();

	void updateRecvUdpJson();
	// �������ڽ�С���ֽ�����ת��Ϊ float
	float ConvertToFloat(const char* buffer);

	// �������ڽ� float ת��ΪС���ֽ�����
	void ConvertToByteArray(float value, char* buffer);
	// ������ֵд�� char ������
	void WriteBoolToChar(char* buffer, int position, bool value);

	// �� char ��������ȡ����ֵ
	bool ReadBoolFromChar(const char* buffer, int position);

	/*bool ReadSingleBool(TS7Client& client, int dbNumber, int start, int position);
	bool WriteSingleBool(TS7Client& client, int dbNumber, int start, int position, bool value);*/

	bool ReadSingleBool( int dbNumber, int start, int position);

	bool WriteSingleBool( int dbNumber, int start, int position, bool value);


	void readPlcValue(stTagInfo& tagInfo, bool& bValue);
	void readPlcValue(stTagInfo& tagInfo, int& bValue);
	void readPlcValue(stTagInfo& tagInfo, float& bValue);
private:
    Ui::MainWindow *ui;

    TS7Client *m_TS7Client =nullptr;

	int m_sendPort;
	QString m_sendIP;
	int m_listenPort;
	sockaddr_in m_sockaddr_in;

	QVector<QString> m_inputNames;
	QVector<QString> m_outputNames;


	CCtrlNetwork* m_udp;
	QTimer* m_timer = nullptr;

	QMap<QString, stTagInfo> m_mapTagData;
};
#endif // MAINWINDOW_H
