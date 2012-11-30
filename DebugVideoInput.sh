#!/bin/bash
clear
killall memcheck-amd64-
 LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libv4l/v4l2convert.so valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes  --track-origins=yes VideoInputTester/bin/Debug/VideoInputTester  2> VideoInputDebug.log&
exit 0
