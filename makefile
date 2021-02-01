all:
	gcc led_matrix.c -o led -I/usr/local/include -L/usr/local/lib -lwiringPi 
release:
	gcc led_matrix.c -o led -I/usr/local/include -L/usr/local/lib -lwiringPi 
	strip led
