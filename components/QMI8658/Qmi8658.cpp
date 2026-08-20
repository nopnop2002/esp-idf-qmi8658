/*
 * Copyright (c) 2024 CHOUCHENE Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "Qmi8658.h"

#define I2C_NUM I2C_NUM_0
#define I2C_TICKS_TO_WAIT 100 // Maximum ticks to wait before issuing a timeout.

i2c_master_bus_handle_t   _bus_handle;
i2c_master_dev_handle_t   _dev_handle;

/* Structure for QMI context */
typedef struct {
	uint16_t acc_sensitivity;	// Sensitivity value for the accelerometer.
	uint8_t acc_scale;			// Scale setting for the accelerometer.
	uint16_t gyro_sensitivity;	// Sensitivity value for the gyroscope.
	uint8_t gyro_scale;			// Scale setting for the gyroscope.
} qmi_ctx_t;

qmi_ctx_t qmi_ctx;	// QMI context instance.

/* Accelerometer sensitivity table */
uint16_t acc_scale_sensitivity_table[4] = {
	ACC_SCALE_SENSITIVITY_2G,	// Sensitivity for ±2g range.
	ACC_SCALE_SENSITIVITY_4G,	// Sensitivity for ±4g range.
	ACC_SCALE_SENSITIVITY_8G,	// Sensitivity for ±8g range.
	ACC_SCALE_SENSITIVITY_16G	// Sensitivity for ±16g range.
};
								  
/* Gyroscope sensitivity table */
uint16_t gyro_scale_sensitivity_table[8] = {
	GYRO_SCALE_SENSITIVITY_16DPS,	  // Sensitivity for ±16 degrees per second range.
	GYRO_SCALE_SENSITIVITY_32DPS,	  // Sensitivity for ±32 degrees per second range.
	GYRO_SCALE_SENSITIVITY_64DPS,	  // Sensitivity for ±64 degrees per second range.
	GYRO_SCALE_SENSITIVITY_128DPS,	  // Sensitivity for ±128 degrees per second range.
	GYRO_SCALE_SENSITIVITY_256DPS,	  // Sensitivity for ±256 degrees per second range.
	GYRO_SCALE_SENSITIVITY_512DPS,	  // Sensitivity for ±512 degrees per second range.
	GYRO_SCALE_SENSITIVITY_1024DPS,   // Sensitivity for ±1024 degrees per second range.
	GYRO_SCALE_SENSITIVITY_2048DPS	  // Sensitivity for ±2048 degrees per second range.
};

/*##################################### Functions for i2c communication #################################*/

// Write a byte of data to a register in the QMI8658 device.

void Qmi8658::qmi8658_write(uint8_t reg, uint8_t value) {
	//printf("qmi8658_write reg=0x%x value=0x%x\n", reg, value);
	// Send register address and data to write
	uint8_t out_buf[2];
	out_buf[0] = reg;
	out_buf[1] = value;
	ESP_ERROR_CHECK(i2c_master_transmit(_dev_handle, out_buf, 2, I2C_TICKS_TO_WAIT));
}

// Read a byte of data from a register in the QMI8658 device.

uint8_t Qmi8658::qmi8658_read(uint8_t reg) {

	uint8_t out_buf[1];
	uint8_t in_buf[1];
	out_buf[0] = reg;
	ESP_ERROR_CHECK(i2c_master_transmit_receive(_dev_handle, out_buf, sizeof(out_buf), in_buf, 1, I2C_TICKS_TO_WAIT));
	return in_buf[0];
}

/* ####################### comman function for accelerometer, gyroscope and magnetometer #################### */

// Set the mode of operation for the QMI8658 device.

void Qmi8658::select_mode(qmi8658_mode_t qmi8658_mode ) {
  
	uint8_t qmi8658_ctrl7_reg = this->qmi8658_read(QMI8658_CTRL7);
	
	qmi8658_ctrl7_reg = (0xFC & qmi8658_ctrl7_reg) | qmi8658_mode;
	// Enable SyncSample mode
	//qmi8658_ctrl7_reg = qmi8658_ctrl7_reg | 0x40;
	// Gyroscope in Snooze Mode
	//qmi8658_ctrl7_reg = qmi8658_ctrl7_reg | 0x10;
	//printf("qmi8658_ctrl7_reg=0x%x\n",  qmi8658_ctrl7_reg);
	this->qmi8658_write(QMI8658_CTRL7,qmi8658_ctrl7_reg);	
}


