# Include script helpers
include(FindLATEX)

# Require PDFLaTeX for further progressing
if(PDFLATEX_COMPILER)
	# Request build options from user
	option(BUILD_DOCS "Generate documents with PDFLaTeX?" ON)
	# Setup documents installation
	if(BUILD_DOCS)
		message(STATUS "Documents installation path: ${CMAKE_INSTALL_PREFIX}/share/doc/${PROJECT_NAME_LOWER}")
		execute_process(COMMAND ${PDFLATEX_COMPILER} -output-directory=${CMAKE_CURRENT_BINARY_DIR} -draftmode main.tex WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})	# Needed to collect forward informations (like table-of-contents) first
		execute_process(COMMAND ${PDFLATEX_COMPILER} -output-directory=${CMAKE_CURRENT_BINARY_DIR} main.tex WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/main.pdf DESTINATION share/doc/${PROJECT_NAME_LOWER} RENAME ${PROJECT_NAME}.pdf)
	endif()
else()
	message(WARNING "PDFLaTeX required to build documents!")
endif()
