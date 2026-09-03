# Open-API specification

Handy Rest API v3 - Swagger UI

# **How to Start Using the API**

Before accessing the API, you need to create a user account on HandyFeeling User web.

Here's how you can do it:

### **Step 1: Request an Account Invitation**

- Visit the invitation request page here to get started.
- Fill in the necessary information to request an invitation to sign up.

### **Step 2: Complete the Registration Process**

- Once you receive your invitation, follow the provided instructions to complete the registration process.
- After successfully registering, you will be provided an **access token**.

### **Important Information about Your Access Token:**

- **Usage:** This access token is your key to log into your account. It's critical for managing your potential account settings, preferences, etc.
- **Restriction:** The access token **cannot** be used for direct API interactions. Its purpose is strictly limited to account access and management.

### **Step 3: Issue API Tokens (Application ID/Key)**

- After logging into your account with your access token, navigate to the section where you can issue API tokens.
- These API tokens are what you'll use for authenticating and interacting with the API for your development needs.

# API access and authentication

Two types of authentication tokens can be issued to access the API:

- **Application ID**
- **Application Key**

Each is intended used for different scenarios, balancing ease of use with security.

### **Application ID:**

- **Purpose:** The Application ID is intended for scenarios where only limited API access is required, and/or you might not have a server-side component in your application or website architecture. It can be used directly within client-side code, such as JavaScript running in a web browser. It will only provide access to non privileged API endpoints.
- **Usage:** It can be embedded directly into your web-pages, making it visible to anyone who inspects the web application's code.
- **Security and Limitations:** If the Application ID is publicly exposed, there's a risk that someone could extract and misuse it. While this would not compromise your account (as no privileged endpoints can be accessed with it), any use would still be attributed to your account. This could lead to increased usage statistics. This in turn may impact any account specific rate limits, quotas, etc, that are in effect.

### **Application Key:**

- **Purpose:** The Application Key can be used to access privileged API endpoints.
- **Usage:** This key should only be used server-side to interact with the privileged API endpoints. It should be kept confidential and not exposed to the public. Ex.  do NOT embed the a key in any client code.

### **Client-token**

To access the API from clients in a more secure fashion, the Application Key can be used to issue "client-tokens". These tokens serve the same purpose as the Application ID but with additional restrictions making them harder to misuse if they were to be shared outside your service. 

The following restrictions can be applied:

- limited lifetime (mandatory)
- restricted to a specific device (optional)
- restricted to a specific client IP (optional)
- restricted to a specific origin (optional)

Issuing a client-token with no device, client IP or origin restrictions works like an Application ID with an expiration time. A client-token can be embedded directly into your web-pages in the same way as an Application ID and will only provide access to non privileged endpoints.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/AUTH

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples

# Device info

The API provides various endpoints to retrieve information about the device.

- **/info** - Provides information about the device model and firmware.
- **/connected** - Checks if the device is online and connected to the Handyfeeling platform.
- **/statistics** - Provides some basic message statistics for the current device session.
- **/mode** - Get or set the current mode of the device. Depending on the protocol you choose to use to control the movement of the device, the device will enter a specific mode. The device will in most cases automatically switch to the correct mode when a protocol specific command is issued, so explicitly setting the mode is in most cases unnecessary.
- **/capabilities** - Get the devices’ hardware capabilities.
- **/sids** - Get the device session ids.

# Handling older firmware

Devices with older firmware (pre 4.0.0) need to update to firmware version 4 (v4) before the device can be used with this API.

To ease this process, the following two API v3 endpoints can be used with older firmware versions:

- **/connected** - Check online status of a device.
- **/info** - Get general information about the device.
- **/sse** - Subscribe to events from the device (fw >= v3.0.0 only). Will provide the **device_status** (if connected) or **device_connected** (when connected) events for firmware v3 devices.

All other endpoints will always return a 'Device not connected' error if you try to use them with non supported firmware versions.

For non supported devices it's important to handle the **fw_status** value returned from the **/info** endpoint properly.

Unsupported devices will always have **fw_status** = UPDATE_REQUIRED(2). See **/info** documentation for more details.

**Suggested workflow for device compatibility check**

