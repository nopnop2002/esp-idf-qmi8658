# esp-idf-qmi8658
A demo showing the pose of the qmi8658 6DoF IMU sensor in 3D using esp-idf.

You can use the Kalman filter or the Madgwick filter to estimate the Euler angle.   
Euler angles are roll, pitch and yaw.   
It's very intuitive and easy to understand.   
However, since qmi8658 is a 6DoF IMU, YAW estimation is not possible.   
![a-Pitch-yaw-and-roll-angles-of-an-aircraft-with-body-orientation-O-u-v-original](https://user-images.githubusercontent.com/6020549/226072914-a7f923fc-eb6e-4d19-b2ff-8c9f2749ee6f.jpg)   
You can view like this.   
![Image](https://github.com/user-attachments/assets/cb9f1408-b372-4e55-84f6-1191f068cd1a)

# Software requirements
ESP-IDF V5.2 or later.   
Because this project uses the new I2C driver.   

# Hardware requirements
QMI8658x Accelerometer Gyroscope module 6 Dof inertial Measurement Sensors.   
The one on the left is the QMI8658C, and the one on the right is the QMI8658A.   
<img width="864" height="576" alt="Image" src="https://github.com/user-attachments/assets/46880cbf-4a87-48c7-97c9-008d4b7a778c" />   

# Wireing
|QMI8658x||ESP32|ESP32-S2/S3|ESP32-Cx||
|:-:|:-:|:-:|:-:|:-:|:-:|
|VCC|--|3.3V|3.3V|3.3V||
|GND|--|GND|GND|GND||
|SCL|--|GPIO22|GPIO12|GPIO5|(*1)|
|SDA|--|GPIO21|GPIO11|GPIO4|(*1)|
|ADO|--|GND/3.3V|GND/3.3V|GND/3.3V|(*2)|

(*1)You can change it to any pin using menuconfig.   

(*2)Choosing an i2c address.   
3.3V:i2c address is 0x6A.   
GND:i2c address is 0x6B.   

# Find the sensor
We can find the sensor using [i2c-tools](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/i2c/i2c_tools).   

- Selcect SCL and SDA using menuconfig.   
	<img width="659" height="486" alt="Image" src="https://github.com/user-attachments/assets/a33ecac0-066e-417a-9340-50b0799ef329" />

- Detect senser.   
	The i2c address for this sensor is 0x6A or 0x6B.   
	<img width="659" height="486" alt="Image" src="https://github.com/user-attachments/assets/4610006d-e636-4a25-8163-0492716f3f93" />

- Read register.   
	Register 0x00 for this sensor is 0x05.   
	<img width="659" height="486" alt="Image" src="https://github.com/user-attachments/assets/de336bcd-98c7-48e6-bbac-219b9209cd57" />

# Get Euler angles from qmi8658 using Kalman filter
I used [this](https://github.com/TKJElectronics/KalmanFilter).
```
git clone https://github.com/nopnop2002/esp-idf-qmi8658
cd esp-idf-qmi8658/Kalman
idf.py menuconfig
idf.py flash
```

### Configuration
<img width="659" height="486" alt="Image" src="https://github.com/user-attachments/assets/8fb8c3a4-b909-464c-a37d-735775cab8cb" />
<img width="659" height="486" alt="Image" src="https://github.com/user-attachments/assets/6c848db7-2607-4a27-ad09-58591f77c897" />

# Get Euler angles from qmi8658 using Madgwick filter
I used [this](https://github.com/arduino-libraries/MadgwickAHRS).
```
git clone https://github.com/nopnop2002/esp-idf-qmi8658
cd esp-idf-qmi8658/Madgwick
idf.py menuconfig
idf.py flash
```

### Configuration
<img width="659" height="486" alt="Image" src="https://github.com/user-attachments/assets/4969ea0c-fee2-4acf-b71d-2c75910b78bd" />
<img width="659" height="486" alt="Image" src="https://github.com/user-attachments/assets/9285deae-4ce4-4e60-a846-3c64017f8f67" />

# View Euler angles with built-in web server   
ESP32 acts as a web server.   
I used [this](https://github.com/Molorius/esp32-websocket) component.   
This component can communicate directly with the browser.   
It's a great job.   
Enter the following in the address bar of your web browser.   
```
http:://{IP of ESP32}/
or
http://esp32.local/
```

![bmi160-euler](https://user-images.githubusercontent.com/6020549/232381988-f4003a78-2145-493c-829e-9d0952117ea6.JPG)

WEB pages are stored in the html folder.   
I used [this](https://threejs.org/) for 3D display.   
I used [this](https://canvas-gauges.com/) for gauge display.   
Configuration Options for the gauge display is [here](https://canvas-gauges.com/documentation/user-guide/configuration).   
You can change the design and color according to your preference like this.   
![Image](https://github.com/user-attachments/assets/dfe82573-27a7-4395-bad6-cef1e3b4299c)   


# View Euler angles using PyTeapot   
You can view Euler angles using [this](https://github.com/thecountoftuscany/PyTeapot-Quaternion-Euler-cube-rotation) tool.   
It works as a UDP display server.   
This is a great application.   

```
+-------------+          +-------------+          +-------------+
|             |          |             |          |             |
|     IMU     |--(i2c)-->|    ESP32    |--(UDP)-->| pyteapot.py |
|             |          |             |          |             |
+-------------+          +-------------+          +-------------+
```

### Installation for Linux
```
$ sudo apt install python3-pip python3-setuptools
$ python3 -m pip install -U pip
$ python3 -m pip install pygame
$ python3 -m pip install PyOpenGL PyOpenGL_accelerate
$ git clone https://github.com/thecountoftuscany/PyTeapot-Quaternion-Euler-cube-rotation
$ cd PyTeapot-Quaternion-Euler-cube-rotation
$ python3 pyteapot.py
```
The posture of your sensor is displayed.   
![bmi180_2023-03-17_06-08-57](https://user-images.githubusercontent.com/6020549/226072858-b5e52dc5-db87-4613-8c05-4008a4bb8170.png)

### Installation for Windows   
Install Git for Windows from [here](https://gitforwindows.org/).   
Install Python Releases for Windows from [here](https://www.python.org/downloads/windows/).   
Open Git Bash and run:   
```
$ python --version
Python 3.11.9
$ python -m pip install -U pip
$ python -m pip install pygame
$ python -m pip install PyOpenGL PyOpenGL_accelerate
$ git clone https://github.com/thecountoftuscany/PyTeapot-Quaternion-Euler-cube-rotation
$ cd PyTeapot-Quaternion-Euler-cube-rotation
$ python pyteapot.py
```
![Image](https://github.com/user-attachments/assets/3aa9fd0d-2a0a-4a7c-ac40-4b84a70acaaf)


# View Euler angles using panda3d library   
You can view Euler angles using [this](https://www.panda3d.org/) library.   
It works as a UDP display server.   

```
+-------------+          +-------------+          +-------------+
|             |          |             |          |             |
|     IMU     |--(ic2)-->|    ESP32    |--(UDP)-->|  panda.py   |
|             |          |             |          |             |
+-------------+          +-------------+          +-------------+
```

### Installation for Linux
```
$ python3 --version
Python 3.11.2
$ sudo apt install python3-pip python3-setuptools
$ python3 -m pip install -U pip
$ python3 -m pip install panda3d
$ git clone https://github.com/nopnop2002/esp-idf-mpu6050-dmp
$ cd esp-idf-mpu6050-dmp/panda3d
$ python3 panda.py --help
usage: panda.py [-h] [--model {jet,biplain,707,fa18}]

options:
  -h, --help            show this help message and exit
  --model {jet,biplain,707,fa18}
```
![Image](https://github.com/user-attachments/assets/83804b5e-3ffe-4e18-966e-0ce180c1ab21)

### Installation for Windows
Install Git for Windows from [here](https://gitforwindows.org/).   
Install Python Releases for Windows from [here](https://www.python.org/downloads/windows/).   
Open Git Bash and run:   
```
$ python --version
Python 3.11.9
$ python -m pip install -U pip
$ python -m pip install panda3d
$ git clone https://github.com/nopnop2002/esp-idf-mpu6050-dmp
$ cd esp-idf-mpu6050-dmp/panda3d
$ python panda.py --help
usage: panda.py [-h] [--model {jet,biplain,707,fa18}]

options:
  -h, --help            show this help message and exit
  --model {jet,biplain,707,fa18}
```
![Image](https://github.com/user-attachments/assets/7f2fbdf4-97d9-40c3-87db-9f8386741220)

### How to use   
See [here](https://github.com/nopnop2002/esp-idf-mpu6050-dmp/blob/main/panda3d/README.md)   


