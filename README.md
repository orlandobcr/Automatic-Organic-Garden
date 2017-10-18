# Automatic organic garden

 A solar powered organic garden using an Arduino controller for sensing the soil humudity, air temperature, air humidity, water ph, light intensity and rain.
 The arduino uses an ethernet shield to create a bi-directional comunication using the MQTT protocol with a rapsberry pi (broker). The Raspbery-pi analizes and stores the data in a cloud-service (ThingSpeak).
 The Rasperry-pi also controls two electro-valvules for the irrigation process.
 