1. Verify the device connection via **/connected**.
2. Check firmware status using **/info**.
3. If a firmware update is needed (**fw_status**=2), redirect users to https://handyverse.com/#/ota for updates.
4. Proceed with your service after confirming firmware status is up-to-date=UP_TO_DATE(0).

# Server-Sent Events (SSE)

Server-Sent Events (SSE) is a standard enabling browsers (or any SSE client) to receive automatic updates from a server via an HTTP connection. The Handy API v3 incorporates SSE technology, offering a robust, efficient way to subscribe to real-time events from devices without the traditional overhead associated with frequent polling.

### Using SSE

Once a client subscribes to the SSE endpoint, the Handy API establishes a persistent, open connection. Through this connection, events are sent as a series of messages, each representing an event from a device. This way a client will receive instant notifications about device events as they happen, ensuring your application always has the most current information.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/SSE/getEvents

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples/handy-rest-api-v3/sse

External resource: 

https://developer.mozilla.org/en-US/docs/Web/API/EventSource

# Slider Control

The API offers endpoints to control the top and bottom positions of the slider on supported devices, adjusting the maximum range and movement region.

- **/slider/stroke**
Sets the minimum and/or maximum positions of the slider. Applies across all device modes, defining the stroke range for any relative position command.

Example: If the stroke zone is set to `min=0.5` and `max=1.0`, only the top half of the slider will be used.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/SLIDER

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples

# Protocols

The API provides different protocols to interact with and control the movements of the device.

## Handy Alternate Motion Protocol (HAMP)

HAMP controls the movement of compatible devices by adjusting their stroke speed. By modifying the stroke zone of the slider, HAMP offers the same functionality as using the device buttons when the device is offline, all through the API.

### **Using HAMP**

#### Start

Use the **/hamp/start** command initiate the device's motion. The command starts the alternate motion with an initial velocity of 0.

#### Stop

Use the **/hamp/stop** command to stop the motion.

#### **Velocity**

Sets the velocity of the slider with **/hamp/velocity**. The velocity is specified as a percentage of the absolute maximum velocity. The absolute maximum velocity is device and model dependent.

#### Stroke

Set the HAMP stroke limits with **/hamp/stroke**. The HAMP stroke limits set a specific stroke region within the current slider stroke region, set by **/slider/stroke**.

Examples:

- If the slider-stroke zone is set to `min=0.5`, `max=1`, and the HAMP stroke is set to `min=0.5` `max=1.0` then the effective stroke region would be `min=0.75`, `max=1`.
- If the slider-stroke zone is set to `min=0.1`, `max=0.9`, and the HAMP stroke is set to `min=0.1` `max=1` then the effective stroke region would be `min=0.18`, `max=0.9`.
- If the slider-stroke zone is set to `min=0`, `max=1`, and the HAMP stroke is set to `min=0.1` `max=1` then the effective stroke region would be `min=0.1`, `max=1`.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/HAMP

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples/

## Handy Vibration Protocol (HVP)

The Handy Vibration Protocol (HVP) is designed to control the vibration of compatible devices. It serves as the vibration equivalent of the Handy Alternate Motion Protocol (HAMP).

### **Using HVP**

#### Start

Use the **/hvp/start** start command initiate the device's vibration. The command will start with amplitude=0 and frequency=0 (no vibration).

#### Stop

Use the **/hvp/stop** command to stop the vibration.

#### **Adjust amplitude and frequency**

Use the **/hvp/state** command to change the amplitude and frequency of the device vibration. The amplitude is a value between 0 and 1 (0-100%). Available on devices with LRA, ERM and sliders with vibration adapter (percent of valid output range). The frequency value must be in the 0-10000Hz range. Frequency is available on LRA devices only. For devices with a slider with a vibration adapter an additional position value (position of the vibration (mm)) can be specified.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/HVP

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples

## Handy Direct Streaming Protocol (HDSP)

The Handy Direct Streaming Protocol (HDSP) is designed for real-time control of the device's movements. Unlike HSSP, it doesn't rely on pre-scripted actions but expects a continuous stream of commands to be sent to the device. Unlike the synchronized protocols HSP/HSSP/STREAM it has no built in synchronization feature.

### **How It Works**

HDSP operates by sending commands to the device for immediate execution.

HDSP commands can be specified in two ways:

