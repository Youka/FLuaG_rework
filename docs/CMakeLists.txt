# Include script helpers
include(FindLATEX)

# Require PDFLaTeX for further progressing
if(PDFLATEX_COMPILER)
	# Request build options from user
	option(BUILD_DOCS "Generate documents with PDFLaTeX?" ON)
	# Setup documents installation
	if(BUILD_DOCS)
		message(STATUS "Documents installation path: ${CMAKE_INSTALL_PREFIX}/share/fluag")
		file(GLOB DOC_FILES *.tex)
		foreach(doc ${DOC_FILES})
			execute_process(COMMAND ${PDFLATEX_COMPILER} -output-directory=${CMAKE_CURRENT_BINARY_DIR} ${doc})
		endforeach()
		file(GLOB DOC_FILES ${CMAKE_CURRENT_BINARY_DIR}/*.pdf)
		install(FILES ${DOC_FILES} DESTINATION share/fluag)
	endif()
endif()
