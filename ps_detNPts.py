# run with python 3.8
# Convert STL files into files containing the coordinates of the points on the ventricle wall, preserving their order in which they were read
# Requires folder named 'STL' containing all STL files with name 'ventricle_a', where a is an index starting with 0

import os
import numpy as np
import matplotlib.pyplot as plt

# result = (dis,t)
# dis = distance between point and line defined by p1, p2
# 0 < t < 1: point lies within boundaries p1, p2 
def pt2Line (Pt, P1, P2):

	# compute direction from p1 to p2 and direction from p1 to Pt
	refDir = (np.array(P2) - np.array(P1)) / np.linalg.norm( np.array(P2) - np.array(P1)) #2-Norm for vector, Frobenius norm for matrix
	dis = np.linalg.norm (np.cross (np.array(Pt) - np.array(P1) , refDir))
	t = np.dot (np.array(Pt) - np.array(P1) , refDir) / np.linalg.norm (np.array(P2) - np.array(P1))

	result = []
	result.append(dis)
	result.append(t)
	
	return result

# check if point lies on one of the three lines bounding the facet
# info[0] = distance to line
# info[1] = relative position between points
def checkLine(facet, Pt, sR):
   
	p1 = facet[0]
	p2 = facet[1]
	p3 = facet[2]
	# line 1 between p1 and p2
	info = pt2Line(Pt, p2, p1) #p22p1
	if info[0] <= sR and info[1] >= 0 and info[1] <= 1:
		return [1, info[1], 0, 2]
	# line 2 between p2 and p3
	info = pt2Line(Pt, p2, p3) #p22p3 
	if info[0] <= sR and info[1] >= 0 and info[1] <= 1:
		return [1, 0, info[1], 2]
	# line 3 between p1 and p3
	info = pt2Line(Pt, p1, p3)  #p12p3  
	if info[0] <= sR and info[1] >= 0 and info[1] <= 1:
		return [1, 1-info[1], info[1], 2]
	
	return [0, 0, 0, 2]

# check if point lies within the facet
def checkSphere(facet, Pt):

	U = np.array(facet[0]) - np.array(facet[1])
	V = np.array(facet[2]) - np.array(facet[1])
	W = np.array(Pt) - np.array(facet[1])
	
	# alpha: relative position of point projected on P2P1
	dir1 = np.cross(V, W)
	vCrossW = np.linalg.norm(dir1)
	dir2 = np.cross(V, U)
	# if condition holds true, dir1 and dir2 span an obtuse angle 
	# -> alpha < 0 
	# -> point not on facet
	if (np.dot(dir1, dir2) < 0): 
		return [0, 0, 0, 1]

	# beta: relative position of point projected on P2P3 
	dir1 = np.cross(U, W)
	uCrossW = np.linalg.norm(dir1)    
	dir2 = np.cross(U, V)
	# if condition holds true, dir1 and dir2 span an obtuse angle 
	# -> beta < 0 
	# -> point not on facet
	if (np.dot(dir1, dir2) < 0):
		return [0, 0, 0, 1]
	
	# relative area with respect to whole area 
	denom = np.linalg.norm(dir2)
	alpha = vCrossW / denom
	beta = uCrossW / denom
	gamma = 1.0 - alpha - beta
	if gamma >= 0:
		# point lies in plane, but not within facet
		return [1, alpha, beta, 1]
	else:
		# point lies within facet
		return [0, 0, 0, 1]

# check if point lies on vertex of facet (alpha, beta, gamma = 0 or 1)
def checkPoint(facet, Pt, sR):

	p1 = facet[0]
	p2 = facet[1]
	p3 = facet[2]
	if np.linalg.norm (np.array(Pt) - np.array(p1)) <= sR:
		return [1, 1, 0, 3]
	if np.linalg.norm (np.array(Pt) - np.array(p2)) <= sR:
		return [1, 0, 0, 3]
	if np.linalg.norm (np.array(Pt) - np.array(p3)) <= sR:
		return [1, 0, 1, 3]
	return [0, 0, 0, 3]

