idf_component_register(SRCS "mqtt.c" "ota_updata.c" "pwm.c" "pid.c" "can.c" "app_main.c" "ota_updata.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)
