# MacでUniversal Binaryをビルドする方法

## pkg-config が無ければまずはインストール
```
brew install pkg-config
```

## x264 のビルド部分
```
git clone https://code.videolan.org/videolan/x264.git
cd x264

./configure --enable-shared --enable-static --prefix=/Users/udon/Desktop/x264/binary/arm64 --enable-strip --disable-asm --host=aarch64-apple-darwin
make
make install

./configure --enable-shared --enable-static --prefix=/Users/udon/Desktop/x264/binary/x86_64 --enable-strip --disable-asm --host=x86_64-apple-darwin
make
make install
```

## lipo -create -output ./binary/libx264.dylib ./binary/x86_64/libx264.dylib ./binary/arm64/libx264.dylib ←ユニバーサルバイナリにするやつだけどいらんかったかも

## ffmpeg のインストール部分
ffmpeg のソースフォルダに移動
```
export CFLAGS="-arch arm64"
export LDFLAGS="-arch arm64"
export PKG_CONFIG_PATH="/Users/udon/Desktop/x264/binary/arm64/lib/pkgconfig" ← さっき作ったx264の.pcファイルの場所を指定
./configure --prefix=/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64 --enable-pic --enable-shared --enable-static --disable-doc --enable-libx264 --enable-gpl --enable-rpath --arch=aarch64
make
make install
```

```
make clean
```

```
export CFLAGS="-arch x86_64"
export LDFLAGS="-arch x86_64"
export PKG_CONFIG_PATH="/Users/udon/Desktop/x264/binary/x86_64/lib/pkgconfig" ← さっき作ったx264の.pcファイルの場所を指定
./configure --prefix=/Users/udon/Desktop/ffmpeg-7.0.2/binary/x86_64 --enable-pic --enable-shared --enable-static --disable-doc --enable-libx264 --enable-gpl --enable-rpath --arch=x86_64
make
make install
````

## 各dylibが参照するパスの確認
```
otool -L libavcodec.dylib
```

## 自身へのパスの書き換え
```
install_name_tool -id "@rpath/libavcodec.61.dylib" libavcodec.dylib
install_name_tool -id "@rpath/libx264.164.dylib" libx264.dylib
```

## 他へのパスの書き換え
```
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libavcodec.dylib
```

--------------以下パス書き換えのコピペ用 (x86_64 なら arm64 を x86_64 に置換) -----------------
```
install_name_tool -id "@rpath/libavcodec.61.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavfilter.10.dylib" "@rpath/libavfilter.10.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswscale.8.dylib" "@rpath/libswscale.8.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libpostproc.58.dylib" "@rpath/libpostproc.58.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavformat.61.dylib" "@rpath/libavformat.61.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavcodec.61.dylib" "@rpath/libavcodec.61.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libavcodec.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libavcodec.dylib

install_name_tool -id "@rpath/libavdevice.61.dylib" libavdevice.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavfilter.10.dylib" "@rpath/libavfilter.10.dylib" libavdevice.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswscale.8.dylib" "@rpath/libswscale.8.dylib" libavdevice.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libpostproc.58.dylib" "@rpath/libpostproc.58.dylib" libavdevice.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavformat.61.dylib" "@rpath/libavformat.61.dylib" libavdevice.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavcodec.61.dylib" "@rpath/libavcodec.61.dylib" libavdevice.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libavdevice.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libavdevice.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libavdevice.dylib

install_name_tool -id "@rpath/libavfilter.10.dylib" libavfilter.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavfilter.10.dylib" "@rpath/libavfilter.10.dylib" libavfilter.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswscale.8.dylib" "@rpath/libswscale.8.dylib" libavfilter.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libpostproc.58.dylib" "@rpath/libpostproc.58.dylib" libavfilter.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavformat.61.dylib" "@rpath/libavformat.61.dylib" libavfilter.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavcodec.61.dylib" "@rpath/libavcodec.61.dylib" libavfilter.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libavfilter.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libavfilter.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libavfilter.dylib

