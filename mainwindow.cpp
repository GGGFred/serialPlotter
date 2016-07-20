#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextStream>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    dataReceived = new QByteArray();

    fillParameters();
    fillPorts();

    //  Plotter Initialization
    //    ui->ploteer->setBackground(QBrush(QColor(48,47,47)));

    ui->plotter->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->plotter->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->plotter->xAxis->setAutoTickStep(false);
    ui->plotter->xAxis->setTickStep(1);
    ui->plotter->axisRect()->setupFullAxesBox();

    connect(ui->plotter->xAxis,SIGNAL(rangeChanged(QCPRange)),ui->plotter->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->plotter->yAxis,SIGNAL(rangeChanged(QCPRange)),ui->plotter->yAxis2, SLOT(setRange(QCPRange)));

    //  Default X number samples
    ui->dsbTime->setValue(30);
    this->time = 30;

    //  Default Y range
    ui->plotter->yAxis->setRange(-100,100);
    ui->dsbYmin->setValue(-100);
    ui->dsbYmax->setValue(100);

    //  Timer for testing, plots sine and cosine functions
    connect(&timer,SIGNAL(timeout()),this,SLOT(sim()));
    //timer.start(16);

    config = false;
    num_signals = 0;
    num_it = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fillParameters()
{
    ui->cbBaud->addItem(QStringLiteral("1200"), QSerialPort::Baud1200);
    ui->cbBaud->addItem(QStringLiteral("2400"), QSerialPort::Baud2400);
    ui->cbBaud->addItem(QStringLiteral("4800"), QSerialPort::Baud4800);
    ui->cbBaud->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->cbBaud->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->cbBaud->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->cbBaud->addItem(QStringLiteral("57600"), QSerialPort::Baud57600);
    ui->cbBaud->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->cbBaud->addItem(tr("Custom"));
    ui->cbBaud->setCurrentIndex(5);

    ui->cbData->addItem(QStringLiteral("5"), QSerialPort::Data5);
    ui->cbData->addItem(QStringLiteral("6"), QSerialPort::Data6);
    ui->cbData->addItem(QStringLiteral("7"), QSerialPort::Data7);
    ui->cbData->addItem(QStringLiteral("8"), QSerialPort::Data8);
    ui->cbData->setCurrentIndex(3);

    ui->cbStop->addItem(QStringLiteral("1"), QSerialPort::OneStop);
    ui->cbStop->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    ui->cbParity->addItem(tr("None"), QSerialPort::NoParity);
    ui->cbParity->addItem(tr("Even"), QSerialPort::EvenParity);
    ui->cbParity->addItem(tr("Odd"), QSerialPort::OddParity);
    ui->cbParity->addItem(tr("Mark"), QSerialPort::MarkParity);
    ui->cbParity->addItem(tr("Space"), QSerialPort::SpaceParity);
}

void MainWindow::fillPorts()
{
    ui->cbPort->clear();

    // List all available serial ports and populate ports combo box
    foreach(QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        ui->cbPort->addItem(info.portName());
    }
}

void MainWindow::dataSlot(double *value, qint8 plots)
{
    // calculate two new data points:
    double key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    static double lastPointKey = 0;
    if (key-lastPointKey > 0.01) // at most add point every 10 ms
    {
        for (qint8 i = 0; i < plots; i++){
            ui->plotter->graph(i)->addData(key, value[i]);      // add data to lines:
            ui->plotter->graph(i)->removeDataBefore(key - this->time);// remove data of lines that's outside visible range:
        }

        lastPointKey = key;
    }
    // make key axis range scroll with the data:
    ui->plotter->xAxis->setRange(key - this->time, key);
    ui->plotter->replot();

    // calculate frames per second:
    static double lastFpsKey;
    static int frameCount;
    ++frameCount;

    if (key-lastFpsKey > 1) // average fps over 1 seconds
    {
        ui->statusBar->showMessage(QString("%1 FPS, Total Data points: %2")
                                   .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
                                   .arg(ui->plotter->graph(0)->data()->count())
                                   );
        lastFpsKey = key;
        frameCount = 0;
    }
}

void MainWindow::sim()
{
    double data[2];
    t += 0.016;
    data[0] = qSin(2*3.1416*t);
    data[1] = qCos(2*3.1416*t);
    dataSlot(data,2);
}

