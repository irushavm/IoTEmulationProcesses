all:
	gcc sensor.c -o sensor
	gcc actuator.c -o actuator
	gcc controller.c -o controller

clean:
	rm -rf *.o sensor actuator controller
