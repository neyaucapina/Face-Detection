//Reconocimiento de rostros
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <Windows.h>

using namespace std;
using namespace cv;

Mat src,src_gray;
CascadeClassifier eye_cascade= ("C:\\opencv\\sources\\data\\haarcascades\\haarcascade_mcs_eyepair_big.xml");
CascadeClassifier nose_cascade= ("C:\\opencv\\sources\\data\\haarcascades\\haarcascade_mcs_nose.xml");//nose

vector<Rect> eyes;
vector<Rect> noses;

int val_x2,val_x1,val_x,val_y2,val_y1,val_y;
Point centere;

int con1,con2;
HANDLE hSerial;
DWORD btsIO;

//Conveersor a strings
string intToString(int number){
    std::stringstream ss;
    ss << number;
    return ss.str();
}


//Funcion comunicacion serial
void iniserial(){
    // Setup serial port connection and needed variables.
    hSerial = hSerial = CreateFile(L"COM2",GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

    if (hSerial !=INVALID_HANDLE_VALUE){
        printf("Port opened! \n");
        DCB dcbSerialParams;
        GetCommState(hSerial,&dcbSerialParams);

        dcbSerialParams.BaudRate = CBR_9600;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.Parity = NOPARITY;
        dcbSerialParams.StopBits = ONESTOPBIT;

        SetCommState(hSerial, &dcbSerialParams);
    }
    else{
        if (GetLastError() == ERROR_FILE_NOT_FOUND){
            printf("Serial port doesn't exist! \n");
        }
        printf("Error while setting up serial port! \n");
    }
}


///Dibujar region de enfoque
void dibujarregion(Mat frame,int x,int y, int z, int k,Scalar color){
    line(frame,Point (x,y), Point (z,y),color,1);
    line(frame,Point (z,y), Point (z,k),color,1);
    line(frame,Point (z,k), Point (x,k),color,1);
    line(frame,Point (x,k), Point (x,y),color,1);
}


int main(){
    VideoCapture video(1);

    iniserial();
    //Region de enfoque
    int x=270,y=190; //Esquina superior izq
    int z=370,t=290; //Esquina inferior der
    Scalar color=Scalar(255,0,0);

    //Variales de los servos
    int servoOrientation = 0;
    char outputChars[] = "c";
    //Posicion inicial del servo= al centro
    WriteFile(hSerial, outputChars, strlen(outputChars), &btsIO, NULL);


    while(true){
        int aux1, aux2;
        aux1=0;
        aux2=0;
        video >> src;

        //Tratamiento de la imagen
        flip(src,src,1);
        cvtColor( src, src_gray, CV_BGR2GRAY );
        equalizeHist( src_gray, src_gray);

        //Para los ojos
        eye_cascade.detectMultiScale(src_gray, eyes, 1.1, 4, 0|CV_HAAR_SCALE_IMAGE, Size(25,25));
        //Para la nariz
        nose_cascade.detectMultiScale(src_gray, noses, 1.3, 4, 0|CV_HAAR_SCALE_IMAGE, Size(25,25));

        //cout<<"Caras: "<<faces.size()<<" Ojos: "<<eyes.size()<<" Nariz: "<<eyes.size()<<endl;

        //Dibujar region de enfoque
        dibujarregion(src,x,y,z,t,color);


        ///Deteccion de ojos
        for(size_t j=0; j<eyes.size(); j++){
            Point centere(eyes[j].x+eyes[j].width*0.5, eyes[j].y+eyes[j].height*0.5);
            //ellipse(src, centere, Size(eyes[j].width*0.5,eyes[j].height*0.5),0,0,360,Scalar(0,255,255),4,8,0);
            double cye=centere.y;
            double cxe=centere.x;

            ///Cada ojo con una nariz
            //if(eyes.size()==noses.size())
                for(size_t k=0; k<noses.size(); k++){
                    Point centern(noses[k].x+noses[k].width*0.5, noses[k].y+noses[k].height*0.5);
                    //ellipse(src, centern, Size(noses[k].width*0.5,noses[k].height*0.5),0,0,360,Scalar(0,255,0),4,8,0);
                    double cyn=centern.y;
                    double cxn=centern.x;

                    double dy=cye/cyn;
                    double dx=cxe/cxn;
                    //cout<<"ojos/nariz x: "<<dx<<endl;
                    //cout<<"ojos/nariz y: "<<dy<<endl;

                    if((centern.y>centere.y)&&
                       (dy>0.65&&dy<0.83)   &&(dx>0.93&&dx<1.15)){
                        ellipse(src, centere, Size(eyes[j].width*0.5,eyes[j].height*0.5),0,0,360,Scalar(0,255,0),3,8,0);
                        ellipse(src, centern, Size(noses[k].width*0.5,noses[k].height*0.5),0,0,360,Scalar(232,162,0),3,8,0);

                        ///Programacion para la posicion
                        circle(src,Point(cxn,cyn),6,Scalar(255,0,0),6);
                        putText(src,"P:"+intToString(cxn)+","+intToString(cyn),Point(cxn+5,cyn-5),FONT_HERSHEY_COMPLEX_SMALL,0.55, Scalar(0, 0, 255), 1, CV_AA);
                        //Algoritmos de control enviar al arduino
                        if(cxn>x&&cxn<z&&cyn>y&&cyn<t){
                             putText(src,"Rostro enfocado :D",Point(  0,30),FONT_HERSHEY_COMPLEX_SMALL,1,Scalar(0,255,0),2,CV_AA);
                        }
                        if (cxn< x){
                            cout<<"Entro al if a"<<endl;
                            outputChars[0] = 'a';
                            WriteFile(hSerial, outputChars, strlen(outputChars), &btsIO, NULL);
                        }
                        if (cxn> z){
                            outputChars[0] = 'b';
                            WriteFile(hSerial, outputChars, strlen(outputChars), &btsIO, NULL);
                        }
                        if (cyn<y){
                            outputChars[0] = 'x';
                            WriteFile(hSerial, outputChars, strlen(outputChars), &btsIO, NULL);
                        }
                        if (cyn>t){
                            outputChars[0] = 'y';
                            WriteFile(hSerial, outputChars, strlen(outputChars), &btsIO, NULL);
                        }

                    }
                }
        }

        namedWindow("Imagen", WINDOW_AUTOSIZE);
        imshow("Imagen",src);
        int c=waitKey(1);

        if(c==27) break;
    }
    destroyAllWindows();
    return 0;
}
