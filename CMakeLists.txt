cmake_minimum_required(VERSION 3.2)

project(communications VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})

include_directories(${PROJECT_SOURCE_DIR})

#configure_file(version.h.in version.h)

add_library(communications_tcp
        communications.h
        communacations_types.h
        ../interface/interface.h
        tcp/impl/client_session.cpp
        tcp/impl/server.cpp
        tcp/impl/client.cpp
        udp/multicast/impl/client.cpp
        )
target_link_libraries(communications_tcp -lboost_system)

add_executable(communications_tcp_test_app
        tcp/test/tcp_server.cpp
        )
target_link_libraries(communications_tcp_test_app
        communications_tcp
        -lpthread
        )

add_library(communications_udp_multicast
        udp/multicast/impl/client.cpp
        )
target_link_libraries(communications_udp_multicast -lboost_system)

add_executable(communications_udp_multicast_test_app
        udp/multicast/test/multicast_client.cpp
        )
target_link_libraries(communications_udp_multicast_test_app
        communications_udp_multicast
        -lpthread
        )

set_target_properties(communications_tcp
        communications_tcp_test_app
        communications_udp_multicast
        communications_udp_multicast_test_app

        PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        COMPILE_OPTIONS -Wpedantic -Wall -Wextra
        )

install(TARGETS communications_tcp communications_tcp_test_app
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT artyom.sorokoumov@gmail.com)

include(CPack)
