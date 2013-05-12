# natpeer-android

The Android application

## Build instructions

### Dependencies / Requirements

* [Android SDK](https://developer.android.com/sdk/index.html)
* [Android NDK](https://developer.android.com/tools/sdk/ndk/index.html)
* [Apache Ant](https://ant.apache.org/)
* [http-request 4.1](http://mvnrepository.com/artifact/com.github.kevinsawicki/http-request/4.1)
* [gcm.jar](https://developer.android.com/google/gcm/gs.html#libs)
* Rooted Android smartphone
* natpeer-android executable built for Android (see libnatpeer folder for
  instructions) and installed to `/system/bin/` folder

### Build

Create `local.properties` file and define two variables which point to
`$SDK_ROOT` and `$NDK_ROOT` respectively
```
sdk.dir=
ndk.dir=
```

Set up a rendezvous server and a
[GCM application](https://developer.android.com/google/gcm/gs.html). Define the
IP address of the rendezvous server and the GCM sender ID in following places
```
src/ee/ut/cs/mc/natpeer/util/Consts.java:13
src/ee/ut/cs/mc/natpeer/util/Consts.java:22
```

Create `libs/` folder and copy `http-request-4.1.jar` and `gcm.jar` into it.

Build the application
```
ant debug
```

And finally install it
```
cd bin/
adb install NATPeerAndroid-debug.apk
```
