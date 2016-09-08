/**
 * Created by Marco on 2016-09-08.
 */
public class ModdingApi {
    static {
        System.loadLibrary("starlight");
    }

    public native void print();

    public static String getString() {
        new ModdingApi().print();
        return "Hello World!";
    }
}
