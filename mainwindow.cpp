#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    dataReceived = new QByteArray();

    fillParameters();
    fillPorts();

    //  Inicialización del graficador
//    ui->graph->setBackground(QBrush(QColor(48,47,47)));

    ui->graph->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    ui->graph->xAxis->setDateTimeFormat("hh:mm:ss");
    ui->graph->xAxis->setAutoTickStep(false);
    ui->graph->xAxis->setTickStep(1);
    ui->graph->axisRect()->setupFullAxesBox();

    connect(ui->graph->xAxis,SIGNAL(rangeChanged(QCPRange)),ui->graph->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->graph->yAxis,SIGNAL(rangeChanged(QCPRange)),ui->graph->yAxis2, SLOT(setRange(QCPRange)));

    //  Timer para ejemplo, grafica funciones seno y coseno
    connect(&timer,SIGNAL(timeout()),this,SLOT(sim()));
    //timer.start(16);

    configurado = false;
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
    QString description;
    QString manufacturer;
    const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

    ui->cbPort->clear();

    foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {                       // List all available serial ports and populate ports combo box
        ui->cbPort->addItem(port.portName());
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
            // add data to lines:
            ui->graph->graph(i)->addData(key, value[i]);
            // remove data of lines that's outside visible range:
            ui->graph->graph(i)->removeDataBefore(key-16);
            // rescale value (vertical) axis to fit the current data:
            ui->graph->graph(i)->rescaleValueAxis();
        }

        lastPointKey = key;
    }
    // make key axis range scroll with the data (at a constant range size of 8):
    ui->graph->xAxis->setRange(key+0.25, 16, Qt::AlignRight);
    ui->graph->replot();

    // calculate frames per second:
    static double lastFpsKey;
    static int frameCount;
    ++frameCount;

    if (key-lastFpsKey > 1) // average fps over 1 seconds
    {
        ui->statusBar->showMessage(QString("%1 FPS, Total Data points: %2")
                                   .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
                                   .arg(ui->graph->graph(0)->data()->count())
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
    QString text;
    char buf[16];
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

            if (!configurado){
                num_signals += list.size();
                num_it++;
                if (num_it>=10){
                    num_signals /= num_it;

                    ui->graph->clearItems();

                    //  Creando los gráficos dependiendo del numero de señales recibidas
                    for (quint8 i = 0; i < num_signals; i++){
                        srand(list.at(i).toDouble() * 1000);
                        ui->graph->addGraph();
                        ui->graph->graph(i)->setBrush(Qt::NoBrush);
                        ui->graph->graph(i)->setAntialiasedFill(false);

                        int r,g,b;
                        r = rand() % 128;
                        g = rand() % 128;
                        b = rand() % 128;
                        ui->graph->graph(i)->setPen(QPen(QColor(r,g,b)));
                    }

                    //  Inicialización del ejemplo
                    t=0;
                    qreal data[10] = {0,0,0,0,0,0,0,0,0,0};
                    dataSlot(data,num_signals);

                    configurado = true;
                }
            } else {
                qreal data[10];
                for (quint8 i = 0; i < list.size(); i++){
                    data[i] = list.at(i).toDouble();
                }

                if (list.size() <= ui->graph->graphCount())
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
        ui->pbConnect->setText("Conectar");
    }

}

void MainWindow::on_pushButton_clicked()
{
    qDebug() << ui->graph->graph(0)->data()->size();

    QList<QCPData> data = ui->graph->graph(0)->data()->values();

    for (quint16 i=0; i < ui->graph->graph(0)->data()->size();i++)
        qDebug() << data.at(i).value;
}