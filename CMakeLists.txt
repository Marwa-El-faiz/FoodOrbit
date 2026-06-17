cmake_minimum_required(VERSION 3.16)
project(Food_Delivery VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network PrintSupport LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network PrintSupport LinguistTools)

set(APP_SOURCES
    main.cpp
    # ── Base de données ──
    database.h database.cpp
    # ── Classes métier ──
    utilisateur.h utilisateur.cpp
    client.h client.cpp
    # ── Langue ──
    languagemanager.h languagemanager.cpp
    # ── Interface Login / Client ──
    loginwindow.h loginwindow.cpp loginwindow.ui
    resetwindow.h resetwindow.cpp resetwindow.ui
    clientwindow.h clientwindow.cpp clientwindow.ui
    # ── Interface Restaurant ──
    Interface_Restaurant.h Interface_Restaurant.cpp Interface_Restaurant.ui
    # ── Interface Livreur ──
    Interface_Livreur.h Interface_Livreur.cpp Interface_Livreur.ui
    AIAssistant.h AIAssistant.cpp
)

set(TS_FILES
    translations/app_fr.ts
    translations/app_ar.ts
    translations/app_en.ts
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Food_Delivery
        MANUAL_FINALIZATION
        ${APP_SOURCES}
        newpasswordwindow.h newpasswordwindow.cpp
        ordervalidator.h ordervalidator.cpp
    )
    qt_add_lupdate(Food_Delivery TS_FILES ${TS_FILES} NO_GLOBAL_TARGET)
    qt_add_lrelease(Food_Delivery
        TS_FILES ${TS_FILES}
        QM_FILES_OUTPUT_VARIABLE QM_FILES
    )
    qt_add_resources(Food_Delivery "translations"
        PREFIX "/translations"
        BASE "${CMAKE_CURRENT_BINARY_DIR}"
        FILES ${QM_FILES}
    )
else()
    if(ANDROID)
        add_library(Food_Delivery SHARED ${APP_SOURCES})
    else()
        add_executable(Food_Delivery ${APP_SOURCES})
    endif()
    qt5_add_translation(QM_FILES ${TS_FILES})
    qt5_add_resources(Food_Delivery "translations"
        PREFIX "/translations"
        FILES ${QM_FILES}
    )
endif()

target_link_libraries(Food_Delivery PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Food_Delivery)
endif()

set_target_properties(Food_Delivery PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Food_Delivery
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Food_Delivery)
endif()