/*####################################### Functions for accelerometr #########################################*/

// function to set output data rate of the accelerometer

void Qmi8658::acc_set_odr(acc_odr_t odr) {

	//printf("acc_set_odr odr=0x%x\n", odr);
	uint8_t qmi8658_ctrl2_reg = this->qmi8658_read(QMI8658_CTRL2);
	
	qmi8658_ctrl2_reg = (0xF0 & qmi8658_ctrl2_reg) | odr;
	//printf("qmi8658_ctrl2_reg=0x%x\n", qmi8658_ctrl2_reg);
	this->qmi8658_write(QMI8658_CTRL2,qmi8658_ctrl2_reg); 
}

// function to set the scale of the accelerometer

void Qmi8658::acc_set_scale(acc_scale_t acc_scale) {

	//printf("acc_scale=0x%x\n", acc_scale);
	uint8_t qmi8658_ctrl2_reg = this->qmi8658_read(QMI8658_CTRL2);
	qmi_ctx.acc_scale = acc_scale;
	qmi_ctx.acc_sensitivity = acc_scale_sensitivity_table[acc_scale];
	
	qmi8658_ctrl2_reg = (0x8F & qmi8658_ctrl2_reg) | (acc_scale << 4);
	//printf("qmi8658_ctrl2_reg=0x%x\n", qmi8658_ctrl2_reg);
	this->qmi8658_write(QMI8658_CTRL2,qmi8658_ctrl2_reg);
}

/*######################################## Functions for gyroscope ##########################################*/

// Set Gyroscope Output Data Rate (ODR)

void Qmi8658::gyro_set_odr(gyro_odr_t odr){
	//printf("gyro_set_odr odr=0x%x\n", odr);

	uint8_t qmi8658_ctrl3_reg = this->qmi8658_read(QMI8658_CTRL3);
	
	qmi8658_ctrl3_reg = (0xF0 & qmi8658_ctrl3_reg) | odr;
	//printf("qmi8658_ctrl3_reg=0x%x\n", qmi8658_ctrl3_reg);
	this->qmi8658_write(QMI8658_CTRL3,qmi8658_ctrl3_reg); 
}

// Set Gyroscope Full-scale

void Qmi8658::gyro_set_scale(gyro_scale_t gyro_scale) {

	//printf("gyro_scale=0x%x\n", gyro_scale);
	uint8_t qmi8658_ctrl3_reg = this->qmi8658_read(QMI8658_CTRL3);
	qmi_ctx.gyro_scale = gyro_scale;
	qmi_ctx.gyro_sensitivity = gyro_scale_sensitivity_table[gyro_scale];
	
	qmi8658_ctrl3_reg = (0x8F & qmi8658_ctrl3_reg) | (gyro_scale << 4);
	//printf("qmi8658_ctrl3_reg=0x%x\n", qmi8658_ctrl3_reg);
	this->qmi8658_write(QMI8658_CTRL3,qmi8658_ctrl3_reg);
}


/*######################################### General functions for qmi configuration ##########################*/

// Reset all regesters of the qmi8658 : 
// To be done before starting interfacing with the sensor to make sure it's on a "konwn" state

void Qmi8658::qmi_reset(){
  
	this->qmi8658_write(QMI8658_RESET,0xB0);
}

uint8_t Qmi8658::qmi_status0() {

	return this->qmi8658_read(QMI8658_STATUS0);
}

uint8_t Qmi8658::qmi_status1() {

	return this->qmi8658_read(QMI8658_STATUS1);
}

/*######################################### constructor function ############################################*/

//Initializes a new instance of the Qmi8658 class with the specified device address and frequency.

