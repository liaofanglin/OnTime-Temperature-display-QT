#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "qcustomplot.h"
#include <qDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QPainter>
#include <QMessageBox>
#include <QMouseEvent>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void windows_init();
    void serialport_init();
    void read_serialport();     //读串口数据
    void find_serialport();     //查找串口
    void insert_table(QString date, QString t);

private slots:
    void onMouseMove(QMouseEvent* event);
    void customsplot_init();
    void on_open_serialport_clicked();
    void on_close_serialport_clicked();
    void on_start_clicked();
    void on_pause_clicked();
    void checkbox_change();
    void on_clear_table_clicked();
    void on_save_table_clicked();

private:
    Ui::Widget *ui;
    QTimer dataTimer;           //时间
    QSerialPort *serialport;    //串口类
    double temperature;         //温度

    QCPItemText *textItem;

    // 定时器
    QTimer *_pTimerUpdate;
    QElapsedTimer _elapsedTimer;        //之间计时
    int _fps;
    int _interval;
    bool _pause;
    double _lastPointKey;
};

#endif // WIDGET_H
