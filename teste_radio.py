# -*- coding: utf-8 -*-
"""
Created on Sat Dec  4 14:42:41 2021

@author: plinio silva
"""

import serial
import time
from datetime import datetime


#%%
try:
    ser1.close()
except:
    pass

try:
    ser2.close()
except:
    pass
#%%
# ser1 = serial.Serial("COM12", 115200,timeout = 0.01)
# ser2 = serial.Serial("COM14", 115200,timeout = 0.1)
ser1 = serial.Serial("COM12", 115200,timeout = 0.1)
# ser2 = serial.Serial("COM12", 460800,timeout = 0.1)
time.sleep(3)

#%%
stringSizeMulti = 3
sleep = 1 #Stream
#####################################
# stringSizeMulti = 600
# sleep = 30 #Instant Transmit
#####################################

totalTxBytes = 0
totalTime = time.time()


ser1.flush()
time.sleep(1)
# ser2.flush()


while(True):
    ti = time.time()
    tin = str(ti).encode('ascii') 
    tin = tin * stringSizeMulti 
    tin = tin + b'\n'
    ser1.write(tin)
    ser1.flush()
    # tout = ser2.readline()
    time.sleep(sleep)
    tout = ser1.readline()
    print("%s bytes, "%(len(tin)),end =" ")
    if tin != tout:
        print("Erro tin != tout")
        print("totalTxBytes = %d"%totalTxBytes)
        print("total time without error %.4f"%(time.time() - totalTime))
        print("tinn= %s\tout= %s"%(tin,tout))
        totalTxBytes = 0
        totalTime = time.time()
        time.sleep(3)
        # tout = ser2.readline()    
        tout = ser1.readline()
        
    else:        
        totalTxBytes += len(tin)
        print(time.time() - ti,end=" ")
        print("%.1f bps, "%(8 * totalTxBytes/(time.time() - totalTime)),end = " ")
        print("%.2f runtime"%(time.time() - totalTime))
    
    
    
    
    
#%%

ser1 = serial.Serial("COM15", 460800,timeout = 0.01)

#%%
stringSizeMulti = 6
sleep = 0.0001 #Stream
#####################################
# stringSizeMulti = 600
# sleep = 30 #Instant Transmit
#####################################

totalTxBytes = 0
totalTime = time.time()

while(True):
    ti = time.ctime()
    tin = str(ti).encode('ascii') + b' '
    tin = tin * stringSizeMulti 
    tin = tin + b'\n'
    ser1.write(tin)
    ser1.flush()
    tout = ser1.readline()
    if (tout != b''):
        print(tout)








    