#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include <stdlib.h>

#include <QDebug>
#include <QThread>

union
{
  int i;
  float f;
}fintConv;

hid_device *handle = nullptr;

float movDist = 0.0;

bool bidirMoveFlag = false;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  static QThread *ReaderThread     = new QThread;
  static QThread *WriterThread     = new QThread;

  static CAN_Reader *CAN_Rdr  = new CAN_Reader;
  static CAN_Writer *CAN_Wrtr = new CAN_Writer;

  connect(CAN_Rdr, &CAN_Reader::readResPassing, this, &MainWindow::on_gettingData);
  connect(CAN_Rdr, &CAN_Reader::writeUsbData, CAN_Wrtr, &CAN_Writer::writeHid);

  connect(this, &MainWindow::readUsbData, CAN_Rdr, &CAN_Reader::readHid);
  connect(this, &MainWindow::writeUsbData, CAN_Wrtr, &CAN_Writer::writeHid);

  CAN_Rdr->moveToThread(ReaderThread);
  CAN_Wrtr->moveToThread(WriterThread);

  ReaderThread->start();
  WriterThread->start();

  ui->setupUi(this);

  QFont font;
  font.setStyleHint(QFont::Monospace);
  ui->textBrowser->setFont(font);
}


MainWindow::~MainWindow()
{
  delete ui;
}


hid_device* MainWindow::connectHid(void)
{
  hid_device *_handle;
  int res;
#define MAX_STR 255
  wchar_t wstr[MAX_STR];

  // Enumerate and print the HID devices on the system
  struct hid_device_info *devs, *cur_dev;

  devs = hid_enumerate(0x0, 0x0);
  cur_dev = devs;
  while (cur_dev) {
      printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls",
             cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
      printf("\n");
      printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
      printf("  Product:      %ls\n", cur_dev->product_string);
      printf("\n");
      cur_dev = cur_dev->next;
    }
  hid_free_enumeration(devs);

  // Open the device using the VID, PID,
  // and optionally the Serial number.
  _handle = hid_open(0x3208, 0x4840, NULL);

  // Read the Manufacturer String
  res = hid_get_manufacturer_string(_handle, wstr, MAX_STR);
  printf("Manufacturer String: %ls\n", wstr);

  // Read the Product String
  res = hid_get_product_string(_handle, wstr, MAX_STR);
  printf("Product String: %ls\n", wstr);

  // Read the Serial Number String
  res = hid_get_serial_number_string(_handle, wstr, MAX_STR);
  printf("Serial Number String: %ls", wstr);
  printf("\n");

  // Set the hid_read() function to be non-blocking.
  hid_set_nonblocking(_handle, 1);

  return _handle;
}


void MainWindow::on_gettingData(unsigned char *inData)
{
  static quint64 cntr = 0;

/*
  QString canIdOut    = "";
  QString canDataOut  = "";
  QString canTimeStampOut = "";
  QString dataToGui = "";
  QString canMsgCounter = "";

  unsigned  CanId = 0;

  QString canIdStr = "";

  if(ui->cb_Read->checkState() == Qt::Checked)
    {
      CanId = ((inData[7] << 24) | (inData[6] << 16) | (inData[5] << 8) | (inData[4]));

      canIdStr = QString("%1").arg( CanId, 8, 16, QChar('0')).toUpper() + "  ";

      for(int i = 0; i<7; i=i+2)
        {
          canIdOut = canIdOut + QString(canIdStr[i]) + QString(canIdStr[i+1]) + " ";
        }

      for(int datCount = 8; datCount < 16; datCount++)
        {
          canDataOut = canDataOut + " " + QString("%1").arg(inData[datCount], 2, 16, QChar('0')).toUpper();
        }

      canTimeStampOut = QString("%1").arg(inData[3], 2, 16, QChar('0')).toUpper() + " " + QString("%1").arg(inData[2], 2, 16, QChar('0')).toUpper();

      //canMsgCounter = QString::number(cntr++);

      ui->textBrowser->append("<font color = 'black'>" + canMsgCounter + " . <b><font color = 'green'>" + canIdOut + " .. <font color = 'black'>" + canDataOut + "<\b> .. <font color = 'blue'>" + canTimeStampOut + " \n");
    }
    */
    this->setWindowTitle(QString::number(cntr++));
}


void CAN_Reader::readHid(hid_device* _handle)
{
  int res;
  unsigned char ioBuf[65] = {0xFF};

  if(_handle != nullptr)
    {
      // Read requested state
      res = hid_read_timeout(_handle, ioBuf, 65, 5000);

      if (res < 0)
        {
          readErrPassing("Unable to read");
        }
      else if(res >= 0)
        {
          res = 0;
          readResPassing(ioBuf);

          if(bidirMoveFlag == true)
            {
              if(ioBuf[8] == 0x4E)
                {
                  movDist = (-1)*movDist;
                  emit writeUsbData(_handle, movDist);
                }
            }

          readHid(_handle);
        }
    }
}


void CAN_Writer::writeHid(hid_device* _handle, float _dist)
{
  int res = 0;
  unsigned char ioBuf[65] = {0xFF};

  fintConv.f = _dist;

  ioBuf[0] = 0x00; // First byte is report number
  ioBuf[1] = 0x00;
  ioBuf[2] = 0x08;
  ioBuf[3] = 0x00;
  ioBuf[4] = 0x00;
  ioBuf[5] = 0x00;
  ioBuf[6] = 0x61;
  ioBuf[7] = 0x00;
  ioBuf[8] = 0x30;
  ioBuf[9] = 0x39;
  ioBuf[10] = 0x00;
  ioBuf[11] = 0x22;
  ioBuf[12] = 0x00;
  ioBuf[13] = fintConv.i & 0xFF;
  ioBuf[14] = (fintConv.i & 0xFF00) >> 8;
  ioBuf[15] = (fintConv.i & 0xFF0000) >> 16;
  ioBuf[16] = (fintConv.i & 0xFF000000) >> 24;

  //остальные члены массива должны быть 0xFF - !! ЗАНУЛЯТЬ НЕЛЬЗЯ - будут глюки !!
  for(int i = 17; i < 65; i++)
    {
      ioBuf[i] = 0xFF;
    }

  if(_handle != nullptr)
    {
      res = hid_write(_handle, ioBuf, 65);

      if(res < 65)
        {
          qDebug() << " >== WRITE ERROR ==< \n";
        }
    }
  else
    {
      qDebug() << " >>> DEVICE ERROR <<< \n";
    }
}


void MainWindow::on_pushButton_clicked()
{
  if(handle != nullptr)
    {
      emit readUsbData(handle);
    }
}


void MainWindow::on_pushButton_2_clicked()
{
  if(handle != nullptr)
    {
      ui->le_floatDist->returnPressed();
      emit writeUsbData(handle, movDist);
    }
}


void MainWindow::on_pushButton_3_clicked()
{
  handle = connectHid();
}


void MainWindow::on_pushButton_4_clicked()
{
  ui->textBrowser->clear();
}

void MainWindow::on_le_floatDist_returnPressed()
{
  if(ui->le_floatDist->text().trimmed() == "")
    {
      movDist = 0;
    }
  else
    {
      movDist = ui->le_floatDist->text().trimmed().toFloat();
    }
}

void MainWindow::on_cb_BidirMove_stateChanged(int arg1)
{
  if(!arg1)
    {
      bidirMoveFlag = false;
    }
  else
    {
      bidirMoveFlag = true;
    }
}
