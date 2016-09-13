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
    public native void copyState(long ptr);
    public native void print();

    public static String start(long ptr) {
        ModdingApi api = new ModdingApi();
        // We need to copy the state over to our own shared library
        api.copyState(ptr);

        // Testing
        api.print();
        return "This is a Java string";
    }
}
