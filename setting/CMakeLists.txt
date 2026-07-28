cmake_minimum_required(VERSION 3.20)

project(lgdm-kcm)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)


find_package(Qt6 REQUIRED COMPONENTS Widgets)


find_package(KF6KCMUtils REQUIRED)
find_package(KF6CoreAddons REQUIRED)


add_library(kcm_lgdm MODULE

    src/main.cpp
    src/main.h
    ../dm/readcfg.c
    ../dm/readcfg.h
)


target_link_libraries(kcm_lgdm

    Qt6::Widgets

    KF6::KCMUtils
    KF6::CoreAddons
)
target_include_directories(kcm_lgdm PRIVATE
    /usr/include/KF6/KCMUtils
)

set_target_properties(kcm_lgdm PROPERTIES

    LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/kcms"
)
