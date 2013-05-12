package ee.ut.cs.mc.natpeer.service;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import com.google.android.gcm.GCMRegistrar;

import ee.ut.cs.mc.natpeer.externalservice.ExternalService;
import ee.ut.cs.mc.natpeer.externalservice.ExternalServiceManager;
import ee.ut.cs.mc.natpeer.jni.NativeLibrary;
import ee.ut.cs.mc.natpeer.util.Common;
import ee.ut.cs.mc.natpeer.util.Consts;
import ee.ut.cs.mc.natpeer.util.ServerCommon;

/**
 * 
 * Service which is responsible for most of the application logic.
 * 
 * @author Kristjan Reinloo
 * 
 */
public class NATPeerAndroidService extends Service {

    private final IBinder mBinder = new LocalBinder();

    private final ExternalServiceManager mManager = new ExternalServiceManager(
        this);

    private AsyncTask<JSONObject, Void, Void> mTask;

    private boolean mIsRegistered;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            try {
                String msg = intent.getExtras().getString(Consts.MESSAGE_EXTRA);
                JSONObject json = new JSONObject(msg);
                String event = json.getString(Consts.EVENT_TYPE);
                Log.i(Consts.TAG, "EVENT: " + event);
                if (event.equals(Consts.EVENT_GCM_REGISTERED)) {
                    handleGCMRegistration(json);
                } else if (event.equals(Consts.EVENT_GCM_UNREGISTERED)) {
                    handleGCMUnregistration(json);
                } else if (event.equals(Consts.EVENT_GCM_MESSAGE_RECEIVED)) {
                    handleGCMMessageReceived(json);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    };

    @Override
    public void onCreate() {
        Log.i(Consts.TAG, "Service started");
        mIsRegistered = false;
        checkGCMStatus();
        registerReceiver(mReceiver,
            new IntentFilter(Consts.MESSAGE_SENT_ACTION));
    }

    @Override
    public void onDestroy() {
        unregisterReceiver(mReceiver);
        new AsyncTask<Void, Void, Void>() {

            @Override
            protected Void doInBackground(Void... params) {
                mManager.onDestroy();
                if (mTask != null)
                    mTask.cancel(true);
                Log.i(Consts.TAG, "Service stopped");
                return null;
            }
        }.execute();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    public class LocalBinder extends Binder {

        public NATPeerAndroidService getService() {
            return NATPeerAndroidService.this;
        }
    }

    public void checkGCMStatus() {
        GCMRegistrar.checkDevice(this);
        GCMRegistrar.checkManifest(this);
        final String id = GCMRegistrar.getRegistrationId(this);
        if (id.equals("")) {
            Log.i(Consts.TAG, "GCM: Trying to register device at GCM");
            GCMRegistrar.register(this, Consts.SENDER_ID);
        } else {
            if (GCMRegistrar.isRegisteredOnServer(this)) {
                Log.i(Consts.TAG,
                    "GCM: Device already registered at the remote server");
                mIsRegistered = true;
            } else {
                Log.i(Consts.TAG,
                    "GCM: Trying to register device at the remote server");
                final Context c = this;
                mTask = new AsyncTask<JSONObject, Void, Void>() {

                    @Override
                    protected Void doInBackground(JSONObject... jsonObjects) {
                        String deviceID = ServerCommon.registerDevice(id);
                        if (deviceID != null) {
                            GCMRegistrar.setRegisteredOnServer(c, true);
                            Common.saveSetting(c, Consts.DEVICE_ID, deviceID);
                            mIsRegistered = true;
                        } else
                            GCMRegistrar.unregister(c);
                        return null;
                    }

                    @Override
                    protected void onPostExecute(Void result) {
                        mTask = null;
                    }
                };
                mTask.execute(null, null, null);
            }
        }
    }

    public void unRegisterGCM() {
        Log.i(Consts.TAG, "GCM: Trying to unregister at GCM");
        GCMRegistrar.unregister(this);
    }

    private void handleGCMRegistration(JSONObject json) {
        final Context context = this;
        mTask = new AsyncTask<JSONObject, Void, Void>() {

            @Override
            protected Void doInBackground(JSONObject... jsonObjects) {
                try {
                    JSONObject json = jsonObjects[0];
                    String gcmID = json.getString(Consts.GCM_ID);
                    String deviceID = ServerCommon.registerDevice(gcmID);
                    if (deviceID != null) {
                        Common.saveSetting(context, Consts.DEVICE_ID, deviceID);
                        mIsRegistered = true;
                        GCMRegistrar.setRegisteredOnServer(context, true);
                        Log.i(Consts.TAG,
                            "GCM: Registered at GCM and remote servers");
                    }
                } catch (JSONException e) {
                    e.printStackTrace();
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                mTask = null;
            }
        };
        mTask.execute(json, null, null);
    }

    private void handleGCMUnregistration(JSONObject json) {
        final Context context = this;
        mTask = new AsyncTask<JSONObject, Void, Void>() {

            @Override
            protected Void doInBackground(JSONObject... jsonObjects) {
                String deviceID = Common.loadSetting(context, Consts.DEVICE_ID);
                if (GCMRegistrar.isRegisteredOnServer(context)) {
                    boolean result = ServerCommon.unregisterDevice(deviceID);
                    if (result) {
                        Log.i(Consts.TAG,
                            "GCM: Unregistered at GCM and remote servers");
                    }
                    GCMRegistrar.setRegisteredOnServer(context, false);
                    mIsRegistered = true;
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                mTask = null;
            }
        };
        mTask.execute(json, null, null);
    }

    public void addServiceButtonHandler(String serviceName, int localPort) {
        new AsyncTask<String[], Void, Void>() {

            @Override
            protected Void doInBackground(String[]... params) {
                if (mIsRegistered) {
                    mManager.createService(params[0][0],
                        Integer.parseInt(params[0][1]));
                }
                return null;
            }
        }.execute(new String[] { serviceName, Integer.toString(localPort) });
    }

    public void removeServiceButtonHandler(String serviceName) {
        if (!mIsRegistered)
            return;
        new AsyncTask<String, Void, Void>() {

            @Override
            protected Void doInBackground(String... params) {
                String serviceName = params[0];
                ExternalService service = mManager
                    .findServiceByName(serviceName);
                if (service == null)
                    return null;
                mManager.removeService(service);
                return null;
            }
        }.execute(serviceName);
    }

    public void natSettingsButtonHandler(boolean value) {
        Log.d(Consts.TAG, "NAT ? " + value);
        final Context c = this;
        Common.saveSetting(c, Consts.NAT_STATUS, Boolean.toString(value));
    }

    private void establishConnection(JSONObject msg) throws JSONException {
        final Context c = this;
        String natSetting = Common.loadSetting(c, Consts.NAT_STATUS);
        boolean natEnabled;
        if (natSetting.equals("")) {
            Log.d(Consts.TAG, "NAT status has not been configured");
            Log.d(Consts.TAG, "Assuming device IS behind NAT");
            natEnabled = true;
        } else {
            natEnabled = Boolean.parseBoolean(natSetting);
        }
        String requestID = msg.getString(Consts.GCM_SERVICE_REQUEST_ID);
        String serviceName = msg.getString(Consts.GCM_SERVICE_REQUEST_NAME);
        String s = ServerCommon.respondToRequest(requestID, natEnabled);
        JSONArray array = new JSONArray(s);
        JSONObject jo1 = array.getJSONObject(0);
        JSONObject jo2 = array.getJSONObject(1);
        Log.d(Consts.TAG, "response json: " + jo1.toString());
        String addr = jo1.getString("peer_ip");
        int port = jo1.getInt("peer_port");
        long seqNumb = jo2.getLong("isn");
        long timeStamp = jo2.getLong("ts_val");

        String localAddr = Common.getInterfaceAddress(Consts.INTERFACE);
        Log.d(Consts.TAG, "Local address: " + localAddr);
        ExternalService service = mManager.findServiceByName(serviceName);
        if (service == null) {
            Log.d(Consts.TAG, "No service found");
            return;
        }
        NativeLibrary.injectFrom(localAddr, service.getPort(),
            Consts.INTERFACE, addr, port, seqNumb, timeStamp, natEnabled);
    }

    private void handleGCMMessageReceived(JSONObject json) {
        new AsyncTask<JSONObject, Void, Void>() {

            @Override
            protected Void doInBackground(JSONObject... objects) {
                try {
                    JSONObject json1 = objects[0];
                    JSONObject msg = new JSONObject(
                        json1.getString(Consts.GCM_MESSAGE_BODY));
                    String event = msg.getString(Consts.GCM_EVENT);
                    Log.d(Consts.TAG, msg.toString());

                    if (event.equals(Consts.GCM_EVENT_SERVICE_REQUEST)) {
                        establishConnection(msg);
                    }

                } catch (Exception e) {
                    e.printStackTrace();
                }
                return null;
            }
        }.execute(json);
    }
}
