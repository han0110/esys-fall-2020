add_library(bsp-stm32l475 INTERFACE)

target_include_directories(bsp-stm32l475
    INTERFACE
        .
        components
        components/common
        components/lsm6dsl
)

target_sources(bsp-stm32l475
    INTERFACE
        bsp.c
        accelero.c
        gyro.c
        components/lsm6dsl/lsm6dsl.c
)
