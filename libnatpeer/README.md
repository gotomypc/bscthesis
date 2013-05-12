# libnatpeer

Simple library which helps to establish a connection with a host behind a NAT
router. Uses the [SYN injection](http://goo.gl/2RfFX) method.

## Build

Dependencies:

* [Jansson](http://www.digip.org/jansson/) JSON library
* to build the executable for Android, make sure you have
  [Android NDK](https://developer.android.com/tools/sdk/ndk/index.html) and the
  `$NDK_ROOT` is in your `$PATH`.

Build it with clang compiler:
```
make
```

Or with gcc:
```
make CC=gcc
```

### Android executable

Clone the [Jansson](https://github.com/akheron/jansson) library to
`$NDK_ROOT/external/` folder and apply following patch:
```
diff --git a/Android.mk b/Android.mk
index eb4fed7..ffedfb1 100644
--- a/Android.mk
+++ b/Android.mk
@@ -26,4 +26,4 @@ LOCAL_CFLAGS += -O3
 
 LOCAL_MODULE:= libjansson
 
-include $(BUILD_SHARED_LIBRARY)
+include $(BUILD_STATIC_LIBRARY)
```

Build the Android executable
```
make android
```

## Run

Minimum set of arguments which have to be passed (assuming you have the
rendezvous server up and running, Android application installed and at least one
service registered):
```
./natpeer --establish -server <RENDEZVOUS_SERVER_IP> -service <SERVICE_NAME>
```
You need to run it with `root` privileges, since it uses raw sockets.

If the connection is successfully established, it tells you to connect to
`localhost:8002` to access the smartphone's service.
