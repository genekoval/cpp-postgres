target_sources(pg++ PUBLIC FILE_SET HEADERS FILES
    channel.hpp
    connection.hpp
    io.hpp
    reader.hpp
    socket.hpp
    writer.hpp
)

add_subdirectory(type)
