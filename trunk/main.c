/* 
Copyright 2008, David Allan

This file is part of raopd.

raopd is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your
option) any later version.

raopd is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with raopd.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "lt.h"
#include "utility.h"
#include "syscalls.h"
#include "client.h"
#include "rtsp_client.h"
#include "raop_play_send_audio.h"

#define DEFAULT_FACILITY LT_MAIN


int main(void)
{
	struct rtsp_session session;

	/* We can't output anything to the user until the lt framework
	 * is initialized, so do that before anything else. */
	lt_init();
	FUNC_ENTER;

	NOTC("raopd starting\n");

	test_audio();

	if (UTILITY_SUCCESS == rtsp_start_client(&session)) {
		rtsp_send_data(&session);
	}

	NOTC("raopd exiting\n");

	FUNC_RETURN;
	return 0;
}

