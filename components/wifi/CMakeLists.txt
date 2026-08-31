idf_component_register(SRCS "wifi_comp.cpp" "wifi_service.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES interfaces message-router)
