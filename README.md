# returnFanContoller

### Problem: 

My house has an issue with stratification in the summer, requiring us to leave a fan in the stairwell. I saw this potentially coming from talking to my neighbors and in addition to good insulation on the roof deck, I also installed a return air duct on the 2nd floor (which it did not have).  This duct is very high in the room and with a proper draw should evacuate hot air in the room quickly with the right amount of negative pressure applied. Because the return ducts in the house start with a (very poorly sealed) run of panning through joist bays and the duct takes a circuitous path upstairs, I am confident I am getting very little, if any draw from the return duct I connected to the 2nd floor.  To remedy this I have purchased a [Fantech ProAir EC-6](https://shop.fantech.net/en-US/prioair--6--ec--inline--duct--fan/p96222#!) I chose this because I would rather not play around with dampers if I can help it.  My furnace is not intended to integrate  with any sort of external fan, nor is my thermostat (nest... maybe 2nd gen?). 

### Goals: 

1. Log the activity of the 24VAC thermostat signals coming from the Nest. Also log the blower fan speeds and some relevant temperatures. 
2. Activate the booster fan when the furnace fan runs.
3. Monitor a temp sensor on the second floor and activate both the furnace and booster fan when the temperature differential exceeds a threshold. 

~~This repostitory consists of the code for the controller which will run on a raspberry pi. Schematics / BOM  will be on a separate hackaday.io project when time permits.~~

This project has been completed and I am in the process of documenting it. Overwhelming success. However, it was exceptionally difficult to get a proper PWM generated out of the raspberry pi and over time it just became more trouble than it was worth to stay with that platform. I've moved the effort to an Arduino which worked fine. I am still going to attempt to add some of the bells and whistles I wanted earlier but for now it works great. 
