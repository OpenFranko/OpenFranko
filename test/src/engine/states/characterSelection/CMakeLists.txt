set(CHARACTER_SELECTION_STATE_TEST_TARGET characterSelectionStateTest)
add_executable(${CHARACTER_SELECTION_STATE_TEST_TARGET}
    characterSelectionStateTest.cpp)
target_link_libraries(${CHARACTER_SELECTION_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Amal Assets Audio Graphics Input States
            StatesCharacterSelection)
add_test(NAME ${CHARACTER_SELECTION_STATE_TEST_TARGET}
    COMMAND ${CHARACTER_SELECTION_STATE_TEST_TARGET})
