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
  dto/container_dto.cpp
  dto/container_dto.h
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
  dto/process_dto.cpp
  dto/process_dto.h
  dto/provider_quota.h
  dto/vitals_dto.cpp
  dto/vitals_dto.h
  controllers/agent_controller.cpp
  controllers/agent_controller.h
  controllers/app_controller.cpp
  controllers/app_controller.h
  controllers/calendar_controller.cpp
  controllers/calendar_controller.h
  controllers/container_controller.cpp
  controllers/container_controller.h
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
  controllers/process_controller.cpp
  controllers/process_controller.h
  controllers/settings_controller.cpp
  controllers/settings_controller.h
  controllers/usage_controller.cpp
  controllers/usage_controller.h
  controllers/vitals_controller.cpp
  controllers/vitals_controller.h
  models/agent_list_model.cpp
  models/agent_list_model.h
  models/calendar_event_model.cpp
  models/calendar_event_model.h
  models/container_list_model.cpp
  models/container_list_model.h
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
  models/process_list_model.cpp
  models/process_list_model.h
  models/usage_list_model.cpp
  models/usage_list_model.h
  services/calendar_store.cpp
  services/calendar_store.h
  services/container_service.cpp
  services/container_service.h
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
  services/monitor_config.cpp
  services/monitor_config.h
  services/openclaw_cli.cpp
  services/openclaw_cli.h
  services/package_service.cpp
  services/package_service.h
  services/process_service.cpp
  services/process_service.h
  services/i_provider_fetcher.cpp
  services/i_provider_fetcher.h
  services/provider_manager.cpp
  services/provider_manager.h
  services/providers/anthropic_fetcher.cpp
  services/providers/anthropic_fetcher.h
  services/providers/codexbar_fetcher.cpp
  services/providers/codexbar_fetcher.h
  services/providers/gemini_fetcher.cpp
  services/providers/gemini_fetcher.h
  services/providers/grok_fetcher.cpp
  services/providers/grok_fetcher.h
  services/providers/minimax_fetcher.cpp
  services/providers/minimax_fetcher.h
  services/providers/openai_fetcher.cpp
  services/providers/openai_fetcher.h
  services/providers/xiaomi_fetcher.cpp
  services/providers/xiaomi_fetcher.h
  services/providers/zai_fetcher.cpp
  services/providers/zai_fetcher.h
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
  PUBLIC Qt6::Core Qt6::Concurrent Qt6::Network Qt6::Sql
)

target_link_libraries(aegis_core PRIVATE Libgit2::Libgit2)

if(TARGET Qt6Keychain::Qt6Keychain)
  target_link_libraries(aegis_core PRIVATE Qt6Keychain::Qt6Keychain)
else()
  target_link_libraries(aegis_core PRIVATE qt6keychain)
endif()

aegis_enable_warnings(aegis_core)

if(AEGIS_GUI_AVAILABLE)
  # The imported Fable sources retain their CamelCase local includes while
  # living under the project's lowercase service filenames. SystemStats also
  # has a literal "*/" in a documentation path, so sanitize only the generated
  # build header while preserving the imported source byte-for-byte.
  file(READ services/system_stats.h FABLE_SYSTEM_STATS_HEADER)
  string(REPLACE "card*/device" "card* /device"
    FABLE_SYSTEM_STATS_HEADER "${FABLE_SYSTEM_STATS_HEADER}")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/SystemStats.h"
    "${FABLE_SYSTEM_STATS_HEADER}")
  configure_file(services/container_list.h
    "${CMAKE_CURRENT_BINARY_DIR}/ContainerList.h" COPYONLY)
  configure_file(services/agent_roster.h
    "${CMAKE_CURRENT_BINARY_DIR}/AgentRoster.h" COPYONLY)

  qt6_wrap_cpp(AEGIS_GUI_MOC_SOURCES
    app/qml_registration.h
    "${CMAKE_CURRENT_BINARY_DIR}/SystemStats.h"
    "${CMAKE_CURRENT_BINARY_DIR}/ContainerList.h"
    "${CMAKE_CURRENT_BINARY_DIR}/AgentRoster.h"
  )

  qt_add_executable(aegis
    app/qml_registration.cpp
    app/qml_registration.h
    main.cpp
    services/system_stats.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/SystemStats.h"
    services/container_list.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/ContainerList.h"
    services/agent_roster.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/AgentRoster.h"
    ${AEGIS_GUI_MOC_SOURCES}
    "${PROJECT_SOURCE_DIR}/resources/aegis.qrc"
  )
  set_target_properties(aegis PROPERTIES AUTOMOC OFF)
  target_include_directories(aegis PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
  target_link_libraries(aegis PRIVATE
    aegis_core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
  )
  set_target_properties(aegis PROPERTIES OUTPUT_NAME aegis)
endif()
