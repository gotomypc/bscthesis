package ee.ut.cs.mc.natpeer.util;

/**
 * Constants used across the application.
 * 
 * @author Kristjan Reinloo
 * 
 */
public final class Consts {

    public static final String TAG = "NATPeerAndroid";

    public static final String SERVER_IP = "";

    public static final int SERVER_PORT = 8000;

    public static final int SERVER_TCP_PORT = 8001;

    public static final String SERVER_API = "http://" + SERVER_IP + ":"
        + SERVER_PORT + "/api";

    public static final String SENDER_ID = "";

    public static final String MESSAGE_SENT_ACTION
        = "ee.ut.cs.mc.natpeer.MESSAGE_SENT_ACTION";

    public static final String MESSAGE_EXTRA = "MESSAGE_EXTRA";

    public static final String MESSAGE = "message";

    public static final String GCM_ID = "gcm";

    public static final int HTTP_OK = 200;

    public static final int HTTP_CREATED = 201;

    public static final String SETTINGS_FILE = "NATPeerAndroid";

    public static final String DEVICE_ID = "_id";

    public static final String GCM_MESSAGE_BODY = "GCM_MESSAGE_BODY";

    public static final String EVENT_TYPE = "EVENT_TYPE";

    public static final String EVENT_GCM_REGISTERED = "EVENT_GCM_REGISTERED";

    public static final String EVENT_GCM_UNREGISTERED
        = "EVENT_GCM_UNREGISTERED";

    public static final String EVENT_GCM_MESSAGE_RECEIVED
        = "EVENT_GCM_MESSAGE_RECEIVED";

    public static final String GCM_EVENT = "gcm_event";

    public static final String GCM_EVENT_SERVICE_REQUEST = "request";

    public static final String GCM_EVENT_CONNECTION_INFO = "connection_info";

    public static final String GCM_SERVICE_REQUEST_ID = "id";

    public static final String GCM_SERVICE_REQUEST_NAME = "service";

    public static final String SERVICE_ID = "_id";

    public static final String INTERFACE = "wlan0";

    public static final String NAT_STATUS = "NAT_STATUS";

}
