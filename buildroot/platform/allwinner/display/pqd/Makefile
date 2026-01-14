# Copyright (C) 2022  Allwinnertech
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

all: pqd

CC := ${CROSS_COMPILE}gcc

srcs := pqdaemon.c pq_core.c
srcs += utils/utils.c utils/debug.c utils/cmdqueue.c utils/helper.c
srcs += pqdata/pqdata.c
srcs += sockets/socket.c sockets/socket_client.c
srcs += hardwares/hardwares.c hardwares/de20x/de20x.c
srcs += hardwares/de20x/fcm/fcm.c hardwares/de20x/fcm/constants.c

objs = $(srcs:.c=.o)

CFLAGS := -std=gnu99 -Wall -Wno-unused-function
CFLAGS += -mfloat-abi=softfp -mfpu=vfpv3
CFLAGS += -I./ -Iutils -g

LDFLAGS := -lpthread -lm

pqd: ${objs}
	@echo "link ...... $@"
	@$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	@echo "compile ... $@"
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	@rm -rf ${objs} pqd
	@echo "clean ... finish"

