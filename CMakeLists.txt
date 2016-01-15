ADD_DEFINITIONS(-Wall)

add_executable(bcd bcd.c bcd_linked.c const.h crc32.c flash.c flash.h linked.c message.c syslog.c) 
add_executable(bcc bcc.c message.c linked.c)

target_link_libraries(bcd pthread) 
add_definitions(-D__ARM)

add_library(message STATIC message.c linked.c)

install(TARGETS bcd bcc DESTINATION bin)
