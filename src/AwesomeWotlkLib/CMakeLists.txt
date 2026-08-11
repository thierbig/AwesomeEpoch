project( AwesomeWotlkLib )

add_library(
	${PROJECT_NAME} SHARED
		"Entry.cpp"

		GameClient.h
        "NamePlates.h" "NamePlates.cpp"
		"Hooks.h" "Hooks.cpp"
		"Misc.h" "Misc.cpp"
		"BugFixes.h" "BugFixes.cpp"
		"Utils.h" "Utils.cpp"
		"CommandLine.cpp" "CommandLine.h"
		"Inventory.cpp" "Inventory.h"
		"Item.h" "Item.cpp"
		"UnitAPI.h" "UnitAPI.cpp"
		"Spell.h" "Spell.cpp"
		"VoiceChat.h" "VoiceChat.cpp"
		"AntiDetection.h" "AntiDetection.cpp"
		"AdvancedEvasion.h" "AdvancedEvasion.cpp"
		"EvasionLogger.h" "EvasionLogger.cpp")

target_include_directories(
    ${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/deps
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
        Detours
        ole32
        oleaut32
        uuid
        sapi
)

target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
        APP_NAME="${PROJECT_NAME}"
)

# Isolated upstream MSDF/D3D font-rendering library (compiled against upstream's headers).
add_subdirectory( msdf )
target_link_libraries( ${PROJECT_NAME} PRIVATE AwesomeMSDF )

# skia.dll is a runtime dependency of the MSDF pipeline: place it next to the built DLL so it
# ships in the build output and must be copied into the game folder alongside AwesomeWotlkLib.dll.
add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/deps/skia/skia.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/skia.dll"
    COMMENT "Copying skia.dll next to AwesomeWotlkLib.dll (MSDF runtime dependency)"
)
