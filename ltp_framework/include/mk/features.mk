#
#    features.mk.in - feature tuning include Makefile.
#
#    Copyright (C) 2010, Linux Test Project.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Garrett Cooper, October 2010
#

# Path to capset program
CAPSET				:= 

# Tools enable knobs
WITH_EXPECT			:= no

WITH_PERL			:= no

WITH_PYTHON			:= no

# Features knobs

# Is securebits[.h], et all support available?
HAVE_SECUREBITS			:= yes

# Test suite knobs

# Enable testcases/kernel/power_management's compile and install?
ifeq ($(UCLINUX),1)
WITH_POWER_MANAGEMENT_TESTSUITE	:= no
else
WITH_POWER_MANAGEMENT_TESTSUITE	:= no
endif

# Enable testcases/open_posix_testsuite's compile and install?
WITH_OPEN_POSIX_TESTSUITE	:= no

# Enable testcases/realtime's compile and install?
ifeq ($(UCLINUX),1)
WITH_REALTIME_TESTSUITE		:= no
else
WITH_REALTIME_TESTSUITE		:= no
endif
