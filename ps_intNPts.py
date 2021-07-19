# run with python 3.8
# interpolation of 'suface_'-files for improved temporal discretization
# requires folder 'PTS' containing the 'surface_'-files created by stl2pts

import os
import numpy as np
from scipy.interpolate import pchip
from scipy.interpolate import interp1d

# linear, quadratic, cubic or monotonic cubic spline interpolation between points 
def itpl(PTS, numPTS, degree, sID, eID):
    t = range(len(PTS))
    ipl_t = np.linspace(sID, eID, numPTS * (eID - sID) // (len(PTS) - 1) + 1)

    if degree == 1:
        newX = interp1d (t, [row[0] for row in PTS], kind='slinear') (ipl_t)
        newY = interp1d (t, [row[1] for row in PTS], kind='slinear') (ipl_t)
        newZ = interp1d (t, [row[2] for row in PTS], kind='slinear') (ipl_t)
    elif degree == 2:
        newX = interp1d (t, [row[0] for row in PTS], kind='quadratic') (ipl_t)
        newY = interp1d (t, [row[1] for row in PTS], kind='quadratic') (ipl_t)
        newZ = interp1d (t, [row[2] for row in PTS], kind='quadratic') (ipl_t)
    elif degree == 3:
        newX = interp1d (t, [row[0] for row in PTS], kind='cubic') (ipl_t)
        newY = interp1d (t, [row[1] for row in PTS], kind='cubic') (ipl_t)
        newZ = interp1d (t, [row[2] for row in PTS], kind='cubic') (ipl_t)
    elif degree == 4:
        newX = pchip (t, [row[0] for row in PTS]) (ipl_t)
        newY = pchip (t, [row[1] for row in PTS]) (ipl_t)
        newZ = pchip (t, [row[2] for row in PTS]) (ipl_t)
    else:
        newX = interp1d (t, [row[0] for row in PTS], kind='quadratic') (ipl_t)
        newY = interp1d (t, [row[1] for row in PTS], kind='quadratic') (ipl_t)
        newZ = interp1d (t, [row[2] for row in PTS], kind='quadratic') (ipl_t)

    outPTS = []
    for i in range(0, len(newX)):
        outPTS.append([newX[i], newY[i], newZ[i]])

    return outPTS

# Create folder
folderName = 'UDFPTS'
if not os.path.exists(folderName):
    os.makedirs(folderName)

print ('Generate UDF Points for FLUENT')
numFrame = int(input('Total number of frames in a cycle: '))
numInter = int(input('Number of intermediate frames: '))
k = int(input('Which Interpolation? \n linear:1, quadratic:2, cubic:3, monotonic cubic spline:4'))

# load points of all frames into one global array
allFrame = []
for frameID in range(0, numFrame):    
    if os.name == 'posix':
        rawPtName = 'PTS/surface_{}.asc'.format(frameID)
    else:
        rawPtName = 'PTS\\surface_{}.asc'.format(frameID)

    eachFrame = np.loadtxt(rawPtName)
        
    allFrame.append(eachFrame)    

print ('Total number of nodes on surface: {}'.format(len(allFrame[0])))


# temporal interpolation between frames and writing of new files
# after every 1000 points: write data to 'udfsurface_'-file in order to minimize memory usage
entirePts = []
i = 0
for ptID in range(0, len(allFrame[0])):
    #extract data of indiviual point ID for all frames, tracking the movement of the individual point over time
    oo = []
    for frameID in range(0, numFrame):
        oo.append(allFrame[frameID][ptID]) 
    
    # movement of point three times in a row. 
    # Reason to this remains a secret to god and the original developer
    pts = oo+oo+oo 
    pts.append(oo[0]) # append array by first entry  
    
	# interpolation for each point ID extracting this point ID from every frame
    denPts = itpl(pts, 3*numInter*numFrame, k, numFrame, 2*numFrame)
    entirePts.append(denPts) # append array with new interpolated points 
    
	# write first point to file
    if ptID == 0:
        print ('0')
        for frame in range(0,numInter*numFrame+1):
            framePts=[]       
            framePts.append(entirePts[ptID][frame])
            if os.name=='posix':
                frameFileName='UDFPTS/udfsurface_{}.asc'.format(frame+1)
            else:
                frameFileName='UDFPTS\\udfsurface_{}.asc'.format(frame+1)
            f = open(frameFileName, "a")
            np.savetxt(f, framePts)
            f.close()
        entirePts = []

    # write data every 1000 points to file
    if ptID%1000 == 0 and ptID != 0:
        print (int(ptID/1000))
        for frame in range(0, numInter*numFrame+1):
            framePts = []      
            for pt in range(0, 1000):
                framePts.append(entirePts[pt][frame])
            if os.name == 'posix':
                frameFileName = 'UDFPTS/udfsurface_{}.asc'.format(frame+1)
            else:
                frameFileName = 'UDFPTS\\udfsurface_{}.asc'.format(frame+1)
            f = open(frameFileName, "a")
            np.savetxt(f, framePts)
            f.close()
        entirePts = []
        i += 1

# write remaining points to file
print ('Total Number of Frames after interpolation: {}'.format(len(denPts)))
for frameID in range(0, numInter*numFrame+1):
    framePts = []
    c = len(allFrame[0]) - i * 1000 -1
    for ptID in range(0, c):
        framePts.append(entirePts[ptID][frameID])
    if os.name == 'posix':
        frameFileName='UDFPTS/udfsurface_{}.asc'.format(frameID+1)
    else:
        frameFileName='UDFPTS\\udfsurface_{}.asc'.format(frameID+1)

    f = open(frameFileName, "a")
    np.savetxt(f, framePts)
    f.close()