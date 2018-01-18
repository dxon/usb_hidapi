#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariant>
#include <windows.h>
#include "hidapi-0.7.0/hidapi/hidapi.h"


struct hid_device_ {
		HANDLE device_handle;
		BOOL blocking;
		size_t input_report_length;
		void *last_error_str;
		DWORD last_error_num;
		BOOL read_pending;
		char *read_buf;
		OVERLAPPED ol;
};

Q_DECLARE_METATYPE(hid_device_);

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  hid_device* connectHid(void);

signals:
  void readUsbData(hid_device*);
  void writeUsbData(hid_device*, float);

private slots:
  void on_pushButton_clicked();

  void on_pushButton_2_clicked();

  void on_pushButton_3_clicked();

 void on_gettingData(unsigned char*);


 void on_pushButton_4_clicked();

 void on_le_floatDist_returnPressed();

 void on_cb_BidirMove_stateChanged(int arg1);

private:
  Ui::MainWindow *ui;
};


class CAN_Reader : public QObject
{
  Q_OBJECT

signals:
  void readResPassing(unsigned char*);
  void readErrPassing(QString);
  void writeUsbData(hid_device*, float);

public slots:
   void readHid(hid_device*);
};


class CAN_Writer : public QObject
{
  Q_OBJECT

signals:
  void writeErrPassing(QString);

public slots:
  void writeHid(hid_device*, float);
};

#endif // MAINWINDOW_H
