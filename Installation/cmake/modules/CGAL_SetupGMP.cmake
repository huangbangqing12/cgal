if ( NOT CGAL_GMP_SETUP )
  
  find_package( GMP REQUIRED )
  
  message( STATUS "GMP include:     ${GMP_INCLUDE_DIR}" )
  message( STATUS "GMP libraries:   ${GMP_LIBRARIES}" )
  message( STATUS "GMP definitions: ${GMP_DEFINITIONS}" )
  
  set ( CGAL_USE_GMP 1 )
  
  include(CGAL_Macros)
  
  get_dependency_version(GMP)
  
  add_to_cached_list(CGAL_3RD_PARTY_INCLUDE_DIRS   ${GMP_INCLUDE_DIR}   )
  add_to_cached_list(CGAL_3RD_PARTY_LIBRARIES_DIRS ${GMP_LIBRARIES_DIR} )
  add_to_cached_list(CGAL_3RD_PARTY_DEFINITIONS    ${GMP_DEFINITIONS}   )
  
  if ( NOT MSVC )
    add_to_cached_list(CGAL_3RD_PARTY_LIBRARIES ${GMP_LIBRARIES} )
  endif()
  
  set( CGAL_GMP_SETUP TRUE )
  
endif()