# check relation between Pt and facet
def touchCheck(facet, Pt, sR):

	# facet is defined by three points
	p1 = facet[0]
	p2 = facet[1]
	p3 = facet[2]
	# verify that points define a facet 
	if np.linalg.norm (np.cross (np.array(p1) - np.array(p2), np.array(p3) - np.array(p2))) == 0:
		return [0, 0, 0]
	
	# compute normal of facet
	v1 = np.array(facet[0]) - np.array(facet[1])
	v2 = np.array(facet[2]) - np.array(facet[1])
	normal = np.cross(v1,v2) / np.linalg.norm (np.cross(v1, v2))
	
    # compute distance between point and facet in normal direction
	s2f = np.dot (normal, np.array(Pt) - np.array(p1))
    # project point on facet
	projectPt = np.array(Pt) - s2f * np.array(normal)

    # if point lies in same plane, check exact location (on vertex, on line, within facet)
	if np.absolute(s2f) <= sR:
		info = checkSphere(facet, projectPt)
		if info[0] == 1:
			return info
		info = checkLine(facet, projectPt, sR)
		if info[0] == 1:
			return info
		info = checkPoint(facet, projectPt, sR)
		if info[0] == 1:
			return info
            
		return [0, 0, 0, 0]
	else:
		return [0, 0, 0, 0]
    

# Load surface points
def loadSurfacePts():

	# Remark: If-statement is only necessary if path contains delimiters
	if os.name == 'posix': #Linux
		ptsName = 'surface'
	else:
		ptsName = 'surface'
    
	X = np.loadtxt(ptsName)
        
	return X

# Load name of STL file
def loadSTLName(frameID):

	# Remark: If-statement is only necessary if path contains delimiters
	if os.name == 'posix': #Linux
		stlVenName='STL/ventricle_{}.stl'.format(frameID)
	else:
		stlVenName='STL\\ventricle_{}.stl'.format(frameID)

	return stlVenName

# load vertex data from STL file
# returns array for all facets, for each facet three nested arrays containing the coordinate entries (X,Y,Z)
def loadVertex(frameID):

	stlName = loadSTLName(frameID)

	strOuter = "outer loop"
	strVertex = "vertex"
	strEnd = "endloop"

	surfaceNodes = []
	faceNum = 0
	f =  open(stlName)
	for line in f:
		if line.find(strOuter) >= 0: 
			surfaceNodes.append([]) 
		if line.find(strVertex) >= 0:
			# extract numeric data within line
			X = line.split() 
			X = [float(n) for n in X[1:len(line)]]
			surfaceNodes[faceNum].append(X)
		if line.find(strEnd) >= 0:
			faceNum = faceNum + 1

	f.close()
	print ('Number of Faces: {} on {} \n'.format(faceNum,stlName))
	
	return surfaceNodes

    
# Main function
# Create PTS-Folder if necessary
folderName = 'PTS'
if not os.path.exists(folderName):
	os.makedirs(folderName)
 
print ("Generating surface nodes according to STL files")
answer = int(input('Do you want to generate a correlation file? Yes: 1. No: 0. '))

