#!/usr/bin/env ruby
#
#  Author: Johnny Lai
#  Copyright (c) 2004 Johnny Lai
#
# =DESCRIPTION
# CMakeLists.txt generater for omnetpp type projects. Currently works with INET
# and generates IPv6Suite's OneBigExe.cmake but should work for others too. 
#

#Pattern for doing wildcard matches of filenames recursively i.e. subdirectories too 
RECURSEDIR="**/*"

#Extension of files used for message subclassing
MSGEXT=".msg"
NEDEXT=".ned"

def traverseDirectory(dir, expression, ignore = nil)
  oldpwd = Dir.pwd
  Dir.chdir(dir)
  arr = Array.new
  Dir[expression].each {|f|
    next if ignore and f =~ Regexp.new(ignore)
    arr.push(f)
  }
  arr
ensure
  Dir.chdir(oldpwd)
end

def customCommands(dir, ignorePattern,projName)  
  
  m = traverseDirectory(dir,RECURSEDIR+MSGEXT, ignorePattern)
 
  string = "\nOPP_WRAP_MSGC(dum dum2 #{m.join("\n")}\n)\n"
	#the following will include all msg files in all subdirs
	#string += "\nOPP_WRAP_MSGC_ALL()\n"  
end

def createNedFilesList(dir, ignorePattern)  
  m = traverseDirectory(dir,RECURSEDIR+NEDEXT, ignorePattern)
  string = "#{m.join("\n")}"
  open("#{dir}/nedfiles.lst","w") {  |x|
    x.print string
  }
end

def addSourceFiles(dir, ignorePattern)
  c = traverseDirectory(dir, RECURSEDIR+".{h,cc,cpp,c}", ignorePattern)

  includeDirs = Array.new
  c.delete_if {|f| 
    header = f =~ /\.h$/     
    includeDirs.push(File.dirname(f)) if header and not includeDirs.include? File.dirname(f) 
    header
  }
  Array[c, includeDirs]
end

def addTests(dir)
  c = traverseDirectory(dir, RECURSEDIR + ".test")
  testDirs = Array.new
  c.each{ |test|
    testDirs.push(File.dirname(test)) if not testDirs.include? File.dirname(test)
  }
  testDirs
end

def writeTest(testDirs, projName)
  testDirs.each{ |d|
    open("#{d}/CMakeLists.txt","w") { |testCMake|
      testCMake.puts "LINK_LIBRARIES(#{projName} ${OPP_LIBRARIES})"
      testCMake.puts "OPP_WRAP_TEST(#{File.basename(d)})" 
    }
  }  
end

  OldRTPDeps = "|Scenario|RSVP|MPLS"

def writeCMakeList(dir, outputName, projName = nil)
				#EtherSwitch
