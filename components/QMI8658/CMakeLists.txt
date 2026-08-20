idf_component_register(
	SRCS "Qmi8658.cpp"
	PRIV_REQUIRES driver esp_driver_i2c esp_timer
	INCLUDE_DIRS "."
)
