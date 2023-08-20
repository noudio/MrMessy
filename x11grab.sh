#!/bin/bash
set -x

# anullsrc is also a source filter so it too needs -f lavfi before it.

#sleep 5
rm -rf x.mp4

# grab with empty audio
#ffmpeg \
#       -f lavfi -t 5 -i anullsrc=channel_layout=stereo:sample_rate=44100 \
#       -f x11grab -framerate 30 -i :1 -t 10 \
#       x.mp4
#

# grab with pulse audio:: ffmpeg -f pulse -ac 2 -i default   ...output.mkv
# grab with alsa  audio:: ffmpeg -f alsa -ac 2 -i hw:0       ...output.mkv
# grab with jack audio
# the jack writable client, is just created by mpeg, but it must be automatically connected with jacctl's patchbay

# -c:v h264_nvenc
#      unfortunetel nvidia gt1030 doesnt have hw encoding

#ffmpeg \
#       -f x11grab -framerate 30 -i :1 -t 20 \
#       -f jack -i ffmpeg-in -t 20 \
#       -shortest -pix_fmt yuv420p  x.mp4

# 1920,1080 -> 1280,720 -> 960,540

ffmpeg \
       -f x11grab -framerate 30 -i :1 -t 120 \
       -f jack -i ffmpeg-in \
       -shortest \
       -c:v libx264 -crf 10 -preset ultrafast -s 960x540 -pix_fmt yuv420p -b:v 1M \
       x.mp4

#ffmpeg -i INPUT -c:a copy -c:v libx264 -crf 10 -preset ultrafast -s 1280x720 -pix_fmt yuv420p -map 0 OUTPUT
