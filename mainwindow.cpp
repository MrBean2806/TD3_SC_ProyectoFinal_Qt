#include <QSerialPort>
#include <QSerialPortInfo>
#include <string>
#include <QChar>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QCategoryAxis>
#include <QTimer>
#include "mainwindow.h"
#include "ui_mainwindow.h"

union dato{
    float dato_float;
    int dato_int;
    char dato_char[4];
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    puertoSerie = new QSerialPort(this);
    data_file = new QFile(data_file_name);
    csv_file = new QFile(csv_file_name);
    chart = new QChart();
    chartView = new QChartView(chart);
    ui->gridLayout_central->addWidget(chartView,0,0);

    statusBar()->showMessage("Agustín Bean - Manuel Arias - 2022");

    QObject::connect(puertoSerie, SIGNAL(readyRead()), this, SLOT(readSerial()));
    QObject::connect(ui->horizontalSlider_vel_ref, SIGNAL(valueChanged(int)), ui->lcdNumber_w_ref, SLOT(display(int)));
    QObject::connect(ui->horizontalSlider_vel_ref, SIGNAL(valueChanged(int)), this, SLOT(enviarReferencia()));
    QObject::connect(ui->pushButton_iniciar, SIGNAL(clicked(bool)), this, SLOT(iniciarMedicion()));
    QObject::connect(ui->pushButton_detener, SIGNAL(clicked(bool)), this, SLOT(detenerMedicion()));

    ui->lcdNumber_w_ref->display(ui->horizontalSlider_vel_ref->value());
    ui->radioButton_graficarVelMedida->setChecked(true);

}

void MainWindow::readSerial(){
    //Recibo los datos de velocidad y los guardo en el archivo
    QByteArray serialBuffer = puertoSerie->readAll();
    QTextStream stream(data_file);
    stream << serialBuffer;
    QByteArray vel_medida;
        if(serialBuffer.contains("-1")){
            qDebug() << "Señal de finalizacion" << '\n';
            puertoSerie->close();
            data_file->close();
            graficarDatos();
        }else{  //para mostrar la velocidad "en vivo" agarro el último valor de todo el bloque de datos que llegó y lo muestro
            int i = serialBuffer.lastIndexOf('\n', -1);
            if(i > 0){
                vel_medida = serialBuffer.sliced(i-4, 4);
                ui->lcdNumber_vel_medida->display(vel_medida.toInt());
            }
        }
}

void MainWindow::abrirArchivo(QFile *file){

    if (!file->open(QIODevice::ReadWrite | QIODevice::Text)){
        qCritical() << "No se pudo abrir el archivo :(";
        qCritical() << file->errorString();
        return;
    }
//    qInfo() << "Archivo abierto ...";
}


void MainWindow::iniciarMedicion(){
    dato Kp, Ki, tiempoMedicion, variableAGraficar;
    QString temp;
    dato velocidad_referencia;

    abrirPuertoSerie();
    data_file->remove();
    abrirArchivo(data_file);
    //leo los parámetros del panel de control y los mando al micro
    tiempoMedicion.dato_int = ui->radioButton_respuestaEscalon->isChecked() ? ui->spinBox_tiempoMedicion->value() : 15000;
    temp = ui->checkBox_Kp->isChecked() ? ui->lineEdit_Kp->text() : "0";
    Kp.dato_float = temp.toFloat();
    temp = ui->lineEdit_Ki->text();
    Ki.dato_float = temp.toFloat();
    variableAGraficar.dato_int = ui->radioButton_graficarVelMedida->isChecked();

    puertoSerie->write("asdw",4);   //por alguna razon hace falta mandar cualquier cosa antes para q lea bien la trama q sigue
    puertoSerie->write("init",4);
    puertoSerie->write(tiempoMedicion.dato_char,4);
    puertoSerie->write(Kp.dato_char, 4);
    puertoSerie->write(Ki.dato_char, 4);
    puertoSerie->write(variableAGraficar.dato_char, 4);

    if(ui->radioButton_respuestaEscalon->isChecked() ){
        //Envio un 0 y espero 250 ms antes de mandar vel_ref
        QTimer::singleShot(250, Qt::PreciseTimer, this, SLOT(enviarReferencia()));
        velocidad_referencia.dato_int = 0;
    }else{
        velocidad_referencia.dato_int = ui->horizontalSlider_vel_ref->value();
    }

    puertoSerie->write(velocidad_referencia.dato_char, 4);

}

void MainWindow::detenerMedicion(){
    //envio la señal para que el micro termine la medicion apagando el motor
    //espero la confirmacion del micro (vel_medida = -1) antes de cerrar todo de este lado
    dato velocidad_referencia;
    velocidad_referencia.dato_int = -1;
    puertoSerie->write(velocidad_referencia.dato_char, 4);
}

