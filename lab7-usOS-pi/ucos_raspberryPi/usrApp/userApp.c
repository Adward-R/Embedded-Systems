#include "uart.h"
#include "ucos/includes.h"
extern void PUT32( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
#define GPIO_BASE 0x20200000
#define GPIO_SET GPIO_BASE+0x1c
#define GPIO_CLR GPIO_BASE+0x28
#define GPIO_GET GPIO_BASE+0x34
#define DHT_11_PIN 10
#define HIGH 0x00000400              //1<<10(DHT_11_PIN)
#define MAX_TIME 85
typedef char byte;
typedef char uint8_t;
int show_num = 8888;
byte digit[10]={0x03,0x9f,0x25,0x0d,0x99,0x49,0x41,0x1f,0x01,0x19};
byte seg[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

int pin[12]={18,3,4,22,27,2,24,8,7,17,23,25};    //segment



void delayUS(int us){
    int i;
    while(us--){
        for(i = 0; i < 10; ++i){
		__asm("nop");        
	}
    }
}

void set_output(unsigned int p, unsigned int *before){
	p %= 10;
	*before = ((0xffffffff-(111<<(p*3)))&(*before))|(1<<(p*3));
}

void set_input(unsigned int p, unsigned int *before){
	p %= 10;
	*before = (0xffffffff-(111<<(p*3)))&(*before);
}

void initial(){
	int i = 0;
	unsigned int before = GET32(GPIO_BASE);
	set_output(2, &before);
	set_output(3, &before);
	set_output(4, &before);
	set_output(7, &before);
	set_output(8, &before);
	PUT32(GPIO_BASE, before);
	before = GET32(GPIO_BASE+4);
	set_output(17, &before);
	set_output(18, &before);
	set_input(DHT_11_PIN, &before);
	PUT32(GPIO_BASE+4, before);
	before = GET32(GPIO_BASE+8);
	set_output(22, &before);
	set_output(23, &before);
	set_output(24, &before);
	set_output(25, &before);
	set_output(27, &before);
	PUT32(GPIO_BASE+8, before);
	for(; i<12; i++){
		if(i<4)
			PUT32(GPIO_CLR, 1<<pin[i]);			
		else
			PUT32(GPIO_SET, 1<<pin[i]);
	}
}

void scanner_one(int num, int row){
	int i = 0;
	num %= 10;
	PUT32(GPIO_SET, 1<<pin[row]);
	for(; i<8; i++){
		if((~digit[num])&seg[i]){
			PUT32(GPIO_CLR, 1<<pin[i+4]);
			OSTimeDlyHMSM(0,0,0,1);
			PUT32(GPIO_SET, 1<<pin[i+4]);			
		}
	}
	PUT32(GPIO_CLR, 1<<pin[row]);
}

void scanner(){
	scanner_one(show_num/1000, 3);
	scanner_one(show_num/100, 2);
	scanner_one(show_num/10, 1);
	scanner_one(show_num, 0);
}

void userApp2(void * args)   //scan segment low
{
	initial();
	while(1)
	{	
		//uart_string("in userApp2");
		//OSTimeDly(100);
		//PUT32(GPIO_CLR, 1<<17);
		scanner();
	}
}

void request(){
  	unsigned int lststate=HIGH;  
  	unsigned int counter=0;
	uint8_t dht11_val[5];  
  	uint8_t j=0,i;  
  	for(i=0;i<5;i++)  
     		dht11_val[i]=0;  
	unsigned int before = GET32(GPIO_BASE+4);
	set_output(DHT_11_PIN, &before);
	PUT32(GPIO_BASE+4, before);
	PUT32(GPIO_SET, 1<<DHT_11_PIN);
	OSTimeDlyHMSM(0,0,0,25);
	PUT32(GPIO_CLR, 1<<DHT_11_PIN);
	OSTimeDlyHMSM(0,0,0,20); //20ms
	PUT32(GPIO_SET, 1<<DHT_11_PIN);
	set_input(DHT_11_PIN, &before);
	delayUS(40); 
	PUT32(GPIO_BASE+4, before);  
  	for(i=0;i<MAX_TIME;i++){
    		counter=0;  
    		while((GET32(GPIO_GET)&(1<<DHT_11_PIN))==lststate){  
      			counter++;  
      			delayUS(1);  
      			if(counter==255)  
        		break;  
    		}  
    		lststate=GET32(GPIO_GET)&(1<<DHT_11_PIN);  
    		if(counter==255)  
       			break;  
    	// top 3 transistions are ignored  
    		if((i>=4)&&(i%2==0)){  
      			dht11_val[j/8]<<=1;  
      			if(counter>40)  
        			dht11_val[j/8]|=1;  
      			j++;  
    		}  
  	}
	//if(dht11_val[4] == ((dht11_val[3]+dht11_val[2]+dht11_val[1]+dht11_val[0])&0xff))
  	show_num = dht11_val[2]*100+dht11_val[0]; 	
}


void userApp1(void * args)  // DHT_11 high
{
	OSTimeDlyHMSM(0,0,5,0);  //let App2 do initial
	while(1)
	{
		//uart_string("in userApp1");
		request();
		
//		getdata();
		OSTimeDlyHMSM(0,0,1,0);   //1s request for a data
		//show_num++;
	}
}
