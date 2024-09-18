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


	// 函数用于将小端字节数组转换为 float
	float ConvertToFloat(const char* buffer);

	// 函数用于将 float 转换为小端字节数组
	void ConvertToByteArray(float value, char* buffer);
	// 将布尔值写入 char 缓冲区
	void WriteBoolToChar(char* buffer, int position, bool value);

	// 从 char 缓冲区读取布尔值
	bool ReadBoolFromChar(const char* buffer, int position);

	bool ReadSingleBool(TS7Client& client, int dbNumber, int start, int position);

	bool WriteSingleBool(TS7Client& client, int dbNumber, int start, int position, bool value);
private:
    Ui::MainWindow *ui;

    TS7Client *m_TS7Client =nullptr;
};
#endif // MAINWINDOW_H