Qmi8658::Qmi8658(uint8_t deviceAdress, uint32_t deviceFrequency) {
	this->deviceFrequency = deviceFrequency;
	this->deviceAdress = deviceAdress;

	// clear context
	memset(&qmi_ctx, 0, sizeof(qmi_ctx_t));
	qmi_ctx.acc_scale = acc_scale_2g;
	qmi_ctx.acc_sensitivity = ACC_SCALE_SENSITIVITY_2G;
	qmi_ctx.gyro_scale = gyro_scale_16dps;
	qmi_ctx.gyro_sensitivity = GYRO_SCALE_SENSITIVITY_16DPS;

	// initiate IIC bus
	i2c_master_bus_config_t i2c_mst_config = {};
	i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
	i2c_mst_config.glitch_ignore_cnt = 7;
	i2c_mst_config.i2c_port = I2C_NUM;
	i2c_mst_config.scl_io_num = (gpio_num_t)CONFIG_GPIO_SCL;
	i2c_mst_config.sda_io_num = (gpio_num_t)CONFIG_GPIO_SDA;
	i2c_mst_config.flags.enable_internal_pullup = true;
	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &_bus_handle));

	i2c_device_config_t dev_cfg = {};
	dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	dev_cfg.device_address = deviceAdress;
	dev_cfg.scl_speed_hz = deviceFrequency;
	ESP_ERROR_CHECK(i2c_master_bus_add_device(_bus_handle, &dev_cfg, &_dev_handle));
}

/*######################################### class functions #################################################*/

// Open communication with the QMI8658 sensor and initializes it with the provided configuration settings.
// Return a status code indicating success or failure of the operation.

qmi8658_result_t Qmi8658::open(qmi8658_cfg_t* qmi8658_cfg) {

	qmi8658_result_t ret;
	uint8_t qmi8658_ctrl7;
	
	this->qmi_reset();
	this->select_mode(qmi8658_cfg->qmi8658_mode);

	// set accelerometr and gyroscope scale and ODR   
	this->acc_set_odr(qmi8658_cfg->acc_odr);
	this->acc_set_scale(qmi8658_cfg->acc_scale);
	this->gyro_set_odr(qmi8658_cfg->gyro_odr);
	this->gyro_set_scale(qmi8658_cfg->gyro_scale);

	// read device ID and revision ID
	this->deviceID = this->qmi8658_read(QMI8658_WHO_AM_I);
	//printf("deviceID=0x%x\n", this->deviceID);
	if (this->deviceID != 0x05) return qmi8658_result_open_error;
	this->deviceRevisionID = this->qmi8658_read(QMI8658_REVISION);
	//printf("deviceRevisionID=0x%x\n", this->deviceRevisionID);

	qmi8658_ctrl7 = qmi8658_read(QMI8658_CTRL7);
	ret =(qmi8658_ctrl7 & 0x03) == qmi8658_cfg->qmi8658_mode ? qmi8658_result_open_success : qmi8658_result_open_error;
	
	acc_offset_xyz = {0.0, 0.0, 0.0};
	gyro_offset_xyz = {0.0, 0.0, 0.0};
	return ret;
}

// Read raw data from the QMI8658 sensor and stores it in the provided data structure.

void Qmi8658::raw_read(qmi_data_t* data) {

	// read accelerometer data
	int16_t acc_x = (((int16_t)this->qmi8658_read(QMI8658_ACC_X_H) << 8) | this->qmi8658_read(QMI8658_ACC_X_L));
	int16_t acc_y = (((int16_t)this->qmi8658_read(QMI8658_ACC_Y_H) << 8) | this->qmi8658_read(QMI8658_ACC_Y_L));
	int16_t acc_z = (((int16_t)this->qmi8658_read(QMI8658_ACC_Z_H) << 8) | this->qmi8658_read(QMI8658_ACC_Z_L));
	//printf("acc_x=%d acc_y=%d acc_z=%d\n", acc_x, acc_y, acc_z);
	data->acc_xyz.x = (float)acc_x;
	data->acc_xyz.y = (float)acc_y;
	data->acc_xyz.z = (float)acc_z;

	// read gyroscope data
	int16_t rot_x = (((int16_t)this->qmi8658_read(QMI8658_GYR_X_H) << 8) | this->qmi8658_read(QMI8658_GYR_X_L));
	int16_t rot_y = (((int16_t)this->qmi8658_read(QMI8658_GYR_Y_H) << 8) | this->qmi8658_read(QMI8658_GYR_Y_L));
	int16_t rot_z = (((int16_t)this->qmi8658_read(QMI8658_GYR_Z_H) << 8) | this->qmi8658_read(QMI8658_GYR_Z_L));
	//printf("rot_x=%d rot_y=%d rot_z=%d\n", rot_x, rot_y, rot_z);
	data->gyro_xyz.x = (float)rot_x;
	data->gyro_xyz.y = (float)rot_y;
	data->gyro_xyz.z = (float)rot_z;

	// read temperature data
	int16_t temp = (((int16_t)this->qmi8658_read(QMI8658_TEMP_H) << 8) | this->qmi8658_read(QMI8658_TEMP_L));
	data->temperature = (float)temp;
}

