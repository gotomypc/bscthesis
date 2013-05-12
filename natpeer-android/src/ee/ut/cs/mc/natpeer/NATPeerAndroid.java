package ee.ut.cs.mc.natpeer;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Switch;
import ee.ut.cs.mc.natpeer.service.NATPeerAndroidService;
import ee.ut.cs.mc.natpeer.service.NATPeerAndroidService.LocalBinder;

/**
 * 
 * Main activity which is launched when the application starts.
 * 
 * @author Kristjan Reinloo
 * 
 */
public class NATPeerAndroid extends Activity {

    private Switch mSwitch;
    private NATPeerAndroidService mService;
    private boolean mBound;

    private final ServiceConnection mConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            LocalBinder binder = (LocalBinder) service;
            mService = binder.getService();
            mBound = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mBound = false;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSwitch = (Switch) findViewById(R.id.btn_switch);
        mSwitch
            .setOnCheckedChangeListener(new CompoundButton.
                OnCheckedChangeListener() {

                @Override
                public void onCheckedChanged(CompoundButton buttonView,
                    boolean isChecked) {
                    if (mSwitch.isChecked()) {
                        Intent intent = new Intent(NATPeerAndroid.this,
                            NATPeerAndroidService.class);
                        bindService(intent, mConnection,
                            Context.BIND_AUTO_CREATE);
                    } else {
                        if (mBound) {
                            unbindService(mConnection);
                            mBound = false;
                        }
                        stopService(new Intent(NATPeerAndroid.this,
                            NATPeerAndroidService.class));
                    }
                }
            });
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.settings, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {

        case R.id.option_add_service:
            showAddServiceDialog();
            return true;

        case R.id.option_delete_service:
            showDeleteServiceDialog();
            return true;

        case R.id.option_check_gcm_status_register:
            if (mBound)
                mService.checkGCMStatus();
            return true;

        case R.id.option_unregister:
            if (mBound)
                mService.unRegisterGCM();
            return true;

        case R.id.option_nat_settings:
            if (mBound)
                showSettingsDialog();
            return true;

        case R.id.option_exit:
            finish();
            return true;

        default:
            return super.onOptionsItemSelected(item);

        }
    }

    private void showAddServiceDialog() {
        final View view = getLayoutInflater().inflate(
            R.layout.dialog_add_service, null);
        new AlertDialog.Builder(NATPeerAndroid.this)
            .setView(view)
            .setMessage("Add new service")
            .setPositiveButton("Add", new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    EditText localPortText = (EditText) view
                        .findViewById(R.id.dialog_add_service_port_local);
                    EditText serviceNameText = (EditText) view
                        .findViewById(R.id.dialog_add_service_name);
                    int localPort = Integer.parseInt(localPortText.getText()
                        .toString());
                    String serviceName = serviceNameText.getText().toString();
                    if (mBound)
                        mService
                            .addServiceButtonHandler(serviceName, localPort);
                }
            })
            .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                }
            }).show();
    }

    private void showDeleteServiceDialog() {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle("Remove service");
        final EditText input = new EditText(this);
        input.setHint("service name");
        alert.setView(input);
        alert.setPositiveButton("Remove",
            new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    String serviceName = input.getText().toString();
                    if (mBound)
                        mService.removeServiceButtonHandler(serviceName);
                }
            });
        alert.setNegativeButton("Cancel",
            new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                }
            });
        alert.show();
    }

    private void showSettingsDialog() {
        final CharSequence[] items = { "Device behind NAT",
                "Device NOT behind NAT" };
        AlertDialog dialog;
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(R.string.option_nat_settings);
        alert.setSingleChoiceItems(items, -1,
            new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int item) {
                    switch (item) {
                    case 0:
                        mService.natSettingsButtonHandler(true);
                        break;
                    case 1:
                        mService.natSettingsButtonHandler(false);
                        break;
                    }
                    dialog.dismiss();
                }
            });
        dialog = alert.create();
        dialog.show();
    }
}
