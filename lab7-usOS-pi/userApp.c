#include "uart.h"
#include "ucos/includes.h"
extern void PUT32( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
#define GPIO_BASE 0x20200000
#define GPIO_SET 0x2020001c
#define GPIO_CLR 0x20200028
#define GPIO_GET 0x20200034
#define DHT_PIN 8
#define HIGH 0x00000100 //1<<8 (DHT_PIN)
#define MAX_TIME 85
#define DELAY_CNT 255 //for dht sensor data requesting ctrl
typedef char byte;
typedef char uint8_t;
typedef int uint16_t;
int disp_num = 8888;
byte digit[10]={0x03,0x9f,0x25,0x0d,0x99,0x49,0x41,0x1f,0x01,0x19};
byte seg[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

/*[0-3] as digit-choose, [4-11] as segments in one digit;
	use BCM port num, not wiringPi port num 
	or physical GPIO number!!!*/
int pin[12] = {2,3,10,9,17,18,27,22,23,24,25,4};

/*insert NOP machine instr to ctrl
	DHT11 data request intervals to around 40us
	yet this is just a util func
	where round of cycle is adjustable.
*/
void delayUS(int us){
    int i;
    while (us--) {
        for(i = 0; i < 10; i++){
			__asm("nop");        
		}
    }
}

/*set gpio status:
	each gpio port is controlled by 3 bits,
	setting "000" means it is output,
	and this way not affects other inrelevant ports.
*/
void set_gpio_out(unsigned int p, unsigned int *before){
	p %= 10; //port number
	//*(gpio+fsel)
	*before = (1<<(p*3)) | ( ~(7<<(p*3)) & (*before) );
}

void set_gpio_in(unsigned int p, unsigned int *before){
	p %= 10;
	*before = ~(7<<(p*3)) & (*before);
}

void led_init(){
	int i;
	unsigned int before = GET32(GPIO_BASE);

	//set gpio's status
	//each reg is capable to be in charge of 10 gpios
	set_gpio_out(2, &before);
	set_gpio_out(3, &before);
	set_gpio_out(4, &before);
	set_gpio_out(9, &before);
	set_gpio_out(10, &before);
	PUT32(GPIO_BASE, before);
	before = GET32(GPIO_BASE+4);

	set_gpio_out(17, &before);
	set_gpio_out(18, &before);
	set_gpio_in(DHT_PIN, &before);
	PUT32(GPIO_BASE+4, before);
	before = GET32(GPIO_BASE+8);

	set_gpio_out(22, &before);
	set_gpio_out(23, &before);
	set_gpio_out(24, &before);
	set_gpio_out(25, &before);
	set_gpio_out(27, &before);
	PUT32(GPIO_BASE+8, before);

	/*LED digits are connected on positive pole,
		thus CPIO_CLR -> selected,
		all kindled as the initial disp_num 8888.
	*/
	for (i = 0; i < 12; i++) {
		if (i < 4) {
			PUT32(GPIO_CLR, 1<<pin[i]);			
		}
		else {
			PUT32(GPIO_SET, 1<<pin[i]);
		}
	}
}

void led_disp(int num, int row) {
	int i;
	num %= 10;
	PUT32(GPIO_SET, 1<<pin[row]); //light the digit
	for (i = 0; i < 8; i++){
		if( ~digit[num] & seg[i] ){
			//when digit & segment both selected:
			PUT32(GPIO_CLR, 1<<pin[i+4]);
			OSTimeDlyHMSM(0,0,0,1);
			PUT32(GPIO_SET, 1<<pin[i+4]);
			//lighted and darkened
		}
	}
	PUT32(GPIO_CLR, 1<<pin[row]);
}

void led_disp_all() {
	led_disp(disp_num/1000, 3);
	led_disp(disp_num/100, 2);
	led_disp(disp_num/10, 1);
	led_disp(disp_num, 0);
}

void userApp2(void * args) {  //scan segment low
	led_init();
	while(1) {	
		//OSTimeDly(100);
		//PUT32(GPIO_CLR, 1<<17);
		led_disp_all();
	}
}

void request(){
  	unsigned int lststate = HIGH, cnt = 0;  
  	unsigned int before;
  	//first 3/8 is abandoned cuz out of normal range
	uint8_t dht11_val[5];
  	uint8_t i, j = 0;

  	//init dht_val buffer
  	for (i = 0; i < 5; i++) {
  		dht11_val[i]=0;  
  	}  
     		
	before = GET32(GPIO_BASE+4);
	set_gpio_out(DHT_PIN, &before);
	PUT32(GPIO_BASE+4, before);
	PUT32(GPIO_SET, 1<<DHT_PIN);
	OSTimeDlyHMSM(0,0,0,25);
	PUT32(GPIO_CLR, 1<<DHT_PIN);
	OSTimeDlyHMSM(0,0,0,20); //20ms
	PUT32(GPIO_SET, 1<<DHT_PIN);
	set_gpio_in(DHT_PIN, &before);
	delayUS(40); 
	PUT32(GPIO_BASE+4, before);  

  	for (i = 0; i < MAX_TIME; i++) {
    	cnt = 0;
    	while ( (GET32(GPIO_GET) & (1<<DHT_PIN)) == lststate ) {
      		cnt++;
      		delayUS(1);
      		if (cnt==DELAY_CNT) break;  
    	}
    	//when get all zeros
    	lststate = GET32(GPIO_GET) & (1<<DHT_PIN);  
    	if (cnt==DELAY_CNT) break;  
	    
    	//if got useful sensor data bits before exceeds delay rounds
    	if ( (i>=4) && (i%2==0) ) {  
      		dht11_val[j/8] <<= 1; //*2 ,use shift for precise time ctrl
      		if (cnt>40) dht11_val[j/8] |= 1;  
      		j++;  
    	}  
  	}
	//if(dht11_val[4] == ((dht11_val[3]+dht11_val[2]+dht11_val[1]+dht11_val[0])&0xff))
  	disp_num = dht11_val[2]*100 + dht11_val[0]; 	
}

void userApp1(void * args) { // DHT high
	OSTimeDlyHMSM(0,0,5,0);  //give userApp2 time to perform init
	while(1) {
		request();
		OSTimeDlyHMSM(0,0,1,0);   //1s request for a data
		disp_num++;
	}
}