1. **xv**: a position (*x*) the slider should move to and a velocity (*v*) at which it should move to the position (*x*)
2. **xt**: a position (*x*) the slider should move to and the time (*t*) it should take to move to the position (*x*)

The position can be specified as an absolute (*a*) or a relative/percent (*p*) value. The bottom position is at 0 in both cases. An absolute position value is specified in millimeter. A relative position value is the percentage of the total the stroke length of the device (0-1, 0=bottom, 0.5=middle, 1=top).

The velocity (*v*) can specified as an absolute (*a*) or a relative/percent (*p*) value. An absolute value is the velocity in millimeter per second (mm/s). A relative velocity value is the percentage of the maximum velocity of the device (0-1, 0=slowest, 0.5=medium, 1=max)

The time value is specified in milliseconds (ms).

This provides a total of six different HDSP commands to choose from:

- **/hsdp/xava**: absolute position, absolute velocity
- **/hdsp/xavp**: absolute position, relative velocity
- **/hdsp/xpva**: relative position, absolute velocity
- **/hdsp/xpvp**: relative position, relative velocity
- **/hdsp/xat**: absolute position, time
- **/hdsp/xpt**: relative position, time

### Using HDSP

Using HDSP involves issuing HDSP commands sequentially. No setup or control commands are required. For continuous device movements a continuous sequence of commands must be forwarded to the device.

In addition the following optional commands flags are available:

- **stop_on_target**: If set to `true`, the device movement will stop when it reaches the position specified in the command it’s currently executing. In order to have a smoother and more continuous movements between commands, setting the the value to `false` is recommended. `false` is the default value.
- **immediate_rsp**: By default, the API will not return a response to the client until the command have been executed on the device. To change the this behavior, set the immediate_rsp option to `false`.

### Limitations of HDSP

HDSP commands are not buffered on the server or the device. Commands are forwarded immediately to the device and executed as soon as they are received by the device on a best-effort basis.

HDSP does not provide any built in form of synchronization. Achieving synchronization with some external source using HDSP involves fine-tuning the flow of commands to the device. Such an approach will be highly dependent on the network conditions of the client and the device and would need to be adjusted accordingly. If you want synchronization, consider using one of the protocol with built in support. HSP or the STREAM protocol would be options to consider for playing dynamic synchronized content.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/HDSP

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples

## Synchronized protocols

The Handy API provides different protocol for playing synchronized movements on a device. To synchronize the device movement with some external source on the client side, the device and the client need a common reference for the current time. This is achieved by synchronizing the clocks of the device and client with the server clock, by calculating the the clock offset between the device and server and the client and server. The clock offset can then be used to estimate the current server-time on the client and the device.

One way to calculate the client-server-offset (*cs_offset*) is as follows:

1. Collect *X* server time (*Ts*) samples using the `/servertime` API endpoint. A higher number of samples will results in longer estimation time but a more accurate result. A good sample size is 30 (*X* = 30).
2. Track the round-trip-delay (*RTD*) for each sample by recording the request send time (*Tsend*) and response received time (*Treceive*). Calculate *RTD* = *Treceive* – *Tsend*.
3. Calculate the estimated server time when the response is received (*Ts_est*) by adding half the *RTD* time to the received server time value (*Ts*). *Ts_est* = *Ts* + *RTD*/2.
4. Calculate the offset between estimated server time (*Ts_est*) and client time (*Tc*). Upon receive *Tc* == *Treceive* => *offset* = *Ts_est* - *Treceive*.
5. Add the offset to the aggregated offset value (*offset_agg*). *offset_agg* = *offset_agg* + *offset*.
6. When all samples have been received calculate the average offset (*cs_offset*) by dividing aggregated offset (*offset_agg*) values by the number of samples (*X*). *cs_offset* = *offset_agg* / *X*
The process above gives you a good estimate of the client-server-offset (*cs_offset*).

Normally you calculate the *cs_offset* once, and use it whenever you need to calculate client-side-estimated-server-time (*Tcest*). However, if the synchronization between device and the service (ex. video/script synchronization) is off (maybe due to changing network topology, clock drift, bad initial calculation, etc.), it might help to re-calculate the *cs_offset*.

The client-side-estimated-server-time (*Tcest*) value is required in the **play** and **synctime** commands of all the synchronized protocols:

- **hsp**
- **hssp**
- **stream**

