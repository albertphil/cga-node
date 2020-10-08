add_executable (cga_node_beta
	daemon.cpp
	daemon.hpp
	entry.cpp)

target_link_libraries (cga_node_beta
	node
	secure
	argon2
	Boost::boost
	${PLATFORM_LIBS}
)

target_compile_definitions(cga_node_beta
	PRIVATE
		-DCGA_VERSION_MAJOR=${CPACK_PACKAGE_VERSION_MAJOR}
		-DCGA_VERSION_MINOR=${CPACK_PACKAGE_VERSION_MINOR}
		-DCGA_VERSION_PATCH=${CPACK_PACKAGE_VERSION_PATCH})

set_target_properties (cga_node_beta
	PROPERTIES
		COMPILE_FLAGS
			"-DQT_NO_KEYWORDS -DBOOST_ASIO_HAS_STD_ARRAY=1")

if ((CGA_GUI) AND NOT APPLE)
	install(TARGETS cga_node_beta
			RUNTIME DESTINATION bin
			LIBRARY DESTINATION lib
			ARCHIVE DESTINATION lib
	)
endif()