void MainWindow::enviarReferencia(){
    dato velocidad_referencia;
    velocidad_referencia.dato_int = ui->horizontalSlider_vel_ref->value();
    if(puertoSerie->isOpen())
        puertoSerie->write(velocidad_referencia.dato_char, 4);
}

void MainWindow::writeFile(QString data){

    QTextStream stream(data_file);
    stream << data;
}

void MainWindow::readFile(QFile *file){
//    if(!file.exists()){
//        qCritical() << "Archivo no encontrado";
//        return;
//    }
    abrirArchivo(file);
    QTextStream stream(file);
    while(!stream.atEnd()){
        QString line = stream.readLine();
        qDebug() << line;
    }

    file->close();
}

void MainWindow::abrirPuertoSerie(){

    if(puertoSerieAbierto == true){
        puertoSerie->close();
        puertoSerieAbierto = false;
    }
    /*
     *  Testing code, prints the description, vendor id, and product id of all ports.
     *  Used it to determine the values for the sasori.
     *
     *
    qDebug() << "Number of ports: " << QSerialPortInfo::availablePorts().length() << "\n";
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
        qDebug() << "Description: " << serialPortInfo.description() << "\n";
        qDebug() << "Has vendor id?: " << serialPortInfo.hasVendorIdentifier() << "\n";
        qDebug() << "Vendor ID: " << serialPortInfo.vendorIdentifier() << "\n";
        qDebug() << "Has product id?: " << serialPortInfo.hasProductIdentifier() << "\n";
        qDebug() << "Product ID: " << serialPortInfo.productIdentifier() << "\n";
    }
    */
    bool sasori_is_available = false;

    QString sasori_port_name;
    //  For each available serial port
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
        //  check if the serialport has both a product identifier and a vendor identifier
        if(serialPortInfo.hasProductIdentifier() && serialPortInfo.hasVendorIdentifier()){
            //  check if the product ID and the vendor ID match those of the sasori uno
            if((serialPortInfo.productIdentifier() == sasori_product_id)
                    && (serialPortInfo.vendorIdentifier() == sasori_vendor_id)){
                sasori_is_available = true; //    sasori uno is available on this port
               sasori_port_name = serialPortInfo.portName();
            }
        }
    }

    if(sasori_is_available){

        qDebug() << "Puerto encontrado...\n";
        puertoSerie->setPortName(sasori_port_name);
        puertoSerie->open(QSerialPort::ReadWrite);
        puertoSerie->setBaudRate(QSerialPort::Baud115200);
        puertoSerie->setDataBits(QSerialPort::Data8);
        puertoSerie->setFlowControl(QSerialPort::NoFlowControl);
        puertoSerie->setParity(QSerialPort::NoParity);
        puertoSerie->setStopBits(QSerialPort::OneStop);
        puertoSerieAbierto = true;

    }else{
        qDebug() << "No pude encontrarse el puerto\n";
        puertoSerieAbierto = false;
//        QMessageBox::information(this, "Serial Port Error", "Couldn't open serial port");
    }



}

void MainWindow::graficarDatos(){

    abrirArchivo(data_file);
    QTextStream stream(data_file);
    QLineSeries *serie_vel_ref = new QLineSeries();
    QLineSeries *serie_vel_medida = new QLineSeries();
    int n = 0;  //numero de muestra

    while(!stream.atEnd()){
        QString line = stream.readLine();
//        qDebug() << line;
        QStringList list = line.split(' ');
        if(list.size() > 1){
            QString vel_ref = list.at(0);
            QString vel_medida = list.at(1);
            serie_vel_ref->append(n, vel_ref.toInt());
            serie_vel_medida->append(n, vel_medida.toInt());
            n++;
        }
    }

    chart->setAnimationOptions(QChart::AllAnimations);
    chart->removeAllSeries();
    serie_vel_ref->setName("Velocidad de referencia");

    if(ui->radioButton_graficarVelMedida->isChecked())
        serie_vel_medida->setName("Velocidad medida");
    else
        serie_vel_medida->setName("Tensión de control");

    chart->addSeries(serie_vel_ref);
    chart->addSeries(serie_vel_medida);
    chart->createDefaultAxes();
    chart->legend()->setVisible(true);
    chartView->setRenderHint(QPainter::Antialiasing);

    exportarArchivo();
}

void MainWindow::exportarArchivo(){
    //convierte el archivo .txt a formato .csv para exportar los graficos a excel
    int n = 1;

    if( !data_file->isOpen() ){
        abrirArchivo(data_file);
    }
    data_file->seek(0);
    csv_file->remove();
    abrirArchivo(csv_file);

    QTextStream stream_out(data_file);
    QTextStream stream_in(csv_file);
//    qDebug() << data_file->pos();

    while(!stream_out.atEnd()){
        QString line = stream_out.readLine();
        line = line.replace(' ', ',');
        stream_in << n++ << ',' << line << '\n';
    }
    data_file->close();
    csv_file->close();

}


MainWindow::~MainWindow()
{
    delete ui;
}

