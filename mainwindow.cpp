#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTextStream>
#include <QMessageBox>
#include <QRegExp>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    dataReceived = new QString();

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

    ui->leStart->setEnabled(false);
    ui->leEnd->setEnabled(false);

    ui->chbStart->setChecked(false);
    ui->chbEnd->setChecked(false);

    config = false;
    first = true;
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
        ui->statusBar->showMessage(QString("Plotting %1 signals @ %2 FPS, Total Data points: %3")
                                   .arg(this->num_signals)
                                   .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
                                   .arg(ui->plotter->graph(0)->data()->count())
                                   );
        lastFpsKey = key;
        frameCount = 0;
        period = (double)(1/(frameCount/(key-lastFpsKey)));
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

    if (sPort.open(QSerialPort::ReadWrite)){
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

void MainWindow::readData()
{
    static bool isStartCode = false;
    static bool isEndCode = false;
    int indexStart = -1;
    int indexEnd = -1;

    serialBuffer.append(sPort.readAll());

    if (ui->chbStart->isChecked()){
        if (!startCode.isEmpty()){
            indexStart = serialBuffer.lastIndexOf(startCode);
            if (indexStart >= 0){
                startCodeIndex = indexStart;
                isStartCode = true;
            }
        } else {
            startCodeIndex = 0;
            isStartCode = true;
        }
    } else {
        startCodeIndex = 0;
        isStartCode = true;
    }

    if (ui->chbEnd->isChecked()){
        if (!endCode.isEmpty()){
            indexEnd = serialBuffer.lastIndexOf(endCode);
        } else
            indexEnd = serialBuffer.lastIndexOf("\r\n");

        if (indexEnd >= 0){
            endCodeIndex = indexEnd;
            isEndCode = true;
        }

    } else {
        indexEnd = serialBuffer.lastIndexOf("\r\n");

        if (indexEnd >= 0){
            endCodeIndex = indexEnd;
            isEndCode = true;
        }
    }

    if (isStartCode && isEndCode){
        if (startCodeIndex < endCodeIndex){
            QByteArray subArray = serialBuffer.mid(startCodeIndex,endCodeIndex);
            dataReceived->append(QString::fromLatin1(subArray));
            serialBuffer.clear();
            isStartCode = false;
            isEndCode = false;
        }
    }

    if (!dataReceived->isEmpty()){
        if (first){
            first = false;
        }else{
            double key = QDateTime::currentDateTime().toMSecsSinceEpoch();
            static double pkey = key; double nkey = key - pkey;

            elapsedTime += nkey / 1000.0;
            pkey = key;

            QRegExp regExp(QString("[") +
                           QString::fromLatin1(startCode) +
                           QString::fromLatin1(endCode) +
                           QString("]"));

            QList<QString> list = dataReceived->remove(regExp).split(' ');

            qDebug() << list;

            if (!config){
                num_signals += list.size();
                num_it++;
                if (num_it>=5){
                    double signal = (double)num_signals / (double)num_it;
                    num_signals = round(signal);

                    //  Reset the plotter
                    ui->plotter->clearItems();
                    ui->plotter->clearGraphs();

                    ui->sbSignal->clear();

                    //  Create graphs by take count the number of received signals
                    for (quint8 i = 0; i < num_signals; i++){
                        srand(list.at(i).toDouble() * 1000);
                        ui->plotter->addGraph();
                        ui->plotter->graph(i)->setBrush(Qt::NoBrush);
                        ui->plotter->graph(i)->setAntialiasedFill(false);
                        ui->plotter->graph(i)->setPen(QPen(QColor(rand() % 128,rand() % 128,rand() % 128)));
                    }

                    ui->sbSignal->setMaximum(num_signals);
                    ui->sbSignal->setMinimum(0);

                    //  Data Plotter Initialization
                    t=0;
                    qreal data[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                    qreal maxval = INT_MIN;
                    qreal minval = INT_MAX;

                    for (quint8 i = 0; i < list.size(); i++){
                        qreal value = list.at(i).toDouble();
                        data[i] = value;

                        if (value != 0){
                            if (value > maxval)
                                maxval = value;

                            if (value < minval)
                                minval = value;

                        }
                    }

                    dataSlot(data,num_signals);

                    ui->dsbYmax->setValue(maxval+150);
                    ui->dsbYmin->setValue(minval-150);

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
        }

        dataReceived->clear();

    }
}

void MainWindow::on_pbConnect_clicked()
{

    if (!sPort.isOpen()){
        connectTTY();
        ui->pbConnect->setText("Disconnect");
    } else{
        disconnectTTY();
        num_signals = 0;
        num_it = 0;
        config = false;
        first = true;
        ui->pbConnect->setText("Connect");
    }

}

void MainWindow::on_pbDumpData_clicked()
{
    quint8 graphCount = ui->plotter->graphCount();
    double t = 0;

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

void MainWindow::on_chbStart_toggled(bool checked)
{
    ui->leStart->setEnabled(checked);
    if (checked)
        startCode = ui->leStart->text().toLatin1();
    else
        startCode.clear();
}

void MainWindow::on_chbEnd_toggled(bool checked)
{
    ui->leEnd->setEnabled(checked);
    if (checked)
        endCode = ui->leEnd->text().toLatin1();
    else
        endCode.clear();
}

void MainWindow::on_leStart_editingFinished()
{
    startCode = ui->leStart->text().toLatin1();
}

void MainWindow::on_leEnd_editingFinished()
{
    endCode = ui->leEnd->text().toLatin1();
}
