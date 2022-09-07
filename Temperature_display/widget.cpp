#include "widget.h"
#include "ui_widget.h"
#include <QTabWidget>
#include <QFileDialog>
#include <QFile>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    _lastPointKey(0)
{
    ui->setupUi(this);
    setFixedSize( this->width(),this->height());    //窗口定大小
    setWindowTitle(tr("TemperatureDisplay"));

    serialport = new QSerialPort;
    windows_init();

    // 显示坐标
    textItem = new QCPItemText(ui->widget);
    connect(ui->widget, &QCustomPlot::mouseMove, this, &Widget::onMouseMove);

    //轴缩放槽函数
    int i = 0;
    connect(ui->checkBox_X, SIGNAL(stateChanged(int)), this, SLOT(checkbox_change()));
    connect(ui->checkBox_Y, SIGNAL(stateChanged(int)), this, SLOT(checkbox_change()));
    connect(ui->checkBox_XY, SIGNAL(stateChanged(int)), this, SLOT(checkbox_change()));
}

Widget::~Widget()
{
    delete ui,serialport;
}


void Widget::onMouseMove(QMouseEvent *event)
{

        QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(sender());
        double x = customPlot->xAxis->pixelToCoord(event->pos().x());
        double y = customPlot->yAxis->pixelToCoord(event->pos().y());
        textItem->setText(QString("(%1, %2)").arg(x).arg(y));
        textItem->position->setCoords(QPointF(x, y));
        textItem->setFont(QFont(font().family(), 10));
        customPlot->replot();


}

// 界面初始化
void Widget::windows_init()
{
//控件初始化
    ui->close_serialport->setEnabled(false);
    ui->start->setEnabled(false);
    ui->pause->setEnabled(false);
// 单选框
    QButtonGroup* pButtonGroup = new QButtonGroup(this);
    pButtonGroup->addButton(ui->checkBox_X,1);
    pButtonGroup->addButton(ui->checkBox_Y,2);
    pButtonGroup->addButton(ui->checkBox_XY,3);

// lcd 初始化
    ui->lcdNumber->setDigitCount(4);    //显示位数
    ui->lcdNumber->setMode(QLCDNumber::Dec);    //十进制
    ui->lcdNumber->setSegmentStyle(QLCDNumber::Flat);       //显示方式
// 表格初始化
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setAlternatingRowColors(true);     //设置隔行变颜色
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);       //选中整行
    //设置表头
    QStringList header;
    header<<tr("时间")<<tr("温度");
    ui->tableWidget->setHorizontalHeaderLabels(header);
    ui->tableWidget->setColumnWidth(0,100);
    ui->tableWidget->setColumnWidth(1,80);

//绘图初始化
    // 添加曲线 1
    ui->widget->addGraph();         // 增加图层
    ui->widget->graph(0)->setPen(QPen(Qt::red));
    ui->widget->graph(0)->setBrush(QBrush(QColor(0,0,205,50)));     // 设置图层画刷颜色
    ui->widget->graph(0)->setAntialiasedFill(false);        // 设置图层反锯齿：关闭
    // 添加点
    ui->widget->addGraph();
    ui->widget->graph(1)->setPen(QPen(Qt::blue,3));       // 设置笔的颜色
    ui->widget->graph(1)->setLineStyle(QCPGraph::lsNone);   // 不画线条
    ui->widget->graph(1)->setScatterStyle(QCPScatterStyle::ssDisc);     // 设置点的形状

    //坐标轴1设置
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%m:%s");             //时间格式：  秒
    ui->widget->xAxis->setTicker(timeTicker);       // 横坐标显示时间
    ui->widget->axisRect()->setupFullAxesBox();     //坐标为矩
    // x轴设置
    ui->widget->xAxis->setLabel("时间(单位s)");         // 设置横轴标签
    ui->widget->yAxis->setRange(-10,70);       //设置温度范围
    ui->widget->xAxis->setLabel("时间");
    ui->widget->yAxis->setLabel("温度");
    //ui->widget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);    //设置属性可缩放，移动
    ui->widget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);    //允许可缩放，移动
    QList < QCPAxis *>  xis;
    xis.append(ui->widget->xAxis);
    ui->widget->axisRect()->setRangeZoomAxes(xis);

    // 轴随动
    connect(ui->widget->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->widget->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->widget->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->widget->yAxis2, SLOT(setRange(QCPRange)));
    // 实时更新槽函数
    _pTimerUpdate = new QTimer(this);
    connect(_pTimerUpdate, SIGNAL(timeout()), this, SLOT(customsplot_init()));

}

