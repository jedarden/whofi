package com.stevenmhernandez.esp32csiserial;


import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

import java.lang.ref.WeakReference;
import java.util.Set;

public class ESP32CSISerial {

    private final Handler handler = new Handler();
    private CSIDataInterface listener;
    private BaseDataCollectorService csiRecorderService = new FileDataCollectorService();
    private String experimentName = null;
    private Activity activity;

    public void setup(CSIDataInterface listener, String experimentName) {
        this.listener = listener;
        this.experimentName = experimentName;
    }

    public void onCreate(Activity activity) {
        this.activity = activity;
        csiRecorderService.setup(activity);

        mHandler = new USBHandler(activity, csiRecorderService, experimentName, listener);

        handler.postDelayed(new Runnable() {
            public void run() {
                if (usbService != null) {
                    String setDataString = String.format("SETTIME: %d\n", (int) (System.currentTimeMillis() / 1000));
                    usbService.write(setDataString.getBytes());
                }
                handler.postDelayed(this, 120000 / 2);
            }
        }, 10000); //Every 120000 ms (2 minutes)
    }

    public void onResume(Activity activity) {
        setFilters();  // Start listening notifications from UsbService
        startService(UsbService.class, usbConnection, null); // Start UsbService(if it was not started before) and Bind it
    }

    public void onPause(Activity activity) {
        activity.unregisterReceiver(mUsbReceiver);
        activity.unbindService(usbConnection);
    }

    public double[] parseCsi(String csi, boolean returnPhases) {
        String[] strs = csi.trim().split(" ");
        try {
            int[] im_ints = new int[strs.length / 2];
            int[] real_ints = new int[strs.length / 2];
            double[] amplitudes = new double[strs.length / 2];
            double[] phases = new double[strs.length / 2];

            for (int i = 0; i < strs.length; i++) {
                if (i % 2 == 0) {
                    im_ints[(int) Math.floor(i / 2.0)] = Integer.parseInt(strs[i]);
                } else {
                    real_ints[(int) Math.floor(i / 2.0)] = Integer.parseInt(strs[i]);
                }
            }

            for (int i = 0; i < strs.length / 2; i++) {
                amplitudes[i] = Math.sqrt(im_ints[i] ^ 2 + real_ints[i] ^ 2);
                phases[i] = Math.atan2(im_ints[i], real_ints[i]);
            }

            if (returnPhases) {
                return phases;
            } else {
                return amplitudes;
            }
        } catch (Exception e) {
            Log.e("Parse CSI String", csi);
            Log.e("ERROR:", e.getLocalizedMessage());
        }

        return null;
    }

    /*
     * This handler will be passed to UsbService. Data received from serial port is displayed through this handler
     */
    private static class USBHandler extends Handler {
        private final WeakReference<Activity> mActivity;
        String experimentName;
        CSIDataInterface listener;
        BaseDataCollectorService dataService;
        public String buffer = "";

        public USBHandler(Activity activity, BaseDataCollectorService dataService, String experimentName, CSIDataInterface listener) {
            mActivity = new WeakReference<>(activity);
            this.experimentName = experimentName;
            this.dataService = dataService;
            this.listener = listener;
        }

        public String updateCsiString(Activity activity, String csi) {
            String deviceId = Settings.Secure.getString(activity.getContentResolver(), Settings.Secure.ANDROID_ID);
            return String.format("%s,%s,%s,%s\n", csi.trim(), deviceId, System.currentTimeMillis(), experimentName);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case UsbService.MESSAGE_FROM_SERIAL_PORT:
                    String data = (String) msg.obj;

                    if (data.contains("\r\n")) {
                        String[] split = data.split("\r\n");

                        String line;
                        if (split.length > 0) {
                            line = buffer + split[0];
                        } else {
                            line = buffer;
                        }

                        // Reset Buffer
                        if (split.length > 1) {
                            buffer = split[1];
                        } else {
                            buffer = "";
                        }

                        if (line.contains("CSI_DATA")) {
                            dataService.handle(updateCsiString(mActivity.get(), line));
                            listener.addCsi(line);
                        }
                    } else {
                        buffer += data;
                    }

                    break;
                case UsbService.CTS_CHANGE:
                    Toast.makeText(mActivity.get(), "CTS_CHANGE", Toast.LENGTH_LONG).show();
                    break;
                case UsbService.DSR_CHANGE:
                    Toast.makeText(mActivity.get(), "DSR_CHANGE", Toast.LENGTH_LONG).show();
                    break;
            }
        }
    }


    /**
     * @See: https://github.com/felHR85/UsbSerial/blob/master/example/src/main/java/com/felhr/serialportexample/MainActivity.java
     */
    private UsbService usbService;
    private USBHandler mHandler;

    /*
     * Notifications from UsbService will be received here.
     */
    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            switch (intent.getAction()) {
                case UsbService.ACTION_USB_PERMISSION_GRANTED: // USB PERMISSION GRANTED
                    Toast.makeText(context, "USB Ready", Toast.LENGTH_SHORT).show();
                    break;
                case UsbService.ACTION_USB_PERMISSION_NOT_GRANTED: // USB PERMISSION NOT GRANTED
                    Toast.makeText(context, "USB Permission not granted", Toast.LENGTH_SHORT).show();
                    break;
                case UsbService.ACTION_NO_USB: // NO USB CONNECTED
                    Toast.makeText(context, "No USB connected", Toast.LENGTH_SHORT).show();
                    break;
                case UsbService.ACTION_USB_DISCONNECTED: // USB DISCONNECTED
                    Toast.makeText(context, "USB disconnected", Toast.LENGTH_SHORT).show();
                    break;
                case UsbService.ACTION_USB_NOT_SUPPORTED: // USB NOT SUPPORTED
                    Toast.makeText(context, "USB device not supported", Toast.LENGTH_SHORT).show();
                    break;
            }
        }
    };

    private final ServiceConnection usbConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName arg0, IBinder arg1) {
            usbService = ((UsbService.UsbBinder) arg1).getService();
            usbService.setHandler(mHandler);
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            usbService = null;
        }
    };

    private void startService(Class<?> service, ServiceConnection serviceConnection, Bundle extras) {
        if (!UsbService.SERVICE_CONNECTED) {
            Intent startServiceIntent = new Intent(this.activity, service);
            if (extras != null && !extras.isEmpty()) {
                Set<String> keys = extras.keySet();
                for (String key : keys) {
                    String extra = extras.getString(key);
                    startServiceIntent.putExtra(key, extra);
                }
            }
            activity.startService(startServiceIntent);
        }
        Intent bindingIntent = new Intent(this.activity, service);
        this.activity.bindService(bindingIntent, serviceConnection, Context.BIND_AUTO_CREATE);
    }

    private void setFilters() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbService.ACTION_USB_PERMISSION_GRANTED);
        filter.addAction(UsbService.ACTION_NO_USB);
        filter.addAction(UsbService.ACTION_USB_DISCONNECTED);
        filter.addAction(UsbService.ACTION_USB_NOT_SUPPORTED);
        filter.addAction(UsbService.ACTION_USB_PERMISSION_NOT_GRANTED);
        this.activity.registerReceiver(mUsbReceiver, filter);
    }
}
