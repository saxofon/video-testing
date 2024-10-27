# some fun with linux graphics, video, gstreamer, libray, v4l2...


## drm-raylib

raylib that comes with fedora is configured for platform_desktop.
We need to clone raylib, configure it for platform_drm and possibly change card path :

```
[per@phlap2 raylib]$ git clone https://github.com/raysan5/raylib
[per@phlap2 raylib]$ git diff
diff --git a/src/platforms/rcore_drm.c b/src/platforms/rcore_drm.c
index eb8ef010..675b6593 100644
--- a/src/platforms/rcore_drm.c
+++ b/src/platforms/rcore_drm.c
@@ -723,6 +723,7 @@ int InitPlatform(void)
     CORE.Window.fullscreen = true;
     CORE.Window.flags |= FLAG_FULLSCREEN_MODE;
 
+#define DEFAULT_GRAPHIC_DEVICE_DRM "/dev/dri/card1"
 #if defined(DEFAULT_GRAPHIC_DEVICE_DRM)
     platform.fd = open(DEFAULT_GRAPHIC_DEVICE_DRM, O_RDWR);
 #else
[per@phlap2 raylib]$ mkdir build
[per@phlap2 raylib]$ cd build
[per@phlap2 raylib/build]$ cmake .. -DPLATFORM=DRM
[per@phlap2 raylib/build]$ make
...
```

To be useful for embedded systems, we should probably make this a parameter for init at runtime...
And also add support for multiple displays.