The *Tcest* is calculated the following way:

*Tcest* = *Tc* + *cs_offset*

where *Tc* is the current client time and *cs_offset* is the client-server-offset.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/UTILS

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples

## Handy Synchronized Script Protocol (HSSP)

The Handy Synchronized Script Protocol (HSSP) is a protocol designed for synchronizing the movement of the Handy device with some external source, e.g. multimedia content, such as video, audio, games etc. The protocol allows for a highly immersive experience by ensuring that the device's movements are in sync with an external media source.

### **Key Features of HSSP**

1. **Time-Stamped Commands:** Scripts are comprised of a series of commands, each associated with a timestamp indicating when the device should perform a specified action. A good comparison is video subtitles.
2. **Precision Synchronization:** The protocol allows for the device's movements to be precisely synchronized with the media content, providing an experience that closely matches the intended action or rhythm of the content. The synchronization is handled through the device-client server-time synchronization mechanism (see Synchronized protocols).
3. **Flexibility:** HSSP can be used with any content for which movements can be represented as a series of actions (timestamp/position).

### **How It Works**

HSSP utilizes scripts that dictate the device's movement. A script consist of one or more actions that the device executes in order. Each action consist of a timestamp (*T*) and position (*X*).

The timestamp (*T*) is specified in milliseconds relative to the start of the script (*T*=0).

E.g. *T*=0 start, *T*=1000 one second

The position (*X*) is a position on the slider (0-100%) relative to the length of the device slider.

E.g. *X*=0 bottom, *X*=50 middle, *X*=100 top

The timestamp tells the device when the device should reach the given position.

While playing, the device will (to the best of it’s capabilities) try to reach the next position *X* at the time *T* ****relative to the play start time.

A script that represent movement in some content are based on the content's timeline and must include precise instructions for the device's movement. If the movements are known or can be calculated in advance, creating scripts is straight forward. If the movement must be transcribed by analyzing the content, the process of creating scripts can be time-consuming.

The HSSP protocol supports the following script formats:

- funscript/json
- handy csv

For more information about script formats and scripting:

Script

Scripting

### **Using HSSP**

#### **Prerequisite**

HSSP uses the client-device server-time synchronization mechanism for synchronization. The client and device clocks must be synchronized with the server time before you issue a /**hssp/play** or /**hssp/synctime** command for optimal synchronization.

#### **Setup**

To initiate a new HSSP session, use the **/hssp/setup** command to specify the script content to play.

The content can be provided as a URL or content pushed directly to the server by specifying one of the following parameters:

- **url**: A URL to a supported content. Note that the URL must be accessible over the internet. Private network URLs are not supported.
- **csv**: CSV script content
- **actions**: JSON script content

If you can not host your own scripts, have a look at the Hosting API.

Hosting API v2

#### **Play**

Use the **/hssp/play** command to play the the script.

Mandatory parameters:

- **start_time**: The timestamp (milliseconds) to start playing from. 0=start. Specify a time>0 to skip forward. Ex. 10000 = 10 seconds into the script
- **server_time**: The client-side-estimated-server-time (*Tcest*) at the time of the play request.

Optional parameters:

- **playback_rate**: The playback rate to use. Default=1.0 1.0=normal speed, 0.5=half speed, 2.0=double speed
- **loop**: Enable looping. The device will start playing from the start of the buffer when the end is reached. Default=`false`

#### **Stop**

Use the **/hssp/stop** command to stop the playback and the device movements.

#### **Fine-tuning the synchronization**

Sometimes, the source you want to synchronize your device with might not provide perfectly accurate timestamps. For example, an HTML video player might provide the current play time with a slight inaccuracy, by design or due to page issues. To compensate for these inaccuracies, you can use the **/hssp/synctime** endpoint to send regular time updates to the device. The device will use these updates to gradually adjust the playback into sync over time, filtering out inaccuracies.

For example, after a **/hsp/play**, you can send a **/hsp/synctime** request every 2 seconds for the first 10 seconds, then every 10 seconds after that for the rest of the session. You should repeat this process whenever you skip back or forward in the script/video using **/hsp/play**.

### Changes from firmware version 3 and API version 2