// 绘图
void Widget::customsplot_init()
{
    static QTime time(QTime::currentTime());        //获取当前时间
    //计算新的数据点
    double key = time.elapsed()/1000.0;
    static double lastPointKey = 0;

    if (key-lastPointKey > 0.002) //大于2ms添加一个数据
    {
        ui->widget->graph(0)->addData(key, temperature);      //温度加到数据中
        ui->widget->graph(1)->addData(key, temperature);
        lastPointKey = key;

        QDateTime time = QDateTime::currentDateTime();//获取系统现在的时间
        QString date = time.toString("MM.dd hh:mm"); //设置显示格式
        QString tempe;
        tempe = QString("%1").arg(temperature);
        insert_table(date,tempe);
    }

    ui->widget->xAxis->setRange(key, 10, Qt::AlignRight);        //设置x轴范围
    ui->widget->replot();       //画图

    // 缩放轴
     QList < QCPAxis *>  axis_x,axis_y,axis_xy;
     axis_x.append(ui->widget->xAxis);
     axis_y.append(ui->widget->yAxis);
     axis_xy.append(ui->widget->xAxis);
     axis_xy.append(ui->widget->yAxis);
     if(ui->checkBox_X->checkState())
     {
        ui->widget->axisRect()->setRangeZoomAxes(axis_x);
     }
     if(ui->checkBox_Y->checkState())
     {
        ui->widget->axisRect()->setRangeZoomAxes(axis_y);
     }
     if(ui->checkBox_XY->checkState())
     {
        ui->widget->axisRect()->setRangeZoomAxes(axis_xy);
     }


}

// 打开串口
void Widget::on_open_serialport_clicked()
{
    find_serialport();

    if(ui->com->count() != 0)   //不为空时
        //初始化串口
            serialport->setPortName(ui->com->currentText());        //设置串口名
            if(serialport->open(QIODevice::ReadWrite))              //打开串口成功
            {
               serialport->setBaudRate(9600);       //设置波特率
               serialport->setDataBits(QSerialPort::Data8);
               serialport->setParity(QSerialPort::NoParity);
               serialport->setStopBits(QSerialPort::OneStop);
               serialport->setStopBits(QSerialPort::TwoStop);
               serialport->setFlowControl(QSerialPort::NoFlowControl);     //设置流控制
               QObject::connect(serialport, &QSerialPort::readyRead, this, &Widget::read_serialport);    //读数据
                //控件可见设置
               ui->start->setEnabled(true);
               ui->close_serialport->setEnabled(true);
               ui->open_serialport->setEnabled(false);
               ui->label_state->setText(tr("串口打开成功"));
               ui->widget->replot();       //画图
                // 操作记录
               QString current_date = ui->textEdit->toPlainText();
               QDateTime current_date_time =QDateTime::currentDateTime();   //获取当前时间
               current_date +=current_date_time.toString("yyyy.MM.dd hh:mm");
               current_date += " 打开串口" ;
               current_date += "\n";
               ui->textEdit->setText(current_date);
            }
            else    //打开失败提示
            {
                QMessageBox::information(this,tr("Erro"),tr("Open the failure"),QMessageBox::Ok);
                ui->label_state->setText(tr("串口未打开"));
            }
}
//关闭串口
void Widget::on_close_serialport_clicked()
{
    //关闭串口
    serialport->clear();        //清空缓存区
    serialport->close();        //关闭串口
    on_pause_clicked();
    ui->start->setEnabled(false);

    // 操作记录
    QString current_date = ui->textEdit->toPlainText();
    QDateTime current_date_time =QDateTime::currentDateTime();   //获取当前时间
    current_date +=current_date_time.toString("yyyy.MM.dd hh:mm");
    current_date += " 关闭串口" ;
    current_date += "\n\n";
    ui->textEdit->setText(current_date);

    //控件设置
    ui->open_serialport->setEnabled(true);
    ui->close_serialport->setEnabled(false);
    ui->label_state->setText(tr("串口 关闭"));

    // lcd 显示 0
    temperature = 00.0;
    ui->lcdNumber->display(temperature);
}
// 读串口数据
void Widget::read_serialport()
{
    QByteArray buf;
    buf = serialport->readAll();
    if(!buf.isEmpty())          //显示数据
    {
        QString ss = tr(buf);
        temperature = ss.toDouble();
        ui->lcdNumber->display(temperature);    //lcd显示数据
    }

    buf.clear();    //清空缓存区
}
//查找串口
void Widget::find_serialport()
{
    //查找可用的串口
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(info);   //设置串口
        if(serial.open(QIODevice::ReadWrite))
        {
            ui->com->addItem(serial.portName());        //显示串口name
            serial.close();
        }
    }
}

