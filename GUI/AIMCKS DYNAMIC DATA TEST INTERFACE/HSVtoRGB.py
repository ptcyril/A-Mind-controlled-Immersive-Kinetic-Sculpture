#HSVtoRGB

def hsvtorgb(hue,sat,val):
    
    saturation = sat/100
    value = val/100

    C = value * saturation

    temp = abs((hue/60) % 2 - 1)
    X = C * (1 - temp)
    m = value - C

    if(0 <= hue and hue < 60):
        r = C
        g = X
        b = 0
        
    elif(60 <= hue and hue < 120):
        r = X
        g = C
        b = 0
        
    elif(120 <= hue and hue < 180):
        r = 0
        g = C
        b = X
        
    elif(180 <= hue and hue < 240):
        r = 0
        g = X
        b = C
        
    elif(240 <= hue and hue < 300):
        r = X
        g = 0
        b = C
        
    elif(300 <= hue and hue < 360):
        r = C
        g = 0
        b = X

    rgb = []
    rgb.append(round((r+m)*255))
    rgb.append(round((g+m)*255))
    rgb.append(round((b+m)*255))
    
    #print(f'R:{rgb[0]} G:{rgb[1]} B:{rgb[2]}')
    return rgb

