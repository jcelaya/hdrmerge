
find_package(Qt5Core QUIET)

if (Qt5Core_FOUND)
  set(_allComponents
      Core
      Gui
      DBus
      Designer
      Declarative
      Script
      ScriptTools
      Network
      Test
      Xml
      Svg
      Sql
      Widgets
      PrintSupport
      Concurrent
      UiTools
      WebKit
      WebKitWidgets
      OpenGL
      X11Extras
      Qml
      Quick
    )
  if (NOT Qt5Transitional_FIND_COMPONENTS)
    foreach(_component ${_allComponents})
      find_package(Qt5${_component})

      list(APPEND QT_LIBRARIES ${Qt5${_component}_LIBRARIES})
    endforeach()
  else()
    set(_components ${Qt5Transitional_FIND_COMPONENTS})
    foreach(_component ${Qt5Transitional_FIND_COMPONENTS})
      find_package(Qt5${_component} REQUIRED)
      if ("${_component}" STREQUAL "WebKit")
        find_package(Qt5WebKitWidgets REQUIRED)
        list(APPEND QT_LIBRARIES ${Qt5WebKitWidgets_LIBRARIES} )
      endif()
      if ("${_component}" STREQUAL "Gui")
        find_package(Qt5Widgets REQUIRED)
        find_package(Qt5PrintSupport REQUIRED)
        find_package(Qt5Svg REQUIRED)
        list(APPEND QT_LIBRARIES ${Qt5Widgets_LIBRARIES}
                                 ${Qt5PrintSupport_LIBRARIES}
                                 ${Qt5Svg_LIBRARIES} )
      endif()
      if ("${_component}" STREQUAL "Core")
        find_package(Qt5Concurrent REQUIRED)
        list(APPEND QT_LIBRARIES ${Qt5Concurrent_LIBRARIES} )
      endif()
    endforeach()
  endif()

  set(Qt5Transitional_FOUND TRUE)
  set(QT5_BUILD TRUE)

  include("${CMAKE_CURRENT_LIST_DIR}/ECMQt4To5Porting.cmake") # TODO: Port away from this.

else()
  foreach(_component ${Qt5Transitional_FIND_COMPONENTS})
    if("${_component}" STREQUAL "Widgets")  # new in Qt5
      set(_component Gui)
    elseif("${_component}" STREQUAL "Concurrent")   # new in Qt5
      set(_component Core)
    endif()
    list(APPEND _components Qt${_component})
  endforeach()
  find_package(Qt4 ${QT_MIN_VERSION} REQUIRED ${_components})

  if(QT4_FOUND)
    set(Qt5Transitional_FOUND TRUE)
  endif()
endif()