if answer == 1:
	# create text file 'correlation' with three lists (len(allPts))
	# list 1: index (on which facet lies point)
	# list 2 and 3: alpha, beta (positions in relation to the vertices of the facet)
	
	frameID = int(input('The reference frameID: '))
	SSphereR = 0.001 # tolerance for determining if point lies on facet
    
    # Load vertices and compute center of mass of each facet

	stlVertices = []
	stlMean = []
	
	oo = loadVertex(frameID)
	stlVertices.append(oo)
	stlMean.append(np.mean (np.array(oo), axis=1)) # center of mass

    # create empty arrays/lists for all points
	
	allPts = loadSurfacePts()
	ptsCheck = np.zeros(len(allPts))    

	status_List = np.zeros(len(allPts))
	index_List = np.zeros(len(allPts))
	alpha_List = np.zeros(len(allPts))
	beta_List = np.zeros(len(allPts))

	ptsCount = 0
    
	print ('allPtsNum: {} \n' .format(len(allPts)))

	for i in range(0, len(allPts)):

        # Sort points by distance to center of mass (low to high)
		udfPt = np.array(allPts[i][1:4]) # array with point coordinates
		oriDis = list (np.linalg.norm (stlMean[0] - np.array(udfPt), axis=1))
		dis = oriDis[0 : len(oriDis)]
		dis.sort()

		# check if point lies within facet
		# start with facets whose center of mass is closest to point
		for d in range(0, 100): # we only check closest 100 facets                      
			index = oriDis.index(dis[d])
			stlFacet = stlVertices[0][index]

			info = touchCheck(stlFacet, udfPt, SSphereR)
			# if point touches the facet on either vertex, line or inner domain, write stlID, index, alpha and beta
			if info[0] == 1:
				status_List[i] = 1
				index_List[i] = index
				alpha_List[i] = info[1]
				beta_List[i] = info[2]

				ptsCount += 1
				break

		# if no matching facet is found, visualize position of the point w.r.t. last tried facet
		# red: P1, green: P2, blue: P3
		# x: target point, >: created point (if possible, otherwise use P2)
		stlFacet = stlVertices[0][int(index)]
		p1 = stlFacet[0]
		p2 = stlFacet[1]
		p3 = stlFacet[2]
		U = np.array(p1) - np.array(p2)
		V = np.array(p3) - np.array(p2)
		reUdfPt = (U * info[1] + V * info[2] + np.array(p2))

		if np.linalg.norm(np.array(reUdfPt)-np.array(udfPt)) > SSphereR:
			figure = plt.figure()
			ax = figure.gca(projection='3d')
			ax.plot([row[0] for row in stlFacet],[row[1] for row in stlFacet], [row[2] for row in stlFacet], '-')
			ax.scatter(stlFacet[0][0], stlFacet[0][1], stlFacet[0][2], c='r', marker='o', s=100)
			ax.scatter(stlFacet[1][0], stlFacet[1][1], stlFacet[1][2], c='g', marker='o', s=100)
			ax.scatter(stlFacet[2][0],stlFacet[2][1],stlFacet[2][2],c='b', marker='o', s=100)
			ax.scatter(udfPt[0], udfPt[1] ,udfPt[2],c='k', marker='x', s=50)
			ax.scatter(reUdfPt[0], reUdfPt[1], reUdfPt[2], c='k', marker='>', s=50)
			print ('Type: {}'.format(info[3]))
			print ('Original: {}'.format(udfPt))
			print ('After: {}'.format(reUdfPt))
			plt.show()    

		if info[0] == 0:
			print ('Type: {} index: {} Point {}'.format(info[3], index, np.array(allPts[i][1:4])))

    # write correlation file 
        
	print ('Überprüfung der Anzahl der Punkte: {} = {} \n'.format(len(ptsCheck), ptsCount))
	np.savetxt('correlation.txt', (index_List, alpha_List, beta_List))

else:

	# if correlation file already exists: read data
	
	correlation = np.loadtxt('correlation.txt')
	index_List = list(correlation[1])
	alpha_List = list(correlation[2])
	beta_List = list(correlation[3])

# Convert STL to PTS
# For each frame between start FrameID and end FrameID, create file in PTS folder 
# containing all points in the order they are read from Fluent 

print ('Convert STl to PTS')

sFrameID = int(input('Start FrameID:  '))
eFrameID = int(input('End   FrameID:  '))

for frameID in range(sFrameID, eFrameID+1):
	print (frameID)
	
	# Remark: If-statement is only necessary if path contains delimiters
	if os.name == 'posix':
		ptsFileName = 'PTS/surface_{}.asc'.format(frameID)
	else:
		ptsFileName = 'PTS\\surface_{}.asc'.format(frameID)

	print (ptsFileName)

	allPts = []
	stlVertices = []
    
	oo = loadVertex( frameID)
	stlVertices.append(oo)

	for i in range(0, len(index_List)):
		index = index_List[i]
		alpha = alpha_List[i]
		beta = beta_List[i]

		stlFacet = stlVertices[0][int(index)]
		p1 = stlFacet[0]
		p2 = stlFacet[1]
		p3 = stlFacet[2]
		U = np.array(p1) - np.array(p2)
		V = np.array(p3) - np.array(p2)
		udfPt = (U * alpha + V * beta + np.array(p2))

		allPts.append(udfPt)

	np.savetxt(ptsFileName, allPts)
	