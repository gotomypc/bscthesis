package ee.ut.cs.mc.natpeer.externalservice;

/**
 * Describes an external service and its attributes.
 * 
 * @author Kristjan Reinloo
 * 
 */
public class ExternalService {

    private final String mName;
    private final int mLocalPort;
    private String mID;

    public ExternalService(String name, int port) {
        mName = name;
        mLocalPort = port;
    }

    public String getName() {
        return mName;
    }

    public int getPort() {
        return mLocalPort;
    }

    public void setID(String id) {
        mID = id;
    }

    public String getID() {
        return mID;
    }

}
