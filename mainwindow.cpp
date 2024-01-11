#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "telegrambot.h"
#include "mat2qimage.h"
#include<QDebug>
#include <QDirIterator>
#include<QDebug>
#include<QFile>
#include <QTextStream>
#include<QtNetwork/QtNetwork>
#include<QTimer>
#include<opencv4/opencv2/cvconfig.h>
#include<opencv2/core/core.hpp>
#include<opencv2/ml/ml.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/video/background_segm.hpp>
#include<opencv2/videoio.hpp>
#include<opencv2/imgcodecs.hpp>
#include<opencv2/objdetect.hpp>


TelegramBot interfacesbot("5373669590:AAELkjzfprK1vA3vt2UFAy5jeGkz4UBJoxM");

using namespace cv;
VideoCapture camara(0);
Mat imagen;
bool activarLeds = false;
int pin = 0;
int estado = 0;
Mat imagenChica;

QString face_cascade_name = "/home/victor/DiseñodeInterfaces/Actividad6/haarcascade_frontalface_alt2.xml";
CascadeClassifier face_cascade;

QString MainWindow::conexionWebPOST(QString url, QString mensaje){
    QString respuesta = "error 1";

    //Paso # 1 - Verificar que la red, por donde se enviará el
    QNetworkInterface redConectada = QNetworkInterface::interfaceFromName("wlo1");
    QList<QNetworkAddressEntry> lista = redConectada.addressEntries();
    //Paso # 2 - Verificar que la red este activa
    if(!lista.empty()){
        QNetworkAddressEntry IP = lista.first();
        qDebug() << "Red activa: " << IP.ip() << endl;

        //Crear el mensaje HTML/HTTP
        QByteArray longitudMensaje = QByteArray::number(mensaje.size());
        QNetworkRequest solicitud;
        QNetworkAccessManager *clienteWeb = new QNetworkAccessManager();
        QUrl servidor(url.toUtf8().constData());

        //Paso # 3 - Verificar que la url sea valida
        if(servidor.isValid()){
            qDebug() << "Servidor valido " << endl;

            //Paso # 4 - Formar el mensaje HTTP
            solicitud.setUrl(servidor);
            solicitud.setRawHeader(QByteArray("User-Agent"),QByteArray("bot"));
            solicitud.setRawHeader(QByteArray("Connection"),QByteArray("close"));
            solicitud.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
            solicitud.setHeader(QNetworkRequest::ContentLengthHeader, longitudMensaje);

            //Paso # 5 - Realizar la conexion
            QNetworkReply *conexionServidor = clienteWeb->post(solicitud, mensaje.toLatin1());
            //QNetworkReply *conexionServidor = clienteWeb->get(QNetworkRequest(servidor));

            //Paso # 6 - Esperar a que servidor responda
            QEventLoop funcionLoop;
            QObject::connect(clienteWeb, SIGNAL(finished(QNetworkReply*)), &funcionLoop, SLOT(quit()));

            funcionLoop.exec();

            //Paso # 7 - Leer la respuesta del servidor
            char datosWeb[300000];
            respuesta.clear();
            int sv=conexionServidor->read(datosWeb,300000);
            for(int i=0;i<sv;i++) {
                respuesta+= datosWeb[i];
            }

        }
        else{
            respuesta = "error 3" ;
        }
    }
    else{
        respuesta = "Error 2";
    }

    return respuesta;
}



void MainWindow::Temporizador(){
    if(activarLeds){
        activarLeds = false;
        QString mensaje = "{";
        mensaje += "\"pin\":\""+QString::number(pin)+"\",";
        mensaje += "\"estado\":\""+QString::number(estado)+"\"}";
        qDebug() << mensaje;
        conexionWebPOST("http://192.168.0.16/", mensaje);
    }

       camara >> imagen;

       if(!imagen.empty()){

           cv::resize(imagen, imagenChica, Size(640,360),0,0,0);
           Mat GRIS;
           cv::cvtColor( imagenChica, GRIS, COLOR_BGR2GRAY );
           cv::equalizeHist(GRIS,GRIS);

           std::vector<Rect> carasEncontradas;
           face_cascade.detectMultiScale( GRIS, carasEncontradas, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(30, 30) );

           for ( size_t i = 0; i < carasEncontradas.size(); i++ ){
                      Point center( carasEncontradas[i].x + carasEncontradas[i].width/2, carasEncontradas[i].y + carasEncontradas[i].height/2 );
                      ellipse( imagenChica, center, Size( carasEncontradas[i].width/2, carasEncontradas[i].height/2 ), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );

            }

       }


       QImage img = Mat2QImage(imagenChica);
       QPixmap img2 = QPixmap::fromImage(img);
       ui->label->clear();
       ui->label->setPixmap(img2);

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if( !face_cascade.load( face_cascade_name.toUtf8().constData() ) ){
    qDebug() << "Error al cargar el detector de rostros";
    }

        QTimer *cronometro = new QTimer(this);
        connect(cronometro, SIGNAL(timeout()), this, SLOT(Temporizador()));
        cronometro->start(50);


        QObject::connect(&interfacesbot, &TelegramBot::newMessage, [&interfacesbot](TelegramBotUpdate update) {
            // only handle Messages
            if(update->type != TelegramBotMessageType::Message) return;

            // simplify message access
            TelegramBotMessage& message = *update->message;

            // send message (Format: Normal)
            TelegramBotMessage msgSent;

            QString mensajeRecibido = message.text;
            
            if(mensajeRecibido == "Foto"){
                interfacesbot.sendMessage(message.chat.id,
                                "La foto esta en proceso",
                                0,
                                TelegramBot::NoFlag,
                                TelegramKeyboardRequest(),
                                &msgSent);

                camara >> imagen;
                cv::imwrite("../imagen.jpg", imagen);

                interfacesbot.sendPhoto(message.chat.id,"../imagen.jpg","Tu Foto...");

            }
            else if(mensajeRecibido == "Encender"){
                interfacesbot.sendMessage(message.chat.id,
                                "Led Encendido",
                                0,
                                TelegramBot::NoFlag,
                                TelegramKeyboardRequest(),
                                &msgSent);
                activarLeds = true;
                pin = 4;
                estado = 1;

            }
            else if(mensajeRecibido == "Apagar"){
                interfacesbot.sendMessage(message.chat.id,
                                "Led Apagado",
                                0,
                                TelegramBot::NoFlag,
                                TelegramKeyboardRequest(),
                                &msgSent);
                activarLeds = true;
                pin = 4;
                estado = 0;
            }
            else{
                interfacesbot.sendMessage(message.chat.id,
                                "Opcion No Valida",
                                0,
                                TelegramBot::NoFlag,
                                TelegramKeyboardRequest(),
                                &msgSent);
            }
        });
        interfacesbot.startMessagePulling();

}

MainWindow::~MainWindow()
{
    delete ui;
}

