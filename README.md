# Video-testing

## Dependencies
v4l2, gstreamer, raylib

## Building
```
make
```
## Testing
```
make test
```
The test defaults to input /dev/video0 and output build/testfil.mp4.
This can be changed in lib.mk/test.mk as needed.