Firmware versions 3 and 4 handle script actions and points differently. In firmware version 3, the device downloads the entire script directly from the URL provided during setup. Due to hardware limitations, there is a fixed maximum script size of 524,288 bytes (in CSV format). If a script exceeds this size, the setup fails.

In firmware version 4, the maximum script size is no longer fixed like in firmware 3. Instead, the device's point buffer size is calculated per HSSP session and depends on the available memory and storage at the time of setup. As a result, each HSSP session will have a different maximum buffer size. With Wi-Fi enabled only, the typical device buffer size ranges between 3,000 and 4,000 points, though sizes outside this range are possible. If both Bluetooth and Wi-Fi are enabled, the buffer size can drop as low as 300 points.

With firmware version 4 and API version 3, the device does not download the script directly from the URL. Instead, the server fetches the script and streams points to the device as needed. During setup, the server initially pushes a minimal set of points to the device, allowing script playback to begin—assuming playback starts from time=0. The server then continues streaming points until all are delivered or the point buffer is full. If the script is smaller than the buffer size (N), the entire script is loaded onto the device. For scripts larger than N, only the first N points are loaded initially. As playback progresses or if you skip to a point outside the current buffer, the server replaces the buffer's points with the necessary ones.

For the HSSP protocol, this introduces the following new features compared to API v2/firmware 3:

- **No script size limit**: Since the device only needs to store the parts of the script it’s currently playing, there is no script size limit.
- **Shorter setup time**: Playback can begin as soon as the first point in a script is received, eliminating the need to load all points beforehand. This allows for near-instant playback and quicker script switches, especially if server-side script caching is utilized.

For typical video/script synchronization use cases, the new streaming behavior is a clear improvement with no functional differences. However, if you’re using scripts and Handy integration for haptic feedback in games or similar applications, there may be side effects depending on your implementation—especially if it relies on the static point buffer size in firmware 3 and loading the entire script on the device.

For instance, if you use a single large script to store all possible Handy movement patterns in your game and play specific parts of the script (similar to how web image sprites work), ensure the entire script fits on the device. If the script exceeds the device’s capacity, you might notice a delay when skipping to parts of the script outside the current device point buffer, depending on the device-server round-trip-time. This is because an additional message is needed between the server and device to push the required points.

If the script is too large to fit entirely on the device, consider splitting it into meaningful sub-scripts wherever possible and loading each sub-script as needed. Since the buffer size can vary between sessions, you should assume a conservative buffer size value, or dynamically adjust the size of the scripts according to the current device state. If splitting isn’t feasible, consider using the HSP protocol instead—it gives you complete control over which points are in the device buffer at any given time.

### Limitations of HSSP

HSSP is not suitable for interactive content where actions can't be predetermined. If you require dynamic movement generation, HSP or the STREAM protocol might be better options.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/HSSP

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples

## Handy Streaming Protocol (HSP)

The Handy Streaming Protocol (HSP) is designed for streaming synchronized movement data to the device. The HSP protocol allows for full control of the data and playback on the device. The device plays points stored in a buffer on the device. The buffer has a fixed size, but you can add or remove data from it as needed. This allows you to stream data to the device on demand, with no maximum limit to the number of points you can play in a session. The protocol is a best suited for services that require quick and dynamic changes of the playback data.

### Using HSP

#### **Prerequisite**

HSP uses the client-device server-time synchronization mechanism for synchronization. The client and device clocks must be synchronized with the server time before you request a **/hsp/play** or **/hsp/synctime** command for optimal synchronization.

#### **Initiate a HSP session**

To start a new HSP session, you need to issue the **/hsp/setup** command. This will reset any existing HSP state on the device and prepare it for a fresh session. In the setup payload you can specify the **stream_id** to use for the session. If no **stream_id** is provided, the server will assign a value. Unless you have a specific reason to use a specific **stream_id**, letting the server assign the **stream_id** is the better option.

The setup command will return the new HSP state. Here are some key state values to note:

