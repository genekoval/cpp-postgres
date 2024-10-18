target_sources(pg++ PRIVATE
    client.cpp
    connect.cpp
    parameters.cpp
    passfile.cpp
    transaction.cpp
)

if(PROJECT_TESTING)
    target_sources(pg++.test PRIVATE
        client.test.cpp
        fetch.test.cpp
        fetch_rows.test.cpp
        notification.test.cpp
        passfile.test.cpp
        portal.test.cpp
        prepare.test.cpp
        query.test.cpp
        simple_query.test.cpp
        transaction.test.cpp
    )
endif()
