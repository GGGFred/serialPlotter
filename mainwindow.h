#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QSerialPortInfo>
#include <QSerialPort>

#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void fillParameters(void);
    void fillPorts(void);

    double elapsedTime;

public slots:
    void dataSlot(double *value, qint8 plots);
    void sim(void);

    void connectTTY(void);
    void disconnectTTY(void);

    void readData();

private slots:
    void on_pbConnect_clicked();
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort sPort;
    QTimer timer;
    qreal t;
    QByteArray *dataReceived;
    bool configurado;
    quint8 num_signals;
    quint8 num_it;
};

#endif // MAINWINDOW_H