- **stream_id:** Identifies the current HSP session on the device. When this value changes, it indicates that a new HSP session has been initiated.
- **max_points:** The maximum number of points the device can keep in its buffer during the current HSP session. This value is dynamically calculated based on the device's available memory at the time of setup. While it may vary between sessions, it remains fixed for the duration of the current session.
- **points:** The current number of points in the device buffer.
- **current_point:** The buffer index (0-based) of the last played point.
- **current_time:** The current play time of the hsp session. Example: 5000 would mean 5000ms into the current stream. NOTE: This is not the same as the timestamp of the last played point.
- **loop:** The current loop state. If `true`, the device will loop through the buffer points when it reaches the end.
- **playback_rate:** The playback speed. For example, 1.0 represents normal speed, 0.5 is half speed, and 2.0 is double speed.
- **first_point_time:** The timestamp of the first point in the device buffer.
- **last_point_time:** The timestamp of the last point in the device buffer.
- **play_state:** The current play state of the device (e.g., playing, stopped, or starving).
- **tail_point_stream_index:** The index of the last point in the device buffer relative to all available points. For instance, if the client has 10,000 total points and the device buffer holds points 4,500 through 5,200, the tail_point_stream_index would be 5,200.
- **tail_point_stream_index_threshold:** The stream index at which the device should notify the client with a threshold notification. For example, if the threshold is set to 5,000, the device will send a threshold notification when it plays the point at stream index (not the buffer index!) 5,000.

#### **Adding points to the device buffer**

Use the **/hsp/add** command to add data to the device buffer. You can add up to 100 points in a single command. For more than 100 points, repeat this process until the buffer is full (**points** = **max_points**). When adding more points than the buffer's remaining capacity, the device removes the oldest points and appends the new ones to the end of the buffer.

To clear all existing points in the device buffer before adding new ones, set **flush** = `true`. By default, **flush** is set to `false`.

Ensure the **tail_point_stream_index** is set correctly for the device to send threshold notifications at the appropriate time.

#### Play

After adding data to the device buffer, use **/hsp/play** to play the data.

Mandatory parameters:

- **start_time**: The timestamp (in milliseconds) to start playing from. Use 0 to start at the beginning of the points. Specify a time greater than 0 to skip ahead. For example, 10000 represents 10 seconds into the client points.
- **server_time**: The client-side estimated server time (*Tcest*) at the moment of the play request. If not provided, the actual current server-time is used. Not providing a server_time estimate will negatively impact synchronization, but might be fine for playing points when no synchronization with some other source is required.

Optional Parameters:

- **playback_rate**: The playback speed to use. Default is 1.0, with 1.0 representing normal speed, 0.5 half speed, and 2.0 double speed.
- **loop**: Enable looping to have the device start playing from the beginning of the buffer once it reaches the end. Default value is `false`.
- **pause_on_starving**: Enables pause-on-starving. This will make the HSP playback to pause when the device enters a starving state (no more playable points the buffer). The playback will resume when new data is received. Default value is `false`.
- **add**: An embedded command to add points, executed before the play command.

#### Stop

Use the **/hsp/stop** command to stop the playback and the device movements.

#### Notifications

A device will send the following HSP notifications to the client:

- **hsp_starving**: Sent when the currently last point in the device buffer is reached and there is no more available points to play.
- **hsp_threshold_reached**: Sent when the device have reached the point in the device buffer specified as the **tail_point_stream_index_threshold**. ****The client should add additional points to the device buffer if more are available at this point.
- **hsp_looping**: Sent when the device starts a new loop.
- **hsp_state_changed**: Sent when one or more values in the HSP state have changed, that was not a triggered by a client issued HSP command.

The HSP notifications can be received through the **/sse** endpoint. Alternatively you can pull the **hsp/state** endpoint and check the returned HSP state for value changes**.**

#### **Update the stream index threshold**

Use the /**hsp/threshold** operation to only update the **tail_point_stream_index_threshold** value.

#### Clear device buffer

Use the /**hsp/flush** command to only clear the device point buffer.

#### Pause/Resume playback

A **/hsp/pause** will stop the playback but keep track of the last played point when the pause was issued. This allows for the playback to be continued at a later point with the **/hsp/resume** command. A resume can be performed in two different ways. If the resume **pick_up** flag is set to `false` the playback will continue from last played point. If **pick_up** is set to `true` the playback will resume from where the playback would be if not pause had been initiated.

#### Pause-on-starving

Use **/hsp/pause/onstarving** to only update the **pause_on_starving** flag. Enabling this will guarantee that data received by a device in a starving state, is never considered old. This could potentially happen if the data is not provided fast enough to the device by the data source.

#### Fine tuning synchronization

