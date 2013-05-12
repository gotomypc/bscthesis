package ee.ut.cs.mc.natpeer.exception;

/**
 * 
 * Custom exception to be thrown only within given application.
 * 
 * @author Kristjan Reinloo
 * 
 */
public class NATPeerAndroidException extends Exception {

    private static final long serialVersionUID = 1L;

    public NATPeerAndroidException() {
        super();
    }

    public NATPeerAndroidException(String message) {
        super(message);
    }

}
