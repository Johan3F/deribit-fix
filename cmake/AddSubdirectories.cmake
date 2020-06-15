macro(AddSubdirectories curdir)
	file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)
	foreach(child ${children})
		if (${child} MATCHES "build")

		else()
			if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
				file(GLOB subdir_sources "${curdir}/${child}/*.cpp")
				file(GLOB subdir_headers "${curdir}/${child}/*.h")
	
				target_sources(${PROJECT_NAME} PRIVATE ${subdir_sources} ${subdir_headers})

				AddSubdirectories(${curdir}/${child})
			endif()
		endif()
	endforeach()
endmacro()