Sometimes, the source you’re synchronizing your device with may not provide perfectly accurate timestamps. For instance, an HTML video player might report the current play time with slight inaccuracies—whether by design or due to page issues. To address these discrepancies, you can use the **/hsp/synctime** endpoint to send regular time updates to the device. These updates enable the device to gradually align playback, filtering out inaccuracies over time.

For example, after issuing a **/hsp/play**, you can send a **/hsp/synctime** request every two seconds for the first 10 seconds, then every 10 seconds for the remainder of the session. Repeat this process also when you skip ahead in the video using **/hsp/play**.

#### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/HSP

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples

# Stream Protocol (STREAM)

The Stream Protocol (STREAM) provides an easier method for handling a common scenario in the Handy Streaming Protocol (HSP), specifically playing live, sequential movement data.

Designed to work alongside the Stream API, the STREAM protocol distinguishes itself by separating the tasks of populating the stream with data and issuing device commands. While the Stream API is responsible for stream creation and managing stream data, the STREAM protocol handles device commands, such as setting up the stream on the device, play, stop, and other control functions.

A stream operates with an append-only data model, meaning data can only be added, not removed. This approach streamlines the process of pushing data to the device, managed entirely server-side, eliminating the need for handling threshold and starving notifications (typically associated with HSP) on the client side.

From a developer perspective, STREAM behaves similarly to the Handy Synchronized Script Protocol (HSSP), but with the added advantage that the data can be dynamic. This makes it better suited for live data. Additionally, the same stream can be used by multiple devices simultaneously, further enhancing its flexibility and utility.

#### Using STREAM

**Prerequisite**

In order to use your own streams with the STREAM protocol you need to create them using the Stream API. See the developer resources for more information.

STREAM uses the client-device server-time synchronization mechanism for synchronization. The client and device clocks must be synchronized with the server time before you request a **/stream/play** or **/stream/synctime** command for optimal synchronization.

#### Setup

To start a new STREAM session, you need to issue the **/stream/setup** command. This will reset any existing HSP state on the device and prepare it for a fresh session. The setup command will return the new HSP state. In the setup payload, you specify the reference (**stream_ref**) of the stream you want to play. If you created the stream yourself using the Stream API, use the **stream_ref** returned from the API during creation, or one provided to you. Any existing, valid, accessible and active stream_ref will work.

By default, the STREAM protocol do not push SSE HSP notifications to the client. To enable it set the payload notify flag to `true`.

#### **Play**

Use the **/stream/play** command to start stream playback.

Mandatory parameters:

- **start_time**: The timestamp (milliseconds) to start playing from. 0=start. Specify a time>0 to skip forward. Ex. 10000 = 10 seconds into the stream.
- **server_time**: The client-side-estimated-server-time (*Tcest*) at the time of the play request.

Optional Parameters:

- **playback_rate**: The playback speed to use. Default is 1.0, with 1.0 representing normal speed, 0.5 half speed, and 2.0 double speed.
- **loop**: Enable looping to have the device start playing from the beginning of the buffer once it reaches the end. Default value is `false`.
- **pause_on_starving**: Enables pause-on-starving. This will make the HSP playback to pause when the device enters a starving state (no more playable points the buffer). The playback will resume when new data is received. Enable this to guarantee that received data is not considered in the past when received by the device. Default value is `false`.

#### **Stop**

Use the **/stream/stop** command to stop the playback and the device movements.

#### Fine tuning synchronization

Sometimes, the source you’re synchronizing your device with may not provide perfectly accurate timestamps. For instance, an HTML video player might report the current play time with slight inaccuracies—whether by design or due to page issues. To address these discrepancies, you can use the **/stream/synctime** endpoint to send regular time updates to the device. These updates enable the device to gradually align playback, filtering out inaccuracies over time.

For example, after issuing a **/stream/play**, you can send a **/stream/synctime** request every two seconds for the first 10 seconds, then every 10 seconds for the remainder of the session. Repeat this process if you skip ahead in the video using **/stream/play**.

### Developer resources

API reference:

https://www.handyfeeling.com/api/handy-rest/v3/docs/#/STREAM

https://www.handyfeeling.com/api/stream/v0/docs/#/

Stream API docs:

Stream API

Code examples:

https://gitlab.com/sweettechas/platform/platform-api-examples