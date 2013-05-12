package ee.ut.cs.mc.natpeer;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.google.android.gcm.GCMBaseIntentService;

import ee.ut.cs.mc.natpeer.util.Common;
import ee.ut.cs.mc.natpeer.util.Consts;

/**
 * 
 * Service for handling GCM incoming GCM messages. It has to be in the root
 * package, otherwise it will not work.
 * 
 * @author Kristjan Reinloo
 * 
 */
public class GCMIntentService extends GCMBaseIntentService {

    public GCMIntentService() {
        super(Consts.SENDER_ID);
    }

    @Override
    protected void onRegistered(Context context, String gcmID) {
        Log.i(Consts.TAG, "GCMRegistrar: Device successfully registered "
            + "at GCM (" + gcmID + ")");
        fireEvent(context, Consts.EVENT_GCM_REGISTERED, Consts.GCM_ID, gcmID);
    }

    @Override
    protected void onUnregistered(Context context, String gcmID) {
        Log.i(Consts.TAG, "GCMRegistrar: Device successfully "
            + "unregistered at GCM (" + gcmID + ")");
        fireEvent(context, Consts.EVENT_GCM_UNREGISTERED, Consts.GCM_ID, gcmID);
    }

    @Override
    protected void onMessage(Context context, Intent intent) {
        Log.i(Consts.TAG, "GCMRegistrar: Message received from GCM");
        String message = intent.getStringExtra("message");
        Log.d(Consts.TAG, "Message: " + message);
        fireEvent(context, Consts.EVENT_GCM_MESSAGE_RECEIVED,
            Consts.GCM_MESSAGE_BODY, message);
    }

    @Override
    protected void onDeletedMessages(Context context, int total) {
        Log.i(Consts.TAG, "GCMRegistrar: Message(s) deleted");
    }

    @Override
    protected void onError(Context context, String error) {
        Log.i(Consts.TAG, "GCMRegistrar: Error occurred");
    }

    @Override
    protected boolean onRecoverableError(Context context, String error) {
        Log.i(Consts.TAG, "GCMRegistrar: Recoverable error occurred");
        return super.onRecoverableError(context, error);
    }

    /**
     * Fires an event when device is either registered, unregistered or a
     * message is received.
     * 
     */
    private void fireEvent(Context context, String eventType, String eventKey,
        String eventValue) {
        Log.d(Consts.TAG, "GCMRegistrar: Firing event: " + eventType);
        JSONObject json = new JSONObject();
        try {
            json.put(Consts.EVENT_TYPE, eventType);
            json.put(eventKey, eventValue);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        Common.fireEvent(context, json);
    }

}
