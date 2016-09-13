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
    void on_pbDumpData_clicked();

    void on_dsbYmax_valueChanged(double arg1);

    void on_dsbYmin_valueChanged(double arg1);

    void on_dsbTime_valueChanged(double arg1);

    void on_chbStart_toggled(bool checked);

    void on_chbEnd_toggled(bool checked);

    void on_leStart_editingFinished();

    void on_leEnd_editingFinished();

private:
    Ui::MainWindow *ui;
    QSerialPort sPort;
    QTimer timer;
    qreal t;
    QString *dataReceived;
    bool config;
    quint8 num_signals;
    quint8 num_it;
    double time;
    bool first;
    double period;

    QByteArray serialBuffer;
    int startCodeIndex;
    int endCodeIndex;
    QByteArray startCode;
    QByteArray endCode;
};

#endif // MAINWINDOW_H