// Set Accelerometer offset.

void Qmi8658::acc_offset(void) {
	acc_axes_t sum = {0, 0, 0};
	qmi_data_t data;
	int counter1 = 0;
	int counter2 = 0;
	while (1) {
		while(1) {
			uint8_t status0 = this->qmi_status0();
			//printf("status0=0x%x\n", status0);
			// Accelerometer new data available
			if ((status0 & 0x01) == 0x01) break;
			vTaskDelay(1);
		}

		this->raw_read(&data);
		//printf("acc x=%f y=%f z=%f\n", data.acc_xyz.x, data.acc_xyz.y, data.acc_xyz.z);
		counter1++;
		if (counter1 > 10) {
			counter2++;
			sum.x = sum.x + data.acc_xyz.x;
			sum.y = sum.y + data.acc_xyz.y;
			sum.z = sum.z + data.acc_xyz.z;
			if (counter2 == 100) break;
		}
		vTaskDelay(1);
	}
	acc_offset_xyz.x = sum.x / 100.0;
	acc_offset_xyz.y = sum.y / 100.0;
	acc_offset_xyz.z = sum.z / 100.0;
	acc_offset_xyz.z = acc_offset_xyz.z - 16384.0;
	printf("acc offset.x=%f y=%f z=%f\n", acc_offset_xyz.x, acc_offset_xyz.y, acc_offset_xyz.z);
}

// Set Gyroscope offset

void Qmi8658::gyro_offset(void) {
	gyro_axes_t sum = {0, 0, 0};
	qmi_data_t data;
	int counter1 = 0;
	int counter2 = 0;
	while (1) {
		while(1) {
			uint8_t status0 = this->qmi_status0();
			//printf("status0=0x%x\n", status0);
			// Accelerometer new data available
			if ((status0 & 0x01) == 0x01) break;
			// Gyroscope new data available 
			//if ((status0 & 0x02) == 0x02) break;
			vTaskDelay(1);
		}

		this->raw_read(&data);
		//printf("gyro x=%f y=%f z=%f\n", data.gyro_xyz.x, data.gyro_xyz.y, data.gyro_xyz.z);
		counter1++;
		if (counter1 > 10) {
			counter2++;
			sum.x = sum.x + data.gyro_xyz.x;
			sum.y = sum.y + data.gyro_xyz.y;
			sum.z = sum.z + data.gyro_xyz.z;
			if (counter2 == 100) break;
		}
		vTaskDelay(1);
	}
	gyro_offset_xyz.x = sum.x / 100.0;
	gyro_offset_xyz.y = sum.y / 100.0;
	gyro_offset_xyz.z = sum.z / 100.0;
	printf("gyro offset.x=%f y=%f z=%f\n", gyro_offset_xyz.x, gyro_offset_xyz.y, gyro_offset_xyz.z);
}

// Read data from the QMI8658 sensor and stores it in the provided data structure.

