FILE(GLOB_RECURSE app_sources
	${CMAKE_SOURCE_DIR}/src/*.c
	${CMAKE_SOURCE_DIR}/src/*.cpp
	${CMAKE_SOURCE_DIR}/lib/*/*.c
	${CMAKE_SOURCE_DIR}/lib/*/*.cpp
	${CMAKE_SOURCE_DIR}/utils/KynoWave/*.c
	${CMAKE_SOURCE_DIR}/utils/KynoWave/*.cpp
)

idf_component_register(
	SRCS ${app_sources}
	INCLUDE_DIRS
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_SOURCE_DIR}/lib/DeviceHub
		${CMAKE_SOURCE_DIR}/lib/Display_LCD
		${CMAKE_SOURCE_DIR}/lib/Logger
		${CMAKE_SOURCE_DIR}/lib/OTA_connection
		${CMAKE_SOURCE_DIR}/lib/Parallel_Process
		${CMAKE_SOURCE_DIR}/utils/KynoWave
)