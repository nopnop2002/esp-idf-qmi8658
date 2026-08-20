set(srcs "main.cpp" "qmi8658.cpp" "wifi.c" "udp_trans.c" "web_server.c" "web_client.c")

idf_component_register(SRCS ${srcs} INCLUDE_DIRS "."
	EMBED_FILES "../html/error.html"
				"../html/favicon.ico"
				"../html/main.js"
				"../html/root.html"
				"../html/main.css")
