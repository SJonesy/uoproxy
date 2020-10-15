rm -rf ./output/
meson . output
ninja -C output
cp output/uoproxy .
