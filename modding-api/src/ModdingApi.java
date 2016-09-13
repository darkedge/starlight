/**
 * Created by Marco on 2016-09-08.
 */
public class ModdingApi {
    // Java keeps its own copy of the shared library, so its state is invalid
    // until we copy our state to it
    static {
        System.loadLibrary("starlight_temp");
    }

    // State synchronization
    public static native void copyState(long ptr);

    public static String start(long ptr) {
        return "This is a Java string";
    }

    // Called once per frame
    public static void setContext(long ptr) {
        copyState(ptr);
    }
}
