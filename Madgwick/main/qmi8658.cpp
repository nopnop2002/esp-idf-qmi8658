/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */

#include <stdio.h>
#include <inttypes.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/message_buffer.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "cJSON.h"

// qmi8658 stuff
#include "Qmi8658.h"

#include "parameter.h"

extern QueueHandle_t xQueueTrans;
extern MessageBufferHandle_t xMessageBufferToClient;

static const char *TAG = "IMU";

// Source: https://github.com/arduino-libraries/MadgwickAHRS
#include "MadgwickAHRS.h"

#define RAD_TO_DEG (180.0/M_PI)
#define DEG_TO_RAD 0.0174533

// Arduino macro
#define micros() (unsigned long) (esp_timer_get_time())
#define delay(ms) esp_rom_delay_us(ms*1000)

// Create the Madgwick instances
Madgwick madgwick;

// Create the qmi8658 instances
#define QMI8658_I2C_FREQUENCY 20000 // Define I2C frequency as 20kHz (in Hz)
Qmi8658 imu(CONFIG_I2C_ADDR, QMI8658_I2C_FREQUENCY);

// Get scaled value
void getMotion6(double *_ax, double *_ay, double *_az, double *_gx, double *_gy, double *_gz) {
	qmi_data_t data; // Declare a variable to store sensor data
	while(1) {
		uint8_t status0 = imu.qmi_status0();
		ESP_LOGD(TAG, "status0=0x%x", status0);
		//if ((status0 & 0x03) == 0x03) break;
		if ((status0 & 0x01) == 0x01) break;
		vTaskDelay(1);
	}

	imu.read(&data);
	*_ax = data.acc_xyz.x;
	*_ay = data.acc_xyz.y;
	*_az = data.acc_xyz.z;
	*_gx = data.gyro_xyz.x;
	*_gy = data.gyro_xyz.y;
	*_gz = data.gyro_xyz.z;
}

// Get time in seconds since boot
// Compatible with ROS's time.toSec() function
double TimeToSec() {
	int64_t _time = esp_timer_get_time(); // Get time in microseconds since boot
	double __time = (double)_time / 1000000;
	return __time;
}

void qmi8658(void *pvParameters)
{
    // QMI8658 configuration
    qmi8658_cfg_t qmi8658_cfg;
    qmi8658_cfg.qmi8658_mode = qmi8658_mode_dual;
    // Set the accelerometer scale
    qmi8658_cfg.acc_scale = acc_scale_2g;
    // Set the accelerometer output data rate (ODR)
    qmi8658_cfg.acc_odr = acc_odr_1000;
    // Set the gyroscope scale
    qmi8658_cfg.gyro_scale = gyro_scale_32dps;
    // Set the gyroscope output data rate (ODR)
    qmi8658_cfg.gyro_odr = gyro_odr_29;

    // Initialize QMI8658 sensor with provided configuration
    qmi8658_result_t qmi8658_result;
    qmi8658_result = imu.open(&qmi8658_cfg);
    ESP_LOGI(TAG, "qmi8658_result=%d", qmi8658_result);
    ESP_LOGI(TAG, "qmi8658_result=%s", imu.resultToString(qmi8658_result));
    if (qmi8658_result != 0) return;

    // Calculate the offset
    ESP_LOGW(TAG, "IMU is currently being calibrated. Please do not move it.");
    imu.acc_offset();
    // Set Gyroscope offset
    imu.gyro_offset();
    ESP_LOGW(TAG, "IMU calibration is complete.");

	double ax, ay, az;
	double gx, gy, gz;
	double last_time_ = TimeToSec();
	int elasped = 0;

	bool initialized = false;
	float initial_roll = 0.0;
	float initial_pitch = 0.0;
	float initial_yaw = 0.0;

	// It takes time for the estimated value to stabilize.
	// It need about 4Sec.
	int initial_period = 400;

	while(1) {
		getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
		//printf("%f %f %f - %f %f %f\n", ax, ay, az, gx, gy, gz);
		
		// Get the elapsed time from the previous
		float dt = (TimeToSec() - last_time_);
		ESP_LOGD(TAG, "dt=%f",dt);
		last_time_ = TimeToSec();

		madgwick.updateIMU(gx, gy, gz, ax, ay, az, dt);
		float roll = madgwick.getRoll();
		float pitch = madgwick.getPitch();
		float yaw = madgwick.getYaw();
		ESP_LOGD(TAG, "roll=%f pitch=%f yaw=%f", roll, pitch, yaw);

		/* Print Data every 2 times */
		if (elasped > initial_period) {
			// Set the first data
			if (!initialized) {
				initial_roll = roll;
				initial_pitch = pitch;
				initial_yaw = yaw;
				initialized = true;
				initial_period = 2;
			}

#if 0
			printf("roll:%f", roll); printf(" ");
			printf("initial_roll:%f", initial_roll); printf(" ");
			printf("roll-initial_roll:%f", roll-initial_roll); printf(" ");
			printf("\n");

			printf("pitch: %f", pitch); printf(" ");
			printf("initial_pitch: %f", initial_pitch); printf(" ");
			printf("pitch-initial_pitch: %f", pitch-initial_pitch); printf(" ");
			printf("\n");
#endif

			// Send UDP packet
			float _roll = roll-initial_roll;
			float _pitch = pitch-initial_pitch;
			float _yaw = yaw-initial_yaw;
			ESP_LOGD(TAG, "roll:%f pitch=%f yaw=%f", roll, pitch, yaw);
			ESP_LOGI(TAG, "roll:%f pitch=%f yaw=%f", _roll, _pitch, _yaw);

			POSE_t pose;
			pose.roll = _roll;
			pose.pitch = _pitch;
			pose.yaw = 0.0;
			if (xQueueSend(xQueueTrans, &pose, 100) != pdPASS ) {
				ESP_LOGE(pcTaskGetName(NULL), "xQueueSend fail");
			}

			// Send WEB request
			cJSON *request;
			request = cJSON_CreateObject();
			cJSON_AddStringToObject(request, "id", "data-request");
			cJSON_AddNumberToObject(request, "roll", _roll);
			cJSON_AddNumberToObject(request, "pitch", _pitch);
			cJSON_AddNumberToObject(request, "yaw", 0.0);
			char *my_json_string = cJSON_Print(request);
			ESP_LOGD(TAG, "my_json_string\n%s",my_json_string);
			size_t xBytesSent = xMessageBufferSend(xMessageBufferToClient, my_json_string, strlen(my_json_string), 100);
			if (xBytesSent != strlen(my_json_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
			}
			cJSON_Delete(request);
			cJSON_free(my_json_string);

			vTaskDelay(1);
			elasped = 0;
		}
	
		elasped++;
		vTaskDelay(1);
	} // end while

	// Never reach here
	vTaskDelete( NULL );
}
