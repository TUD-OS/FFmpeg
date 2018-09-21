# ./configure --enable-debug=3 --disable-ffmpeg --disable-ffprobe --disable-doc --prefix=~/local
make -j9

# build decoder frontend
gcc -O3 -I. hevc_decode.c -DHAVE_INLINE_ASM ./libavcodec/libavcodec.a -lavformat -lavdevice ./libavutil/libavutil.a $(pkg-config --libs --static libavcodec)  -o hevc_decode
