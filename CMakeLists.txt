## CMakeLists.txt --- 
## 
######################################################################

## Commentary: 
## 
## 
## 
######################################################################

## Change Log:
## 
## 
######################################################################
## 
## *This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 3, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; see the file COPYING.  If not, write to
## the Free Software Foundation, Inc., 51 Franklin Street, Fifth
## Floor, Boston, MA 02110-1301, USA.
## 
######################################################################

## Code:
cmake_minimum_required(VERSION 2.6)
project(ale_modulator)

set(MOD_VERSION_MAJOR 0)
set(MOD_VERSION_MINOR 1)
set(CMAKE_CXX_FLAGS "-Wall -O2")
#set(CMAKE_CC_FLAGS "-g -fmudflap -fmudflapth -funwind-tables")
set(CMAKE_CC_FLAGS "-Wall -O2")
#set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_INSTALL_PREFIX "/usr")

add_subdirectory(src)

# install (FILES "${CMAKE_CURRENT_SOURCE_DIR}/etc/modem_backend.conf" DESTINATION /etc)

######################################################################