// 插入数据
void Widget::insert_table(QString date, QString t)
{
    update();
    int row_count = ui->tableWidget->rowCount();   //获取总行数
    ui->tableWidget->insertRow(row_count);         //插入行
    QTableWidgetItem *item0 = new QTableWidgetItem();
    QTableWidgetItem *item1 = new QTableWidgetItem();
    item0->setText(date);
    item1->setText(t);
    ui->tableWidget->setItem(row_count,0,item0);
    ui->tableWidget->setItem(row_count,1,item1);

}

// 开始绘图
void Widget::on_start_clicked()
{
    _pTimerUpdate->start(1000); //1s更新一次
    ui->pause->setEnabled(true);
    ui->start->setEnabled(false);
}

// 暂停绘图
void Widget::on_pause_clicked()
{
    _pTimerUpdate->stop();
    ui->pause->setEnabled(false);
    ui->start->setEnabled(true);
}

void Widget::checkbox_change()
{
    // 缩放轴
     QList < QCPAxis *>  axis_x,axis_y,axis_xy;
     axis_x.append(ui->widget->xAxis);
     axis_y.append(ui->widget->yAxis);
     axis_xy.append(ui->widget->xAxis);
     axis_xy.append(ui->widget->yAxis);
     if(ui->checkBox_X->checkState())
     {
        ui->widget->axisRect()->setRangeZoomAxes(axis_x);
     }
     if(ui->checkBox_Y->checkState())
     {
        ui->widget->axisRect()->setRangeZoomAxes(axis_y);
     }
     if(ui->checkBox_XY->checkState())
     {
        ui->widget->axisRect()->setRangeZoomAxes(axis_xy);
     }

}


// 清空表
void Widget::on_clear_table_clicked()
{
    while(ui->tableWidget->rowCount())
    {
        ui->tableWidget->removeRow(0);
    }
}

//保存到文本文件
void Widget::on_save_table_clicked()
{
    // 获取文件目录
    QString filename = QFileDialog::getExistingDirectory(this,tr("file dialog"),"D:");
   //文件名
    QDateTime time = QDateTime::currentDateTime();//获取系统现在的时间
    QString date = time.toString("MM.dd"); //设置显示格式
    filename += date;
    filename += ".txt";
    //文件对象
    QFile file(filename);
    //只写方式打开
    if(!file.open(QFile::WriteOnly | QFile::Text))      //只写方式
    {
        QMessageBox::warning(this,tr("double file edit"),tr("no write ").arg(filename).arg(file.errorString()));
        return ;
    }
    //文件流对象
    QTextStream out(&file);

    //遍历数据
    int romcount = ui->tableWidget->rowCount();     //获取总行数
    for(int i = 0; i < romcount; i++)
    {
        QString rowstring;
        for(int j = 0; j < 2; j++)
        {
            rowstring += ui->tableWidget->item(i,j)->text();
            rowstring += "  ";
        }
        rowstring += "\n";
        out << rowstring;
    }

    file.close();
}
