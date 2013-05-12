package ee.ut.cs.mc.natpeer.util;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

import org.json.JSONObject;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.util.Log;

/**
 * Common routines used across the application.
 * 
 * @author Kristjan Reinloo
 * 
 */
public final class Common {

    public static void displayMessage(Context context, String message) {
        Intent intent = new Intent(Consts.MESSAGE_SENT_ACTION);
        intent.putExtra(Consts.MESSAGE_EXTRA, message);
        context.sendBroadcast(intent);
    }

    /**
     * Saves a string based key-value pair in the application settings.
     * 
     * @param context
     * @param key
     * @param value
     */
    public static void saveSetting(Context context, String key, String value) {
        SharedPreferences settings = context.getSharedPreferences(
            Consts.SETTINGS_FILE, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putString(key, value);
        editor.commit();
        Log.d(Consts.TAG, "Saved a setting: " + key + " -> " + value);
    }

    /**
     * Searches from its saved settings for a key-value pair where the key is
     * the given parameter.
     * 
     * @param context
     * @param key
     *            - search argument
     * @return - value which was associated with the given key
     */
    public static String loadSetting(Context context, String key) {
        SharedPreferences settings = context.getSharedPreferences(
            Consts.SETTINGS_FILE, 0);
        String value = settings.getString(key, "");
        Log.d(Consts.TAG, "Loaded a setting: " + key + " -> " + value);
        return value;
    }

    /**
     * Sends out a broadcast with an optional message attached.
     * 
     * @param context
     * @param json
     *            - JSON object which will be stringified and passed with the
     *            intent
     */
    public static void fireEvent(Context context, JSONObject json) {
        Intent intent = new Intent(Consts.MESSAGE_SENT_ACTION);
        intent.putExtra(Consts.MESSAGE_EXTRA, json.toString());
        context.sendBroadcast(intent);
    }

    /**
     * Returns first address in string format of given interface
     * 
     * @param interFace
     *            - interface whose address will be searched
     * @return - address of given interface, or null if not found
     */
    public static String getInterfaceAddress(String interFace) {
        try {
            Enumeration<InetAddress> e = NetworkInterface.getByName(
                Consts.INTERFACE).getInetAddresses();
            InetAddress ia = null;
            while (e.hasMoreElements()) {
                ia = e.nextElement();
            }
            return ia.getHostAddress();
        } catch (SocketException e) {
            e.printStackTrace();
        }
        return null;
    }
}