void MainWindow::connectTTY(){
    QString dev = ui->cbPort->currentText();
    qint32 baudrate=ui->cbBaud->currentText().toInt();
    QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(ui->cbData->itemData(ui->cbData->currentIndex()).toInt());
    QSerialPort::Parity parity = static_cast<QSerialPort::Parity>(ui->cbParity->itemData(ui->cbParity->currentIndex()).toInt());
    QSerialPort::StopBits stop = static_cast<QSerialPort::StopBits>(ui->cbStop->itemData(ui->cbStop->currentIndex()).toInt());

    sPort.setPortName(dev);

    if (sPort.open(QSerialPort::ReadOnly)){
        sPort.setBaudRate(baudrate);
        sPort.setDataBits(dataBits);
        sPort.setParity(parity);
        sPort.setStopBits(stop);
        sPort.flush();
    } else {
        QMessageBox::information(this, tr("Error"), tr("Could not open %1").arg(dev));
    }

    connect(&sPort,SIGNAL(readyRead()),this,SLOT(readData()));
}

void MainWindow::disconnectTTY(){
    dataReceived->clear();

    if (sPort.isOpen()){
        sPort.close();
    }

    disconnect(&sPort,SIGNAL(readyRead()),this,SLOT(readData()));
}

static char f1;
static char f2;

void MainWindow::readData()
{
    qint32 bytes = sPort.bytesAvailable();
    QByteArray buffer(sPort.readAll());
    static char count = 0;

    const char *c= buffer.data();

    for (int i=0;i < bytes;i++){
        dataReceived->append(*c);
        count++;

        if (*c == 0x0d){
            f1 = true;
        }
        if (f1){
            if (*c == 0x0a){
                f2 = true;
            }
        }

        if ( f1 && f2 ){
            double key = QDateTime::currentDateTime().toMSecsSinceEpoch();
            static double pkey = key; double nkey = key - pkey;

            elapsedTime += nkey / 1000.0;
            pkey = key;

            QList<QByteArray> list = dataReceived->remove(count-2,2).split(' ');

            if (!config){
                num_signals += list.size();
                num_it++;
                if (num_it>=10){
                    num_signals /= num_it;

                    ui->plotter->clearItems();

                    //  Create graphs by take count the number of received signals
                    for (quint8 i = 0; i < num_signals; i++){
                        srand(list.at(i).toDouble() * 1000);
                        ui->plotter->addGraph();
                        ui->plotter->graph(i)->setBrush(Qt::NoBrush);
                        ui->plotter->graph(i)->setAntialiasedFill(false);
                        ui->plotter->graph(i)->setPen(QPen(QColor(rand() % 128,rand() % 128,rand() % 128)));
                    }

                    //  Data Plotter Initialization
                    t=0;
                    qreal data[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                    dataSlot(data,num_signals);

                    config = true;
                }
            } else {
                qreal data[20];
                for (quint8 i = 0; i < list.size(); i++){
                    data[i] = list.at(i).toDouble();
                }

                if (list.size() <= ui->plotter->graphCount())
                    dataSlot(data,list.size());

            }

            dataReceived->clear();

            f1 = false;
            f2 = false;
            count = 0;
        }
        c++;
    }
}

void MainWindow::on_pbConnect_clicked()
{

    if (!sPort.isOpen()){
        connectTTY();
        ui->pbConnect->setText("Desconectar");
    } else{
        disconnectTTY();
        num_signals = 0;
        config = false;
        ui->pbConnect->setText("Conectar");
    }

}

void MainWindow::on_pbDumpData_clicked()
{
    quint8 graphCount = ui->plotter->graphCount();

    if (graphCount != 0){
        QString filename = QFileDialog::getSaveFileName(
                    this,
                    tr("Select the dump data file"),
                    QDir::homePath(),
                    "All files (*.*);;Text File (*.txt)"
                    );

        if (!filename.isEmpty()){
            QFile file(filename);

            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return;

            QTextStream out(&file);

            for (quint16 i=0; i < ui->plotter->graph(0)->data()->size();i++){
                for (quint16 j=0; j < graphCount; j++){
                    out << ui->plotter->graph(j)->data()->values().at(i).value << "\t";
                }
                out << "\r\n";
            }

        } else {
            QMessageBox::information(this,"Information","No file to dump");
        }
    } else {
        QMessageBox::information(this,"Information","No data to dump");
    }

}


void MainWindow::on_dsbYmax_valueChanged(double arg1)
{
    ui->plotter->yAxis->setRangeUpper(arg1);
}

void MainWindow::on_dsbYmin_valueChanged(double arg1)
{
    ui->plotter->yAxis->setRangeLower(arg1);
}

void MainWindow::on_dsbTime_valueChanged(double arg1)
{
    this->time = ui->dsbTime->value();
}
