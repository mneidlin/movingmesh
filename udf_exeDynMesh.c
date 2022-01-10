﻿/* This UDF reads the "UDFPTS" folder that has to be in the FLUENT simulation location and prescribes the movement of the nodes */
#include "udf.h"
#include "dynamesh_tools.h"

#define nTimeSteps 1236	/* total number of frames after interpolation: output of ps_intNPts */
#define allPtsNum 3260	/* total number of nodes on moving boundaries: output of ps_detNPts */

#define Scale 0.001 	/* factor for scaling units; already considered in udf_extPts (AssignID) */  
#define fakeTime 0

/* define the number of nodes per core (except for the last one); values are provided by AssignID (udf_extPts) in the console, adapt from there */  
const static int PtsNum[] = { 1645, 1615 };

static real vel, Grid_X[allPtsNum], Grid_Y[allPtsNum], Grid_Z[allPtsNum];

/* assigns loaded node coordinates to the face zones (ventricle, inlet, outlet); note: function gets called by each dynamic mesh zone 
that uses this UDF for mesh motion. Further, smoothing applies a pre-processing step and thus function gets called twice 
(only the single mesh motion gets applied though) */
DEFINE_GRID_MOTION(Ven_move, domain, dt, time, dtime)
{
	#if !RP_HOST
		Thread *tf=DT_THREAD(dt);
		face_t f;
		Node *node_p;

		real X[3];
		int nTime, n, ptID, i;

		nTime=(N_TIME+fakeTime)%nTimeSteps+1; /* N_TIME integer number of time steps; nTime: actual timestep in cycle; restarts at 1 after one cycle */

		if (!Data_Valid_P()) /* do nothing if gradient isn’t allocated yet. */
			return;
	
		SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));
	
		begin_f_loop(f,tf)
		if PRINCIPAL_FACE_P(f,tf) /* checks if face truly belongs to computing node: important for faces at the partition interface */
		{
			f_node_loop(f,tf,n)
			{
				node_p=F_NODE(f,tf,n);  /* obtain the global face node number*/
				if (NODE_POS_NEED_UPDATE(node_p))
				{
			  		NODE_POS_UPDATED(node_p);

			  		ptID=N_UDMI(node_p,myid);

					/* obtain global index in array from offset due to other computing nodes; array sequentially ordered from node_0 to node_(N-1) */
			  		if (myid != 0)
			  		{
			  			for (i = 0 ; i < myid ; i++)
			  			{	
			  				ptID = ptID + PtsNum[i];
			  			}
			  		}	

			  		X[0]=Grid_X[ptID];
			  		X[1]=Grid_Y[ptID];
			  		X[2]=Grid_Z[ptID];

			  		NODE_X(node_p)=X[0]*Scale;
			  		NODE_Y(node_p)=X[1]*Scale;
			  		NODE_Z(node_p)=X[2]*Scale;
			}
		}
	}
	end_f_loop(f,tf)
	#endif /* RP_NODE */ 
}

/* at the end of timestep new node coordinates get loaded from udfsurface file; function hooked under "Define at end" */
DEFINE_EXECUTE_AT_END(loadMESH)
{
	#if !RP_NODE
		int nTime, i;
		char nFileName[100];
		FILE *meshFrame;
	
		nTime=(N_TIME+fakeTime)%nTimeSteps+1;

		sprintf(nFileName,"UDFPTS/udfsurface_%d.asc",nTime); /* string saved in nFileName  */
		meshFrame=fopen(nFileName,"r");
		Message("\n %s \n", nFileName);
		Message("\n %d \n", N_TIME);

		for (i=0; i<allPtsNum; i++)
		{
			fscanf(meshFrame,"%lf %lf %lf", &Grid_X[i],&Grid_Y[i],&Grid_Z[i]); 
		}
	
		fclose(meshFrame);
		Message("Finish Loading \n");
	#endif /* RP_NODE */ 

	host_to_node_real(Grid_X,allPtsNum);  /* integer and real variables passed from host to nodes */
	host_to_node_real(Grid_Y,allPtsNum);
	host_to_node_real(Grid_Z,allPtsNum);
}

/* function has to be run before starting simulation, performs same as loadMESH function only for first timestep. Loads current mesh. */
DEFINE_ON_DEMAND(First_loadMESH)
{
	/* initial surface data gets read by host process */
	#if !RP_NODE
		int nTime, i;
		char nFileName[100];
		FILE *meshFrame;
	
		nTime=(N_TIME+fakeTime)%nTimeSteps+1;   /* N_TIME integer number of time steps; nTime remaining timesteps */

		sprintf(nFileName,"UDFPTS/udfsurface_%d.asc",nTime);
		meshFrame=fopen(nFileName,"r");
		Message("\n %s \n", nFileName);
		Message("\n %d \n", N_TIME);

		for (i=0; i<allPtsNum; i++)
		{
			fscanf(meshFrame,"%lf %lf %lf", &Grid_X[i], &Grid_Y[i], &Grid_Z[i]);
		}
	
		fclose(meshFrame);
		Message("Finish Loading \n");
	#endif /* RP_HOST */ 
	
	/* send data to each compute node */
	host_to_node_real(Grid_X,allPtsNum);
	host_to_node_real(Grid_Y,allPtsNum);
	host_to_node_real(Grid_Z,allPtsNum);
}