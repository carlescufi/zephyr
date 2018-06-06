include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})


add_definitions( -DCONFIG_APP_SHARED_MEM )
zephyr_library_compile_definitions( -DAPP_SHARED_MEM)
target_compile_definitions( tests__ztest PRIVATE -DAPP_SHARED_MEM=y)
target_compile_definitions( kernel PRIVATE -DAPP_SHARED_MEM=y)
