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
#ifndef SDP_H
#define SDP_H

utility_retcode_t get_msg_body(struct rtsp_request *request);
utility_retcode_t add_announce_sdp_fields(struct rtsp_request *request);
utility_retcode_t build_msg_body(struct rtsp_request *request);
utility_retcode_t add_msg_body_blank_line(struct rtsp_request *request);

#endif /* #ifndef SDP_H */
