# PANOPTICON
A distributed system to provide an indoor device localization and monitoring by collecting and analysing WiFi Probe Requests, developed for the course "Programmazione di Sistema" (System and Device Programming) at Politecnico di Torino.    

Multiple ESP32 boards, used as fixed sniffing stations, intercept and collect the Probe Request packets sent by devices in the area, forwarding them periodically to a centralized server node, which joins all the receptions for a packet and analyses the contained information (as sender MAC address, requested SSID) and exploits the detected RSSIs, combined to the stations' known locations, to triangulate the source devices. 
      
The resulting device info is stored in a PostgreSQL database for long term storage and on-demand analytics support, and shown in real time on the server GUI, which allows also to query and display area and device statistics, replay past devices movements and manage the multiple areas ("rooms") and stations deployed.

Authors:
- Dario Salza
- Enrico Cecchetti
- Federico Brignone
<br>
Professor: Giovanni Malnati
<br>
