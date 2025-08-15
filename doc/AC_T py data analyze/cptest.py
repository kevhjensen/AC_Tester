import numpy as np



def ADS_raw24sInt_to_mV(raw):
    return raw * 512 / (2**23)

def AC_V_from_mV(AC_V_mV):
    return AC_V_mV / 1.009


def AC_I_from_mV(AC_I_mV):
    return AC_I_mV / 3.12 #3.12 = 7.8/2.5 AX80 burden resistor / divider ratio/1000


def AC_V_from_raw(raw):
    return AC_V_from_mV(ADS_raw24sInt_to_mV(raw))
def AC_I_from_raw(raw):
    return AC_I_from_mV(ADS_raw24sInt_to_mV(raw))


rawV = 15687794051524

adsV = 247
print(AC_V_from_mV(adsV))