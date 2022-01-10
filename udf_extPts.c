/* UDF to assign nodes to a user defined memory location. The number of user-defined memory locations
has to be the same as the number of used cores => Otherwise ANSYS crashes. In the end a "surface" file with the local node numbering and x,y,z coordinates of 
the corresponding nodes is written */

#include "udf.h"

/* define zoneIDs of ventricle, inlet and outlet */
#define nFZones 3
#define ventricle 5      
#define inlet 6
#define outlet 7

/* assigns zoneIDs to an array */
void loadZoneID15(int *zoneID)
{
	zoneID[0]=ventricle;
	zoneID[1]=inlet;
	zoneID[2]=outlet;
	
	if (I_AM_NODE_ZERO_P)
	{
		
		Message("\n Ventricle: %d", ventricle);
		Message("\n Inlet: %d", inlet);
		Message("\n Outlet: %d", outlet);
		
	}
}

DEFINE_ON_DEMAND(First_AssignID)  /* perform the UDF on demand */
{
	#if !RP_HOST 
		Domain *domain;			/* get the domain using ANSYS FLUENT utility */
		Thread *tFace;
		face_t f;               /* integer data type that identifies a particular face within a face thread */
		Node *v;
	
		int zoneID[nFZones];	/* array containing face zone IDs */
		int i, n;				/* indices for loops */
		int gNodeID, ptID=0;	/* global and local node numbering on each computing node */
		int dummy;				/* DUMMY; STILL TO BE DEFINED */
		
		real nodeX,nodeY,nodeZ;	/* local node coordinates */
		FILE *ptFile;			/* pointer to file containing local node numbering and corresponding coordinates */ 
		FILE *nIDFile;
		int ptNum[compute_node_count];
		
		/* dummy variable is send in round-robin fashion (each node sends to neighboring node to the right) */
  		if (!I_AM_NODE_ZERO_P) PRF_CRECV_INT(myid - 1, &dummy, 1, myid - 1); 
		
		/* node_0 opens file in write mode and all other nodes in append mode */
		ptFile=fopen("surface",(I_AM_NODE_ZERO_P ? "w" : "a"));
		nIDFile=fopen("nodeID_List",(I_AM_NODE_ZERO_P ? "w" : "a"));

		domain=Get_Domain(1);

		/* loads zoneIDs of current simulation setup */
		loadZoneID15(zoneID);

		/* loop over all face zones and sets the user defined memory location of every computing node to -1 */
		for (i=0; i<nFZones; i++)
		{
			tFace=Lookup_Thread(domain,zoneID[i]);  /* thread pointer to Zone ID */
			begin_f_loop(f,tFace)					/* loop over all faces in thread */
			if PRINCIPAL_FACE_P(f,tFace)			/* checks if face truly belongs to computing node: important for faces at the partition interface */
			{
				f_node_loop(f,tFace,n)      		/* loop over all nodes in face thread tFace */
				{
					v=F_NODE(f,tFace,n);   			/* obtain the global face node number*/
					N_UDMI(v,myid)=-1;          	/* access or store the value of the user-defined memory in a mesh node with index of the current core as -1*/
				}
			}
			end_f_loop(f,tFace)
		}
	
		/* loop over all face zones. loop over all nodes, that have not been checked (all with entries -1)
		and connects the node on the core with the index of the point. In other words, point ID and coordinates of each node is assigned in the file "surface" 	*/	
		for (i=0; i<nFZones; i++)
		{
			tFace=Lookup_Thread(domain,zoneID[i]);	/* thread pointer to Zone ID */
		
			begin_f_loop(f,tFace)      				/* loop over all faces in a given face thread tFace */
			if PRINCIPAL_FACE_P(f,tFace)			/* checks if face truly belongs to computing node: important for faces at the partition interface */
			{
				f_node_loop(f,tFace,n)              /* loop over all nodes in face Thread tFace */
				{
					v=F_NODE(f,tFace,n);      		
					gNodeID=NODE_ID(v);				/* obtain the global face node number */
					nodeX=NODE_X(v);          		/* returns real x coordinate of node v to nodeX */
					nodeY=NODE_Y(v);
					nodeZ=NODE_Z(v);
					if (N_UDMI(v,myid)==-1)			/* if N_UDMI contains -1, current node belongs to computing node and local node numbering and coordinates will be written to file */
					{
						fprintf(ptFile,"%d %lg %lg %lg \n", ptID, 1000*nodeX, 1000*nodeY, 1000*nodeZ);
						fprintf(nIDFile,"%d \n", gNodeID);
						N_UDMI(v,myid)=ptID;		/* local node numbering will be stored in user-defined memory */
						ptID+=1;
						ptNum[myid] = ptID;
					}
				}
			}
			end_f_loop(f,tFace)
		}

		Message(" \n %d : %d ", myid, ptNum[myid]);
	
  		if (!I_AM_NODE_LAST_P) PRF_CSEND_INT(myid + 1, &dummy, 1, myid);
		fclose(ptFile);
		fclose(nIDFile);

	#endif  /* RP_NODE */ 
}
