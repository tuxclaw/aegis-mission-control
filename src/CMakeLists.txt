set(AEGIS_CORE_SOURCES
  app/app_context.cpp
  app/app_context.h
  config/config_service.cpp
  config/config_service.h
  config/secret_store.cpp
  config/secret_store.h
  core/aegis_error.cpp
  core/aegis_error.h
  core/async.h
  core/atomic_file.cpp
  core/atomic_file.h
  core/http_client.cpp
  core/http_client.h
  core/logging.cpp
  core/logging.h
  core/path_guard.cpp
  core/path_guard.h
  core/result.h
  dto/agent_dto.cpp
  dto/agent_dto.h
  dto/calendar_event_dto.cpp
  dto/calendar_event_dto.h
  dto/creative_dto.cpp
  dto/creative_dto.h
  dto/cron_job_dto.cpp
  dto/cron_job_dto.h
  dto/enums.h
  dto/git_status_dto.cpp
  dto/git_status_dto.h
  dto/json_validation.cpp
  dto/json_validation.h
  dto/memory_file_dto.cpp
  dto/memory_file_dto.h
  dto/model_dto.cpp
  dto/model_dto.h
  dto/package_dto.cpp
  dto/package_dto.h
  dto/vitals_dto.cpp
  dto/vitals_dto.h
)

add_library(aegis_core STATIC ${AEGIS_CORE_SOURCES})
target_include_directories(aegis_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/third_party"
)
target_link_libraries(aegis_core
  PUBLIC Qt6::Core Qt6::Concurrent Qt6::Network
)

if(TARGET Qt6Keychain::Qt6Keychain)
  target_link_libraries(aegis_core PRIVATE Qt6Keychain::Qt6Keychain)
  target_compile_definitions(aegis_core PRIVATE AEGIS_HAS_QTKEYCHAIN=1)
elseif(TARGET qt6keychain)
  target_link_libraries(aegis_core PRIVATE qt6keychain)
  target_compile_definitions(aegis_core PRIVATE AEGIS_HAS_QTKEYCHAIN=1)
else()
  message(WARNING "QtKeychain development files not found; SecretStore will fail closed")
endif()

aegis_enable_warnings(aegis_core)

if(AEGIS_GUI_AVAILABLE)
  qt_add_executable(aegis
    app/qml_registration.cpp
    app/qml_registration.h
    main.cpp
    "${PROJECT_SOURCE_DIR}/resources/aegis.qrc"
  )
  target_link_libraries(aegis PRIVATE
    aegis_core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
  )
  set_target_properties(aegis PROPERTIES OUTPUT_NAME aegis)
endif()
