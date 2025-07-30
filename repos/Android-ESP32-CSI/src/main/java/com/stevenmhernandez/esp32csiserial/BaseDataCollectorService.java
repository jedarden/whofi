package com.stevenmhernandez.esp32csiserial;

import android.content.Context;

/**
 * This abstract base class allows us to very easily create new methods to store our data.
 * For example, if you wish to simple send an HTTP PUT request to a server, create a
 * subclass of this abstract class and implement the following methods.
 */
public abstract class BaseDataCollectorService {
    /**
     * Do any required setup here (i.e. initiate connection to a server or creating a file)
     *
     * @param context application context if required.
     */
    public abstract void setup(Context context);

    /**
     * Handle new data (i.e. HTTP PUT request or append to a local file)
     *
     * @param csi string data direct from the ESP32 serial monitor
     */
    public abstract void handle(String csi);
}
