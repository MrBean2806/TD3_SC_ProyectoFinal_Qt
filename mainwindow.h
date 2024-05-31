#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QByteArray>
#include <QFile>
#include <QTimer>
#include <QtCharts>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void abrirPuertoSerie();
    void readSerial();
    void iniciarMedicion();
    void enviarReferencia();
    void detenerMedicion();
    void readFile(QFile *);
    void graficarDatos();
    void exportarArchivo();

private:
    Ui::MainWindow *ui;
    QSerialPort *puertoSerie;
    QFile *data_file;
    QFile *csv_file;
    QString data_file_name = "data.txt";
    QString csv_file_name = "data.csv";
    QChart *chart;
    QChartView *chartView;
    QTimer *timer;
    bool puertoSerieAbierto = false;
    static const quint16 sasori_vendor_id = 1027;
    static const quint16 sasori_product_id = 24577;
    void abrirArchivo(QFile *);
    void writeFile(QString);
};
#endif // MAINWINDOW_H
