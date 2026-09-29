set(FOTO_SEQUENCE_TEST_TARGET fotoSequenceTest)
add_executable(${FOTO_SEQUENCE_TEST_TARGET}
    fotoSequenceTest.cpp
)
target_link_libraries(${FOTO_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsSequences)
add_test(NAME ${FOTO_SEQUENCE_TEST_TARGET} COMMAND ${FOTO_SEQUENCE_TEST_TARGET})

set(STORY_SEQUENCE_TEST_TARGET storySequenceTest)
add_executable(${STORY_SEQUENCE_TEST_TARGET}
    storySequenceTest.cpp
)
target_link_libraries(${STORY_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsSequences)
add_test(NAME ${STORY_SEQUENCE_TEST_TARGET} COMMAND ${STORY_SEQUENCE_TEST_TARGET})

set(BLYSK_SEQUENCE_TEST_TARGET blyskSequenceTest)
add_executable(${BLYSK_SEQUENCE_TEST_TARGET}
    blyskSequenceTest.cpp
)
target_link_libraries(${BLYSK_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsSequences)
add_test(NAME ${BLYSK_SEQUENCE_TEST_TARGET} COMMAND ${BLYSK_SEQUENCE_TEST_TARGET})

set(MENU_SEQUENCE_TEST_TARGET menuSequenceTest)
add_executable(${MENU_SEQUENCE_TEST_TARGET}
    menuSequenceTest.cpp
)
target_link_libraries(${MENU_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsSequences)
add_test(NAME ${MENU_SEQUENCE_TEST_TARGET} COMMAND ${MENU_SEQUENCE_TEST_TARGET})

set(ATTRACT_SEQUENCE_TEST_TARGET attractSequenceTest)
add_executable(${ATTRACT_SEQUENCE_TEST_TARGET}
    attractSequenceTest.cpp
)
target_link_libraries(${ATTRACT_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsSequences)
add_test(NAME ${ATTRACT_SEQUENCE_TEST_TARGET} COMMAND ${ATTRACT_SEQUENCE_TEST_TARGET})

set(CHARACTER_SELECTION_SEQUENCE_TEST_TARGET characterSelectionSequenceTest)
add_executable(${CHARACTER_SELECTION_SEQUENCE_TEST_TARGET}
    characterSelectionSequenceTest.cpp
)
target_link_libraries(${CHARACTER_SELECTION_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsSequences)
add_test(NAME ${CHARACTER_SELECTION_SEQUENCE_TEST_TARGET} COMMAND ${CHARACTER_SELECTION_SEQUENCE_TEST_TARGET})
