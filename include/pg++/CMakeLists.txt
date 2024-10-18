target_sources(pg++ PUBLIC FILE_SET HEADERS FILES
    pg++
)

add_subdirectory(client)
add_subdirectory(connection)
add_subdirectory(except)
add_subdirectory(result)
add_subdirectory(sql)
