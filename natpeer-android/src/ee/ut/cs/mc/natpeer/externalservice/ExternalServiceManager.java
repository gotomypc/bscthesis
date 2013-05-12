package ee.ut.cs.mc.natpeer.externalservice;

import java.util.LinkedHashSet;
import java.util.Set;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.Log;
import ee.ut.cs.mc.natpeer.util.Common;
import ee.ut.cs.mc.natpeer.util.Consts;
import ee.ut.cs.mc.natpeer.util.ServerCommon;

/**
 * Manages external (running outside of this application, but in given
 * smartphone) services.
 * 
 * @author Kristjan Reinloo
 * 
 */
public class ExternalServiceManager {

    private final Set<ExternalService> mServices;
    private final Context mContext;

    /**
     * Creates a new ExternalServiceManager instance.
     * 
     * @param context
     */
    @SuppressLint("Instantiatable")
    public ExternalServiceManager(Context context) {
        mServices = new LinkedHashSet<ExternalService>();
        mContext = context;
        Log.i(Consts.TAG, "ExternalServiceManager created");
    }

    /**
     * Unregisters all services at the remote server.
     * 
     */
    public void onDestroy() {
        for (ExternalService service : mServices) {
            boolean removed = ServerCommon.unregisterService(service);
            if (!removed)
                Log.d(Consts.TAG, "Unable to unregister " + service.getName()
                    + " service.");
        }
        Log.i(Consts.TAG, "ExternalServiceManager stopped, all services "
            + "unregistered");
    }

    /**
     * Unregisters an ExternalService service.
     * 
     * @param service
     *            - service to be unregistered
     */
    public void removeService(ExternalService service) {
        boolean response = ServerCommon.unregisterService(service);
        if (response) {
            mServices.remove(service);
            Log.i(Consts.TAG, "ExternalServiceManager: Removed service: "
                + service.getName());
        }
    }

    /**
     * Creates and registers a new ExternalService service at remote server.
     * 
     * @param name
     *            - name of the service
     * @param port
     *            - port number which the service uses
     */
    public void createService(String name, int port) {
        ExternalService service = new ExternalService(name, port);
        String deviceID = Common.loadSetting(mContext, Consts.DEVICE_ID);
        if (deviceID.equals(""))
            return;
        String serviceID = ServerCommon.registerService(service, deviceID);
        if (serviceID == null)
            return;
        service.setID(serviceID);
        mServices.add(service);
        Log.i(Consts.TAG,
            "ExternalServiceManager: Added service: " + service.getName()
                + " on local port " + service.getPort());
    }

    /**
     * Searches amongst its services for a service specified by the parameter.
     * 
     * @param name
     *            - name of the service
     * @return - service with given name, null otherwise
     */
    public ExternalService findServiceByName(String name) {
        for (ExternalService service : mServices) {
            if (service.getName().equals(name))
                return service;
        }
        return null;
    }

}
