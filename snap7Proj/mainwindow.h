#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


class TS7Client;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


	// �������ڽ�С���ֽ�����ת��Ϊ float
	float ConvertToFloat(const char* buffer);

	// �������ڽ� float ת��ΪС���ֽ�����
	void ConvertToByteArray(float value, char* buffer);
	// ������ֵд�� char ������
	void WriteBoolToChar(char* buffer, int position, bool value);

	// �� char ��������ȡ����ֵ
	bool ReadBoolFromChar(const char* buffer, int position);

	bool ReadSingleBool(TS7Client& client, int dbNumber, int start, int position);

	bool WriteSingleBool(TS7Client& client, int dbNumber, int start, int position, bool value);
private:
    Ui::MainWindow *ui;

    TS7Client *m_TS7Client =nullptr;
};
#endif // MAINWINDOW_H
