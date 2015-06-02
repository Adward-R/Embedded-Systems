#include<wiringPi.h>
#include<stdio.h>
#include<stdlib.h>
const int BTN0=10,BTN1=11,W[4]={8,9,12,13};
const int digit[10][8]={0,0,0,0,0,0,1,1,
                        1,0,0,1,1,1,1,1,
                        0,0,1,0,0,1,0,1,
                        0,0,0,0,1,1,0,1,
                        1,0,0,1,1,0,0,1,
                        0,1,0,0,1,0,0,1,
                        0,1,0,0,0,0,0,1,
                        0,0,0,1,1,1,1,1,
                        0,0,0,0,0,0,0,1,
                        0,0,0,0,1,0,0,1
                        };
int lEnable=0,wEnable=1,rEnable=1,bEnable=1,number=0000;
int i,j,tnumber,out;
unsigned int t1=0,t0=0,t2=0,t3=0;
int main(){
        if (wiringPiSetup()==1)
                return 1;
        pinMode(BTN0,INPUT);
        pinMode(BTN1,INPUT);
        for (i=0;i<8;i++)
                pinMode(i,OUTPUT);
        for (i=0;i<4;i++)
                pinMode(W[i],OUTPUT);

        while (1){
                t1=millis();
                if (!digitalRead(BTN0)&&t1-t2>=500){
                        delay(30);
                        if (digitalRead(BTN1)){
                                bEnable=1-bEnable;
                                wEnable=1;
                                t2=t1;
                                printf("Blink State Change!\n");
                                continue;
                        }
                        if (rEnable==0)
                                printf("Start!\n");
                        else
                                printf("Stop!\n");
                        rEnable=1-rEnable;
                        wEnable=1;
                        t2=t1;
                }
                if (digitalRead(BTN1)&&t1-t2>=500){
                        delay(30);
                        if (!digitalRead(BTN0)){
                                bEnable=1-bEnable;
                                wEnable=1;
                                t2=t1;
                                printf("Blink State Change!\n");
                                continue;
                        }
                        printf("Reset!\n");
                        number=0;
                        t2=t1;
                }
                if (lEnable==0)
                        tnumber=number;
                else
                        tnumber/=10;
                out=tnumber%10;
                for (i=0;i<4;i++)
                        digitalWrite(W[i],i==(3-lEnable)?1&wEnable:0);
                for (i=0;i<8;i++)
                        digitalWrite(i,digit[out][i]);
                lEnable=(1+lEnable)%4;
                if (t1-t0>=100){
                        number=rEnable?(number+1)%10000:number;
                        t0=t1;
                }
                if (!rEnable&&bEnable&&t1-t3>1000){
                        wEnable=1-wEnable;
                        t3=t1;
                }
                delay(1);
        }
}
