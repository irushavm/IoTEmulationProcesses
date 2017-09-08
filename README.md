# Operating Systems IoT Emulation Processes
* Built using message queues and other IPCs
* Runs on a POSIX Complient system. This has only been tested in a Fedora Envrionment
### Components
* Controller: An emulated process that is used to collect information from sensors and provide feedback via the actuators
* Actuators: Output devices of the system that depend on 
* Cloud: A third party monitoring changes of the system

## Authors:
* Monty Dhanani
* Irusha Vidanamadura
*****************************************

Running Instructions

1. Have 6 terminal windows open
2. Run "make clean" command
3. Run "make all" command
4. On one terminal, run command "./controller"
5. On a second terminal, run command "./actuator AC"
6. On a third terminal, run command "./sensor TEMP 100"
7. On a fourth terminal, run command "./actuator FAN"
8. On a fifth terminal, run command "./sensor Humidity 70"
9. On a sixth terminal, run command "./cloud"

### NOTE: DO NOT run command with quotes

### NOTE: IF Controller Hangs, it is due to the  MAX_PC_PROCESS Being too high.
	IF  MAX_PC_PROCESS in msgProtocol.h is 100000, change it to 10000	
	ELSE IF  MAX_PC_PROCESS in msgProtocol.h is 10000, change it to 100000

### NOTE: To change the Number of allowed devices(actuators and sensors)to initialize with the controller, change MAX_ACT_SEN_AMOUNT in msgProtocol.h 
