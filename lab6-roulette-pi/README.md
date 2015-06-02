According to [Example of wiringPi Library](https://github.com/WiringPi/WiringPi/blob/master/examples/Makefile), use the following command to compile the program after installing wiringPi to RPi:

`gcc -o game game.c -L /usr/local/lib -lwiringPi -lpthread -lm`

BTN0 to start, stop;

BTN1 to reset;

BTN0+BTN1 to start or stop blinking of LED.