install_name_tool -id "@rpath/libavformat.61.dylib" libavformat.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavfilter.10.dylib" "@rpath/libavfilter.10.dylib" libavformat.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswscale.8.dylib" "@rpath/libswscale.8.dylib" libavformat.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libpostproc.58.dylib" "@rpath/libpostproc.58.dylib" libavformat.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavformat.61.dylib" "@rpath/libavformat.61.dylib" libavformat.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavcodec.61.dylib" "@rpath/libavcodec.61.dylib" libavformat.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libavformat.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libavformat.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libavformat.dylib

install_name_tool -id "@rpath/libavutil.59.dylib" libavutil.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavfilter.10.dylib" "@rpath/libavfilter.10.dylib" libavutil.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswscale.8.dylib" "@rpath/libswscale.8.dylib" libavutil.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libpostproc.58.dylib" "@rpath/libpostproc.58.dylib" libavutil.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavformat.61.dylib" "@rpath/libavformat.61.dylib" libavutil.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavcodec.61.dylib" "@rpath/libavcodec.61.dylib" libavutil.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libavutil.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libavutil.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libavutil.dylib

install_name_tool -id "@rpath/libpostproc.58.dylib" libpostproc.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavfilter.10.dylib" "@rpath/libavfilter.10.dylib" libpostproc.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswscale.8.dylib" "@rpath/libswscale.8.dylib" libpostproc.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libpostproc.58.dylib" "@rpath/libpostproc.58.dylib" libpostproc.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavformat.61.dylib" "@rpath/libavformat.61.dylib" libpostproc.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavcodec.61.dylib" "@rpath/libavcodec.61.dylib" libpostproc.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libpostproc.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libpostproc.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libpostproc.dylib

install_name_tool -id "@rpath/libswresample.5.dylib" libswresample.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavfilter.10.dylib" "@rpath/libavfilter.10.dylib" libswresample.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswscale.8.dylib" "@rpath/libswscale.8.dylib" libswresample.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libpostproc.58.dylib" "@rpath/libpostproc.58.dylib" libswresample.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavformat.61.dylib" "@rpath/libavformat.61.dylib" libswresample.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavcodec.61.dylib" "@rpath/libavcodec.61.dylib" libswresample.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libswresample.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libswresample.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libswresample.dylib

install_name_tool -id "@rpath/libswscale.8.dylib" libswscale.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavfilter.10.dylib" "@rpath/libavfilter.10.dylib" libswscale.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswscale.8.dylib" "@rpath/libswscale.8.dylib" libswscale.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libpostproc.58.dylib" "@rpath/libpostproc.58.dylib" libswscale.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavformat.61.dylib" "@rpath/libavformat.61.dylib" libswscale.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavcodec.61.dylib" "@rpath/libavcodec.61.dylib" libswscale.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libswresample.5.dylib" "@rpath/libswresample.5.dylib" libswscale.dylib
install_name_tool -change "/Users/udon/Desktop/ffmpeg-7.0.2/binary/arm64/lib/libavutil.59.dylib" "@rpath/libavutil.59.dylib" libswscale.dylib
install_name_tool -change "/Users/udon/Desktop/x264/binary/arm64/lib/libx264.164.dylib" "@rpath/libx264.164.dylib" libswscale.dylib
```

## Universal Binary にする
```
lipo -create -output libavcodec.61.dylib x86_64/libavcodec.dylib arm64/libavcodec.dylib
lipo -create -output libavdevice.61.dylib x86_64/libavdevice.dylib arm64/libavdevice.dylib
lipo -create -output libavfilter.10.dylib x86_64/libavfilter.dylib arm64/libavfilter.dylib
lipo -create -output libavformat.61.dylib x86_64/libavformat.dylib arm64/libavformat.dylib
lipo -create -output libavutil.59.dylib x86_64/libavutil.dylib arm64/libavutil.dylib
lipo -create -output libpostproc.58.dylib x86_64/libpostproc.dylib arm64/libpostproc.dylib
lipo -create -output libswresample.5.dylib x86_64/libswresample.dylib arm64/libswresample.dylib
lipo -create -output libswscale.8.dylib x86_64/libswscale.dylib arm64/libswscale.dylib
lipo -create -output libx264.164.dylib x86_64/libx264.dylib arm64/libx264.dylib
```
