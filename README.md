# RoverAndEventHub
Arduino rover robot that uploads telemtry via WiFi + MQTT to Azure EventHub

The Rover has 3 front facing HC-04 UltraSonic distance sensors and 2 DC Brushed motors, controlled by an Adafruit Huzzah32 FeatherWing.

Using MQTT the system had triple the throughput when compared to straight HTTP (non-ssl) GET requests.

It uploads the distance readings and battery level to an Azure EventHub (similar to Apache Kafka). An Azure Function is triggered which 
pulls the data from the EventHub and submits to a .NET Core WebAPI project. 

To disseminate the info, the Web project sends to the data to a SignalR Hub. This allows for real-time viewing of the data. The webpage's 
JavaScript create charts to visualize the data.

Due to the upload performance of the Arduino, the daily quota of my web server plan quickly was exhausted.