void Qmi8658::read(qmi_data_t* data) {

	// read accelerometer data
	int16_t acc_x = (((int16_t)this->qmi8658_read(QMI8658_ACC_X_H) << 8) | this->qmi8658_read(QMI8658_ACC_X_L));
	int16_t acc_y = (((int16_t)this->qmi8658_read(QMI8658_ACC_Y_H) << 8) | this->qmi8658_read(QMI8658_ACC_Y_L));
	int16_t acc_z = (((int16_t)this->qmi8658_read(QMI8658_ACC_Z_H) << 8) | this->qmi8658_read(QMI8658_ACC_Z_L));
	float _acc_x = (float)acc_x - acc_offset_xyz.x;
	float _acc_y = (float)acc_y - acc_offset_xyz.y;
	float _acc_z = (float)acc_z - acc_offset_xyz.z;
	//printf("_acc_x=%f _acc_y=%f _acc_z=%f\n", _acc_x, _acc_y, _acc_z);
	data->acc_xyz.x = _acc_x/qmi_ctx.acc_sensitivity;
	data->acc_xyz.y = _acc_y/qmi_ctx.acc_sensitivity;
	data->acc_xyz.z = _acc_z/qmi_ctx.acc_sensitivity;

	// read gyroscope data
	int16_t rot_x = (((int16_t)this->qmi8658_read(QMI8658_GYR_X_H) << 8) | this->qmi8658_read(QMI8658_GYR_X_L));
	int16_t rot_y = (((int16_t)this->qmi8658_read(QMI8658_GYR_Y_H) << 8) | this->qmi8658_read(QMI8658_GYR_Y_L));
	int16_t rot_z = (((int16_t)this->qmi8658_read(QMI8658_GYR_Z_H) << 8) | this->qmi8658_read(QMI8658_GYR_Z_L));
	float _rot_x = (float)rot_x - gyro_offset_xyz.x;
	float _rot_y = (float)rot_y - gyro_offset_xyz.y;
	float _rot_z = (float)rot_z - gyro_offset_xyz.z;
	//printf("_rot_x=%f _rot_y=%f _rot_z=%f\n", _rot_x, _rot_y, _rot_z);
	data->gyro_xyz.x = _rot_x/qmi_ctx.gyro_sensitivity;
	data->gyro_xyz.y = _rot_y/qmi_ctx.gyro_sensitivity;
	data->gyro_xyz.z = _rot_z/qmi_ctx.gyro_sensitivity;

	// read temperature data
	int16_t temp = (((int16_t)this->qmi8658_read(QMI8658_TEMP_H) << 8) | this->qmi8658_read(QMI8658_TEMP_L));
	data->temperature = (float)temp/TEMPERATURE_SENSOR_RESOLUTION;
}

// Close communication with the QMI8658 sensor.
// Return a status code indicating success or failure of the operation.

qmi8658_result_t Qmi8658::close() {
	uint8_t qmi8658_ctrl7_reg, qmi8658_ctrl1_reg;
	qmi8658_result_t ret;
  
	qmi8658_ctrl7_reg = this->qmi8658_read(QMI8658_CTRL7);
  
	// disable accelerometer, gyroscope, magnetometer and attitude engine
	qmi8658_ctrl7_reg &= 0xF0; 
	this->qmi8658_write(QMI8658_CTRL7,qmi8658_ctrl7_reg);
  
	// disable sensor by turning off the internal 2 MHz oscillator 
	qmi8658_ctrl1_reg = this->qmi8658_read(QMI8658_CTRL1);
	qmi8658_ctrl1_reg |= (1 << 0); 
	this->qmi8658_write(QMI8658_CTRL1,qmi8658_ctrl1_reg);

	// read these two registers
	qmi8658_ctrl7_reg = this->qmi8658_read(QMI8658_CTRL7);
	qmi8658_ctrl1_reg = this->qmi8658_read(QMI8658_CTRL1);

	ret = (!(qmi8658_ctrl7_reg & 0x0F) && (qmi8658_ctrl1_reg & 0x01)) ? qmi8658_result_close_success : qmi8658_result_close_error;

   return ret;
}

// Convert a qmi8658_result_t enum value into a corresponding string.
const char* Qmi8658::resultToString(qmi8658_result_t result) {
   switch(result){
	case qmi8658_result_open_success :
	  return "open-success";
	case qmi8658_result_open_error :
	  return "open-error";
	case qmi8658_result_close_success :
	  return "close-success";
	case qmi8658_result_close_error:
	  return "close-error";
  }
  return "unknown-error"; // make the compiler happy!
}
