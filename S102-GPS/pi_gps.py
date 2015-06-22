import serial
import traceback
import thread
import time
import os
import commands
import RPi.GPIO as GPIO
import string

led_digit = [0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f]
gpio_ucf = [17,18,27,22,23,24,25,4,2,3,10,9] #2,3,10,9 are select, bcm mode port num

arr = []
buf = ''
tim = ''

def led(n,ind):
	sig = led_digit[n]
	GPIO.output(2, False)
	GPIO.output(3, False)
    GPIO.output(10, False)
	GPIO.output(9, False)
	GPIO.output(gpio_ucf[ind], True)

	for i in range(0,8):
		if (sig>>i)&1:
			GPIO.output(gpio_ucf[i], False) #ON
		else:
			GPIO.output(gpio_ucf[i], True)  #OFF

def show(digit):
	for i in range(0, 4):
		led(int(digit[i]), i+8)
		time.sleep(0.004)

def show_time(i,interval):
#	global num
	while True:
		if num!='0000':
			print num
		try:
			show(num)
		except Exception as ep:
			print "show_time err: "+str(ep)

if __name__ == "__main__":
	global num
	#initial GPIO
	GPIO.setmode(GPIO.BCM)
	for i in gpio_ucf:
		GPIO.setup(i, GPIO.OUT)
		GPIO.output(i, False)  #COM1
		num='0000'

	ser = serial.Serial('/dev/ttyAMA0')
	ser.baudrate = 9600
	ser.timeout = 3 #preventing block
#	print ser.isOpen()
	thread.start_new_thread(show_time,(1,1)) 
	while True :
		try:
			tmp=ser.readline()
			if tmp.find('GPRMC')==1:
				print tmp
				buf = tmp.split(',')
				tim = buf[1]
				#print tim
				if len(tim)==6:
					num = tim[2:6]
				#show(num)
		except Exception as e:
		#	print e
			# ser.close()
			ser = serial.Serial('/dev/ttyAMA0')
		#time.sleep(5)