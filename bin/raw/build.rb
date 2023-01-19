#!/bin/ruby
require 'fileutils'
require 'inifile'
require 'io/console'                                                                                                       

action = ARGV[0] 
library = ARGV[1]
if action == "help"
  print "\n"
  puts 'usage: ./build.exe action argument'
  print "\n"
  puts '-action: '
  puts 'build, rebuild, compile, clean, '
  puts '--build: builds a specified library, ourputs binary'
  puts '--clean: cleans .o files from specified library, no arguments removes .hex and .elf files'
  puts '--rebuild: cleans then builds a specified library'
  puts '--compile: builds .cpp file in main directory, outputs .elf and .hex file'
  print "\n"
  puts '-argument'
  puts '--library: builds, rebuilds, or cleans specific library in src folder'
  puts '--all: builds, rebuilds, or cleans all libraries in src folder'
  exit
end

# read an existing file
file = IniFile.load('bin/settings.ini',:comment => ';')
data = file["Settings"]
CODENAME=data["CODENAME"] #here to get the .cpp code
PROGRAMMER=data["PROGRAMMER"] #programmer type
LIBS= data["LIBS"]
Llibraries= LIBS.split(' ')
temp =Llibraries.map{|item| "-l"+item}
LibString= temp.join(" ")
LibString.delete("'")
COMPORT=data["COMPORT"]
VARIANT=data["VARIANT"] #where to get the `pins_arduino.h` file
CPUFREQ=data["CPUFREQ"]
MCU=data["MCU"]
PARTNO=data["PARTNO"]
CFLAGS=data["CFLAGS"]
CPPFLAGS=data["CPPFLAGS"]
ARFLAGS=data["ARFLAGS"]
CC="avr-gcc"
CPP="avr-g++"
AR="avr-ar"
OBJC ="avr-objcopy"
SOURCES=data["SOURCES"]
OUTPUTS=data["OUTPUTS"]
OUT = "../../#{OUTPUTS}"
CCFLAGS = "-I#{OUT}/include -I./#{SOURCES}/ -I./ -I./utility/ -I./variants/#{VARIANT} -I./#{OUTPUTS}/include -mmcu=#{MCU} -DF_CPU=#{CPUFREQ}"

print "\n"
print "PROGRAMMER: "
puts PROGRAMMER
print "COMPORT: "
puts COMPORT
print "MCU: "
puts MCU
print "LIBS: "
puts LIBS
print "PARTNO: "
puts PARTNO
print "CPU Frequency: "
puts CPUFREQ
print "CFLAGS: "
puts CFLAGS
print "CPPFLAGS: "
puts CPPFLAGS
print "ARFLAGS: "
puts ARFLAGS
print "Source Folder:" 
puts SOURCES
print "Output Folder:" 
puts OUTPUTS
FileUtils.mkdir_p "#{OUTPUTS}/include"
FileUtils.mkdir_p "#{OUTPUTS}/lib"
FileUtils.mkdir_p "#{OUTPUTS}/codes"
`cp variants/#{VARIANT}/*.h #{OUTPUTS}/include/`
print "\n-------------------------Program Start---------------------------------\n"

def continue_story                                                                                                               
  print "Press Any Key to Continue"                                                                                                    
  STDIN.getch                                                                                                              
  print "            \r" # extra space to overwrite in case next sentence is short                                                                                                              
end        

def getfiles(dir, ending)
  t=""
  Dir["#{dir}/*.#{ending}"].each{ |f|  t+='"'+File.basename(f)+'" '; }
  t
end

def docompile(lib, action)
 cflags = CCFLAGS
 cppflags = cflags
 arflags = ""
if lib == "all"
  Dir.entries("#{SOURCES}/").reject{ |e| e.start_with? '.' }.each { 
  |f| puts f 
  library = f
  dir="#{SOURCES}/#{library}"
` cp ./bin/template.makefile #{SOURCES}/#{library}/Makefile`
  objs = getfiles(dir, "c").gsub(".c",".o");
  objs += " "+getfiles(dir, "cpp").gsub(".cpp", ".o");
  print "\n"
  hdrs = getfiles(dir, "h")
  domake("#{dir}", action, objs, hdrs, cppflags, cflags, arflags, OUT, "lib#{library}.a") 
  print "\n"
 `rm -f "#{SOURCES}/#{library}/Makefile"`
 }
else
 library = lib
 dir="#{SOURCES}/#{library}"
`cp ./bin/template.makefile #{SOURCES}/#{library}/Makefile`
 objs = getfiles(dir, "c").gsub(".c",".o");
 objs += " "+getfiles(dir, "cpp").gsub(".cpp", ".o");
 print "\n"
 hdrs = getfiles(dir, "h")
 domake("#{dir}", action, objs, hdrs, cppflags, cflags, arflags, OUT, "lib#{library}.a")
 print "\n"
 `rm -f "#{SOURCES}/#{library}/Makefile"`
end
end

def domake(src, action, objs,hdrs, cppflags, cflags, arflags, out, lib)
   `make -C #{src} #{action} OBJS="#{objs}" CPPFLAGS="#{CPPFLAGS} #{cppflags}" CFLAGS="#{CFLAGS} #{cflags}"\
      ARFLAGS="#{arflags}" OUTPUTS="#{OUT}" HDRS="#{hdrs}" LIBOUT="#{lib}" \
      AR="#{AR}" CC="#{CC}" CPP="#{CPP}" \
      1>&2`
  #make sure to redirect STDOUT to STDERR so it's visible
end

if action == "build" || action == "rebuild"
if ARGV.length < 2
  puts 'usage: ./build.rb action library'
  continue_story
  exit
end
docompile(library, "init")
docompile(library, action)
print "\n"
print "Library Compilation Done"
print "\n"
elsif  action == "clean"
 docompile(library, action)
if ARGV.length <2
`cp ./bin/template.makefile ./Makefile`
`make #{action}2  OUTPUTS="#{OUTPUTS}" CODENAME="#{CODENAME}"`
`rm -f ./Makefile"`
print "\n"
print ".hex &. elf Files Cleaned"
end
print "\n"
print "Clean Done"
print "\n"
else
 `cp ./bin/template.makefile ./Makefile`
 cflags = CCFLAGS
 cppflags = cflags
 arflags = ""
 `make #{action} CPP="#{CPP}" OBJC="#{OBJC}" CFLAGS="#{CFLAGS} #{cflags}" OUTPUTS="#{OUTPUTS}" CODENAME="#{CODENAME}" COMPORT="#{COMPORT}" PROGRAMMER="#{PROGRAMMER}" PARTNO="#{PARTNO}" LIBSTRING="#{LibString}"`
 `rm -f ./Makefile"`
 print "\n"
 print "Compilation Done"
 print "\n"
end
