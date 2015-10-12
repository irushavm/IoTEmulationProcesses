all:
	gcc sensor.c -o sensor
	gcc actuator.c -o actuator
	gcc controller.c -o controller
	gcc cloud.c -o cloud

clean:
	rm -rf *.o sensor actuator controller cloud
