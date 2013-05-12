package ee.ut.cs.mc.natpeer.jni;

/**
 * Class which handles communication with the native library.
 * 
 * @author Kristjan Reinloo
 * 
 */
public class NativeLibrary {

    public static native void injectFrom(String localAddr, int localPort,
        String interFace, String remoteAddr, int remotePort, long seqNumb,
        long timeStamp, boolean nat);

    static {
        System.loadLibrary("natpeer-android");
    }

}