#Contract requires IPv4
#ARP|IPv4|TCP|FlatNetwork|Queue
#RTP because SocketInterface does not yet exist
  commonIgnore =
	"CMake|Unsupported|_m\.|test|Topology|PPP/|LDP|Tests|IPv4d" + OldRTPDeps
  
  sources, includes = addSourceFiles(dir, commonIgnore)    
  
  projName ||= File.basename(dir)  
  
  open("#{dir}/#{outputName}","w") {  |x|

    x.puts "# -*- CMAKE -*-"
    x.puts %{#Generated by "#{$0} #{ARGV.join(" ")}"}

    if not @customise
      x.puts("PROJECT(#{projName})") if projName and projName.length > 0
     
xvar = <<EOF
# set_dir_props generated from customCommands requires this
CMAKE_MINIMUM_REQUIRED(VERSION 2.0)
SET(CMAKE_BACKWARDS_COMPATIBILITY 2.0 CACHE STRING "2.4 uses default path before our custom path for omnet libs unless we do NO_DEFAULT_PATH in every FIND_LIBRARY" FORCE)
SET(OPP_USE_TK OFF CACHE BOOL "OFF unless you are sure tk gui can build")
SET(CMAKEFILES_PATH ${PROJECT_SOURCE_DIR}/Etc/CMake)
SET(MISCDIR ${PROJECT_SOURCE_DIR}/Etc)
SET(SCRIPTDIR ${MISCDIR}/scripts)
OPTION(BUILD_SHARED_LIBS "Build with shared libraries." ON)
SET(ONE_BIG_EXE ON)

INCLUDE(${CMAKEFILES_PATH}/Options.cmake)

#INCLUDE(${CMAKEFILES_PATH}/IntelICCOptions.cmake)

#INCLUDE(${CMAKE_ROOT}/Modules/Dart.cmake)

INCLUDE(${CMAKEFILES_PATH}/Configure.cmake)

INCLUDE(${CMAKEFILES_PATH}/DocTargets.cmake)

INCLUDE(${CMAKEFILES_PATH}/LinkLibraries.cmake)

ADD_DEFINITIONS(-DBOOST_WITH_LIBS -DWITH_IPv6 -DUSE_MOBILITY -DFASTRS -DFASTRA -DUSE_HMIP -DEDGEHANDOVER=1)
 
INCLUDE_DIRECTORIES(${OPP_INCLUDE_PATH})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
EOF
      x.puts xvar

    end

    createNedFilesList(dir, commonIgnore + "|Examples|Research")
    customCommandsLines = customCommands(dir, commonIgnore, projName)
    
    x.print customCommandsLines

    #It appears that these source files properties only exist in the current
    #Dir/cmakelist.txt because in subdir's cmakelist.txt cannot use the source
    #file here as it complains source does not exist unless we also set generated property in there for these files again 
    #this does not work however
    #x.print "SET_SOURCE_FILES_PROPERTIES(${GENERATED_MSGC_FILES} GENERATED PROPERTIES COMPILE_FLAGS -Wall)\n\n"

    x.print "\nSET( ", projName, "_SRCS\n"

    #necessary otherwise any subsequent SUBDIRS commands will change
    #the relative source file to an incorrect absolute path
    basepath="${PROJECT_SOURCE_DIR}/"
    
    sources.each{|c| 
      x.puts basepath + c
    }

    x.puts ")"


    x.puts "SET_SOURCE_FILES_PROPERTIES(${#{projName}_SRCS} PROPERTIES  COMPILE_FLAGS -Wall)"
    x.puts "SET(#{projName}_SRCS ${GENERATED_MSGC_FILES} ${#{projName}_SRCS})"
    x.puts
    x.puts

    x.puts "INCLUDE_DIRECTORIES("
    includes.each{|inc|         
      x.puts basepath + inc
    }
    x.puts ")\n\n"

    if not @customise
#      outputdir = projName == "INET" ? "bin" : "."
      outputdir = "."
xvar = <<EOF
SET(OUTPUTDIR #{outputdir})
 
# No longer works cause simplemodules don't export their type no more in gcc output
#      if not projName == "INET" then
#        x.puts(sprintf("ADD_LIBRARY(%s ${%s})\n", projName, projName + "_SRCS"))
#      end

SET(#{projName} ${OUTPUTDIR}/#{projName})
ADD_EXECUTABLE(${#{projName}} ${#{projName}_SRCS})
TARGET_LINK_LIBRARIES(${#{projName}} ${OPP_CMDLIBRARIES} -lstdc++)

IF(NOT LIBCWD_DEBUG)
IF(CMAKE_CACHE_MINOR_VERSION EQUAL 0)
IF(OPP_USE_TK)
SET(tk#{projName} ${OUTPUTDIR}/tk#{projName})
ADD_EXECUTABLE(${tk#{projName}} ${#{projName}_SRCS})
TARGET_LINK_LIBRARIES(${tk#{projName}} ${OPP_TKGUILIBRARIES} -lstdc++)
ENDIF(OPP_USE_TK)
ENDIF(CMAKE_CACHE_MINOR_VERSION EQUAL 0)
ENDIF(NOT LIBCWD_DEBUG)

ENABLE_TESTING()
SUBDIRS(
Research/Networks/
)

EOF
      x.puts xvar
    end
  }
end

## main

if ARGV.length < 2 then
  print "Usage ",  " <source dir name> <Project Name>\n", \
  " where [source dir] is where MakeLists.txt will be generated for\n", \
  "Generated in current working directory"
  exit
else
  projName = ARGV[1]
	#customise used to be for making OneBigStaticExe.cmake except forgot how to generate sourcelist sourcelist 
  @customise = false
  outname = @customise ? "OneBigStaticExe.cmake" : "CMakeLists.txt"
  writeCMakeList(ARGV[0], outname, projName)
end
