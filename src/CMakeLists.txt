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
  controllers/agent_controller.cpp
  controllers/agent_controller.h
  controllers/app_controller.cpp
  controllers/app_controller.h
  controllers/calendar_controller.cpp
  controllers/calendar_controller.h
  controllers/creative_controller.cpp
  controllers/creative_controller.h
  controllers/cron_controller.cpp
  controllers/cron_controller.h
  controllers/git_controller.cpp
  controllers/git_controller.h
  controllers/memory_controller.cpp
  controllers/memory_controller.h
  controllers/model_controller.cpp
  controllers/model_controller.h
  controllers/package_controller.cpp
  controllers/package_controller.h
  controllers/settings_controller.cpp
  controllers/settings_controller.h
  controllers/vitals_controller.cpp
  controllers/vitals_controller.h
  models/agent_list_model.cpp
  models/agent_list_model.h
  models/calendar_event_model.cpp
  models/calendar_event_model.h
  models/cron_job_model.cpp
  models/cron_job_model.h
  models/git_file_model.cpp
  models/git_file_model.h
  models/memory_file_model.cpp
  models/memory_file_model.h
  models/model_list_model.cpp
  models/model_list_model.h
  models/package_list_model.cpp
  models/package_list_model.h
  services/calendar_store.cpp
  services/calendar_store.h
  services/creative_service.cpp
  services/creative_service.h
  services/cron_service.cpp
  services/cron_service.h
  services/gateway_service.cpp
  services/gateway_service.h
  services/git_service.cpp
  services/git_service.h
  services/memory_service.cpp
  services/memory_service.h
  services/model_service.cpp
  services/model_service.h
  services/openclaw_cli.cpp
  services/openclaw_cli.h
  services/package_service.cpp
  services/package_service.h
  services/vitals_service.cpp
  services/vitals_service.h
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

target_link_libraries(aegis_core PRIVATE Libgit2::Libgit2)

if(TARGET Qt6Keychain::Qt6Keychain)
  target_link_libraries(aegis_core PRIVATE Qt6Keychain::Qt6Keychain)
else()
  target_link_libraries(aegis_core PRIVATE qt6keychain)
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
