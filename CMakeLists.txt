cmake_minimum_required(VERSION 3.10)
project(cleanhome C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(EFL REQUIRED elementary evas ecore ecore-evas)
pkg_check_modules(APPFW REQUIRED capi-appfw-application capi-appfw-app-manager capi-appfw-app-control capi-appfw-app-common dlog)
pkg_check_modules(SDL2 REQUIRED sdl2 SDL2_image SDL2_ttf)

add_executable(cleanhome
    src/main_tizen.c
    src/platform_tizen.c
    src/render.c
    src/text.c
    src/image.c
    src/launcher.c
    src/config.c
)

target_compile_definitions(cleanhome PRIVATE CLEANHOME_TIZEN)
target_include_directories(cleanhome PRIVATE
    src
    ${EFL_INCLUDE_DIRS}
    ${APPFW_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
)
link_directories(
    ${EFL_LIBRARY_DIRS}
    ${APPFW_LIBRARY_DIRS}
    ${SDL2_LIBRARY_DIRS}
)
target_link_libraries(cleanhome PRIVATE
    ${EFL_LIBRARIES}
    ${APPFW_LIBRARIES}
    ${SDL2_LIBRARIES}
    GLESv2
    m
)

install(TARGETS cleanhome RUNTIME DESTINATION bin)
install(DIRECTORY assets DESTINATION res)
install(FILES tizen/shared/res/icon.png DESTINATION shared/res)
