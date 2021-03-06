# = NMatrix
#
# A linear algebra library for scientific computation in Ruby.
# NMatrix is part of SciRuby.
#
# NMatrix was originally inspired by and derived from NArray, by
# Masahiro Tanaka: http://narray.rubyforge.org
#
# == Copyright Information
#
# SciRuby is Copyright (c) 2010 - 2012, Ruby Science Foundation
# NMatrix is Copyright (c) 2012, Ruby Science Foundation
#
# Please see LICENSE.txt for additional copyright notices.
#
# == Contributing
#
# By contributing source code to SciRuby, you agree to be bound by
# our Contributor Agreement:
#
# * https://github.com/SciRuby/sciruby/wiki/Contributor-Agreement
#
# == extconf.rb
#
# This file mostly derived from NArray.

require "mkmf"


def have_type(type, header=nil)
  printf "checking for %s... ", type
  STDOUT.flush

  src = <<"SRC"
#include <ruby.h>
SRC


  src << <<"SRC" unless header.nil?
#include <#{header}>
SRC

  r = try_link(src + <<"SRC")
  int main() { return 0; }
  int t() { #{type} a; return 0; }
SRC

  unless r
    print "no\n"
    return false
  end

  $defs.push(format("-DHAVE_%s", type.upcase))

  print "yes\n"

  return true
end

def create_conf_h(file)
  print "creating #{file}\n"
  File.open(file, 'w') do |hfile|
  	header_guard = file.upcase.sub(/\s|\./, '_')
		
		hfile.puts "#ifndef #{header_guard}"
		hfile.puts "#define #{header_guard}"
		hfile.puts
		
		for line in $defs
		  line =~ /^-D(.*)/
		  hfile.printf "#define %s 1\n", $1
		end
		
		hfile.puts
		hfile.puts "#endif"
  end
end

if RUBY_VERSION < '1.9'
  raise(NotImplementedError, "Sorry, you need Ruby 1.9!")
else
  $INSTALLFILES = [['nmatrix.h', '$(archdir)'], ['nmatrix.hpp', '$(archdir)'], ['nmatrix_config.h', '$(archdir)']]
  if /cygwin|mingw/ =~ RUBY_PLATFORM
	 $INSTALLFILES << ['libnmatrix.a', '$(archdir)']
  end
end

if /cygwin|mingw/ =~ RUBY_PLATFORM
  CONFIG["DLDFLAGS"] << " --output-lib libnmatrix.a"
end

$DEBUG = true
$CFLAGS = ["-Wall ",$CFLAGS].join(" ")

$srcs = [
	'nmatrix.cpp',
	'ruby_constants.cpp',

	'data/data.cpp',
	'util/math.cpp',
  'util/sl_list.cpp',
  'util/io.cpp',
  'storage/common.cpp',
	'storage/storage.cpp',
	'storage/dense.cpp',
  'storage/yale.cpp',
  'storage/list.cpp'
]
# add smmp in to get generic transp; remove smmp2 to eliminate funcptr transp

# The next line allows the user to supply --with-atlas-include=/usr/local/atlas, for example,
# and tell the compiler where to look for ATLAS.
dir_config("atlas")

# Is g++ having trouble finding your header files?
# Try this:
#   export C_INCLUDE_PATH=/usr/local/atlas/include
#   export CPLUS_INCLUDE_PATH=/usr/local/atlas/include
# (substituting in the path of your cblas.h and clapack.h for the path I used). -- JW 8/27/12

find_library("lapack", "clapack_dgetrf", "/usr/local/lib", "/usr/local/atlas/lib")
have_header("clapack.h")

find_library("cblas", "cblas_dgemm", "/usr/local/lib", "/usr/local/atlas/lib")
find_library("atlas", "ATL_dgemmNN", "/usr/local/lib", "/usr/local/atlas/lib", "/usr/lib")
have_header("cblas.h")

# Order matters here: ATLAS has to go after LAPACK: http://mail.scipy.org/pipermail/scipy-user/2007-January/010717.html
$libs += " -llapack -lcblas -latlas "

$objs = %w{nmatrix ruby_constants data/data util/io util/math util/sl_list storage/common storage/storage storage/dense storage/yale storage/list}.map { |i| i + ".o" }

#CONFIG['CXX'] = 'clang++'

if CONFIG['CXX'] == 'clang++'
	$CPP_STANDARD = 'c++11'

else
	version = `g++ -v 2>&1`.lines.to_a.last.match(/gcc\sversion\s(\d\.\d.\d)/).captures.first
	
	if version < '4.7.0'
		$CPP_STANDARD = 'c++0x'
	else
		$CPP_STANDARD = 'c++11'
	end
end

$CFLAGS += " -O0 "
$CPPFLAGS += " -O0 -std=#{$CPP_STANDARD} " #-fmax-errors=10 -save-temps

CONFIG['warnflags'].gsub!('-Wdeclaration-after-statement', '')
CONFIG['warnflags'].gsub!('-Wimplicit-function-declaration', '')

create_conf_h("nmatrix_config.h")
create_makefile("nmatrix")

Dir.mkdir("data") unless Dir.exists?("data")
Dir.mkdir("util") unless Dir.exists?("util")
Dir.mkdir("storage") unless Dir.exists?("storage")

# to clean up object files in subdirectories:
open('Makefile', 'a') do |f|
  f.write <<EOS
CLEANOBJS := $(CLEANOBJS) data/*.#{CONFIG["OBJEXT"]} storage/*.#{CONFIG["OBJEXT"]} util/*.#{CONFIG["OBJEXT"]}
EOS
end
