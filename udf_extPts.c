/* UDF to assign nodes to a user defined memory location. The number of user-defined memory locations
has to be the same as the number of used cores => Otherwise ANSYS crashes. In the end a "surface" file with the x,y,z coordinates of 
the nodes is written*/

#include "udf.h"

/* Define zones of venctricle, inlet and outlet*/
#define ventricle 5      
#define inlet 6
#define outlet 7

/* Set first entry of zoneID to the ventricle and provide output */
void loadZoneID15(int *zoneID)
{
	zoneID[0]=ventricle;
	/*Message("Ventricle: %d \n", ventricle);*/
	zoneID[1]=inlet;
	/*Message("Inlet: %d \n", inlet);  */
	zoneID[2]=outlet;
	/*Message("Outlet: %d \n", outlet); */   
}

DEFINE_ON_DEMAND(First_AssignID)  /* Perform the UDF on demand */
{
	#if !RP_HOST 
	Domain *domain;  /* Get the domain using ANSYS FLUENT utility */
	Thread *tFace;
	face_t f;                    /* integer data type that identifies a particular face within a face thread */
	Node *v;
	
	int zoneID[3],i,n, ptID=0, dummy;             /* erzeugen der Integer */
	real nodeX,nodeY,nodeZ;
	FILE *ptFile;
	int ptNum[100];

	#if RP_NODE
  		if (! I_AM_NODE_ZERO_P) PRF_CRECV_INT(myid - 1, &dummy, 1, myid - 1); 
  		/*integer dummy is (1 element in array)sent from myid-1; For `receive' messages, the argument from is the ID of the sending node. 
  		buffer is the name of an array of the appropriate type that will be received. nelem is the number of elements in the array and tag is the ID of the receiving node. 
  		The tag convention for receive messages is the `from' node (same as the first argument). */
		ptFile=fopen("surface",(I_AM_NODE_ZERO_P ? "w" : "a"));      /* open a file named surfacel in updating write mode and assign it to ptfile */
	
	#endif

	domain=Get_Domain(1);

	loadZoneID15(zoneID);                   /* Perform the functions*/

	/* for-loop for the 3 zones ventricle, inlet and outlet
	puts the user defined memory location of every node to -1*/
	for (i=0;i<3;i++)
	{
		tFace=Lookup_Thread(domain,zoneID[i]);         /* Thread pointer to Zone ID */
		begin_f_loop(f,tFace)
		{
			f_node_loop(f,tFace,n)      /* Loop over all nodes in face Thread tFace */
			{
				v=F_NODE(f,tFace,n);   /* obtain the global face node number*/
				N_UDMI(v,myid)=-1;          /* access or store the value of the user-defined memory in a mesh node with index of the current core as -1*/
			}
		}
		end_f_loop(f,tFace)
	}
	
	/* for loop for the 3 zones ventricle, inlet and outlet. Goes through all nodes, that have not been checked (all with entries -1)
and connects the node on the core with the index of the point. In other words, point ID and coordinates of each node is assigned in the file "surface" 	*/	
	
	for (i=0;i<3;i++)
	{
		tFace=Lookup_Thread(domain,zoneID[i]);        /* Thread pointer to Zone ID mit aufsteigenden Einträgen */
		
		begin_f_loop(f,tFace)      /*for loop over all faces in a given face thread tFace*/
		{
			f_node_loop(f,tFace,n)              /* Loop over all nodes in face Thread tFace */
			{
				v=F_NODE(f,tFace,n);      /* obtain the global face node number*/
				nodeX=NODE_X(v);          /* returns real x coordinate of node v to node X*/
				nodeY=NODE_Y(v);
				nodeZ=NODE_Z(v);
				if (N_UDMI(v,myid)==-1)
				{
					fprintf(ptFile,"%d %f %f %f \n", ptID, 1000*nodeX, 1000*nodeY, 1000*nodeZ);
					N_UDMI(v,myid)=ptID;
					ptID+=1;
					ptNum[myid] = ptID;
				}
			}
		}
		end_f_loop(f,tFace)
	}

	Message(" \n %d : %d ", myid, ptNum [myid]);
	

	#if RP_NODE
  		if (! I_AM_NODE_LAST_P) PRF_CSEND_INT(myid + 1, &dummy, 1, myid);
		fclose(ptFile);

	#else
		fclose(ptFile);
	#endif  

#endif  
}
