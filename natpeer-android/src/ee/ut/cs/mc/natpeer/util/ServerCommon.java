package ee.ut.cs.mc.natpeer.util;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;

import org.json.JSONArray;
import org.json.JSONObject;

import android.util.Log;

import com.github.kevinsawicki.http.HttpRequest;

import ee.ut.cs.mc.natpeer.exception.NATPeerAndroidException;
import ee.ut.cs.mc.natpeer.externalservice.ExternalService;

/**
 * Common routines which handle communication with a remote server.
 * 
 * @author Kristjan Reinloo
 * 
 */
public class ServerCommon {

    /**
     * Registers device with a remote server.
     * 
     * @param gcmID
     *            - GCM id
     * @return unique device ID after successful registration, null otherwise
     */
    public static String registerDevice(String gcmID) {
        HttpRequest request = HttpRequest.post(Consts.SERVER_API + "/devices")
            .send(Consts.GCM_ID + "=" + gcmID);
        if (request.code() == Consts.HTTP_CREATED) {
            try {
                JSONObject response = new JSONObject(request.body());
                String deviceID = response.getString(Consts.DEVICE_ID);
                if (deviceID.equals(""))
                    throw new NATPeerAndroidException("No Device ID received");
                Log.d(Consts.TAG, "ServerUtils: Device registered at "
                    + "the remote server");
                return deviceID;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        Log.d(Consts.TAG,
            "ServerUtils: Failed to register device at the remote server");
        return null;

    }

    /**
     * Unregisters device with a remote server.
     * 
     * @param deviceID
     *            - ID of the device which was obtained from the remote server
     * @return true if unregistration at the remote server was successful, false
     *         otherwise
     */
    public static boolean unregisterDevice(String deviceID) {
        HttpRequest request = HttpRequest.delete(Consts.SERVER_API
            + "/devices/" + deviceID);
        if (request.code() == Consts.HTTP_OK) {
            Log.d(Consts.TAG, "ServerUtils: Device unregistered at "
                + "the remote server");
            return true;
        } else {
            Log.d(Consts.TAG, "ServerUtils: Failed to unregister device "
                + "at the remote server");
            return false;
        }
    }

    /**
     * Registers new service with a remote server.
     * 
     * @param service
     *            - service to be registered
     * @param deviceID
     *            - ID of the device
     * @return serviceID if registration was successful, null otherwise
     */
    public static String registerService(ExternalService service,
        String deviceID) {
        try {
            HttpRequest request = HttpRequest.post(
                Consts.SERVER_API + "/services").send(
                "name=" + service.getName() + "&device=" + deviceID);
            if (request.code() == Consts.HTTP_CREATED) {
                JSONObject json = new JSONObject(request.body());
                String serviceID = json.getString(Consts.SERVICE_ID);
                Log.d(Consts.TAG, "ServerCommon: Registered new service with "
                    + "the remote server.");
                return serviceID;
            } else {
                Log.d(Consts.TAG, request.code() + "");
                Log.d(Consts.TAG, request.body());
                throw new NATPeerAndroidException(
                    "Unable to register a service");
            }
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Unregisters an existing server with a remote server.
     * 
     * @param service
     *            - service to be unregistered
     * @return true if service was unregistered was successful, false otherwise
     */
    public static boolean unregisterService(ExternalService service) {
        try {
            HttpRequest req = HttpRequest.delete(Consts.SERVER_API
                + "/services/" + service.getID());
            Log.d(Consts.TAG, "ServerCommon: " + req.toString());
            if (req.code() == Consts.HTTP_OK) {
                return true;
            } else {
                throw new NATPeerAndroidException("Unexpected response: "
                    + req.body());
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    /**
     * Responds to a service request.
     * 
     * @param requestID
     *            - ID of the request which was made
     * @param natStatus
     *            - boolean indicating whether this device is behind a NAT
     *            router or not
     * @return - stringified JSON array which contains information about the
     *         remote peer who wants to access given host
     */
    public static String respondToRequest(String requestID, boolean natStatus) {
        try {
            Socket s = new Socket();
            s.setReuseAddress(true);
            s.connect(new InetSocketAddress(Consts.SERVER_IP,
                Consts.SERVER_TCP_PORT));

            DataInputStream dis = new DataInputStream(s.getInputStream());
            DataOutputStream dos = new DataOutputStream(s.getOutputStream());
            JSONObject json = new JSONObject();
            json.put("event", "response");
            json.put("id", requestID);
            if (natStatus) {
                json.put("nat", true);
            }
            dos.writeBytes(json.toString());
            dos.flush();

            byte[] buffer = new byte[2048];
            int len = dis.read(buffer);
            String response = new String(buffer, 0, len);
            JSONArray array = new JSONArray(response);
            len = dis.read(buffer);
            response = new String(buffer, 0, len);
            JSONObject jo = new JSONObject(response);
            array.put(jo);
            Log.d(Consts.TAG, "response: " + array.toString(2));
            s.close();
            return array.toString();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

}
