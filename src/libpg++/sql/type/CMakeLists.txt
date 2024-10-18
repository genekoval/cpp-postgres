target_sources(pg++ PRIVATE
    bool.cpp
    bytea.cpp
    int.cpp
    json.cpp
    string.cpp
    uuid.cpp
)

if(PROJECT_TESTING)
    target_sources(pg++.test PRIVATE
        bool.test.cpp
        bytea.test.cpp
        composite.test.cpp
        enum.test.cpp
        int.test.cpp
        json.test.cpp
        optional.test.cpp
        string.test.cpp
        timestamp.test.cpp
        tuple.test.cpp
        uuid.test.cpp
        vector.test.cpp
    )
endif()
