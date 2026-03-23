idf_component_register(SRCS "storage.cpp"
                            "test/storage_unit_tests.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES fatfs catch2 perfmon log
                    WHOLE_ARCHIVE)